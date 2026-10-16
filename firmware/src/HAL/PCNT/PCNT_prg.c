#include "PCNT_int.h"
#include "PCNT_cfg.h"

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
}

int16_t PCNT_s16GetCount(PCNT_Unit_t Copy_xUnit) {
    int16_t s16Count = 0;
    pcnt_get_counter_value((pcnt_unit_t)Copy_xUnit, &s16Count);
    return s16Count;
}

void PCNT_vClearCount(PCNT_Unit_t Copy_xUnit) {
    pcnt_counter_clear((pcnt_unit_t)Copy_xUnit);
}