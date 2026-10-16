#ifndef MOTOR_CFG_H_
#define MOTOR_CFG_H_

/* L298N pin mapping — placeholders; assign final GPIOs later. */
#define MOTOR_LEFT_IN1_PIN   25
#define MOTOR_LEFT_IN2_PIN   23
#define MOTOR_RIGHT_IN1_PIN  18
#define MOTOR_RIGHT_IN2_PIN  19

#define MOTOR_LEFT_PWM_PIN   32
#define MOTOR_RIGHT_PWM_PIN  33

/* PWM configuration */
#define MOTOR_PWM_FREQ_HZ    20000
#define MOTOR_PWM_MAX_DUTY   1023

#endif