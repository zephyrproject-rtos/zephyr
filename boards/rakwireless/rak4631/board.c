/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 RAKwireless Technology Limited
 */

#include <zephyr/init.h>

#include <cmsis_core.h>
#include <hal/nrf_power.h>

#if defined(CONFIG_PWM)
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/nrf-pinctrl.h>

#define WISBLOCK_PWM_MARK(node, ctlr, ch)						\
	COND_CODE_1(DT_NODE_HAS_PROP(node, pwms),					\
		(COND_CODE_1(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(node, 0), ctlr),		\
			(COND_CODE_1(IS_EQ(DT_PWMS_CHANNEL_BY_IDX(node, 0), ch),	\
				(x), ())),						\
			())),								\
		())

#define WISBLOCK_PWM_ACTIVE(ctlr, ch)							\
	UTIL_NOT(IS_EMPTY(DT_FOREACH_STATUS_OKAY_NODE_VARGS(WISBLOCK_PWM_MARK, ctlr, ch)))

#define WISBLOCK_PWM_OUT(ctlr, ch, fun, port, pin)					\
	COND_CODE_1(WISBLOCK_PWM_ACTIVE(DT_NODELABEL(ctlr), ch),			\
		(NRF_PSEL(fun, port, pin)), (NRF_PSEL_DISCONNECTED(fun)))

#define WISBLOCK_PWM_STATES(name)							\
	PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(name));				\
	static const struct pinctrl_state name##_states[] = {				\
		{.pins = name##_pins, .pin_cnt = ARRAY_SIZE(name##_pins),		\
		 .id = PINCTRL_STATE_DEFAULT},						\
		{.pins = name##_sleep, .pin_cnt = ARRAY_SIZE(name##_sleep),		\
		 .id = PINCTRL_STATE_SLEEP},						\
	}

#define WISBLOCK_PWM_UPDATE(name)							\
	(void)pinctrl_update_states(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(name)),	\
				    name##_states, ARRAY_SIZE(name##_states))

static const pinctrl_soc_pin_t pwm0_pins[] = {
	WISBLOCK_PWM_OUT(pwm0, 0, PWM_OUT0, 0, 17),
	WISBLOCK_PWM_OUT(pwm0, 1, PWM_OUT1, 1, 2),
};
static const pinctrl_soc_pin_t pwm1_pins[] = {
	WISBLOCK_PWM_OUT(pwm1, 0, PWM_OUT0, 0, 21),
	WISBLOCK_PWM_OUT(pwm1, 1, PWM_OUT1, 0, 4),
};
static const pinctrl_soc_pin_t pwm2_pins[] = {
	WISBLOCK_PWM_OUT(pwm2, 0, PWM_OUT0, 0, 9),
	WISBLOCK_PWM_OUT(pwm2, 1, PWM_OUT1, 0, 10),
	WISBLOCK_PWM_OUT(pwm2, 2, PWM_OUT2, 0, 28),
};

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

WISBLOCK_PWM_STATES(pwm0);
WISBLOCK_PWM_STATES(pwm1);
WISBLOCK_PWM_STATES(pwm2);

static void wisblock_pwm_connect(void)
{
	WISBLOCK_PWM_UPDATE(pwm0);
	WISBLOCK_PWM_UPDATE(pwm1);
	WISBLOCK_PWM_UPDATE(pwm2);
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

	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
	}

	NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
			    (UICR_REGOUT0_VOUT_3V3 << UICR_REGOUT0_VOUT_Pos);

	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
	}

	/* UICR is only read out of reset */
	NVIC_SystemReset();
}

void board_early_init_hook(void)
{
	rak4630_regout0_set_3v3();
	wisblock_pwm_connect();
}
