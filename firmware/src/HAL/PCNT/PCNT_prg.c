#include "PCNT_int.h"
#include "PCNT_cfg.h"

#define PCNT_UNIT_COUNT 8

/* Per-unit state for PCNT_s32GetDelta / PCNT_s32GetTotal */
static int16_t as16Last[PCNT_UNIT_COUNT];
static int32_t as32Total[PCNT_UNIT_COUNT];
static int8_t  as8Sign[PCNT_UNIT_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1};

void PCNT_vInitEncoder(PCNT_Unit_t Copy_xUnit, int Copy_s32PulseGpio, int Copy_s32CtrlGpio) {
    /* Configure Channel 0 (Phase A as Pulse, Phase B as Control) */
    pcnt_config_t xPcntConfigChan0 = {
        .pulse_gpio_num = Copy_s32PulseGpio,
        .ctrl_gpio_num  = Copy_s32CtrlGpio,
        .lctrl_mode     = PCNT_MODE_REVERSE, // Reverse count direction when Ctrl is Low
        .hctrl_mode     = PCNT_MODE_KEEP,    // Keep count direction when Ctrl is High
        .pos_mode       = PCNT_COUNT_INC,   // Count up on rising edge
        .neg_mode       = PCNT_COUNT_DEC,   // Count down on falling edge
        .counter_h_lim  = PCNT_H_LIM_VAL,
        .counter_l_lim  = PCNT_L_LIM_VAL,
        .unit           = (pcnt_unit_t)Copy_xUnit,
        .channel        = PCNT_CHANNEL_0
    };
    pcnt_unit_config(&xPcntConfigChan0);

    /* Configure Channel 1 (Phase B as Pulse, Phase A as Control for 4x resolution) */
    pcnt_config_t xPcntConfigChan1 = {
        .pulse_gpio_num = Copy_s32CtrlGpio,
        .ctrl_gpio_num  = Copy_s32PulseGpio,
        .lctrl_mode     = PCNT_MODE_KEEP,    // Keep count direction when Ctrl is Low
        .hctrl_mode     = PCNT_MODE_REVERSE, // Reverse count direction when Ctrl is High
        .pos_mode       = PCNT_COUNT_INC,   // Count up on rising edge
        .neg_mode       = PCNT_COUNT_DEC,   // Count down on falling edge
        .counter_h_lim  = PCNT_H_LIM_VAL,
        .counter_l_lim  = PCNT_L_LIM_VAL,
        .unit           = (pcnt_unit_t)Copy_xUnit,
        .channel        = PCNT_CHANNEL_1
    };
    pcnt_unit_config(&xPcntConfigChan1);

    /* Hardware Glitch Filter Setup */
    pcnt_set_filter_value((pcnt_unit_t)Copy_xUnit, PCNT_GLITCH_FILTER_VAL);
    pcnt_filter_enable((pcnt_unit_t)Copy_xUnit);

    /* Clear and start counter */
    pcnt_counter_pause((pcnt_unit_t)Copy_xUnit);
    pcnt_counter_clear((pcnt_unit_t)Copy_xUnit);
    pcnt_counter_resume((pcnt_unit_t)Copy_xUnit);

    /* Fresh reference for the delta/total functions */
    if ((unsigned)Copy_xUnit < PCNT_UNIT_COUNT) {
        as16Last[Copy_xUnit] = 0;
        as32Total[Copy_xUnit] = 0;
    }
}

int16_t PCNT_s16GetCount(PCNT_Unit_t Copy_xUnit) {
    int16_t s16Count = 0;
    pcnt_get_counter_value((pcnt_unit_t)Copy_xUnit, &s16Count);
    return s16Count;
}

void PCNT_vClearCount(PCNT_Unit_t Copy_xUnit) {
    pcnt_counter_clear((pcnt_unit_t)Copy_xUnit);
    if ((unsigned)Copy_xUnit < PCNT_UNIT_COUNT) {
        as16Last[Copy_xUnit] = 0;   /* keep the next delta correct */
    }
}

void PCNT_vSetInverted(PCNT_Unit_t Copy_xUnit, bool Copy_bInverted) {
    if ((unsigned)Copy_xUnit < PCNT_UNIT_COUNT) {
        as8Sign[Copy_xUnit] = Copy_bInverted ? -1 : 1;
    }
}

int32_t PCNT_s32GetDelta(PCNT_Unit_t Copy_xUnit) {
    if ((unsigned)Copy_xUnit >= PCNT_UNIT_COUNT) {
        return 0;
    }

    int16_t s16Now = PCNT_s16GetCount(Copy_xUnit);
    int32_t s32Delta = (int32_t)s16Now - (int32_t)as16Last[Copy_xUnit];
    as16Last[Copy_xUnit] = s16Now;

    /* The hardware counter resets to 0 when it reaches +-PCNT_H_LIM_VAL,
     * which shows up as a huge jump. Undo it. */
    if (s32Delta > PCNT_H_LIM_VAL / 2) {
        s32Delta -= PCNT_H_LIM_VAL;
    } else if (s32Delta < -(PCNT_H_LIM_VAL / 2)) {
        s32Delta += PCNT_H_LIM_VAL;
    }

    s32Delta *= as8Sign[Copy_xUnit];
    as32Total[Copy_xUnit] += s32Delta;
    return s32Delta;
}

int32_t PCNT_s32GetTotal(PCNT_Unit_t Copy_xUnit) {
    return ((unsigned)Copy_xUnit < PCNT_UNIT_COUNT) ? as32Total[Copy_xUnit] : 0;
}