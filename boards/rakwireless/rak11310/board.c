/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/rpi-pico-pinctrl-common.h>
#include <zephyr/dt-bindings/pinctrl/rpi-pico-rp2040-pinctrl.h>

#define WISBLOCK_MARK_IF_ELEM_SELECTS(node, prop, idx, ctlr, cell, sel)			\
	COND_CODE_1(DT_SAME_NODE(DT_PHANDLE_BY_IDX(node, prop, idx), ctlr),		\
		(COND_CODE_1(IS_EQ(DT_PHA_BY_IDX(node, prop, idx, cell), sel),		\
			(x), ())),							\
		())

#define WISBLOCK_MARK_IF_NODE_SELECTS(node, prop, ctlr, cell, sel)			\
	COND_CODE_1(DT_NODE_HAS_PROP(node, prop),					\
		(DT_FOREACH_PROP_ELEM_VARGS(node, prop,					\
					    WISBLOCK_MARK_IF_ELEM_SELECTS,		\
					    ctlr, cell, sel)),				\
		())

#define WISBLOCK_HAS_CONSUMER(prop, ctlr, cell, sel)					\
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE_VARGS(				\
		WISBLOCK_MARK_IF_NODE_SELECTS, prop, ctlr, cell, sel)))

#define WISBLOCK_PINMUX(pm)								\
	{										\
		.pin_num = RP2_GET_PIN_NUM(pm),						\
		.alt_func = RP2_GET_PIN_ALT_FUNC(pm),					\
	}

#define WISBLOCK_ADC_IN_PIN(ch, pm)							\
	COND_CODE_1(WISBLOCK_HAS_CONSUMER(io_channels, DT_NODELABEL(adc), input, ch),	\
		(WISBLOCK_PINMUX(pm),), ())

#define WISBLOCK_PWM_OUT_PIN(ch, pm)							\
	COND_CODE_1(WISBLOCK_HAS_CONSUMER(pwms, DT_NODELABEL(pwm), channel, ch),	\
		(WISBLOCK_PINMUX(pm),), ())

/*
 * pinctrl_update_states() rejects an array whose length differs from the one
 * devicetree produced, and PINCTRL_SKIP_SLEEP drops the sleep state from that
 * one, so the sleep entry has to follow the same condition.
 */
#define WISBLOCK_PINCTRL_STATES(name)							\
	PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(name));				\
	static const struct pinctrl_state name##_states[] = {				\
		{.pins = name##_pins, .pin_cnt = ARRAY_SIZE(name##_pins),		\
		 .id = PINCTRL_STATE_DEFAULT},						\
		IF_ENABLED(CONFIG_PINCTRL_KEEP_SLEEP_STATE,				\
			({.pins = name##_sleep,						\
			  .pin_cnt = ARRAY_SIZE(name##_sleep),				\
			  .id = PINCTRL_STATE_SLEEP},))					\
	}

#define WISBLOCK_PINCTRL_APPLY(name)							\
	wisblock_pinctrl_apply(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(name)),		\
			       name##_states, ARRAY_SIZE(name##_states))

static void wisblock_pinctrl_apply(struct pinctrl_dev_config *config,
				   const struct pinctrl_state *states, uint8_t state_cnt)
{
	int ret = pinctrl_update_states(config, states, state_cnt);

	__ASSERT(ret == 0, "WisBlock pinctrl update failed: %d", ret);
	ARG_UNUSED(ret);
}

#if defined(CONFIG_ADC)
static const pinctrl_soc_pin_t adc_pins[] = {
	WISBLOCK_ADC_IN_PIN(0, ADC_CH0_P26) /* AIN0 */
	WISBLOCK_ADC_IN_PIN(1, ADC_CH1_P27) /* AIN1 */
	WISBLOCK_ADC_IN_PIN(2, ADC_CH2_P28) /* IO4 */
};
#if defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
static const pinctrl_soc_pin_t adc_sleep[] = {
	WISBLOCK_ADC_IN_PIN(0, GPIO_P26)
	WISBLOCK_ADC_IN_PIN(1, GPIO_P27)
	WISBLOCK_ADC_IN_PIN(2, GPIO_P28)
};
#endif

WISBLOCK_PINCTRL_STATES(adc);
#endif /* CONFIG_ADC */

#if defined(CONFIG_PWM)
static const pinctrl_soc_pin_t pwm_pins[] = {
	WISBLOCK_PWM_OUT_PIN(6, PWM_3A_P6)   /* IO1 */
	WISBLOCK_PWM_OUT_PIN(7, PWM_3B_P7)   /* IO3 */
	WISBLOCK_PWM_OUT_PIN(8, PWM_4A_P8)   /* IO6 */
	WISBLOCK_PWM_OUT_PIN(9, PWM_4B_P9)   /* IO5 */
	WISBLOCK_PWM_OUT_PIN(12, PWM_6A_P28) /* IO4 */
};
#if defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
static const pinctrl_soc_pin_t pwm_sleep[] = {
	WISBLOCK_PWM_OUT_PIN(6, GPIO_P6)
	WISBLOCK_PWM_OUT_PIN(7, GPIO_P7)
	WISBLOCK_PWM_OUT_PIN(8, GPIO_P8)
	WISBLOCK_PWM_OUT_PIN(9, GPIO_P9)
	WISBLOCK_PWM_OUT_PIN(12, GPIO_P28)
};
#endif

WISBLOCK_PINCTRL_STATES(pwm);
#endif /* CONFIG_PWM */

void board_early_init_hook(void)
{
#if defined(CONFIG_ADC)
	WISBLOCK_PINCTRL_APPLY(adc);
#endif
#if defined(CONFIG_PWM)
	WISBLOCK_PINCTRL_APPLY(pwm);
#endif
}
