#include "../COMM_TASK/COMM_TASK_int.h"
#include "CONTROL_TASK_int.h"
#include <Arduino.h>
#include <math.h>

/*
 * Runs every CONTROLTASK_PERIOD_MS on core 1. Owns every driver (motors,
 * encoders, IMU, IR, ultrasonic, servos) once they exist; talks to the comms
 * task only through its mailboxes. Never blocks, never prints: logging is the
 * comms task's job.
 */

/* Owned by the control task only */
static MotionCommand_t xActive;     // the command being executed
static bool bHaveActive = false;    // false until the first command is read
static uint32_t u32GripperSeq = 0;  // last gripper command acted on
static bool bHaveGripper = false;
static Odometry xOdom = {};         // all zeros until the drivers exist
static Sensors xSensors = {NAN};    // NAN = no reading

/* Reads every sensor and updates the pose. All drivers are called from here. */
static void vOdometryStep(void) {
  // TODO encoders: read both PCNT units (PCNT_s16GetCount), accumulate the
  //      16-bit deltas into 32-bit positions, estimate wheel velocities.
  // TODO gyro: MPU6050_vReadData, subtract the calibrated bias, integrate the
  //      z rate into heading (recalibrate the bias while stationary).
  // TODO odometry: integrate x, y, theta from the wheel distances and the
  //      gyro heading into xOdom (x, y, theta, v, omega).
  // TODO ultrasonic: latest median-filtered range into xSensors.rangeM.
  // TODO IR line sensors: read the trip latches set by their interrupts.

  COMMTASK_vSetOdometry(&xOdom);
  COMMTASK_vSetSensors(&xSensors);
}

/* Runs once whenever a new command (mode or cmdSeq change) arrives */
static void vEnterMode(const MotionCommand_t *Copy_pxCmd) {
  (void)Copy_pxCmd;
  // TODO reset the wheel-loop integrators, latch the current pose/heading as
  //      the start point, init the slew limiters from the measured v, omega.
}

static void vStepStop(void) {
  // TODO ramp v, omega to 0 through the slew limiter, then set duty 0 (coast).
}

static void vStepTwist(const MotionCommand_t *Copy_pxCmd) {
  (void)Copy_pxCmd;
  // TODO slew-limit Copy_pxCmd->v / omega, inverse kinematics, desaturate,
  //      run both wheel loops (FF + PI), duty = V / V_batt.
  //      Hold heading on the gyro while omega == 0.
}

static void vStepSquare(const MotionCommand_t *Copy_pxCmd, bool Copy_bEntering) {
  /* TODO real SQUARE, using Copy_pxCmd->params.maxSearch / vSearch:
   *   SEARCH       drive slowly forward with heading hold, IR interrupts armed
   *   FIRST_TRIP   one sensor hit the line: latch its encoder distance
   *   SECOND_TRIP  both hit: theta_err = atan(delta_distance / IR spacing)
   *   ALIGN        turn by -theta_err, then report DONE
   * Report each phase with COMMTASK_vSetMotionResult(RUNNING, phase).
   * FAILED reasons: no_line (max_search driven), one_side_only, timeout. */
  if (Copy_bEntering) {
    MotionResult_t xResult = {Copy_pxCmd->cmdSeq, Result::Failed, "IDLE",
                              "not_implemented"};
    COMMTASK_vSetMotionResult(&xResult);
  }
}

static void vMotionStep(void) {
  MotionCommand_t xCmd;
  if (!COMMTASK_bGetCommand(&xCmd)) {
    return; // comms task not started
  }
  bool bNew = !bHaveActive || xCmd.mode != xActive.mode ||
              xCmd.cmdSeq != xActive.cmdSeq;
  xActive = xCmd; // v, omega change every tick in TWIST
  bHaveActive = true;
  if (bNew) {
    vEnterMode(&xActive);
  }

  switch (xActive.mode) {
  case RobotMode::Twist:
    vStepTwist(&xActive);
    break;
  case RobotMode::Square:
    vStepSquare(&xActive, bNew);
    break;
  default:
    vStepStop(); // STOP, and anything not implemented yet
    break;
  }
}

static void vGripperStep(void) {
  GripperCommand_t xGripper;
  if (!COMMTASK_bGetGripper(&xGripper) ||
      (bHaveGripper && xGripper.cmdSeq == u32GripperSeq)) {
    return; // nothing new
  }
  u32GripperSeq = xGripper.cmdSeq;
  bHaveGripper = true;
  // TODO map xGripper.lift / xGripper.grip to servo angles (gripper _cfg.h)
  //      and call PWM_vSetServoAngle() on the lift and grip channels.
}

static void vControlTask(void *pvParameters) {
  (void)pvParameters;
  TickType_t xLastWake = xTaskGetTickCount();
  while (true) {
    vOdometryStep();
    vMotionStep();
    vGripperStep();
    vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(CONTROLTASK_PERIOD_MS));
  }
}

bool CONTROLTASK_bStart(void) {
  return xTaskCreatePinnedToCore(vControlTask, "control",
                                 CONTROLTASK_STACK_BYTES, NULL,
                                 CONTROLTASK_PRIORITY, NULL,
                                 CONTROLTASK_CORE) == pdPASS;
}
