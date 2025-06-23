/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_pwm

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/clock/nct_clock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_nct, LOG_LEVEL_ERR);

/* 16-bit period cycles/prescaler in NCT PWM modules */
#define NCT_PWM_MAX_PRESCALER      (1UL << (16))
#define NCT_PWM_MAX_PERIOD_CYCLES  (1UL << (16))

#define NCT_PWM_LFCLK              32768

/* PWM clock sources */
#define NCT_PWM_CLOCK_APB2_LFCLK   0
#define NCT_PWM_CLOCK_FX           1
#define NCT_PWM_CLOCK_FR           2
#define NCT_PWM_CLOCK_RESERVED     3

/* PWM heart-beat mode selection */
#define NCT_PWM_HBM_NORMAL         0

#define PWM_TYPE_ODPPMSK 0x80

/* Device config */
struct pwm_nct_config {
	/* pwm controller base address */
	struct pwm_reg *base;
	/* clock configuration */
	uint32_t clk_cfg;
	/* pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
};

/* Driver data */
struct pwm_nct_data {
	/* PWM cycles per second */
	uint32_t cycles_per_sec;
};

/* PWM local functions */
static int pwm_nct_configure(const struct device *dev, uint32_t cycles)
{
	const struct pwm_nct_config *config = dev->config;
	struct pwm_reg *inst = config->base;

	/* Disable PWM for module configuration first */
	inst->PWMCTL &= ~BIT(NCT_PWMCTL_PWR);

	/* Set default PWM polarity to normal */
	inst->PWMCTL &= ~BIT(NCT_PWMCTL_INVP);

	/* Turn off PWM heart-beat mode */
	SET_FIELD(inst->PWMCTL, NCT_PWMCTL_HB_DC_CTL_FIELD,
			NCT_PWM_HBM_NORMAL);

	/* Select APB CLK/LFCLK clock sources to PWM module by default */
	SET_FIELD(inst->PWMCTLEX, NCT_PWMCTLEX_FCK_SEL_FIELD,
			NCT_PWM_CLOCK_APB2_LFCLK);

	/* Select input clock source to LFCLK or APB2 */
	if (cycles == NCT_PWM_LFCLK) {
		inst->PWMCTL |= BIT(NCT_PWMCTL_CKSEL);
	} else {
		inst->PWMCTL &= ~BIT(NCT_PWMCTL_CKSEL);
	}

	return 0;
}

/* PWM api functions */
static int pwm_nct_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(channel);
	const struct pwm_nct_config *config = dev->config;
	struct pwm_nct_data *const data = dev->data;
	struct pwm_reg *inst = config->base;
	int prescaler;
	uint32_t ctl;
	uint32_t ctr;
	uint32_t dcr;
	uint32_t prsc;

	ctl = inst->PWMCTL | BIT(NCT_PWMCTL_PWR);

	/* Select PWM inverted polarity (ie. active-low pulse). */
	if (flags & PWM_POLARITY_INVERTED) {
		ctl |= BIT(NCT_PWMCTL_INVP);
	} else {
		ctl &= ~BIT(NCT_PWMCTL_INVP);
	}

	/* Select PWM type*/
	if (flags & PWM_TYPE_ODPPMSK) {
		inst->PWMCTLEX |= BIT(NCT_PWMCTLEX_OD_OUT);
	} else {
		inst->PWMCTLEX &= ~BIT(NCT_PWMCTLEX_OD_OUT);
	}

	/* If pulse_cycles is 0, switch PWM off and return. */
	if (pulse_cycles == 0) {
		ctl &= ~BIT(NCT_PWMCTL_PWR);
		inst->PWMCTL = ctl;
		return 0;
	}

	/*
	 * Calculate PWM prescaler that let period_cycles map to
	 * maximum pwm period cycles and won't exceed it.
	 * Then prescaler = ceil (period_cycles / pwm_max_period_cycles)
	 */
	prescaler = DIV_ROUND_UP(period_cycles, NCT_PWM_MAX_PERIOD_CYCLES);
	if (prescaler > NCT_PWM_MAX_PRESCALER) {
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
		inst->PWMCTL &= ~BIT(NCT_PWMCTL_PWR);

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

static int pwm_nct_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(channel);
	struct pwm_nct_data *const data = dev->data;

	*cycles = data->cycles_per_sec;
	return 0;
}

/* PWM driver registration */
static const struct pwm_driver_api pwm_nct_driver_api = {
	.set_cycles = pwm_nct_set_cycles,
	.get_cycles_per_sec = pwm_nct_get_cycles_per_sec
};

static int pwm_nct_init(const struct device *dev)
{
	const struct pwm_nct_config *const config = dev->config;
	struct pwm_nct_data *const data = dev->data;
	struct pwm_reg *const inst = config->base;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	int ret;

	/*
	 * NCT PWM module mixes byte and word registers together. Make sure
	 * word reg access via structure won't break into two byte reg accesses
	 * unexpectedly by toolchains options or attributes. If so, stall here.
	 */
	NCT_REG_WORD_ACCESS_CHECK(inst->PRSC, 0xA55A);


	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)
							config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on PWM clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)
			config->clk_cfg, &data->cycles_per_sec);
	if (ret < 0) {
		LOG_ERR("Get PWM clock rate error %d", ret);
		return ret;
	}

	/* Bus only could be APB2 or Low frequency clock */
	if (((config->clk_cfg >> NCT_CLOCK_BUS_OFFSET) & NCT_CLOCK_BUS_MASK) !=
			NCT_CLOCK_BUS_APB2) {
		if ((data->cycles_per_sec != NCT_PWM_LFCLK)) {
			LOG_ERR("PWM only support source LF or APB2");
			return -EINVAL;
		}
	}

	/* Configure PWM device initially */
	ret = pwm_nct_configure(dev, data->cycles_per_sec);
	if (ret < 0) {
		LOG_ERR("PWM configure failed (%d)", ret);
		return ret;
	}

	/* Configure pin-mux for PWM device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}

#define NCT_PWM_INIT(inst)                                                             \
	PINCTRL_DT_INST_DEFINE(inst);		 			                \
									                \
	static const struct pwm_nct_config pwm_nct_cfg_##inst = {                     \
		.base = (struct pwm_reg *)DT_INST_REG_ADDR(inst),                       \
		.clk_cfg = DT_INST_PHA(inst, clocks, clk_cfg),                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                           \
	};                                                                              \
									                \
	static struct pwm_nct_data pwm_nct_data_##inst;                               \
									                \
	DEVICE_DT_INST_DEFINE(inst,					                \
			    &pwm_nct_init, NULL,			                \
			    &pwm_nct_data_##inst, &pwm_nct_cfg_##inst,                \
			    PRE_KERNEL_1, CONFIG_PWM_INIT_PRIORITY,	                \
			    &pwm_nct_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NCT_PWM_INIT)
