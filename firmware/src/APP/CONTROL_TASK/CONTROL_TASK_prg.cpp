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
static Odometry xOdom = {};         // set to the boot pose in CONTROLTASK_bStart
static Sensors xSensors = {NAN};    // no reading, IR not read (hasIr false)
static bool bHaveHeadingRef = false; // a square_ref has set theta since boot
static bool bSquareDone = false;     // the current SQUARE has used its square_ref

/* Heading (rad) when facing an arena edge, in the arena frame (see _cfg.h) */
static float fSideHeading(Side Copy_xSide) {
  switch (Copy_xSide) {
  case Side::East:
    return 0.0f;
  case Side::North:
    return (float)M_PI / 2.0f;
  case Side::West:
    return (float)M_PI;
  default:
    return -(float)M_PI / 2.0f; // South
  }
}

/* The robot is square on an edge, facing it, with its IR sensor line on it:
 * that fixes theta and the coordinate perpendicular to the edge. */
static void vApplySquareRef(Side Copy_xSide) {
  const float fNear = CONTROLTASK_IR_FORWARD_OFFSET_M;
  const float fFar = CONTROLTASK_ARENA_SIZE_M - CONTROLTASK_IR_FORWARD_OFFSET_M;
  xOdom.theta = fSideHeading(Copy_xSide);
  switch (Copy_xSide) {
  case Side::North:
    xOdom.y = fFar;
    break;
  case Side::South:
    xOdom.y = fNear;
    break;
  case Side::East:
    xOdom.x = fFar;
    break;
  case Side::West:
    xOdom.x = fNear;
    break;
  }
  bHaveHeadingRef = true;
  // TODO once the drivers exist: rebase the gyro heading integrator and the
  //      encoder odometry so they continue from the corrected pose.
}

/* Reads every sensor and updates the pose. All drivers are called from here. */
static void vOdometryStep(void) {
  // TODO encoders: read both PCNT units (PCNT_s16GetCount), accumulate the
  //      16-bit deltas into 32-bit positions, estimate wheel velocities.
  // TODO gyro: MPU6050_vReadData, subtract the calibrated bias, integrate the
  //      z rate into heading (recalibrate the bias while stationary).
  // TODO odometry: integrate x, y, theta from the wheel distances and the
  //      gyro heading into xOdom (x, y, theta, v, omega).
  // TODO ultrasonic: latest median-filtered range into xSensors.rangeM.
  // TODO IR line sensors: read both inputs (GPIO_u8GetPinValue, pins and
  //      active level in _cfg.h) into xSensors.irLeft / irRight and set
  //      xSensors.hasIr = true. That also arms the edge guard. SQUARE will
  //      additionally need the trip latches set by their interrupts.

  COMMTASK_vSetOdometry(&xOdom);
  COMMTASK_vSetSensors(&xSensors);
}

/* Runs once whenever a new command (mode or cmdSeq change) arrives */
static void vEnterMode(const MotionCommand_t *Copy_pxCmd) {
  (void)Copy_pxCmd;
  // TODO reset the wheel-loop integrators, latch the current pose/heading as
  //      the start point, init the slew limiters from the measured v, omega.
}

/* Edge guard for one wheel: while its IR sees the boundary line it may stop
 * or reverse, but not drive forward (towards the line). */
static float fGuardWheel(float Copy_fTarget, bool Copy_bOnLine) {
  return (Copy_bOnLine && Copy_fTarget > 0.0f) ? 0.0f : Copy_fTarget;
}

/* Every mode's wheel speed targets (rad/s, + = forward) go through here, so
 * the edge guard can't be bypassed. SQUARE is exempt: it drives onto the
 * line on purpose. The guard only covers forward motion (no rear sensors). */
static void vSetWheelTargets(float Copy_fWl, float Copy_fWr) {
  if (xActive.mode != RobotMode::Square && xSensors.hasIr) {
    Copy_fWl = fGuardWheel(Copy_fWl, xSensors.irLeft);
    Copy_fWr = fGuardWheel(Copy_fWr, xSensors.irRight);
  }
  // TODO wheel loops: FF + PI per wheel towards Copy_fWl / Copy_fWr,
  //      duty = V / V_batt, written to the motor driver.
  (void)Copy_fWl;
  (void)Copy_fWr;
}

static void vStepStop(void) {
  // TODO ramp to 0 through the slew limiter first, then coast (duty 0).
  vSetWheelTargets(0.0f, 0.0f);
}

static void vStepTwist(const MotionCommand_t *Copy_pxCmd) {
  // TODO slew-limit v / omega first, and hold heading on the gyro while
  //      omega == 0.
  const float fR = CONTROLTASK_WHEEL_RADIUS_M;
  const float fHalfB = CONTROLTASK_TRACK_WIDTH_M / 2.0f;
  float fWl = (Copy_pxCmd->v - Copy_pxCmd->omega * fHalfB) / fR;
  float fWr = (Copy_pxCmd->v + Copy_pxCmd->omega * fHalfB) / fR;
  // TODO desaturate: scale both by min(1, w_max / max(|fWl|, |fWr|)).
  vSetWheelTargets(fWl, fWr);
}

/* Reports progress of the current mode. DONE/FAILED end it: the comms task
 * then returns to STOP. phase and reason must be string literals. */
static void vReportResult(const MotionCommand_t *Copy_pxCmd, Result Copy_xResult,
                          const char *Copy_pcPhase, const char *Copy_pcReason) {
  MotionResult_t xResult = {Copy_pxCmd->cmdSeq, Copy_xResult, Copy_pcPhase,
                            Copy_pcReason};
  COMMTASK_vSetMotionResult(&xResult);
}

static void vReportFailed(const MotionCommand_t *Copy_pxCmd,
                          const char *Copy_pcReason) {
  vReportResult(Copy_pxCmd, Result::Failed, "IDLE", Copy_pcReason);
}

static void vStepMove(const MotionCommand_t *Copy_pxCmd, bool Copy_bEntering) {
  /* TODO real MOVE, using Copy_pxCmd->params.distance (m, required),
   * vMax / aMax (optional, NAN = use defaults):
   *   - trapezoid profile on distance -> d_ref, v_ff
   *   - v_cmd = v_ff + K_d * (d_ref - avg encoder distance)
   *   - omega_cmd: heading hold on the gyro (heading latched in vEnterMode)
   *   - DONE when |distance error| < 1 cm and |v| ~ 0 for 100 ms
   * Report progress with COMMTASK_vSetMotionResult(RUNNING, "DRIVING").
   * FAILED reasons: timeout (> 2x planned time), stall (duty saturated, no
   * encoder motion), obstacle (ultrasonic range below the guard distance).
   * Route the wheel targets through vSetWheelTargets (edge guard). */
  if (Copy_bEntering) {
    vReportFailed(Copy_pxCmd, isnan(Copy_pxCmd->params.distance)
                                  ? "missing_distance"
                                  : "not_implemented");
  }
}

static void vStepTurn(const MotionCommand_t *Copy_pxCmd, bool Copy_bEntering) {
  /* TODO real TURN. Target, exactly one of:
   *   params.angle  (rad, relative, + = CCW): theta_target = start + angle
   *   params.facing (arena side, absolute):   theta_target = fSideHeading()
   * omegaMax / alphaMax optional (NAN = use defaults):
   *   - trapezoid profile on the wrapped heading error -> theta_ref, omega_ff
   *   - omega_cmd = omega_ff + K_theta * wrap(theta_ref - gyro heading)
   *   - v_cmd: distance loop holding 0 so the robot doesn't creep
   *   - DONE when |heading error| < 1 deg and |omega| ~ 0 for 100 ms
   * Report progress with COMMTASK_vSetMotionResult(RUNNING, "TURNING").
   * FAILED reasons: timeout, stall.
   * Route the wheel targets through vSetWheelTargets (edge guard). */
  if (Copy_bEntering) {
    bool bHasAngle = !isnan(Copy_pxCmd->params.angle);
    bool bHasFacing = Copy_pxCmd->params.hasFacing;
    if (!bHasAngle && !bHasFacing) {
      vReportFailed(Copy_pxCmd, "missing_angle");
    } else if (bHasAngle && bHasFacing) {
      vReportFailed(Copy_pxCmd, "conflicting_target");
    } else if (bHasFacing && !bHaveHeadingRef) {
      vReportFailed(Copy_pxCmd, "no_heading_ref"); // arena frame not known yet
    } else {
      vReportFailed(Copy_pxCmd, "not_implemented");
    }
  }
}

static void vStepSquare(const MotionCommand_t *Copy_pxCmd, bool Copy_bEntering) {
  /* TODO real SQUARE, using Copy_pxCmd->params.maxSearch / vSearch:
   *   SEARCH       drive slowly forward with heading hold, IR interrupts armed
   *   FIRST_TRIP   one sensor hit the line: latch its encoder distance
   *   SECOND_TRIP  both hit: theta_err = atan(delta_distance / IR spacing)
   *   ALIGN        turn by -theta_err, stop
   *   WAITING      wait (no timeout) for the driver's square_ref
   * Report each phase with vReportResult(RUNNING, phase).
   * FAILED reasons: no_line (max_search driven), one_side_only, timeout.
   * Route the wheel targets through vSetWheelTargets (the edge guard is off
   * in SQUARE, since it drives onto the line on purpose).
   * MOCK: the physical phases are skipped; SQUARE goes straight to WAITING. */
  if (Copy_bEntering) {
    bSquareDone = false;
    vReportResult(Copy_pxCmd, Result::Running, "WAITING", "");
  } else if (!bSquareDone && Copy_pxCmd->hasSquareRef) {
    vApplySquareRef(Copy_pxCmd->squareRef);
    bSquareDone = true;
    vReportResult(Copy_pxCmd, Result::Done, "IDLE", "");
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
  case RobotMode::Move:
    vStepMove(&xActive, bNew);
    break;
  case RobotMode::Turn:
    vStepTurn(&xActive, bNew);
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
  xOdom.x = CONTROLTASK_BOOT_X_M;
  xOdom.y = CONTROLTASK_BOOT_Y_M;
  xOdom.theta = fSideHeading(CONTROLTASK_BOOT_FACING);
  return xTaskCreatePinnedToCore(vControlTask, "control",
                                 CONTROLTASK_STACK_BYTES, NULL,
                                 CONTROLTASK_PRIORITY, NULL,
                                 CONTROLTASK_CORE) == pdPASS;
}
