#ifndef GRIPPER_CFG_H_
#define GRIPPER_CFG_H_

/* Servo signal pins (avoid boot-strapping pins). Power the servos from their
 * own 5-6 V supply and join its ground to the ESP32 ground. */
#define GRIPPER_GRIP_PIN        32   /* servo that closes/opens the fingers */
#define GRIPPER_LIFT_PIN        33   /* servo that tilts the whole gripper  */

/* LEDC resources: must be a different timer than the motors (50 Hz vs 20 kHz) */
#define GRIPPER_PWM_SPEED_MODE  PWM_MODE_LOW_SPEED
#define GRIPPER_PWM_TIMER       PWM_TIMER1
#define GRIPPER_GRIP_PWM_CH     PWM_CH2
#define GRIPPER_LIFT_PWM_CH     PWM_CH3

/* Servo angles in degrees. PLACEHOLDERS: find the real ones with the mechanism
 * assembled. Stay inside the mechanical limits so a servo never pushes against a
 * hard stop, and pick CLOSED so the 6 cm cardboard box is held without being crushed. */
#define GRIPPER_GRIP_OPEN_DEG     40.0f
#define GRIPPER_GRIP_CLOSED_DEG   110.0f
#define GRIPPER_LIFT_DOWN_DEG     20.0f
#define GRIPPER_LIFT_UP_DEG       120.0f

/* Pose at power-on, normalized 0..1 (grip: 0 open, 1 closed; lift: 0 down, 1 up) */
#define GRIPPER_START_GRIP        0.0f
#define GRIPPER_START_LIFT        1.0f

/* Maximum servo speed (deg/s), so the box is not thrown around */
#define GRIPPER_GRIP_SPEED_DPS    180.0f
#define GRIPPER_LIFT_SPEED_DPS    90.0f

#endif /* GRIPPER_CFG_H_ */
