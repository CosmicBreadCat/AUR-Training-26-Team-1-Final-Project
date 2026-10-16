#ifndef PCNT_INT_H_
#define PCNT_INT_H_

#include <stdint.h>
#include <stdbool.h>
#include "driver/pcnt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PCNT Hardware Units */
typedef enum {
    PCNT_UNIT_INDEX_0 = PCNT_UNIT_0,
    PCNT_UNIT_INDEX_1 = PCNT_UNIT_1,
    PCNT_UNIT_INDEX_2 = PCNT_UNIT_2,
    PCNT_UNIT_INDEX_3 = PCNT_UNIT_3,
    PCNT_UNIT_INDEX_4 = PCNT_UNIT_4,
    PCNT_UNIT_INDEX_5 = PCNT_UNIT_5,
    PCNT_UNIT_INDEX_6 = PCNT_UNIT_6,
    PCNT_UNIT_INDEX_7 = PCNT_UNIT_7
} PCNT_Unit_t;

/* Function Prototypes */

/**
 * @brief Configures a PCNT unit for full 4x Quadrature Encoder decoding.
 * @param Copy_xUnit PCNT hardware unit index (0-7).
 * @param Copy_s32PulseGpio GPIO pin connected to Encoder Phase A.
 * @param Copy_s32CtrlGpio GPIO pin connected to Encoder Phase B.
 */
void PCNT_vInitEncoder(PCNT_Unit_t Copy_xUnit, int Copy_s32PulseGpio, int Copy_s32CtrlGpio);

/**
 * @brief Reads the current 16-bit hardware counter value.
 * @param Copy_xUnit Target PCNT unit.
 * @return int16_t Current pulse count value.
 */
int16_t PCNT_s16GetCount(PCNT_Unit_t Copy_xUnit);

/**
 * @brief Resets the PCNT hardware counter value to zero. Also resets the
 *        reference used by PCNT_s32GetDelta, so the next delta stays correct.
 * @param Copy_xUnit Target PCNT unit.
 */
void PCNT_vClearCount(PCNT_Unit_t Copy_xUnit);

/**
 * @brief Flips the counting direction seen by PCNT_s32GetDelta/GetTotal, so
 *        that driving forward counts positive (mirrored motors need this).
 * @param Copy_xUnit Target PCNT unit.
 * @param Copy_bInverted true to negate the counts.
 */
void PCNT_vSetInverted(PCNT_Unit_t Copy_xUnit, bool Copy_bInverted);

/**
 * @brief Counts moved since the previous call for this unit (sign-corrected).
 *        Handles the hardware counter reset at +-PCNT_H_LIM_VAL and loses no
 *        counts, because the counter is never cleared. Call it exactly once
 *        per sample period per unit: each call consumes the movement.
 * @param Copy_xUnit Target PCNT unit.
 * @return int32_t Counts since the last call (positive = forward).
 */
int32_t PCNT_s32GetDelta(PCNT_Unit_t Copy_xUnit);

/**
 * @brief Total counts accumulated by PCNT_s32GetDelta since init (32-bit).
 *        Only changes when PCNT_s32GetDelta is called.
 * @param Copy_xUnit Target PCNT unit.
 */
int32_t PCNT_s32GetTotal(PCNT_Unit_t Copy_xUnit);

#ifdef __cplusplus
}
#endif

#endif /* PCNT_INT_H_ */