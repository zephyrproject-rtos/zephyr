/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_pwm

#include <assert.h>
#include <drivers/pwm.h>
#include <dt-bindings/clock/npcx_clock.h>
#include <drivers/clock_control.h>
#include <kernel.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_npcx, LOG_LEVEL_ERR);

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
	uintptr_t base;
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* pinmux configuration */
	const uint8_t alts_size;
	const struct npcx_alt *alts_list;
};

/* Driver data */
struct pwm_npcx_data {
	/* PWM cycles per second */
	uint32_t cycles_per_sec;
};

/* Driver convenience defines */
#define DRV_CONFIG(dev) ((const struct pwm_npcx_config *)(dev)->config)

#define DRV_DATA(dev) ((struct pwm_npcx_data *)(dev)->data)

#define HAL_INSTANCE(dev) (struct pwm_reg *)(DRV_CONFIG(dev)->base)

/* PWM local functions */
static void pwm_npcx_configure(const struct device *dev, int clk_bus)
{
	struct pwm_reg *const inst = HAL_INSTANCE(dev);

	/* Disable PWM for module configuration first */
	inst->PWMCTL &= ~BIT(NPCX_PWMCTL_PWR);

	/* Set default PWM polarity to normal */
	inst->PWMCTL &= ~BIT(NPCX_PWMCTL_INVP);

	/* Turn off PWM heart-beat mode */
	SET_FIELD(inst->PWMCTL, NPCX_PWMCTL_HB_DC_CTL_FIELD,
			NPCX_PWM_HBM_NORMAL);

	/* Select APB CLK/LFCLK clock sources to PWM module by default */
	SET_FIELD(inst->PWMCTLEX, NPCX_PWMCTLEX_FCK_SEL_FIELD,
			NPCX_PWM_CLOCK_APB2_LFCLK);

	/* Select clock source to LFCLK by flag, otherwise APB clock source */
	if (clk_bus == NPCX_CLOCK_BUS_LFCLK)
		inst->PWMCTL |= BIT(NPCX_PWMCTL_CKSEL);
	else
		inst->PWMCTL &= ~BIT(NPCX_PWMCTL_CKSEL);
}

/* PWM api functions */
static int pwm_npcx_pin_set(const struct device *dev, uint32_t pwm,
			   uint32_t period_cycles, uint32_t pulse_cycles,
			   pwm_flags_t flags)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(pwm);
	struct pwm_npcx_data *const data = DRV_DATA(dev);
	struct pwm_reg *const inst = HAL_INSTANCE(dev);
	int prescaler;

	if (pulse_cycles > period_cycles)
		return -EINVAL;

	/* Disable PWM before configuring */
	inst->PWMCTL &= ~BIT(NPCX_PWMCTL_PWR);

	/* Select PWM inverted polarity (ie. active-low pulse). */
	if (flags & PWM_POLARITY_INVERTED)
		inst->PWMCTL |= BIT(NPCX_PWMCTL_INVP);
	else
		inst->PWMCTL &= ~BIT(NPCX_PWMCTL_INVP);

	/* If pulse_cycles is 0, return directly since PWM is already off */
	if (pulse_cycles == 0)
		return 0;

	/*
	 * Calculate PWM prescaler that let period_cycles map to
	 * maximum pwm period cycles and won't exceed it.
	 * Then prescaler = ceil (period_cycles / pwm_max_period_cycles)
	 */
	prescaler = ceiling_fraction(period_cycles, NPCX_PWM_MAX_PERIOD_CYCLES);
	if (prescaler > NPCX_PWM_MAX_PRESCALER)
		return -EINVAL;

	/* Set PWM prescaler.*/
	inst->PRSC = prescaler - 1;

	/* Set PWM period cycles */
	inst->CTR = (period_cycles / prescaler) - 1;

	/* Set PWM pulse cycles */
	inst->DCR = (pulse_cycles / prescaler) - 1;

	LOG_DBG("freq %d, pre %d, period %d, pulse %d",
			data->cycles_per_sec / period_cycles,
			inst->PRSC + 1, inst->CTR + 1, inst->DCR + 1);

	/* Enable PWM now */
	inst->PWMCTL |= BIT(NPCX_PWMCTL_PWR);

	return 0;
}

static int pwm_npcx_get_cycles_per_sec(const struct device *dev, uint32_t pwm,
				      uint64_t *cycles)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(pwm);
	struct pwm_npcx_data *const data = DRV_DATA(dev);

	*cycles = data->cycles_per_sec;
	return 0;
}

/* PWM driver registration */
static const struct pwm_driver_api pwm_npcx_driver_api = {
	.pin_set = pwm_npcx_pin_set,
	.get_cycles_per_sec = pwm_npcx_get_cycles_per_sec
};

static int pwm_npcx_init(const struct device *dev)
{
	const struct pwm_npcx_config *const config = DRV_CONFIG(dev);
	struct pwm_npcx_data *const data = DRV_DATA(dev);
	struct pwm_reg *const inst = HAL_INSTANCE(dev);
	const struct device *const clk_dev =
					device_get_binding(NPCX_CLK_CTRL_NAME);
	int ret;

	/*
	 * NPCX PWM modulee mixes byte and word registers together. Make sure
	 * word reg access via structure won't break into two byte reg accesses
	 * unexpectedly by toolchains options or attributes. If so, stall here.
	 */
	NPCX_REG_WORD_ACCESS_CHECK(inst->PRSC, 0xA55A);


	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t *)
							&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on PWM clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t *)
			&config->clk_cfg, &data->cycles_per_sec);
	if (ret < 0) {
		LOG_ERR("Get PWM clock rate error %d", ret);
		return ret;
	}

	/* Configure PWM device initially */
	pwm_npcx_configure(dev, config->clk_cfg.bus);

	/* Configure pin-mux for PWM device */
	npcx_pinctrl_mux_configure(config->alts_list, config->alts_size, 1);

	return 0;
}

#define NPCX_PWM_INIT(inst)                                                    \
	static const struct npcx_alt pwm_alts##inst[] =			       \
					NPCX_DT_ALT_ITEMS_LIST(inst);          \
									       \
	static const struct pwm_npcx_config pwm_npcx_cfg_##inst = {            \
		.base = DT_INST_REG_ADDR(inst),                                \
		.clk_cfg = NPCX_DT_CLK_CFG_ITEM(inst),                         \
		.alts_size = ARRAY_SIZE(pwm_alts##inst),                       \
		.alts_list = pwm_alts##inst,                                   \
	};                                                                     \
									       \
	static struct pwm_npcx_data pwm_npcx_data_##inst;                      \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			    &pwm_npcx_init, device_pm_control_nop,             \
			    &pwm_npcx_data_##inst, &pwm_npcx_cfg_##inst,       \
			    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			    &pwm_npcx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NPCX_PWM_INIT)
