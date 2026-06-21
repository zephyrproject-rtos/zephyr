/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 RAKwireless Technology Limited
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl.h>

#define WISBLOCK_STM32_PINMUX(pm)                                                                  \
	(Z_PINCTRL_STM32_PMUX2PCFG_PORT_LINE(pm) |                                                 \
	 Z_PINCTRL_STM32_PMUX2PCFG_MODE_AF((((pm) >> STM32_MODE_SHIFT) & STM32_MODE_MASK), 0))

#if defined(CONFIG_ADC)
#include <zephyr/devicetree/io-channels.h>

#define WISBLOCK_ADC_MATCH_INPUT(node, prop, idx, input)                                           \
	COND_CODE_1(								\
		DT_SAME_NODE(DT_IO_CHANNELS_CTLR_BY_IDX(node, idx),		\
			     DT_NODELABEL(adc1)),				\
		(COND_CODE_1(							\
			IS_EQ(DT_IO_CHANNELS_INPUT_BY_IDX(node, idx), input),	\
			(x),							\
			())),							\
		()								\
	)

#define WISBLOCK_ADC_MARK_INPUT(node, input)                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, io_channels),			\
		(DT_FOREACH_PROP_ELEM_VARGS(node, io_channels,			\
					    WISBLOCK_ADC_MATCH_INPUT, input)),	\
		())

#define WISBLOCK_ADC_AIN0_MARK(node) WISBLOCK_ADC_MARK_INPUT(node, 2)
#define WISBLOCK_ADC_AIN1_MARK(node) WISBLOCK_ADC_MARK_INPUT(node, 3)
#define WISBLOCK_ADC_IO4_MARK(node)  WISBLOCK_ADC_MARK_INPUT(node, 4)
#define WISBLOCK_ADC_IO5_MARK(node)  WISBLOCK_ADC_MARK_INPUT(node, 11)
#define WISBLOCK_ADC_IO7_MARK(node)  WISBLOCK_ADC_MARK_INPUT(node, 6)

#define WISBLOCK_ADC_AIN0_ACTIVE                                                                   \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_AIN0_MARK)))
#define WISBLOCK_ADC_AIN1_ACTIVE                                                                   \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_AIN1_MARK)))
#define WISBLOCK_ADC_IO4_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_IO4_MARK)))
#define WISBLOCK_ADC_IO5_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_IO5_MARK)))
#define WISBLOCK_ADC_IO7_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_IO7_MARK)))

static const pinctrl_soc_pin_t adc1_default[] = {
#if WISBLOCK_ADC_AIN0_ACTIVE
	WISBLOCK_STM32_PINMUX(STM32_PINMUX('B', 3, ANALOG)),
#endif
#if WISBLOCK_ADC_AIN1_ACTIVE
	WISBLOCK_STM32_PINMUX(STM32_PINMUX('B', 4, ANALOG)),
#endif
#if WISBLOCK_ADC_IO4_ACTIVE
	WISBLOCK_STM32_PINMUX(STM32_PINMUX('B', 2, ANALOG)),
#endif
#if WISBLOCK_ADC_IO5_ACTIVE
	WISBLOCK_STM32_PINMUX(STM32_PINMUX('A', 15, ANALOG)),
#endif
#if WISBLOCK_ADC_IO7_ACTIVE
	WISBLOCK_STM32_PINMUX(STM32_PINMUX('A', 10, ANALOG)),
#endif
};

static const pinctrl_soc_pin_t adc1_sleep[] = {};

PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(adc1));

static const struct pinctrl_state adc1_states[] = {
	{.pins = adc1_default, .pin_cnt = ARRAY_SIZE(adc1_default), .id = PINCTRL_STATE_DEFAULT},
	{.pins = adc1_sleep, .pin_cnt = ARRAY_SIZE(adc1_sleep), .id = PINCTRL_STATE_SLEEP},
};
#endif /* defined(CONFIG_ADC) */

#if defined(CONFIG_PWM)

#define WISBLOCK_PWM_IO2_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(pwm1)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 1),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO7_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(pwm1)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 3),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO5_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(pwm2)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 1),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO6_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(pwm1)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 2),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO2_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO2_MARK)))
#define WISBLOCK_PWM_IO5_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO5_MARK)))
#define WISBLOCK_PWM_IO6_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO6_MARK)))
#define WISBLOCK_PWM_IO7_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO7_MARK)))

static const pinctrl_soc_pin_t pwm1_default[] = {
#if WISBLOCK_PWM_IO2_ACTIVE
	WISBLOCK_STM32_PINMUX(STM32_PINMUX('A', 8, AF1)),
#endif
#if WISBLOCK_PWM_IO6_ACTIVE
	WISBLOCK_STM32_PINMUX(STM32_PINMUX('A', 9, AF1)),
#endif
#if WISBLOCK_PWM_IO7_ACTIVE
	WISBLOCK_STM32_PINMUX(STM32_PINMUX('A', 10, AF1)),
#endif
};
static const pinctrl_soc_pin_t pwm1_sleep[] = {};

static const pinctrl_soc_pin_t pwm2_default[] = {
#if WISBLOCK_PWM_IO5_ACTIVE
	WISBLOCK_STM32_PINMUX(STM32_PINMUX('A', 15, AF1)),
#endif
};
static const pinctrl_soc_pin_t pwm2_sleep[] = {};

PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(pwm1));
PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(pwm2));

static const struct pinctrl_state pwm1_states[] = {
	{.pins = pwm1_default, .pin_cnt = ARRAY_SIZE(pwm1_default), .id = PINCTRL_STATE_DEFAULT},
	{.pins = pwm1_sleep, .pin_cnt = ARRAY_SIZE(pwm1_sleep), .id = PINCTRL_STATE_SLEEP},
};
static const struct pinctrl_state pwm2_states[] = {
	{.pins = pwm2_default, .pin_cnt = ARRAY_SIZE(pwm2_default), .id = PINCTRL_STATE_DEFAULT},
	{.pins = pwm2_sleep, .pin_cnt = ARRAY_SIZE(pwm2_sleep), .id = PINCTRL_STATE_SLEEP},
};
#endif /* defined(CONFIG_PWM) */

static int wisblock_init(void)
{
	int ret = 0;

#if defined(CONFIG_ADC)
	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(adc1)), adc1_states,
				    ARRAY_SIZE(adc1_states));
	if (ret < 0) {
		return ret;
	}
#endif

#if defined(CONFIG_PWM)
	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pwm1)), pwm1_states,
				    ARRAY_SIZE(pwm1_states));
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pwm2)), pwm2_states,
				    ARRAY_SIZE(pwm2_states));
	if (ret < 0) {
		return ret;
	}
#endif

	return ret;
}

SYS_INIT(wisblock_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
