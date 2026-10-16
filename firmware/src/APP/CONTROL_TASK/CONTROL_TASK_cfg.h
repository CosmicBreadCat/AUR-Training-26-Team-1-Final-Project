#ifndef CONTROL_TASK_CFG_H_
#define CONTROL_TASK_CFG_H_

/* FreeRTOS task settings. Core 1 is reserved for this task; it must be the
 * highest priority there so nothing delays its tick. */
#define CONTROLTASK_CORE 1
#define CONTROLTASK_PRIORITY 10
#define CONTROLTASK_STACK_BYTES 4096
#define CONTROLTASK_PERIOD_MS 1 // 1 kHz control loop

/*
 * Arena frame: origin at the bottom-left (south-west) corner, x towards EAST,
 * y towards NORTH, theta from +x, counter-clockwise positive.
 *
 * PLACEHOLDER VALUES: measure the real ones and replace.
 */
#define CONTROLTASK_ARENA_SIZE_M 3.0f         // side length of the square arena
#define CONTROLTASK_IR_FORWARD_OFFSET_M 0.10f // IR sensor line ahead of center

/* Rough start pose in the bottom-left corner, used until squaring fixes it */
#define CONTROLTASK_BOOT_X_M 0.15f
#define CONTROLTASK_BOOT_Y_M 0.15f
#define CONTROLTASK_BOOT_FACING Side::North

/* Drivetrain geometry, for inverse kinematics. PLACEHOLDERS: measure. */
#define CONTROLTASK_WHEEL_RADIUS_M 0.026f
#define CONTROLTASK_TRACK_WIDTH_M 0.27f
#define CONTROLTASK_ENCODER_COUNTS_PER_WHEEL_REV 2024.0f
#define CONTROLTASK_MAX_WHEEL_RAD_S 9.5f
/* Front-corner IR line sensors (edge guard, SQUARE). Not read yet: an
 * unconnected pin floats and would trip the guard at random.
 * TODO set the real pins and the level that means "on the line", e.g.
 * #define CONTROLTASK_IR_LEFT_PIN GPIO_NUM_xx
 * #define CONTROLTASK_IR_RIGHT_PIN GPIO_NUM_xx
 * #define CONTROLTASK_IR_ON_LINE_LEVEL 1 */

#endif /* CONTROL_TASK_CFG_H_ */
