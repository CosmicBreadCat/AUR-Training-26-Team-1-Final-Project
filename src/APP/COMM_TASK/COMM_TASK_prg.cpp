#include "../../SERVICE/COMM/COMM_int.h"
#include "COMM_TASK_int.h"
#include <Arduino.h>
#include <WiFi.h>
#include <math.h>
#include <string.h>

typedef struct {
  RobotMode mode;
  const char *phase;
  Result result;
  const char *reason;
  uint32_t cmdSeq;
  ModeParams params;
  float v;
  float omega;
  bool haveTwist;
  uint32_t lastTwistMs;
  bool hasSquareRef;
  Side squareRef;
} RobotState_t;

/* Owned by the comms task only */
static Comms xComms;
static RobotState_t xState = {
    RobotMode::Stop, "IDLE", Result::Running, "", 0, {}, 0.0f, 0.0f, false, 0};
static GripperCommand_t xGripper = {COMMTASK_GRIPPER_BOOT_LIFT,
                                    COMMTASK_GRIPPER_BOOT_GRIP, 0};
static Status xLastSentStatus = {RobotMode::Stop,
                                 "",
                                 Result::Running,
                                 "",
                                 0,
                                 COMMTASK_GRIPPER_BOOT_LIFT,
                                 COMMTASK_GRIPPER_BOOT_GRIP,
                                 false,
                                 0,
                                 0};
static bool bWasAlive = false;
static uint32_t u32LastOdometryMs = 0;
static uint32_t u32LastSensorsMs = 0;
static uint32_t u32LastStatusMs = 0;
static uint32_t u32LastTwistLogMs = 0;
static uint32_t u32LastRxBad = 0;
static bool bWasWifiUp = false;

/* Length-1 mailboxes shared with other tasks */
static QueueHandle_t xMotionMailbox = NULL;   // comms -> control
static QueueHandle_t xGripperMailbox = NULL;  // comms -> control
static QueueHandle_t xResultMailbox = NULL;   // control -> comms
static QueueHandle_t xOdometryMailbox = NULL; // control -> comms
static QueueHandle_t xSensorsMailbox = NULL;  // control -> comms

static ModeParams xNoParams(void) {
  ModeParams xParams = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
  return xParams;
}

/* Modes whose phase and result come from the control task */
static bool bControlOwned(RobotMode Copy_xMode) {
  return Copy_xMode == RobotMode::Move || Copy_xMode == RobotMode::Turn ||
         Copy_xMode == RobotMode::Square;
}

/* Modes the firmware accepts. The rest are acked "not_implemented". */
static bool bModeSupported(RobotMode Copy_xMode) {
  return Copy_xMode == RobotMode::Stop || Copy_xMode == RobotMode::Twist ||
         bControlOwned(Copy_xMode);
}

static Status xCurrentStatus(void) {
  Status xStatus = {};
  xStatus.mode = xState.mode;
  xStatus.phase = xState.phase;
  xStatus.result = xState.result;
  xStatus.reason = xState.reason;
  xStatus.cmdSeq = xState.cmdSeq;
  xStatus.lift = xGripper.lift;
  xStatus.grip = xGripper.grip;
  xStatus.watchdog = !xComms.linkAlive();
  return xStatus;
}

static bool bStatusChanged(const Status *a, const Status *b) {
  return a->mode != b->mode || strcmp(a->phase, b->phase) != 0 ||
         a->result != b->result || strcmp(a->reason, b->reason) != 0 ||
         a->cmdSeq != b->cmdSeq || a->lift != b->lift || a->grip != b->grip ||
         a->watchdog != b->watchdog;
}

static void vPublishStatus(void) {
  Status xStatus = xCurrentStatus();
  xComms.sendStatus(xStatus);
  xLastSentStatus = xStatus;
  u32LastStatusMs = millis();
}

/* Ends the current command with a DONE/FAILED result and returns to STOP,
 * keeping cmdSeq so the laptop can tell which command ended. */
static void vEndInStop(Result Copy_xResult, const char *Copy_pcReason) {
  Serial.printf("mode %s %s%s%s (cmd %u) -> STOP\n",
                PROTOCOL_pcModeName(xState.mode),
                PROTOCOL_pcResultName(Copy_xResult), *Copy_pcReason ? "/" : "",
                Copy_pcReason, xState.cmdSeq);
  xState.mode = RobotMode::Stop;
  xState.phase = "IDLE";
  xState.result = Copy_xResult;
  xState.reason = Copy_pcReason;
  xState.params = xNoParams();
  xState.v = 0.0f;
  xState.omega = 0.0f;
  xState.hasSquareRef = false;
}

static void vEnterMode(RobotMode Copy_xMode, const ModeParams *Copy_pxParams,
                       uint32_t Copy_u32CmdSeq) {
  if (xState.mode != RobotMode::Stop && xState.result == Result::Running) {
    xState.result =
        Result::Preempted; // report the old command before switching
    vPublishStatus();
  }
  xState.mode = Copy_xMode;
  if (Copy_xMode == RobotMode::Stop) {
    xState.phase = "IDLE";
  } else if (Copy_xMode == RobotMode::Twist) {
    xState.phase = "NO_INPUT";
  } else {
    xState.phase = "STARTING"; // until the control task reports a phase
  }
  xState.result = Result::Running;
  xState.reason = "";
  xState.cmdSeq = Copy_u32CmdSeq;
  xState.params = *Copy_pxParams;
  xState.v = 0.0f;
  xState.omega = 0.0f;
  xState.haveTwist = false;
  xState.hasSquareRef = false;
  Serial.printf("mode -> %s (cmd %u)\n", PROTOCOL_pcModeName(Copy_xMode),
                Copy_u32CmdSeq);
}

static void vHandleMode(const Command *Copy_pxCmd) {
  if (!bModeSupported(Copy_pxCmd->mode)) {
    xComms.sendAck(Copy_pxCmd->seq, false, "not_implemented");
    Serial.printf("mode %s rejected: not implemented\n",
                  PROTOCOL_pcModeName(Copy_pxCmd->mode));
    return;
  }
  // Same mode while it's still running = a retry after a lost ack: just ack
  // again. Changing cmdSeq here would make the control task restart the mode.
  if (Copy_pxCmd->mode != xState.mode || xState.result != Result::Running) {
    vEnterMode(Copy_pxCmd->mode, &Copy_pxCmd->params, Copy_pxCmd->seq);
  }
  xComms.sendAck(Copy_pxCmd->seq, true, "");
}

static void vHandleTwist(const Command *Copy_pxCmd) {
  if (xState.mode != RobotMode::Twist) {
    return; // only TWIST uses twist; the packet still fed the watchdog
  }
  xState.v = Copy_pxCmd->v;
  xState.omega = Copy_pxCmd->omega;
  xState.haveTwist = true;
  xState.lastTwistMs = millis();
}

static void vHandleGripper(const Command *Copy_pxCmd) {
  if (Copy_pxCmd->hasLift) {
    xGripper.lift = Copy_pxCmd->lift;
  }
  if (Copy_pxCmd->hasGrip) {
    xGripper.grip = Copy_pxCmd->grip;
  }
  xGripper.cmdSeq = Copy_pxCmd->seq;
  xQueueOverwrite(xGripperMailbox, &xGripper);
  xComms.sendAck(Copy_pxCmd->seq, true, "");
  Serial.printf("gripper -> lift=%s grip=%s (cmd %u)\n",
                PROTOCOL_pcLiftName(xGripper.lift),
                PROTOCOL_pcGripName(xGripper.grip), xGripper.cmdSeq);
}

/* The driver names the edge SQUARE just aligned to. Only accepted while
 * SQUARE is waiting for it; the control task then corrects the pose. */
static void vHandleSquareRef(const Command *Copy_pxCmd) {
  if (xState.mode != RobotMode::Square || xState.result != Result::Running ||
      strcmp(xState.phase, "WAITING") != 0) {
    xComms.sendAck(Copy_pxCmd->seq, false, "not_waiting");
    Serial.printf("square_ref %s rejected: not waiting\n",
                  PROTOCOL_pcSideName(Copy_pxCmd->side));
    return;
  }
  xState.hasSquareRef = true;
  xState.squareRef = Copy_pxCmd->side;
  xComms.sendAck(Copy_pxCmd->seq, true, "");
  Serial.printf("square_ref -> %s (cmd %u)\n",
                PROTOCOL_pcSideName(Copy_pxCmd->side), Copy_pxCmd->seq);
}

/* Adopts the control task's progress for the mode it is running */
static void vApplyMotionResult(void) {
  MotionResult_t xResult;
  if (!bControlOwned(xState.mode) || xState.result != Result::Running ||
      xQueuePeek(xResultMailbox, &xResult, 0) != pdTRUE ||
      xResult.cmdSeq != xState.cmdSeq) {
    return; // nothing new for the command we're running
  }
  if (xResult.result == Result::Done || xResult.result == Result::Failed) {
    vEndInStop(xResult.result, xResult.reason);
  } else {
    xState.phase = xResult.phase;
  }
}

static void vStep(void) {
  /* WiFi drops are logged only; the comms watchdog handles stopping */
  bool bWifiUp = xComms.wifiConnected();
  if (bWifiUp && !bWasWifiUp) {
    Serial.printf("WiFi connected, IP %s\n",
                  xComms.localIp().toString().c_str());
  } else if (!bWifiUp && bWasWifiUp) {
    Serial.println("WiFi lost, reconnecting");
  }
  bWasWifiUp = bWifiUp;

  Command xCmd;
  while (xComms.poll(xCmd)) {
    switch (xCmd.type) {
    case MsgType::Mode:
      vHandleMode(&xCmd);
      break;
    case MsgType::Twist:
      vHandleTwist(&xCmd);
      break;
    case MsgType::Gripper:
      vHandleGripper(&xCmd);
      break;
    case MsgType::SquareRef:
      vHandleSquareRef(&xCmd);
      break;
    default:
      break; // wheel_vel: WHEEL_VEL is not implemented yet
    }
  }

  if (xComms.rxBad() != u32LastRxBad) {
    Serial.printf("rejected packet: %s (rx_bad=%u)\n", xComms.lastReject(),
                  xComms.rxBad());
    u32LastRxBad = xComms.rxBad();
  }

  vApplyMotionResult();

  uint32_t u32Now = millis();

  /* Watchdog: no valid packet for COMM_WATCHDOG_MS -> STOP.
   * The gripper holds its state, so a WiFi blip doesn't drop a box. */
  bool bAlive = xComms.linkAlive();
  if (bWasAlive && !bAlive) {
    Serial.println("watchdog: link lost");
    if (xState.mode != RobotMode::Stop) {
      vEndInStop(Result::Failed, "watchdog");
    }
  } else if (!bWasAlive && bAlive) {
    Serial.println("link up");
  }
  bWasAlive = bAlive;

  if (xState.mode == RobotMode::Twist) {
    bool bFresh = xState.haveTwist &&
                  u32Now - xState.lastTwistMs < COMMTASK_TWIST_STALE_MS;
    if (!bFresh) {
      xState.v = 0.0f;
      xState.omega = 0.0f;
    }
    xState.phase = bFresh ? "DRIVING" : "NO_INPUT";
    if (u32Now - u32LastTwistLogMs >= COMMTASK_TWIST_LOG_MS) {
      Serial.printf("twist v=%.3f omega=%.3f (%s)\n", xState.v, xState.omega,
                    xState.phase);
      u32LastTwistLogMs = u32Now;
    }
  }

  MotionCommand_t xMotion = {xState.mode,        xState.v,
                             xState.omega,       xState.params,
                             xState.cmdSeq,      xState.hasSquareRef,
                             xState.squareRef};
  xQueueOverwrite(xMotionMailbox, &xMotion);

  if (u32Now - u32LastOdometryMs >= COMM_ODOMETRY_PERIOD_MS) {
    Odometry xOdom = {};
    xQueuePeek(xOdometryMailbox, &xOdom, 0); // stays zero until someone sets it
    xComms.sendOdometry(xOdom);
    u32LastOdometryMs = u32Now;
  }

  if (u32Now - u32LastSensorsMs >= COMM_SENSORS_PERIOD_MS) {
    Sensors xSensors = {NAN};
    xQueuePeek(xSensorsMailbox, &xSensors,
               0); // stays NAN until someone sets it
    xComms.sendSensors(xSensors);
    u32LastSensorsMs = u32Now;
  }

  Status xStatus = xCurrentStatus();
  if (bStatusChanged(&xStatus, &xLastSentStatus) ||
      u32Now - u32LastStatusMs >= COMM_STATUS_PERIOD_MS) {
    vPublishStatus();
  }
}

static void vCommTask(void *pvParameters) {
  (void)pvParameters;
  TickType_t xLastWake = xTaskGetTickCount();
  while (true) {
    vStep();
    vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(COMMTASK_PERIOD_MS));
  }
}

bool COMMTASK_bStart(void) {
  if (xMotionMailbox != NULL) {
    return true; // already running
  }
  xMotionMailbox = xQueueCreate(1, sizeof(MotionCommand_t));
  xGripperMailbox = xQueueCreate(1, sizeof(GripperCommand_t));
  xResultMailbox = xQueueCreate(1, sizeof(MotionResult_t));
  xOdometryMailbox = xQueueCreate(1, sizeof(Odometry));
  xSensorsMailbox = xQueueCreate(1, sizeof(Sensors));
  if (xMotionMailbox == NULL || xGripperMailbox == NULL ||
      xResultMailbox == NULL || xOdometryMailbox == NULL ||
      xSensorsMailbox == NULL) {
    return false;
  }
  // Seed the outgoing mailboxes so a reader always finds a valid command
  xState.params = xNoParams();
  MotionCommand_t xInitial = {RobotMode::Stop, 0.0f, 0.0f, xState.params, 0};
  xQueueOverwrite(xMotionMailbox, &xInitial);
  xQueueOverwrite(xGripperMailbox, &xGripper);

  Serial.printf("Joining \"%s\"...\n", UDP_WIFI_SSID);
  if (!xComms.begin()) {
    return false;
  }
  if (xComms.wifiConnected()) {
    Serial.printf("WiFi up, IP %s, UDP port %u\n",
                  xComms.localIp().toString().c_str(), UDP_LOCAL_PORT);
  } else {
    Serial.println("WiFi not up yet, retrying in background");
  }

  return xTaskCreatePinnedToCore(vCommTask, "comm", COMMTASK_STACK_BYTES, NULL,
                                 COMMTASK_PRIORITY, NULL,
                                 COMMTASK_CORE) == pdPASS;
}

bool COMMTASK_bGetCommand(MotionCommand_t *Copy_pxCommand) {
  if (xMotionMailbox == NULL) {
    return false;
  }
  return xQueuePeek(xMotionMailbox, Copy_pxCommand, 0) == pdTRUE;
}

bool COMMTASK_bGetGripper(GripperCommand_t *Copy_pxGripper) {
  if (xGripperMailbox == NULL) {
    return false;
  }
  return xQueuePeek(xGripperMailbox, Copy_pxGripper, 0) == pdTRUE;
}

void COMMTASK_vSetMotionResult(const MotionResult_t *Copy_pxResult) {
  if (xResultMailbox != NULL) {
    xQueueOverwrite(xResultMailbox, Copy_pxResult);
  }
}

void COMMTASK_vSetOdometry(const Odometry *Copy_pxOdometry) {
  if (xOdometryMailbox != NULL) {
    xQueueOverwrite(xOdometryMailbox, Copy_pxOdometry);
  }
}

void COMMTASK_vSetSensors(const Sensors *Copy_pxSensors) {
  if (xSensorsMailbox != NULL) {
    xQueueOverwrite(xSensorsMailbox, Copy_pxSensors);
  }
}
