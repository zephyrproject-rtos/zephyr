/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * PWM driver using the SAM0 Timer/Counter (TCC) in Normal PWM (NPWM) mode.
 * Supports the SAMD21 and SAMD5x series.
 */

#define DT_DRV_COMPAT atmel_sam0_tcc_pwm

#include <device.h>
#include <errno.h>
#include <drivers/pwm.h>
#include <soc.h>

/* Static configuration */
struct pwm_sam0_config {
	Tcc *regs;
	uint8_t channels;
	uint8_t counter_size;
	uint16_t prescaler;
	uint32_t freq;

#ifdef MCLK
	volatile uint32_t *mclk;
	uint32_t mclk_mask;
	uint16_t gclk_id;
#else
	uint32_t pm_apbcmask;
	uint16_t gclk_clkctrl_id;
#endif
};

#define DEV_CFG(dev) ((const struct pwm_sam0_config *const)(dev)->config)

/* Wait for the peripheral to finish all commands */
static void wait_synchronization(Tcc *regs)
{
	while (regs->SYNCBUSY.reg != 0) {
	}
}

static int pwm_sam0_get_cycles_per_sec(const struct device *dev, uint32_t ch,
				       uint64_t *cycles)
{
	const struct pwm_sam0_config *const cfg = DEV_CFG(dev);

	if (ch >= cfg->channels) {
		return -EINVAL;
	}
	*cycles = cfg->freq;

	return 0;
}

static int pwm_sam0_pin_set(const struct device *dev, uint32_t ch,
			    uint32_t period_cycles, uint32_t pulse_cycles,
			    pwm_flags_t flags)
{
	const struct pwm_sam0_config *const cfg = DEV_CFG(dev);
	Tcc *regs = cfg->regs;
	uint32_t top = 1 << cfg->counter_size;
	uint32_t invert_mask = 1 << ch;
	bool invert = ((flags & PWM_POLARITY_INVERTED) != 0);
	bool inverted = ((regs->DRVCTRL.vec.INVEN & invert_mask) != 0);

	if (ch >= cfg->channels) {
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
	regs->CCBUF[ch].reg = TCC_CCBUF_CCBUF(pulse_cycles);
	regs->PERBUF.reg = TCC_PERBUF_PERBUF(period_cycles);
#else
	/* SAMD21 naming */
	regs->CCB[ch].reg = TCC_CCB_CCB(pulse_cycles);
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
	const struct pwm_sam0_config *const cfg = DEV_CFG(dev);
	Tcc *regs = cfg->regs;

	/* Enable the clocks */
#ifdef MCLK
	GCLK->PCHCTRL[cfg->gclk_id].reg =
		GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
	*cfg->mclk |= cfg->mclk_mask;
#else
	GCLK->CLKCTRL.reg = cfg->gclk_clkctrl_id | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;
	PM->APBCMASK.reg |= cfg->pm_apbcmask;
#endif

	regs->CTRLA.bit.SWRST = 1;
	wait_synchronization(regs);

	regs->CTRLA.reg = cfg->prescaler;
	regs->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
	regs->PER.reg = TCC_PER_PER(1);

	regs->CTRLA.bit.ENABLE = 1;
	wait_synchronization(regs);

	return 0;
}

static const struct pwm_driver_api pwm_sam0_driver_api = {
	.pin_set = pwm_sam0_pin_set,
	.get_cycles_per_sec = pwm_sam0_get_cycles_per_sec,
};

#ifdef MCLK
#define PWM_SAM0_INIT_CLOCKS(inst)					       \
	.mclk = (volatile uint32_t *)MCLK_MASK_DT_INT_REG_ADDR(inst),	       \
	.mclk_mask = BIT(DT_INST_CLOCKS_CELL_BY_NAME(inst, mclk, bit)),	       \
	.gclk_id = DT_INST_CLOCKS_CELL_BY_NAME(inst, gclk, periph_ch)
#else
#define PWM_SAM0_INIT_CLOCKS(inst)					       \
	.pm_apbcmask = BIT(DT_INST_CLOCKS_CELL_BY_NAME(inst, pm, bit)),	       \
	.gclk_clkctrl_id = DT_INST_CLOCKS_CELL_BY_NAME(inst, gclk, clkctrl_id)
#endif

#define PWM_SAM0_INIT(inst)						       \
	static const struct pwm_sam0_config pwm_sam0_config_##inst = {	       \
		.regs = (Tcc *)DT_INST_REG_ADDR(inst),			       \
		.channels = DT_INST_PROP(inst, channels),		       \
		.counter_size = DT_INST_PROP(inst, counter_size),	       \
		.prescaler = UTIL_CAT(TCC_CTRLA_PRESCALER_DIV,		       \
				      DT_INST_PROP(inst, prescaler)),	       \
		.freq = SOC_ATMEL_SAM0_GCLK0_FREQ_HZ /			       \
			DT_INST_PROP(inst, prescaler),			       \
		PWM_SAM0_INIT_CLOCKS(inst),				       \
	};								       \
									       \
	DEVICE_AND_API_INIT(pwm_sam0_##inst, DT_INST_LABEL(inst),	       \
			    &pwm_sam0_init, NULL, &pwm_sam0_config_##inst,     \
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,   \
			    &pwm_sam0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_SAM0_INIT)
