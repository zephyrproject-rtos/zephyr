/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_pwm

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/clock/npcx_clock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_npcx, CONFIG_PWM_LOG_LEVEL);

/* 16-bit period cycles/prescaler in NPCX PWM modules */
#define NPCX_PWM_MAX_PRESCALER      (1UL << (16))
#define NPCX_PWM_MAX_PERIOD_CYCLES  (1UL << (16))

/* PWM clock sources */
#define NPCX_PWM_CLOCK_APB2_LFCLK   0
#define NPCX_PWM_CLOCK_FX           1
#define NPCX_PWM_CLOCK_FR           2
#define NPCX_PWM_CLOCK_RESERVED     3

/* PWM heart-beat mode selection */
#define NPCX_PWM_HBM_NORMAL         0
#define NPCX_PWM_HBM_25             1
#define NPCX_PWM_HBM_50             2
#define NPCX_PWM_HBM_100            3

/* Device config */
struct pwm_npcx_config {
	/* pwm controller base address */
	struct pwm_reg *base;
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
};

/* Driver data */
struct pwm_npcx_data {
	/* PWM cycles per second */
	uint32_t cycles_per_sec;
};

/* PWM local functions */
static void pwm_npcx_configure(const struct device *dev, int clk_bus)
{
	const struct pwm_npcx_config *config = dev->config;
	struct pwm_reg *inst = config->base;

	/* Disable PWM for module configuration first */
	inst->PWMCTL &= ~BIT(NPCX_PWMCTL_PWR);

	/*
	 * NPCX PWM module mixes byte and word registers together. Make sure
	 * word reg access via structure won't break into two byte reg accesses
	 * unexpectedly by toolchains options or attributes. If so, stall here.
	 */
	NPCX_REG_WORD_ACCESS_CHECK(inst->PRSC, 0xA55A);

	/* Set default PWM polarity to normal */
	inst->PWMCTL &= ~BIT(NPCX_PWMCTL_INVP);

	/* Turn off PWM heart-beat mode */
	SET_FIELD(inst->PWMCTL, NPCX_PWMCTL_HB_DC_CTL_FIELD,
			NPCX_PWM_HBM_NORMAL);

	/* Select APB CLK/LFCLK clock sources to PWM module by default */
	SET_FIELD(inst->PWMCTLEX, NPCX_PWMCTLEX_FCK_SEL_FIELD,
			NPCX_PWM_CLOCK_APB2_LFCLK);

	/* Select clock source to LFCLK by flag, otherwise APB clock source */
	if (clk_bus == NPCX_CLOCK_BUS_LFCLK) {
		inst->PWMCTL |= BIT(NPCX_PWMCTL_CKSEL);
	} else {
		inst->PWMCTL &= ~BIT(NPCX_PWMCTL_CKSEL);
	}
}

/* PWM api functions */
static int pwm_npcx_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(channel);
	const struct pwm_npcx_config *config = dev->config;
	struct pwm_npcx_data *const data = dev->data;
	struct pwm_reg *inst = config->base;
	int prescaler;
	uint32_t ctl;
	uint32_t ctr;
	uint32_t dcr;
	uint32_t prsc;

	ctl = inst->PWMCTL | BIT(NPCX_PWMCTL_PWR);

	/* Select PWM inverted polarity (ie. active-low pulse). */
	if (flags & PWM_POLARITY_INVERTED) {
		ctl |= BIT(NPCX_PWMCTL_INVP);
	} else {
		ctl &= ~BIT(NPCX_PWMCTL_INVP);
	}

	/* If pulse_cycles is 0, switch PWM off and return. */
	if (pulse_cycles == 0) {
		ctl &= ~BIT(NPCX_PWMCTL_PWR);
		inst->PWMCTL = ctl;
		return 0;
	}

	/*
	 * Calculate PWM prescaler that let period_cycles map to
	 * maximum pwm period cycles and won't exceed it.
	 * Then prescaler = ceil (period_cycles / pwm_max_period_cycles)
	 */
	prescaler = DIV_ROUND_UP(period_cycles, NPCX_PWM_MAX_PERIOD_CYCLES);
	if (prescaler > NPCX_PWM_MAX_PRESCALER) {
		return -EINVAL;
	}

	/* Set PWM prescaler. */
	prsc = prescaler - 1;

	/* Set PWM period cycles. */
	ctr = (period_cycles / prescaler) - 1;

	/* Set PWM pulse cycles. */
	dcr = (pulse_cycles / prescaler) - 1;

	LOG_DBG("freq %d, pre %d, period %d, pulse %d",
		data->cycles_per_sec / period_cycles, prsc, ctr, dcr);

	/* Reconfigure only if necessary. */
	if (inst->PWMCTL != ctl || inst->PRSC != prsc || inst->CTR != ctr) {
		/* Disable PWM before configuring. */
		inst->PWMCTL &= ~BIT(NPCX_PWMCTL_PWR);

		inst->PRSC = prsc;
		inst->CTR = ctr;
		inst->DCR = dcr;

		/* Enable PWM now. */
		inst->PWMCTL = ctl;

		return 0;
	}

	inst->DCR = dcr;

	return 0;
}

static int pwm_npcx_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(channel);
	struct pwm_npcx_data *const data = dev->data;

	*cycles = data->cycles_per_sec;
	return 0;
}

/* PWM driver registration */
static DEVICE_API(pwm, pwm_npcx_driver_api) = {
	.set_cycles = pwm_npcx_set_cycles,
	.get_cycles_per_sec = pwm_npcx_get_cycles_per_sec
};

static int pwm_npcx_init(const struct device *dev)
{
	const struct pwm_npcx_config *const config = dev->config;
	struct pwm_npcx_data *const data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)
							&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on PWM clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)
			&config->clk_cfg, &data->cycles_per_sec);
	if (ret < 0) {
		LOG_ERR("Get PWM clock rate error %d", ret);
		return ret;
	}

	/* Configure PWM device initially */
	pwm_npcx_configure(dev, config->clk_cfg.bus);

	/* Configure pin-mux for PWM device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}

#define NPCX_PWM_INIT(inst)                                                    \
	PINCTRL_DT_INST_DEFINE(inst);					       \
									       \
	static const struct pwm_npcx_config pwm_npcx_cfg_##inst = {            \
		.base = (struct pwm_reg *)DT_INST_REG_ADDR(inst),              \
		.clk_cfg = NPCX_DT_CLK_CFG_ITEM(inst),                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                  \
	};                                                                     \
									       \
	static struct pwm_npcx_data pwm_npcx_data_##inst;                      \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			    &pwm_npcx_init, NULL,			       \
			    &pwm_npcx_data_##inst, &pwm_npcx_cfg_##inst,       \
			    PRE_KERNEL_1, CONFIG_PWM_INIT_PRIORITY,	       \
			    &pwm_npcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPCX_PWM_INIT)
