/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 RAKwireless Technology Limited
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>

#if defined(CONFIG_PWM) && !defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
#error "CONFIG_PINCTRL_KEEP_SLEEP_STATE must be enabled for wisblock dynamic pinctrl"
#endif

#if defined(CONFIG_ADC)
#include <zephyr/devicetree/io-channels.h>

#define WISBLOCK_ADC_IO2_MATCH(node, prop, idx)                                                    \
	COND_CODE_1(DT_SAME_NODE(DT_IO_CHANNELS_CTLR_BY_IDX(node, idx),	\
				 DT_NODELABEL(adc1)),				\
		(COND_CODE_1(IS_EQ(DT_IO_CHANNELS_INPUT_BY_IDX(node, idx),	\
				   3),						\
			(x), ())),						\
		())

#define WISBLOCK_ADC_IO2_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, io_channels),			\
		(DT_FOREACH_PROP_ELEM(node, io_channels,			\
				      WISBLOCK_ADC_IO2_MATCH)),			\
		())

#define WISBLOCK_ADC_IO2_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_ADC_IO2_MARK)))

#if defined(CONFIG_WIFI) && WISBLOCK_ADC_IO2_ACTIVE
#error "WISBLOCK_ADC_IO2 (ADC2/GPIO14) cannot be used with CONFIG_WIFI=y on ESP32-S3"
#endif
#endif /* defined(CONFIG_ADC) */

#if defined(CONFIG_PWM)
#include <zephyr/dt-bindings/pinctrl/esp32s3-pinctrl.h>

#define WISBLOCK_PWM_IO1_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(ledc0)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 0),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO2_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(ledc0)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 1),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO3_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(ledc0)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 2),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO4_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(ledc0)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 3),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO5_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(ledc0)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 4),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO6_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),				\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),	\
					  DT_NODELABEL(ledc0)),			\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 5),\
				(x), ())),					\
			())),							\
		())

#define WISBLOCK_PWM_IO1_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO1_MARK)))
#define WISBLOCK_PWM_IO2_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO2_MARK)))
#define WISBLOCK_PWM_IO3_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO3_MARK)))
#define WISBLOCK_PWM_IO4_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO4_MARK)))
#define WISBLOCK_PWM_IO5_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO5_MARK)))
#define WISBLOCK_PWM_IO6_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO6_MARK)))

static const pinctrl_soc_pin_t ledc0_default[] = {
#if WISBLOCK_PWM_IO1_ACTIVE
	{LEDC_CH0_GPIO21},
#endif
#if WISBLOCK_PWM_IO2_ACTIVE
	{LEDC_CH1_GPIO14},
#endif
#if WISBLOCK_PWM_IO3_ACTIVE
	{LEDC_CH2_GPIO41},
#endif
#if WISBLOCK_PWM_IO4_ACTIVE
	{LEDC_CH3_GPIO42},
#endif
#if WISBLOCK_PWM_IO5_ACTIVE
	{LEDC_CH4_GPIO38},
#endif
#if WISBLOCK_PWM_IO6_ACTIVE
	{LEDC_CH5_GPIO39},
#endif
};

static const pinctrl_soc_pin_t ledc0_sleep[] = {};

PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(ledc0));

static const struct pinctrl_state ledc0_states[] = {
	{.pins = ledc0_default, .pin_cnt = ARRAY_SIZE(ledc0_default), .id = PINCTRL_STATE_DEFAULT},
	{.pins = ledc0_sleep, .pin_cnt = ARRAY_SIZE(ledc0_sleep), .id = PINCTRL_STATE_SLEEP},
};
#endif /* defined(CONFIG_PWM) */

static int wisblock_init(void)
{
	int ret = 0;

#if defined(CONFIG_PWM)
	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(ledc0)), ledc0_states,
				    ARRAY_SIZE(ledc0_states));
	if (ret < 0) {
		return ret;
	}
#endif

	return ret;
}

SYS_INIT(wisblock_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
