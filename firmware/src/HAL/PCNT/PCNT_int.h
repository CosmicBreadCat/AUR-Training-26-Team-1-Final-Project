#ifndef PCNT_INT_H_
#define PCNT_INT_H_

#include <stdint.h>
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
 * @brief Resets the PCNT hardware counter value to zero.
 * @param Copy_xUnit Target PCNT unit.
 */
void PCNT_vClearCount(PCNT_Unit_t Copy_xUnit);

#ifdef __cplusplus
}
#endif

#endif /* PCNT_INT_H_ */