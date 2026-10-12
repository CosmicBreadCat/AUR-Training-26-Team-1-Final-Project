#ifndef COMM_TASK_CFG_H_
#define COMM_TASK_CFG_H_

/* FreeRTOS task settings. Core 0 is shared with the WiFi stack;
 * core 1 is reserved for the control loop. */
#define COMMTASK_CORE 0
#define COMMTASK_PRIORITY 5
#define COMMTASK_STACK_BYTES 6144
#define COMMTASK_PERIOD_MS 1 // how often packets are polled

/* In TWIST, a twist older than this counts as stale and v, omega drop to 0,
 * even if heartbeats keep the link alive. */
#define COMMTASK_TWIST_STALE_MS 300

/* Serial log of the current twist while in TWIST */
#define COMMTASK_TWIST_LOG_MS 500

#endif /* COMM_TASK_CFG_H_ */
