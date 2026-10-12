#ifndef COMM_CFG_H_
#define COMM_CFG_H_

/* No valid packet for this long -> link is considered dead */
#define COMM_WATCHDOG_MS 300

/* Robot -> laptop message rates */
#define COMM_ODOMETRY_PERIOD_MS 50 // 20 Hz
#define COMM_STATUS_PERIOD_MS 200  // 5 Hz, plus immediately on change

#endif /* COMM_CFG_H_ */
