/*
 * Copyright (c) 2020 Google LLC.
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * PWM driver using the SAM0 Timer/Counter (TCC) in Normal PWM (NPWM) mode.
 * Supports the SAMD21 and SAMD5x series.
 */

#define DT_DRV_COMPAT atmel_sam0_tcc_pwm

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

/* clang-format off */

/* Static configuration */
struct pwm_sam0_config {
	Tcc *regs;
	const struct pinctrl_dev_config *pcfg;
	uint8_t channels;
	uint8_t counter_size;
	uint16_t prescaler;
	uint32_t freq;
	volatile uint32_t *mclk;
	uint32_t mclk_mask;
	uint32_t gclk_gen;
	uint16_t gclk_id;
};

/* Wait for the peripheral to finish all commands */
static void wait_synchronization(Tcc *regs)
{
	while (regs->SYNCBUSY.reg != 0) {
	}
}

static int pwm_sam0_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	const struct pwm_sam0_config *const cfg = dev->config;

	if (channel >= cfg->channels) {
		return -EINVAL;
	}
	*cycles = cfg->freq;

	return 0;
}

static int pwm_sam0_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_sam0_config *const cfg = dev->config;
	Tcc *regs = cfg->regs;
	uint32_t top = 1 << cfg->counter_size;
	uint32_t invert_mask = 1 << channel;
	bool invert = ((flags & PWM_POLARITY_INVERTED) != 0);
	bool inverted = ((regs->DRVCTRL.vec.INVEN & invert_mask) != 0);

	if (channel >= cfg->channels) {
		return -EINVAL;
	}
	if (period_cycles >= top || pulse_cycles >= top) {
		return -EINVAL;
	}

	/*
	 * Update the buffered width and period.  These will be automatically
	 * loaded on the next cycle.
	 */
#ifdef TCC_PERBUF_PERBUF
	/* SAME51 naming */
	regs->CCBUF[channel].reg = TCC_CCBUF_CCBUF(pulse_cycles);
	regs->PERBUF.reg = TCC_PERBUF_PERBUF(period_cycles);
#else
	/* SAMD21 naming */
	regs->CCB[channel].reg = TCC_CCB_CCB(pulse_cycles);
	regs->PERB.reg = TCC_PERB_PERB(period_cycles);
#endif

	if (invert != inverted) {
		regs->CTRLA.bit.ENABLE = 0;
		wait_synchronization(regs);

		regs->DRVCTRL.vec.INVEN ^= invert_mask;
		regs->CTRLA.bit.ENABLE = 1;
		wait_synchronization(regs);
	}

	return 0;
}

static int pwm_sam0_init(const struct device *dev)
{
	const struct pwm_sam0_config *const cfg = dev->config;
	Tcc *regs = cfg->regs;
	int retval;

	*cfg->mclk |= cfg->mclk_mask;

#ifdef MCLK
	GCLK->PCHCTRL[cfg->gclk_id].reg = GCLK_PCHCTRL_CHEN
					| GCLK_PCHCTRL_GEN(cfg->gclk_gen);
#else
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN
			  | GCLK_CLKCTRL_GEN(cfg->gclk_gen)
			  | GCLK_CLKCTRL_ID(cfg->gclk_id);
#endif

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	regs->CTRLA.bit.SWRST = 1;
	wait_synchronization(regs);

	regs->CTRLA.reg = cfg->prescaler;
	regs->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
	regs->PER.reg = TCC_PER_PER(1);

	regs->CTRLA.bit.ENABLE = 1;
	wait_synchronization(regs);

	return 0;
}

static DEVICE_API(pwm, pwm_sam0_driver_api) = {
	.set_cycles = pwm_sam0_set_cycles,
	.get_cycles_per_sec = pwm_sam0_get_cycles_per_sec,
};

#define ASSIGNED_CLOCKS_CELL_BY_NAME						\
	ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_NAME

#define PWM_SAM0_INIT(inst)							\
	PINCTRL_DT_INST_DEFINE(inst);						\
	static const struct pwm_sam0_config pwm_sam0_config_##inst = {		\
		.regs = (Tcc *)DT_INST_REG_ADDR(inst),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.channels = DT_INST_PROP(inst, channels),			\
		.counter_size = DT_INST_PROP(inst, counter_size),		\
		.prescaler = UTIL_CAT(TCC_CTRLA_PRESCALER_DIV,			\
				      DT_INST_PROP(inst, prescaler)),		\
		.freq = SOC_ATMEL_SAM0_GCLK0_FREQ_HZ /				\
			DT_INST_PROP(inst, prescaler),				\
		.gclk_gen = ASSIGNED_CLOCKS_CELL_BY_NAME(inst, gclk, gen),	\
		.gclk_id = DT_INST_CLOCKS_CELL_BY_NAME(inst, gclk, id),		\
		.mclk = ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(inst),	\
		.mclk_mask = ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(inst, bit),	\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, &pwm_sam0_init, NULL,			\
			      NULL, &pwm_sam0_config_##inst,			\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,		\
			      &pwm_sam0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_SAM0_INIT)

/* clang-format on */
