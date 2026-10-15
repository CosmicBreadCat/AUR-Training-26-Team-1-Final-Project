#ifndef I2C_CFG_H_
#define I2C_CFG_H_

/* Default I2C Hardware Pins */
#define I2C_MASTER_SDA_PIN      21
#define I2C_MASTER_SCL_PIN      22

/* Clock Speed (400 kHz Fast Mode) */
#define I2C_MASTER_FREQ_HZ      400000

/* Driver Transaction Timeout in milliseconds */
#define I2C_MASTER_TIMEOUT_MS   1000

#endif /* I2C_CFG_H_ */