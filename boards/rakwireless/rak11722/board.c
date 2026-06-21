/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 RAKwireless Technology Limited
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/ambiq-apollo3-pinctrl.h>

#define APOLLO3_PINMUX_CFG(pinmux)                                                                 \
	{                                                                                          \
		.pin_num = APOLLO3_GET_PIN_NUM(pinmux),                                            \
		.alt_func = APOLLO3_GET_PIN_ALT_FUNC(pinmux),                                      \
	}

#if defined(CONFIG_ADC) && !defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
#error "RAK11722 dynamic ADC pinctrl requires CONFIG_PINCTRL_KEEP_SLEEP_STATE=y"
#endif

#if defined(CONFIG_ADC)
#include <zephyr/devicetree/io-channels.h>

#define WISBLOCK_ADC_MATCH_INPUT(node, prop, idx, input)                                           \
	COND_CODE_1(								\
		DT_SAME_NODE(DT_IO_CHANNELS_CTLR_BY_IDX(node, idx),		\
			     DT_NODELABEL(adc0)),				\
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

#define WISBLOCK_ADC_AIN0_MARK(node) WISBLOCK_ADC_MARK_INPUT(node, 8)
#define WISBLOCK_ADC_AIN1_MARK(node) WISBLOCK_ADC_MARK_INPUT(node, 5)
#define WISBLOCK_ADC_IO4_MARK(node)  WISBLOCK_ADC_MARK_INPUT(node, 3)
#define WISBLOCK_ADC_IO5_MARK(node)  WISBLOCK_ADC_MARK_INPUT(node, 9)

#define WISBLOCK_ADC_AIN0_ACTIVE                                                                   \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_AIN0_MARK)))
#define WISBLOCK_ADC_AIN1_ACTIVE                                                                   \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_AIN1_MARK)))
#define WISBLOCK_ADC_IO4_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_IO4_MARK)))
#define WISBLOCK_ADC_IO5_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_IO5_MARK)))

#if DT_NODE_EXISTS(DT_NODELABEL(pwm3)) && DT_NODE_HAS_STATUS(DT_NODELABEL(pwm3), okay) &&          \
	WISBLOCK_ADC_IO4_ACTIVE
#error "IO4 (GPIO31) cannot be used for both ADC and PWM"
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(pwm0)) && DT_NODE_HAS_STATUS(DT_NODELABEL(pwm0), okay) &&          \
	WISBLOCK_ADC_IO5_ACTIVE
#error "IO5 (GPIO12) cannot be used for both ADC and PWM"
#endif

static const pinctrl_soc_pin_t adc0_default[] = {
#if WISBLOCK_ADC_AIN0_ACTIVE
	APOLLO3_PINMUX_CFG(ADCD0PSE8_P13),
#endif
#if WISBLOCK_ADC_AIN1_ACTIVE
	APOLLO3_PINMUX_CFG(ADCSE5_P33),
#endif
#if WISBLOCK_ADC_IO4_ACTIVE
	APOLLO3_PINMUX_CFG(ADCSE3_P31),
#endif
#if WISBLOCK_ADC_IO5_ACTIVE
	APOLLO3_PINMUX_CFG(ADCD0NSE9_P12),
#endif
};

static const pinctrl_soc_pin_t adc0_sleep[] = {};

PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(adc0));

static const struct pinctrl_state adc0_states[] = {
	{.pins = adc0_default, .pin_cnt = ARRAY_SIZE(adc0_default), .id = PINCTRL_STATE_DEFAULT},
	{.pins = adc0_sleep, .pin_cnt = ARRAY_SIZE(adc0_sleep), .id = PINCTRL_STATE_SLEEP},
};
#endif /* defined(CONFIG_ADC) */

static int wisblock_init(void)
{
	int ret = 0;

#if defined(CONFIG_ADC)
	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(adc0)), adc0_states,
				    ARRAY_SIZE(adc0_states));
	if (ret < 0) {
		return ret;
	}
#endif

	return ret;
}

SYS_INIT(wisblock_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
