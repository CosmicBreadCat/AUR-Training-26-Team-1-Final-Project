#ifndef CONTROL_TASK_CFG_H_
#define CONTROL_TASK_CFG_H_

/* FreeRTOS task settings. Core 1 is reserved for this task; it must be the
 * highest priority there so nothing delays its tick. */
#define CONTROLTASK_CORE 1
#define CONTROLTASK_PRIORITY 10
#define CONTROLTASK_STACK_BYTES 4096
#define CONTROLTASK_PERIOD_MS 1 // 1 kHz control loop

#endif /* CONTROL_TASK_CFG_H_ */
