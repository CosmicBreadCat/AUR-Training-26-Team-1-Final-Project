#include "../../SERVICE/COMM/COMM_int.h"
#include "COMM_TASK_int.h"
#include <Arduino.h>
#include <WiFi.h>
#include <string.h>

typedef struct {
  RobotMode mode;
  const char *phase;
  Result result;
  const char *reason;
  uint32_t cmdSeq;
  float v;
  float omega;
  bool haveTwist;
  uint32_t lastTwistMs;
} RobotState_t;

/* Owned by the comms task only */
static Comms xComms;
static RobotState_t xState = {
    RobotMode::Stop, "IDLE", Result::Running, "", 0, 0.0f, 0.0f, false, 0};
static Status xLastSentStatus = {
    RobotMode::Stop, "", Result::Running, "", 0, false, 0, 0};
static bool bWasAlive = false;
static uint32_t u32LastOdometryMs = 0;
static uint32_t u32LastStatusMs = 0;
static uint32_t u32LastTwistLogMs = 0;
static uint32_t u32LastRxBad = 0;
static int s32LastStationCount = -1;

/* Length-1 mailboxes shared with other tasks */
static QueueHandle_t xCommandMailbox = NULL;
static QueueHandle_t xOdometryMailbox = NULL;

static Status xCurrentStatus(void) {
  Status xStatus = {};
  xStatus.mode = xState.mode;
  xStatus.phase = xState.phase;
  xStatus.result = xState.result;
  xStatus.reason = xState.reason;
  xStatus.cmdSeq = xState.cmdSeq;
  xStatus.watchdog = !xComms.linkAlive();
  return xStatus;
}

static bool bStatusChanged(const Status *a, const Status *b) {
  return a->mode != b->mode || strcmp(a->phase, b->phase) != 0 ||
         a->result != b->result || strcmp(a->reason, b->reason) != 0 ||
         a->cmdSeq != b->cmdSeq || a->watchdog != b->watchdog;
}

static void vPublishStatus(void) {
  Status xStatus = xCurrentStatus();
  xComms.sendStatus(xStatus);
  xLastSentStatus = xStatus;
  u32LastStatusMs = millis();
}

static void vEnterMode(RobotMode Copy_xMode, uint32_t Copy_u32CmdSeq) {
  if (xState.mode != RobotMode::Stop && xState.result == Result::Running) {
    xState.result =
        Result::Preempted; // report the old command before switching
    vPublishStatus();
  }
  xState.mode = Copy_xMode;
  xState.phase = (Copy_xMode == RobotMode::Stop) ? "IDLE" : "NO_INPUT";
  xState.result = Result::Running;
  xState.reason = "";
  xState.cmdSeq = Copy_u32CmdSeq;
  xState.v = 0.0f;
  xState.omega = 0.0f;
  xState.haveTwist = false;
  Serial.printf("mode -> %s (cmd %u)\n", PROTOCOL_pcModeName(Copy_xMode),
                Copy_u32CmdSeq);
}

static void vHandleMode(const Command *Copy_pxCmd) {
  if (Copy_pxCmd->mode != RobotMode::Stop &&
      Copy_pxCmd->mode != RobotMode::Twist) {
    xComms.sendAck(Copy_pxCmd->seq, false, "not_implemented");
    Serial.printf("mode %s rejected: not implemented\n",
                  PROTOCOL_pcModeName(Copy_pxCmd->mode));
    return;
  }
  if (Copy_pxCmd->mode == xState.mode && xState.result == Result::Running) {
    xState.cmdSeq = Copy_pxCmd->seq; // repeat, e.g. a retry after a lost ack
  } else {
    vEnterMode(Copy_pxCmd->mode, Copy_pxCmd->seq);
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

static void vStep(void) {
  int s32Stations = xComms.stationCount();
  if (s32Stations != s32LastStationCount) {
    Serial.printf("Stations connected: %d\n", s32Stations);
    s32LastStationCount = s32Stations;
  }

  Command xCmd;
  while (xComms.poll(xCmd)) {
    switch (xCmd.type) {
    case MsgType::Mode:
      vHandleMode(&xCmd);
      break;
    case MsgType::Twist:
      vHandleTwist(&xCmd);
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

  uint32_t u32Now = millis();

  /* Watchdog: no valid packet for COMM_WATCHDOG_MS -> STOP */
  bool bAlive = xComms.linkAlive();
  if (bWasAlive && !bAlive) {
    Serial.println("watchdog: link lost");
    if (xState.mode != RobotMode::Stop) {
      xState.mode = RobotMode::Stop;
      xState.phase = "IDLE";
      xState.result = Result::Failed;
      xState.reason = "watchdog";
      xState.v = 0.0f;
      xState.omega = 0.0f;
      Serial.println("mode -> STOP (watchdog)");
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

  MotionCommand_t xMotion = {xState.mode, xState.v, xState.omega,
                             xState.cmdSeq};
  xQueueOverwrite(xCommandMailbox, &xMotion);

  if (u32Now - u32LastOdometryMs >= COMM_ODOMETRY_PERIOD_MS) {
    Odometry xOdom = {};
    xQueuePeek(xOdometryMailbox, &xOdom, 0); // stays zero until someone sets it
    xComms.sendOdometry(xOdom);
    u32LastOdometryMs = u32Now;
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
  if (xCommandMailbox != NULL) {
    return true; // already running
  }
  xCommandMailbox = xQueueCreate(1, sizeof(MotionCommand_t));
  xOdometryMailbox = xQueueCreate(1, sizeof(Odometry));
  if (xCommandMailbox == NULL || xOdometryMailbox == NULL) {
    return false;
  }
  MotionCommand_t xInitial = {RobotMode::Stop, 0.0f, 0.0f, 0};
  xQueueOverwrite(xCommandMailbox, &xInitial);

  if (!xComms.begin()) {
    return false;
  }
  Serial.printf("AP \"%s\" up, IP %s, UDP port %u\n", UDP_AP_SSID,
                WiFi.softAPIP().toString().c_str(), UDP_LOCAL_PORT);

  return xTaskCreatePinnedToCore(vCommTask, "comm", COMMTASK_STACK_BYTES, NULL,
                                 COMMTASK_PRIORITY, NULL,
                                 COMMTASK_CORE) == pdPASS;
}

bool COMMTASK_bGetCommand(MotionCommand_t *Copy_pxCommand) {
  if (xCommandMailbox == NULL) {
    return false;
  }
  return xQueuePeek(xCommandMailbox, Copy_pxCommand, 0) == pdTRUE;
}

void COMMTASK_vSetOdometry(const Odometry *Copy_pxOdometry) {
  if (xOdometryMailbox != NULL) {
    xQueueOverwrite(xOdometryMailbox, Copy_pxOdometry);
  }
}
