/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 RAKwireless Technology Limited
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>

#if defined(CONFIG_PWM)
#include <zephyr/dt-bindings/pinctrl/nrf-pinctrl.h>

#define WISBLOCK_PWM_IO1_MARK(node)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm20)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 0),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO2_MARK(node)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm20)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 1),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO3_MARK(node)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm21)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 0),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO4_MARK(node)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm21)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 1),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO5_MARK(node)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm22)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 0),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO6_MARK(node)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm22)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 1),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO7_MARK(node)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm22)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 2),\
				(x), ())),				\
			())),						\
		())

#define WISBLOCK_PWM_IO1_ACTIVE \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO1_MARK)))
#define WISBLOCK_PWM_IO2_ACTIVE \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO2_MARK)))
#define WISBLOCK_PWM_IO3_ACTIVE \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO3_MARK)))
#define WISBLOCK_PWM_IO4_ACTIVE \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO4_MARK)))
#define WISBLOCK_PWM_IO5_ACTIVE \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO5_MARK)))
#define WISBLOCK_PWM_IO6_ACTIVE \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO6_MARK)))
#define WISBLOCK_PWM_IO7_ACTIVE \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO7_MARK)))

static const pinctrl_soc_pin_t pwm20_pins[2] = {
	COND_CODE_1(WISBLOCK_PWM_IO1_ACTIVE,
		(NRF_PSEL(PWM_OUT0, 2, 0)), (NRF_PSEL_DISCONNECTED(PWM_OUT0))),
	COND_CODE_1(WISBLOCK_PWM_IO2_ACTIVE,
		(NRF_PSEL(PWM_OUT1, 2, 3)), (NRF_PSEL_DISCONNECTED(PWM_OUT1))),
};
static const pinctrl_soc_pin_t pwm21_pins[2] = {
	COND_CODE_1(WISBLOCK_PWM_IO3_ACTIVE,
		(NRF_PSEL(PWM_OUT0, 2, 6)), (NRF_PSEL_DISCONNECTED(PWM_OUT0))),
	COND_CODE_1(WISBLOCK_PWM_IO4_ACTIVE,
		(NRF_PSEL(PWM_OUT1, 2, 6)), (NRF_PSEL_DISCONNECTED(PWM_OUT1))),
};
static const pinctrl_soc_pin_t pwm22_pins[3] = {
	COND_CODE_1(WISBLOCK_PWM_IO5_ACTIVE,
		(NRF_PSEL(PWM_OUT0, 2, 6)), (NRF_PSEL_DISCONNECTED(PWM_OUT0))),
	COND_CODE_1(WISBLOCK_PWM_IO6_ACTIVE,
		(NRF_PSEL(PWM_OUT1, 2, 6)), (NRF_PSEL_DISCONNECTED(PWM_OUT1))),
	COND_CODE_1(WISBLOCK_PWM_IO7_ACTIVE,
		(NRF_PSEL(PWM_OUT2, 2, 6)), (NRF_PSEL_DISCONNECTED(PWM_OUT2))),
};

static const pinctrl_soc_pin_t pwm20_sleep[2] = {
	NRF_PSEL_DISCONNECTED(PWM_OUT0),
	NRF_PSEL_DISCONNECTED(PWM_OUT1),
};
static const pinctrl_soc_pin_t pwm21_sleep[2] = {
	NRF_PSEL_DISCONNECTED(PWM_OUT0),
	NRF_PSEL_DISCONNECTED(PWM_OUT1),
};
static const pinctrl_soc_pin_t pwm22_sleep[3] = {
	NRF_PSEL_DISCONNECTED(PWM_OUT0),
	NRF_PSEL_DISCONNECTED(PWM_OUT1),
	NRF_PSEL_DISCONNECTED(PWM_OUT2),
};

PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(pwm20));
PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(pwm21));
PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(pwm22));

static const struct pinctrl_state pwm20_states[] = {
	{
		.pins = pwm20_pins,
		.pin_cnt = ARRAY_SIZE(pwm20_pins),
		.id = PINCTRL_STATE_DEFAULT
	},
	{
		.pins = pwm20_sleep,
		.pin_cnt = ARRAY_SIZE(pwm20_sleep),
		.id = PINCTRL_STATE_SLEEP
	},
};
static const struct pinctrl_state pwm21_states[] = {
	{
		.pins = pwm21_pins,
		.pin_cnt = ARRAY_SIZE(pwm21_pins),
		.id = PINCTRL_STATE_DEFAULT
	},
	{
		.pins = pwm21_sleep,
		.pin_cnt = ARRAY_SIZE(pwm21_sleep),
		.id = PINCTRL_STATE_SLEEP
	},
};
static const struct pinctrl_state pwm22_states[] = {
	{
		.pins = pwm22_pins,
		.pin_cnt = ARRAY_SIZE(pwm22_pins),
		.id = PINCTRL_STATE_DEFAULT
	},
	{
		.pins = pwm22_sleep,
		.pin_cnt = ARRAY_SIZE(pwm22_sleep),
		.id = PINCTRL_STATE_SLEEP
	},
};

#endif /* defined(CONFIG_PWM) */

static int wisblock_init(void)
{
	int ret = 0;

#if defined(CONFIG_PWM)
	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pwm20)),
				    pwm20_states, ARRAY_SIZE(pwm20_states));
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pwm21)),
				    pwm21_states, ARRAY_SIZE(pwm21_states));
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pwm22)),
				    pwm22_states, ARRAY_SIZE(pwm22_states));
	if (ret < 0) {
		return ret;
	}
#endif /* defined(CONFIG_PWM) */

	return ret;
}

SYS_INIT(wisblock_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
