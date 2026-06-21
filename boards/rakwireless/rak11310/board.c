/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 RAKwireless Technology Limited
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/rpi-pico-pinctrl-common.h>

#define RP2_PINMUX_CFG(pinmux)                                                                     \
	{                                                                                          \
		.pin_num = RP2_GET_PIN_NUM(pinmux),                                                \
		.alt_func = RP2_GET_PIN_ALT_FUNC(pinmux),                                          \
	}

#if (defined(CONFIG_PWM) || defined(CONFIG_ADC)) && !defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
#error "RAK11310 dynamic pinctrl requires CONFIG_PINCTRL_KEEP_SLEEP_STATE=y"
#endif

#if defined(CONFIG_PWM)

#define WISBLOCK_PWM_MATCH_CHANNEL(node, idx, channel)                                             \
	COND_CODE_1(							\
		DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, idx),		\
			     DT_NODELABEL(pwm)),			\
		(COND_CODE_1(						\
			IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, idx), channel), \
			(x),						\
			())),						\
		()							\
	)

#define WISBLOCK_PWM_IO1_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(WISBLOCK_PWM_MATCH_CHANNEL(node, 0, 6)),		\
		())
#define WISBLOCK_PWM_IO3_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(WISBLOCK_PWM_MATCH_CHANNEL(node, 0, 7)),		\
		())
#define WISBLOCK_PWM_IO4_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(WISBLOCK_PWM_MATCH_CHANNEL(node, 0, 12)),		\
		())
#define WISBLOCK_PWM_IO5_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(WISBLOCK_PWM_MATCH_CHANNEL(node, 0, 9)),		\
		())
#define WISBLOCK_PWM_IO6_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(WISBLOCK_PWM_MATCH_CHANNEL(node, 0, 8)),		\
		())

#define WISBLOCK_PWM_IO1_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO1_MARK)))
#define WISBLOCK_PWM_IO3_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO3_MARK)))
#define WISBLOCK_PWM_IO4_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO4_MARK)))
#define WISBLOCK_PWM_IO5_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO5_MARK)))
#define WISBLOCK_PWM_IO6_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO6_MARK)))

static const pinctrl_soc_pin_t pwm_pins[] = {
#if WISBLOCK_PWM_IO1_ACTIVE
	RP2_PINMUX_CFG(PWM_3A_P6),
#endif
#if WISBLOCK_PWM_IO3_ACTIVE
	RP2_PINMUX_CFG(PWM_3B_P7),
#endif
#if WISBLOCK_PWM_IO4_ACTIVE
	RP2_PINMUX_CFG(PWM_6A_P28),
#endif
#if WISBLOCK_PWM_IO5_ACTIVE
	RP2_PINMUX_CFG(PWM_4B_P9),
#endif
#if WISBLOCK_PWM_IO6_ACTIVE
	RP2_PINMUX_CFG(PWM_4A_P8),
#endif
};

static const pinctrl_soc_pin_t pwm_sleep[] = {
#if WISBLOCK_PWM_IO1_ACTIVE
	RP2_PINMUX_CFG(GPIO_P6),
#endif
#if WISBLOCK_PWM_IO3_ACTIVE
	RP2_PINMUX_CFG(GPIO_P7),
#endif
#if WISBLOCK_PWM_IO4_ACTIVE
	RP2_PINMUX_CFG(GPIO_P28),
#endif
#if WISBLOCK_PWM_IO5_ACTIVE
	RP2_PINMUX_CFG(GPIO_P9),
#endif
#if WISBLOCK_PWM_IO6_ACTIVE
	RP2_PINMUX_CFG(GPIO_P8),
#endif
};

PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(pwm));

static const struct pinctrl_state pwm_states[] = {
	{.pins = pwm_pins, .pin_cnt = ARRAY_SIZE(pwm_pins), .id = PINCTRL_STATE_DEFAULT},
	{.pins = pwm_sleep, .pin_cnt = ARRAY_SIZE(pwm_sleep), .id = PINCTRL_STATE_SLEEP},
};
#endif /* defined(CONFIG_PWM) */

#if defined(CONFIG_ADC)
#include <zephyr/devicetree/io-channels.h>
#include <zephyr/dt-bindings/pinctrl/rpi-pico-rp2040-pinctrl.h>

#define WISBLOCK_ADC_MATCH_INPUT(node, prop, idx, input)                                           \
	COND_CODE_1( \
		DT_SAME_NODE(DT_IO_CHANNELS_CTLR_BY_IDX(node, idx), \
			     DT_NODELABEL(adc)), \
		(COND_CODE_1( \
			IS_EQ(DT_IO_CHANNELS_INPUT_BY_IDX(node, idx), input), \
			(x), \
			())), \
		() \
	)

#define WISBLOCK_ADC_MARK_INPUT(node, input)                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, io_channels), \
		(DT_FOREACH_PROP_ELEM_VARGS(node, io_channels, \
					    WISBLOCK_ADC_MATCH_INPUT, input)), \
		())

#define WISBLOCK_ADC_AIN0_MARK(node) WISBLOCK_ADC_MARK_INPUT(node, 0)
#define WISBLOCK_ADC_AIN1_MARK(node) WISBLOCK_ADC_MARK_INPUT(node, 1)
#define WISBLOCK_ADC_IO4_MARK(node)  WISBLOCK_ADC_MARK_INPUT(node, 2)

#define WISBLOCK_ADC_AIN0_ACTIVE                                                                   \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_AIN0_MARK)))
#define WISBLOCK_ADC_AIN1_ACTIVE                                                                   \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_AIN1_MARK)))
#define WISBLOCK_ADC_IO4_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_IO4_MARK)))

static const pinctrl_soc_pin_t adc_pins[] = {
#if WISBLOCK_ADC_AIN0_ACTIVE
	RP2_PINMUX_CFG(ADC_CH0_P26),
#endif
#if WISBLOCK_ADC_AIN1_ACTIVE
	RP2_PINMUX_CFG(ADC_CH1_P27),
#endif
#if WISBLOCK_ADC_IO4_ACTIVE
	RP2_PINMUX_CFG(ADC_CH2_P28),
#endif
};

static const pinctrl_soc_pin_t adc_sleep[] = {
#if WISBLOCK_ADC_AIN0_ACTIVE
	RP2_PINMUX_CFG(GPIO_P26),
#endif
#if WISBLOCK_ADC_AIN1_ACTIVE
	RP2_PINMUX_CFG(GPIO_P27),
#endif
#if WISBLOCK_ADC_IO4_ACTIVE
	RP2_PINMUX_CFG(GPIO_P28),
#endif
};

PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(adc));

static const struct pinctrl_state adc_states[] = {
	{.pins = adc_pins, .pin_cnt = ARRAY_SIZE(adc_pins), .id = PINCTRL_STATE_DEFAULT},
	{.pins = adc_sleep, .pin_cnt = ARRAY_SIZE(adc_sleep), .id = PINCTRL_STATE_SLEEP},
};
#endif /* defined(CONFIG_ADC) */

static int wisblock_init(void)
{
	int ret = 0;

#if defined(CONFIG_PWM)
	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pwm)), pwm_states,
				    ARRAY_SIZE(pwm_states));
	if (ret < 0) {
		return ret;
	}
#endif /* defined(CONFIG_PWM) */

#if defined(CONFIG_ADC)
	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(adc)), adc_states,
				    ARRAY_SIZE(adc_states));
	if (ret < 0) {
		return ret;
	}
#endif /* defined(CONFIG_ADC) */

	return ret;
}

SYS_INIT(wisblock_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
