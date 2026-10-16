#ifndef PCNT_CFG_H_
#define PCNT_CFG_H_

/* Maximum and minimum hardware counter limits before reset/overflow */
#define PCNT_H_LIM_VAL        30000
#define PCNT_L_LIM_VAL       -30000

/* Glitch Filter Threshold (Ignores noise pulses shorter than 100 APB clock cycles) */
#define PCNT_GLITCH_FILTER_VAL  100

/* Default Encoder Pin Assignments */
#define ENCODER_LEFT_A_PIN     34
#define ENCODER_LEFT_B_PIN     35

#define ENCODER_RIGHT_A_PIN    36
#define ENCODER_RIGHT_B_PIN    39

/* Which PCNT hardware unit reads which wheel */
#define ENCODER_LEFT_UNIT      PCNT_UNIT_INDEX_0
#define ENCODER_RIGHT_UNIT     PCNT_UNIT_INDEX_1

/* 1 flips the sign so that driving forward counts positive. Mirrored motors
 * usually need the right one flipped. Test by rolling the robot forward by hand. */
#define ENCODER_LEFT_INVERTED  0
#define ENCODER_RIGHT_INVERTED 1

/* Counts per WHEEL revolution as seen by PCNT (4x decoding).
 * = encoder pulses per motor rev (one phase) * 4 * gearbox ratio.
 * PLACEHOLDER: measure it (turn the wheel exactly one turn, read PCNT_s32GetTotal). */
#define ENCODER_COUNTS_PER_WHEEL_REV   1320.0f

#endif /* PCNT_CFG_H_ */