/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 RAKwireless Technology Limited
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl.h>

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
	(Z_PINCTRL_STM32_PMUX2PCFG_PORT_LINE(pm) |					\
	 Z_PINCTRL_STM32_PMUX2PCFG_MODE_AF((((pm) >> STM32_MODE_SHIFT) &		\
					   STM32_MODE_MASK), 0))

#define WISBLOCK_ADC_IN_PIN(ch, port, pin)						\
	COND_CODE_1(WISBLOCK_HAS_CONSUMER(io_channels, DT_NODELABEL(adc1), input, ch),	\
		(WISBLOCK_PINMUX(STM32_PINMUX(port, pin, ANALOG)),), ())

#define WISBLOCK_PWM_OUT_PIN(ctlr, ch, port, pin)					\
	COND_CODE_1(WISBLOCK_HAS_CONSUMER(pwms, DT_NODELABEL(ctlr), channel, ch),	\
		(WISBLOCK_PINMUX(STM32_PINMUX(port, pin, AF1)),), ())

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
static const pinctrl_soc_pin_t adc1_pins[] = {
	WISBLOCK_ADC_IN_PIN(2, 'B', 3)   /* AIN0 */
	WISBLOCK_ADC_IN_PIN(3, 'B', 4)   /* AIN1 */
	WISBLOCK_ADC_IN_PIN(4, 'B', 2)   /* IO4 */
	WISBLOCK_ADC_IN_PIN(11, 'A', 15) /* IO5 */
};
#if defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
static const pinctrl_soc_pin_t adc1_sleep[] = {};
#endif

WISBLOCK_PINCTRL_STATES(adc1);
#endif /* CONFIG_ADC */

#if defined(CONFIG_PWM)
static const pinctrl_soc_pin_t pwm1_pins[] = {
	WISBLOCK_PWM_OUT_PIN(pwm1, 1, 'A', 8)  /* IO2 */
};
#if defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
static const pinctrl_soc_pin_t pwm1_sleep[] = {};
#endif

static const pinctrl_soc_pin_t pwm2_pins[] = {
	WISBLOCK_PWM_OUT_PIN(pwm2, 1, 'A', 15) /* IO5 */
};
#if defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
static const pinctrl_soc_pin_t pwm2_sleep[] = {};
#endif

WISBLOCK_PINCTRL_STATES(pwm1);
WISBLOCK_PINCTRL_STATES(pwm2);
#endif /* CONFIG_PWM */

void board_early_init_hook(void)
{
#if defined(CONFIG_ADC)
	WISBLOCK_PINCTRL_APPLY(adc1);
#endif
#if defined(CONFIG_PWM)
	WISBLOCK_PINCTRL_APPLY(pwm1);
	WISBLOCK_PINCTRL_APPLY(pwm2);
#endif
}
