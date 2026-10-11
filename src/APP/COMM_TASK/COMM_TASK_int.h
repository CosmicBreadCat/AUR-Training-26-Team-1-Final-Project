#ifndef COMM_TASK_INT_H_
#define COMM_TASK_INT_H_

#include <stdint.h>
#include "COMM_TASK_cfg.h"
#include "../../SERVICE/PROTOCOL/PROTOCOL_int.h"

/* What the robot should currently be doing, as decided by the laptop link */
typedef struct {
    RobotMode mode;
    float     v;       // m/s, 0 when the twist input is stale
    float     omega;   // rad/s, 0 when the twist input is stale
    uint32_t  cmdSeq;  // seq of the mode command that set `mode`
} MotionCommand_t;

/* Function Prototypes */

/**
 * @brief Starts the SoftAP/UDP link and the comms task on COMMTASK_CORE.
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
 * @brief Hands the latest odometry to the comms task for sending.
 *        Safe to call from any task; never blocks.
 * @param Copy_pxOdometry Current pose and velocity.
 */
void COMMTASK_vSetOdometry(const Odometry *Copy_pxOdometry);

#endif /* COMM_TASK_INT_H_ */
