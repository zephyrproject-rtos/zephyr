/*
 * Copyright (c) 2019 Aurelien Jarno
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_pwm

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

#ifdef CONFIG_PWM_INTERRUPT
#include <zephyr/irq.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_sam, CONFIG_PWM_LOG_LEVEL);

/* Some SoCs use a slightly different naming scheme */
#if !defined(PWMCHNUM_NUMBER) && defined(PWMCH_NUM_NUMBER)
#define PWMCHNUM_NUMBER PWMCH_NUM_NUMBER
#endif

/* The SAMV71 HALs change the name of the field, so we need to
 * define it this way to match how the other SoC variants name it
 */
#if defined(CONFIG_SOC_ATMEL_SAMV71) || defined(CONFIG_SOC_ATMEL_SAMV71_REVB)
#define PWM_CH_NUM PwmChNum
#endif

struct sam_pwm_config {
	Pwm *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	uint8_t prescaler;
	uint8_t divider;
#ifdef CONFIG_PWM_INTERRUPT
	void (*irq_config)(void);
#endif /* CONFIG_PWM_INTERRUPT */
};

struct sam_pwm_data {
#ifdef CONFIG_PWM_INTERRUPT
	pwm_interrupt_callback_handler_t callback;
	void *user_data;
#endif /* CONFIG_PWM_INTERRUPT */
};

static int sam_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	const struct sam_pwm_config *config = dev->config;
	uint8_t prescaler = config->prescaler;
	uint8_t divider = config->divider;

	*cycles = SOC_ATMEL_SAM_MCK_FREQ_HZ /
		  ((1 << prescaler) * divider);

	return 0;
}

static int sam_pwm_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	const struct sam_pwm_config *config = dev->config;

	Pwm * const pwm = config->regs;
	uint32_t cmr;

	if (channel >= PWMCHNUM_NUMBER) {
		return -EINVAL;
	}

	if (period_cycles == 0U) {
		return -ENOTSUP;
	}

	if (period_cycles > 0xffff) {
		return -ENOTSUP;
	}

	/* Select clock A */
	cmr = PWM_CMR_CPRE_CLKA;

	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_NORMAL) {
		cmr |= PWM_CMR_CPOL;
	}

	/* Disable the output if changing polarity (or clock) */
	if (pwm->PWM_CH_NUM[channel].PWM_CMR != cmr) {
		pwm->PWM_DIS = 1 << channel;

		pwm->PWM_CH_NUM[channel].PWM_CMR = cmr;
		pwm->PWM_CH_NUM[channel].PWM_CPRD = period_cycles;
		pwm->PWM_CH_NUM[channel].PWM_CDTY = pulse_cycles;
	} else {
		/* Update period and pulse using the update registers, so that the
		 * change is triggered at the next PWM period.
		 */
		pwm->PWM_CH_NUM[channel].PWM_CPRDUPD = period_cycles;
		pwm->PWM_CH_NUM[channel].PWM_CDTYUPD = pulse_cycles;
	}

	/* Enable the output */
	pwm->PWM_ENA = 1 << channel;

	return 0;
}

#ifdef CONFIG_PWM_INTERRUPT
static void sam_pwm_isr(const struct device *dev)
{
	const struct sam_pwm_config *config = dev->config;
	struct sam_pwm_data *data = dev->data;
	Pwm *const pwm = config->regs;
	pwm_flags_t flags;

	uint32_t status = pwm->PWM_ISR1;

	if (data->callback == NULL) {
		return;
	}

	for (uint32_t i = 0; i < PWMCHNUM_NUMBER; i++) {
		flags = 0;
		if ((status & (PWM_ISR1_CHID0 << i)) != 0) {
			flags |= PWM_INTERRUPT_TYPE_PERIOD;
		}
		if ((status & (PWM_ISR1_FCHID0 << i)) != 0) {
			flags |= PWM_INTERRUPT_TYPE_FAULT;
		}

		if (flags > 0) {
			data->callback(dev, i, flags, data->user_data);
		}
	}
}

static int sam_pwm_configure_interrupt(const struct device *dev,
				       pwm_interrupt_callback_handler_t cb, void *user_data)
{
	struct sam_pwm_data *data = dev->data;

	data->callback = cb;
	data->user_data = user_data;

	return 0;
}

static int sam_pwm_enable_interrupt(const struct device *dev, uint32_t channel, pwm_flags_t flags)
{
	const struct sam_pwm_config *config = dev->config;
	uint32_t pwm_ier1 = 0;

	Pwm *const pwm = config->regs;

	/* Dummy read to clear status register*/
	(void)pwm->PWM_ISR1;

	if ((flags & PWM_INTERRUPT_TYPE_PERIOD) != 0) {
		pwm_ier1 |= (PWM_ENA_CHID0 << channel);
	}
	if ((flags & PWM_INTERRUPT_TYPE_FAULT) != 0) {
		pwm_ier1 |= (PWM_IER1_FCHID0 << channel);
	}

	pwm->PWM_IER1 = pwm_ier1;

	return 0;
}

static int sam_pwm_disable_interrupt(const struct device *dev, uint32_t channel, pwm_flags_t flags)
{
	const struct sam_pwm_config *config = dev->config;
	uint32_t pwm_idr1 = 0;

	Pwm *const pwm = config->regs;

	if ((flags & PWM_INTERRUPT_TYPE_PERIOD) != 0) {
		pwm_idr1 |= (PWM_IDR1_CHID0 << channel);
	}
	if ((flags & PWM_INTERRUPT_TYPE_FAULT) != 0) {
		pwm_idr1 |= (PWM_IDR1_FCHID0 << channel);
	}

	pwm->PWM_IDR1 = pwm_idr1;

	return 0;
}
#endif /* CONFIG_PWM_INTERRUPT */

static int sam_pwm_init(const struct device *dev)
{
	const struct sam_pwm_config *config = dev->config;

	Pwm * const pwm = config->regs;
	uint8_t prescaler = config->prescaler;
	uint8_t divider = config->divider;
	int retval;

	/* FIXME: way to validate prescaler & divider */

#ifdef CONFIG_PWM_INTERRUPT
	config->irq_config();
#endif

	/* Enable PWM clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&config->clock_cfg);

	retval = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	/* Configure the clock A that will be used by all 4 channels */
	pwm->PWM_CLK = PWM_CLK_PREA(prescaler) | PWM_CLK_DIVA(divider);

	return 0;
}

static DEVICE_API(pwm, sam_pwm_driver_api) = {
	.set_cycles = sam_pwm_set_cycles,
	.get_cycles_per_sec = sam_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_INTERRUPT
	.configure_interrupt = sam_pwm_configure_interrupt,
	.enable_interrupt = sam_pwm_enable_interrupt,
	.disable_interrupt = sam_pwm_disable_interrupt,
#endif /* CONFIG_PWM_INTERRUPT */
};

#define SAM_PWM_INTERRUPT_INIT(inst)					\
	static void sam_pwm_irq_config_##inst(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst), 0, sam_pwm_isr,		\
			    DEVICE_DT_INST_GET(inst), 0);		\
		irq_enable(DT_INST_IRQN(inst));				\
	}

#define SAM_INST_INIT(inst)						\
	PINCTRL_DT_INST_DEFINE(inst);					\
									\
	IF_ENABLED(CONFIG_PWM_INTERRUPT, (SAM_PWM_INTERRUPT_INIT(inst)))\
									\
	static const struct sam_pwm_config sam_pwm_config_##inst = {	\
		.regs = (Pwm *)DT_INST_REG_ADDR(inst),			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),		\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(inst),		\
		.prescaler = DT_INST_PROP(inst, prescaler),		\
		.divider = DT_INST_PROP(inst, divider),			\
		IF_ENABLED(CONFIG_PWM_INTERRUPT,			\
			   (.irq_config = sam_pwm_irq_config_##inst,))	\
	};								\
									\
	static struct sam_pwm_data sam_pwm_data_##inst;			\
									\
	DEVICE_DT_INST_DEFINE(inst, &sam_pwm_init, NULL,		\
			      &sam_pwm_data_##inst,			\
			      &sam_pwm_config_##inst, POST_KERNEL,	\
			      CONFIG_PWM_INIT_PRIORITY,			\
			      &sam_pwm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SAM_INST_INIT)
