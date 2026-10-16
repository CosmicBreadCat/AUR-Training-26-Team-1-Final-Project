#ifndef LEDC_CFG_H_
#define LEDC_CFG_H_

/*
 * Default PWM Frequencies and Resolutions
 */

/* Motor PWM Configuration (High Frequency to eliminate motor hum) */
#define LEDC_MOTOR_PWM_FREQ_HZ       20000     // 20 kHz PWM
#define LEDC_MOTOR_PWM_RES           LEDC_TIMER_10_BIT // 0 to 1023 duty cycle resolution

/* Servo PWM Configuration (Standard 50Hz RC Servo signal) */
#define LEDC_SERVO_PWM_FREQ_HZ       50        // 50 Hz PWM (20ms period)
#define LEDC_SERVO_PWM_RES           LEDC_TIMER_13_BIT // 0 to 8191 duty cycle resolution

/* Servo Pulse Limits in microseconds */
#define LEDC_SERVO_MIN_PULSE_US      500       // 0 degrees (~0.5ms)
#define LEDC_SERVO_MAX_PULSE_US      2500      // 180 degrees (~2.5ms)

#endif /* LEDC_CFG_H_ */