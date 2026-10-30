/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 RAKwireless Technology Limited
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/esp32s3-pinctrl.h>

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

#define WISBLOCK_PWM_OUT_PIN(ch, pinmux)						\
	COND_CODE_1(WISBLOCK_HAS_CONSUMER(pwms, DT_NODELABEL(ledc0), channel, ch),	\
		({pinmux},), ())

#define WISBLOCK_PINCTRL_APPLY(name)							\
	wisblock_pinctrl_apply(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(name)),		\
			       name##_states, ARRAY_SIZE(name##_states))

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

static void wisblock_pinctrl_apply(struct pinctrl_dev_config *config,
				   const struct pinctrl_state *states, uint8_t state_cnt)
{
	int ret = pinctrl_update_states(config, states, state_cnt);

	__ASSERT(ret == 0, "WisBlock pinctrl update failed: %d", ret);
	ARG_UNUSED(ret);
}

#if defined(CONFIG_PWM)
static const pinctrl_soc_pin_t ledc0_pins[] = {
	WISBLOCK_PWM_OUT_PIN(0, LEDC_CH0_GPIO21) /* IO1 */
	WISBLOCK_PWM_OUT_PIN(1, LEDC_CH1_GPIO14) /* IO2 */
	WISBLOCK_PWM_OUT_PIN(2, LEDC_CH2_GPIO41) /* IO3 */
	WISBLOCK_PWM_OUT_PIN(3, LEDC_CH3_GPIO42) /* IO4 */
	WISBLOCK_PWM_OUT_PIN(4, LEDC_CH4_GPIO38) /* IO5 */
	WISBLOCK_PWM_OUT_PIN(5, LEDC_CH5_GPIO39) /* IO6 */
};
#if defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
static const pinctrl_soc_pin_t ledc0_sleep[] = {};
#endif

WISBLOCK_PINCTRL_STATES(ledc0);
#endif /* CONFIG_PWM */

void board_early_init_hook(void)
{
#if defined(CONFIG_PWM)
	WISBLOCK_PINCTRL_APPLY(ledc0);
#endif
}
