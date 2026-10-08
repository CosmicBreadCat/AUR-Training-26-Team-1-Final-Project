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

#endif /* PCNT_CFG_H_ */