#ifndef MOTOR_INT_H_
#define MOTOR_INT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOTOR_LEFT = 0,
    MOTOR_RIGHT = 1,
    MOTOR_COUNT = 2
} Motor_Side_t;

/** @brief Sets up the PWM timer, the PWM channels and the direction pins. Motors start stopped. */
void MOTOR_vInit(void);

/**
 * @brief Drives one motor.
 * @param Copy_xSide MOTOR_LEFT or MOTOR_RIGHT.
 * @param Copy_fDuty -1.0 (full reverse) .. +1.0 (full forward). 0 = coast. NaN = 0.
 */
void MOTOR_vSetDuty(Motor_Side_t Copy_xSide, float Copy_fDuty);

/** @brief Coasts one motor (bridge off, wheel spins freely). */
void MOTOR_vStop(Motor_Side_t Copy_xSide);

/** @brief Shorts the motor terminals so the wheel stops quickly. */
void MOTOR_vBrake(Motor_Side_t Copy_xSide);

/** @brief Coasts both motors. */
void MOTOR_vStopAll(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_INT_H_ */
