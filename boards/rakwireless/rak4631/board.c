/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 RAKwireless Technology Limited
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>

#if defined(CONFIG_PWM)
#include <zephyr/dt-bindings/pinctrl/nrf-pinctrl.h>

#define WISBLOCK_PWM_IO1_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm0)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 0),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO2_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm0)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 1),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO3_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm1)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 0),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO4_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm1)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 1),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO5_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm2)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 0),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO6_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm2)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 1),\
				(x), ())),				\
			())),						\
		())
#define WISBLOCK_PWM_IO7_MARK(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),			\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0),\
					  DT_NODELABEL(pwm2)),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), 2),\
				(x), ())),				\
			())),						\
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
#define WISBLOCK_PWM_IO7_ACTIVE                                                                    \
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE(WISBLOCK_PWM_IO7_MARK)))

static const pinctrl_soc_pin_t pwm0_pins[2] = {
	COND_CODE_1(WISBLOCK_PWM_IO1_ACTIVE,
		(NRF_PSEL(PWM_OUT0, 0, 17)), (NRF_PSEL_DISCONNECTED(PWM_OUT0))),
		     COND_CODE_1(WISBLOCK_PWM_IO2_ACTIVE,
		(NRF_PSEL(PWM_OUT1, 1, 2)),  (NRF_PSEL_DISCONNECTED(PWM_OUT1))),
};
static const pinctrl_soc_pin_t pwm1_pins[2] = {
	COND_CODE_1(WISBLOCK_PWM_IO3_ACTIVE,
		(NRF_PSEL(PWM_OUT0, 0, 21)), (NRF_PSEL_DISCONNECTED(PWM_OUT0))),
		     COND_CODE_1(WISBLOCK_PWM_IO4_ACTIVE,
		(NRF_PSEL(PWM_OUT1, 0, 4)),  (NRF_PSEL_DISCONNECTED(PWM_OUT1))),
};
static const pinctrl_soc_pin_t pwm2_pins[3] = {
	COND_CODE_1(WISBLOCK_PWM_IO5_ACTIVE,
		(NRF_PSEL(PWM_OUT0, 0, 9)),  (NRF_PSEL_DISCONNECTED(PWM_OUT0))),
		     COND_CODE_1(WISBLOCK_PWM_IO6_ACTIVE,
		(NRF_PSEL(PWM_OUT1, 0, 10)), (NRF_PSEL_DISCONNECTED(PWM_OUT1))),
				  COND_CODE_1(WISBLOCK_PWM_IO7_ACTIVE,
		(NRF_PSEL(PWM_OUT2, 0, 28)), (NRF_PSEL_DISCONNECTED(PWM_OUT2))),
};

static const pinctrl_soc_pin_t pwm0_sleep[2] = {
	NRF_PSEL_DISCONNECTED(PWM_OUT0),
	NRF_PSEL_DISCONNECTED(PWM_OUT1),
};
static const pinctrl_soc_pin_t pwm1_sleep[2] = {
	NRF_PSEL_DISCONNECTED(PWM_OUT0),
	NRF_PSEL_DISCONNECTED(PWM_OUT1),
};
static const pinctrl_soc_pin_t pwm2_sleep[3] = {
	NRF_PSEL_DISCONNECTED(PWM_OUT0),
	NRF_PSEL_DISCONNECTED(PWM_OUT1),
	NRF_PSEL_DISCONNECTED(PWM_OUT2),
};

PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(pwm0));
PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(pwm1));
PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(pwm2));

static const struct pinctrl_state pwm0_states[] = {
	{.pins = pwm0_pins, .pin_cnt = ARRAY_SIZE(pwm0_pins), .id = PINCTRL_STATE_DEFAULT},
	{.pins = pwm0_sleep, .pin_cnt = ARRAY_SIZE(pwm0_sleep), .id = PINCTRL_STATE_SLEEP},
};
static const struct pinctrl_state pwm1_states[] = {
	{.pins = pwm1_pins, .pin_cnt = ARRAY_SIZE(pwm1_pins), .id = PINCTRL_STATE_DEFAULT},
	{.pins = pwm1_sleep, .pin_cnt = ARRAY_SIZE(pwm1_sleep), .id = PINCTRL_STATE_SLEEP},
};
static const struct pinctrl_state pwm2_states[] = {
	{.pins = pwm2_pins, .pin_cnt = ARRAY_SIZE(pwm2_pins), .id = PINCTRL_STATE_DEFAULT},
	{.pins = pwm2_sleep, .pin_cnt = ARRAY_SIZE(pwm2_sleep), .id = PINCTRL_STATE_SLEEP},
};

#endif /* defined(CONFIG_PWM) */

static int wisblock_init(void)
{
	int ret = 0;

#if defined(CONFIG_PWM)
	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pwm0)), pwm0_states,
				    ARRAY_SIZE(pwm0_states));
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pwm1)), pwm1_states,
				    ARRAY_SIZE(pwm1_states));
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(pwm2)), pwm2_states,
				    ARRAY_SIZE(pwm2_states));
#endif /* defined(CONFIG_PWM) */

	return ret;
}

SYS_INIT(wisblock_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
