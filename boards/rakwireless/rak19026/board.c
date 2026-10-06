/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <cmsis_core.h>
#include <hal/nrf_nvmc.h>
#include <hal/nrf_power.h>

#if defined(CONFIG_PWM)
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/nrf-pinctrl.h>

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

#define WISBLOCK_PWM_OUT_PIN(ctlr, ch, fun, port, pin)					\
	COND_CODE_1(WISBLOCK_HAS_CONSUMER(pwms, DT_NODELABEL(ctlr), channel, ch),	\
		(NRF_PSEL(fun, port, pin)), (NRF_PSEL_DISCONNECTED(fun)))

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

static const pinctrl_soc_pin_t pwm0_pins[] = {
	WISBLOCK_PWM_OUT_PIN(pwm0, 0, PWM_OUT0, 0, 17),
	WISBLOCK_PWM_OUT_PIN(pwm0, 1, PWM_OUT1, 1, 2),
};
static const pinctrl_soc_pin_t pwm1_pins[] = {
	WISBLOCK_PWM_OUT_PIN(pwm1, 0, PWM_OUT0, 0, 21),
	WISBLOCK_PWM_OUT_PIN(pwm1, 1, PWM_OUT1, 0, 4),
};
static const pinctrl_soc_pin_t pwm2_pins[] = {
	WISBLOCK_PWM_OUT_PIN(pwm2, 0, PWM_OUT0, 0, 9),
	WISBLOCK_PWM_OUT_PIN(pwm2, 1, PWM_OUT1, 0, 10),
	WISBLOCK_PWM_OUT_PIN(pwm2, 2, PWM_OUT2, 0, 28),
};

#if defined(CONFIG_PINCTRL_KEEP_SLEEP_STATE)
static const pinctrl_soc_pin_t pwm0_sleep[] = {
	NRF_PSEL_DISCONNECTED(PWM_OUT0),
	NRF_PSEL_DISCONNECTED(PWM_OUT1),
};
static const pinctrl_soc_pin_t pwm1_sleep[] = {
	NRF_PSEL_DISCONNECTED(PWM_OUT0),
	NRF_PSEL_DISCONNECTED(PWM_OUT1),
};
static const pinctrl_soc_pin_t pwm2_sleep[] = {
	NRF_PSEL_DISCONNECTED(PWM_OUT0),
	NRF_PSEL_DISCONNECTED(PWM_OUT1),
	NRF_PSEL_DISCONNECTED(PWM_OUT2),
};
#endif

WISBLOCK_PINCTRL_STATES(pwm0);
WISBLOCK_PINCTRL_STATES(pwm1);
WISBLOCK_PINCTRL_STATES(pwm2);

static void wisblock_pwm_connect(void)
{
	WISBLOCK_PINCTRL_APPLY(pwm0);
	WISBLOCK_PINCTRL_APPLY(pwm1);
	WISBLOCK_PINCTRL_APPLY(pwm2);
}
#else
static void wisblock_pwm_connect(void)
{
}
#endif /* CONFIG_PWM */

static void rak4630_regout0_set_3v3(void)
{
	/* The module supplies the SoC through VDDH, where the GPIO rail runs
	 * from the internal regulator. UICR picks its voltage and comes up at
	 * 1.8 V, too low for the WisBlock connector, so raise it to 3.3 V. A
	 * chip erase puts UICR back to the default, which is why this cannot
	 * be left to whatever programmed the board last.
	 */
	if ((nrf_power_mainregstatus_get(NRF_POWER) != NRF_POWER_MAINREGSTATUS_HIGH) ||
	    ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) !=
	     (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))) {
		return;
	}

	nrf_nvmc_mode_set(NRF_NVMC, NRF_NVMC_MODE_WRITE);
	while (!nrf_nvmc_ready_check(NRF_NVMC)) {
	}

	NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
			    (UICR_REGOUT0_VOUT_3V3 << UICR_REGOUT0_VOUT_Pos);

	nrf_nvmc_mode_set(NRF_NVMC, NRF_NVMC_MODE_READONLY);
	while (!nrf_nvmc_ready_check(NRF_NVMC)) {
	}

	/* UICR is only read out of reset */
	NVIC_SystemReset();
}

void board_early_init_hook(void)
{
	rak4630_regout0_set_3v3();
	wisblock_pwm_connect();
}
