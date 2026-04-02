/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bee_timer_common.h"
#include <soc.h>

#if defined(CONFIG_HAL_REALTEK_BEE_TIM)

/* Basic Timer Implementations */

static void basic_tim_init(uint32_t reg, uint8_t prescaler_idx, uint32_t top_val,
			   enum bee_timer_mode mode)
{
	TIM_TimeBaseInitTypeDef timer_init_struct;

	TIM_StructInit(&timer_init_struct);

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
	timer_init_struct.TIM_ClockSrc = CK_40M_TIMER;
	timer_init_struct.TIM_SOURCE_DIV_En = ENABLE;
#endif

	timer_init_struct.TIM_SOURCE_DIV = prescaler_idx;
	timer_init_struct.TIM_Mode = TIM_Mode_UserDefine;

	if (mode == BEE_TIMER_MODE_PWM) {
		timer_init_struct.TIM_PWM_En = ENABLE;
		timer_init_struct.TIM_PWM_High_Count = 0;
		timer_init_struct.TIM_PWM_Low_Count = UINT32_MAX;
	} else {
		timer_init_struct.TIM_Period = top_val;
	}

	TIM_TimeBaseInit((TIM_TypeDef *)reg, &timer_init_struct);
}

static void basic_tim_start(uint32_t reg)
{
	TIM_Cmd((TIM_TypeDef *)reg, ENABLE);
}

static void basic_tim_stop(uint32_t reg)
{
	TIM_Cmd((TIM_TypeDef *)reg, DISABLE);
}

static uint32_t basic_tim_get_count(uint32_t reg)
{
	return TIM_GetCurrentValue((TIM_TypeDef *)reg);
}

static uint32_t basic_tim_get_top(uint32_t reg)
{
	return TIM_GetCurrentLoadCnt(reg);
}

static void basic_tim_set_top(uint32_t reg, uint32_t top_val)
{
	TIM_ChangePeriod((TIM_TypeDef *)reg, top_val);
}

static void basic_tim_int_enable(uint32_t reg)
{
	TIM_INTConfig((TIM_TypeDef *)reg, ENABLE);
}

static void basic_tim_int_disable(uint32_t reg)
{
	TIM_INTConfig((TIM_TypeDef *)reg, DISABLE);
}

static void basic_tim_int_clear(uint32_t reg)
{
	TIM_ClearINT((TIM_TypeDef *)reg);
}

static bool basic_tim_int_status(uint32_t reg)
{
	return TIM_GetINTStatus((TIM_TypeDef *)reg) ? true : false;
}

static void basic_tim_set_pwm_duty(uint32_t reg, uint32_t period_cyc, uint32_t pulse_cyc,
				   bool inverted)
{
	uint32_t high, low;

	/* Basic Timer uses High Count vs Low Count logic */
	if (inverted) {
		if (period_cyc == 0U || pulse_cyc == 0U) {
			high = UINT32_MAX;
			low = 0; /* duty = 0% */
		} else if (period_cyc == pulse_cyc) {
			high = 0;
			low = UINT32_MAX; /* duty = 100% */
		} else {
			high = period_cyc - pulse_cyc;
			low = pulse_cyc;
		}
	} else {
		if (period_cyc == 0U || pulse_cyc == 0U) {
			high = 0;
			low = UINT32_MAX;
		} else if (period_cyc == pulse_cyc) {
			high = UINT32_MAX;
			low = 0;
		} else {
			high = pulse_cyc;
			low = period_cyc - pulse_cyc;
		}
	}
	TIM_PWMChangeFreqAndDuty((TIM_TypeDef *)reg, high, low);
}

static const struct bee_timer_ops bee_basic_ops = {
	.init = basic_tim_init,
	.start = basic_tim_start,
	.stop = basic_tim_stop,
	.get_count = basic_tim_get_count,
	.get_top = basic_tim_get_top,
	.set_top = basic_tim_set_top,
	.set_pwm_duty = basic_tim_set_pwm_duty,
	.int_enable = basic_tim_int_enable,
	.int_disable = basic_tim_int_disable,
	.int_clear = basic_tim_int_clear,
	.int_status = basic_tim_int_status,
};
#endif /* CONFIG_HAL_REALTEK_BEE_TIM */

#if defined(CONFIG_HAL_REALTEK_BEE_ENHTIM)

/* Enhanced Timer Implementations */

static void enh_tim_init(uint32_t reg, uint8_t prescaler_idx, uint32_t top_val,
			 enum bee_timer_mode mode)
{
	ENHTIM_InitTypeDef enh_tim_init_struct;

	ENHTIM_StructInit(&enh_tim_init_struct);

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
	enh_tim_init_struct.ENHTIM_ClockSource = CK_40M_TIMER;
	enh_tim_init_struct.ENHTIM_ClockDiv_En = ENABLE;
	if (mode == BEE_TIMER_MODE_PWM) {
		enh_tim_init_struct.ENHTIM_PWMOutputEn = ENABLE;
	}
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
	if (mode == BEE_TIMER_MODE_PWM) {
		enh_tim_init_struct.ENHTIM_PWMOutputEn = ENHTIM_PWM_ENABLE;
	}
#endif

	enh_tim_init_struct.ENHTIM_ClockDiv = prescaler_idx;
	enh_tim_init_struct.ENHTIM_Mode =
		ENHTIM_MODE_PWM_MANUAL; /* Used for both Count and PWM modes */
	enh_tim_init_struct.ENHTIM_MaxCount = top_val;

	if (mode == BEE_TIMER_MODE_PWM) {
		enh_tim_init_struct.ENHTIM_PWMStartPolarity = ENHTIM_PWM_START_WITH_LOW;
		enh_tim_init_struct.ENHTIM_CCValue = UINT32_MAX;
	}

	ENHTIM_Init((ENHTIM_TypeDef *)reg, &enh_tim_init_struct);
}

static void enh_tim_start(uint32_t reg)
{
	ENHTIM_Cmd((ENHTIM_TypeDef *)reg, ENABLE);
}

static void enh_tim_stop(uint32_t reg)
{
	ENHTIM_Cmd((ENHTIM_TypeDef *)reg, DISABLE);
}

static uint32_t enh_tim_get_count(uint32_t reg)
{
	return ENHTIM_GetCurrentCount((ENHTIM_TypeDef *)reg);
}

static uint32_t enh_tim_get_top(uint32_t reg)
{
	return ENHTIM_GetCurrentMAXCNT(reg);
}

static void enh_tim_set_top(uint32_t reg, uint32_t top_val)
{
	ENHTIM_SetMaxCount((ENHTIM_TypeDef *)reg, top_val);
}

static void enh_tim_int_enable(uint32_t reg)
{
	ENHTIM_INTConfig((ENHTIM_TypeDef *)reg, ENHTIM_INT_TIM, ENABLE);
}

static void enh_tim_int_disable(uint32_t reg)
{
	ENHTIM_INTConfig((ENHTIM_TypeDef *)reg, ENHTIM_INT_TIM, DISABLE);
}

static void enh_tim_int_clear(uint32_t reg)
{
	ENHTIM_ClearINTPendingBit((ENHTIM_TypeDef *)reg, ENHTIM_INT_TIM);
}

static bool enh_tim_int_status(uint32_t reg)
{
	return ENHTIM_GetINTStatus((ENHTIM_TypeDef *)reg, ENHTIM_INT_TIM) ? true : false;
}

static void enh_tim_set_pwm_duty(uint32_t reg, uint32_t period_cyc, uint32_t pulse_cyc,
				 bool inverted)
{
	uint32_t cc_val, max_val;

	if (inverted) {
		if (period_cyc == 0U || pulse_cyc == 0U) {
			max_val = UINT32_MAX - 1;
			cc_val = 0;
		} else if (period_cyc == pulse_cyc) {
			max_val = UINT32_MAX - 1;
			cc_val = UINT32_MAX;
		} else {
			max_val = period_cyc;
			cc_val = period_cyc - pulse_cyc;
		}
	} else {
		if (period_cyc == 0U || pulse_cyc == 0U) {
			max_val = UINT32_MAX - 1;
			cc_val = UINT32_MAX;
		} else if (period_cyc == pulse_cyc) {
			max_val = UINT32_MAX - 1;
			cc_val = 0;
		} else {
			max_val = period_cyc;
			cc_val = pulse_cyc;
		}
	}

	ENHTIM_SetMaxCount((ENHTIM_TypeDef *)reg, max_val);
	ENHTIM_SetCCValue((ENHTIM_TypeDef *)reg, cc_val);
}

static const struct bee_timer_ops bee_enh_ops = {
	.init = enh_tim_init,
	.start = enh_tim_start,
	.stop = enh_tim_stop,
	.get_count = enh_tim_get_count,
	.get_top = enh_tim_get_top,
	.set_top = enh_tim_set_top,
	.set_pwm_duty = enh_tim_set_pwm_duty,
	.int_enable = enh_tim_int_enable,
	.int_disable = enh_tim_int_disable,
	.int_clear = enh_tim_int_clear,
	.int_status = enh_tim_int_status,
};
#endif /* CONFIG_HAL_REALTEK_BEE_ENHTIM */

/* Public API Implementation */
const struct bee_timer_ops *bee_timer_get_ops(bool enhanced)
{
	if (enhanced) {
#if defined(CONFIG_HAL_REALTEK_BEE_ENHTIM)
		return &bee_enh_ops;
#else
		return NULL;
#endif
	} else {
#if defined(CONFIG_HAL_REALTEK_BEE_TIM)
		return &bee_basic_ops;
#else
		return NULL;
#endif
	}
}
