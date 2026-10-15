#ifndef COMM_TASK_INT_H_
#define COMM_TASK_INT_H_

#include "../../SERVICE/PROTOCOL/PROTOCOL_int.h"
#include "COMM_TASK_cfg.h"
#include <stdint.h>

/* What the drive should currently be doing, as decided by the laptop link.
 * A change in mode or cmdSeq means a new command. */
typedef struct {
  RobotMode mode;
  float v;           // m/s, 0 when the twist input is stale
  float omega;       // rad/s, 0 when the twist input is stale
  ModeParams params; // the mode command's parameters (NAN = not given)
  uint32_t cmdSeq;   // seq of the mode command that set `mode`
  bool hasSquareRef; // SQUARE: the driver named the edge (square_ref)
  Side squareRef;    // SQUARE: that edge
} MotionCommand_t;

/* Where the gripper should be. Independent of the drive mode. */
typedef struct {
  LiftState lift;
  GripState grip;
  uint32_t cmdSeq; // seq of the last gripper command (0 = boot state)
} GripperCommand_t;

/* Progress of a control-task mode, reported back for the status message.
 * phase and reason must point to string literals. */
typedef struct {
  uint32_t cmdSeq; // the MotionCommand_t.cmdSeq this result is about
  Result result;
  const char *phase;
  const char *reason;
} MotionResult_t;

/* Function Prototypes */

/**
 * @brief Joins the router and starts the UDP link and the comms task on
 * COMMTASK_CORE.
 * @return false if the link or the task could not be started.
 */
bool COMMTASK_bStart(void);

/**
 * @brief Reads the latest motion command. Safe to call from any task.
 * @param Copy_pxCommand Filled with the current command.
 * @return false if the comms task has not been started.
 */
bool COMMTASK_bGetCommand(MotionCommand_t *Copy_pxCommand);

/**
 * @brief Reads the latest gripper command. Safe to call from any task.
 * @param Copy_pxGripper Filled with the current gripper command.
 * @return false if the comms task has not been started.
 */
bool COMMTASK_bGetGripper(GripperCommand_t *Copy_pxGripper);

/**
 * @brief Reports the progress of the current mode (phase, DONE, FAILED).
 *        Safe to call from any task; never blocks.
 * @param Copy_pxResult Result tagged with the cmdSeq it belongs to.
 */
void COMMTASK_vSetMotionResult(const MotionResult_t *Copy_pxResult);

/**
 * @brief Hands the latest odometry to the comms task for sending.
 *        Safe to call from any task; never blocks.
 * @param Copy_pxOdometry Current pose and velocity.
 */
void COMMTASK_vSetOdometry(const Odometry *Copy_pxOdometry);

/**
 * @brief Hands the latest sensor readings to the comms task for sending.
 *        Safe to call from any task; never blocks.
 * @param Copy_pxSensors Current readings (NAN = no reading).
 */
void COMMTASK_vSetSensors(const Sensors *Copy_pxSensors);

#endif /* COMM_TASK_INT_H_ */
