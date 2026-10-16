#ifndef CONTROL_TASK_INT_H_
#define CONTROL_TASK_INT_H_

#include "CONTROL_TASK_cfg.h"

/* Function Prototypes */

/**
 * @brief Starts the control task on CONTROLTASK_CORE. It reads commands from
 *        and reports results/odometry/sensors to the comms task's mailboxes,
 *        so start the comms task first.
 * @return false if the task could not be created.
 */
bool CONTROLTASK_bStart(void);

#endif /* CONTROL_TASK_INT_H_ */
