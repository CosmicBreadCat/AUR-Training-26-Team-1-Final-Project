#ifndef MOTOR_INT_H_
#define MOTOR_INT_H_

#include <stdint.h>

void MOTOR_vInit(void);

/*
 * Signed duty:
 *   positive = forward
 *   negative = reverse
 *   zero     = stop
 *
 * Range: -1023 to +1023
 */
void MOTOR_vSetLeft(int16_t Copy_s16Duty);
void MOTOR_vSetRight(int16_t Copy_s16Duty);
void MOTOR_vStop(void);

#endif