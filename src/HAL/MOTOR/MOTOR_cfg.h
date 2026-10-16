#ifndef MOTOR_CFG_H_
#define MOTOR_CFG_H_

/*
 * H-bridge wiring: two direction inputs + one PWM (enable) pin per motor.
 * VERIFY against your PCB. These must not collide with: I2C (21, 22),
 * ultrasonic (26, 27), encoders (34, 35, 36, 39), servos (32, 33).
 * Delete the four MOTOR_*_DIR*_PIN lines in GPIO_cfg.h: pin 22 clashes with I2C SCL.
 */
#define MOTOR_LEFT_IN1_PIN    18
#define MOTOR_LEFT_IN2_PIN    19
#define MOTOR_LEFT_PWM_PIN    25

#define MOTOR_RIGHT_IN1_PIN   23
#define MOTOR_RIGHT_IN2_PIN   13
#define MOTOR_RIGHT_PWM_PIN   4

/* 1 flips a motor's direction. Set so that positive duty = robot forward.
 * Mirrored motors usually need the right one flipped. Test with wheels lifted. */
#define MOTOR_LEFT_INVERTED   0
#define MOTOR_RIGHT_INVERTED  1

/* LEDC resources (frequency/resolution come from PWM_cfg.h) */
#define MOTOR_PWM_SPEED_MODE  PWM_MODE_LOW_SPEED
#define MOTOR_PWM_TIMER       PWM_TIMER0
#define MOTOR_LEFT_PWM_CH     PWM_CH0
#define MOTOR_RIGHT_PWM_CH    PWM_CH1

#endif /* MOTOR_CFG_H_ */
