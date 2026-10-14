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
#define CONTROLTASK_ARENA_SIZE_M 2.0f         // side length of the square arena
#define CONTROLTASK_IR_FORWARD_OFFSET_M 0.10f // IR sensor line ahead of center

/* Rough start pose in the bottom-left corner, used until squaring fixes it */
#define CONTROLTASK_BOOT_X_M 0.15f
#define CONTROLTASK_BOOT_Y_M 0.15f
#define CONTROLTASK_BOOT_FACING Side::North

#endif /* CONTROL_TASK_CFG_H_ */
