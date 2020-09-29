/*
 * Copyright (c) 2018, Diego Sueiro <diego.sueiro@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <drivers/pwm.h>
#include <soc.h>
#include <device_imx.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_imx);

#define PWM_PWMSR_FIFOAV_4WORDS	0x4

#define PWM_PWMCR_SWR(x) (((uint32_t)(((uint32_t)(x)) \
				<<PWM_PWMCR_SWR_SHIFT))&PWM_PWMCR_SWR_MASK)

#define DEV_CFG(dev) \
	((const struct imx_pwm_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct imx_pwm_data * const)(dev)->data)
#define DEV_BASE(dev) \
	((PWM_Type *)(DEV_CFG(dev))->base)

struct imx_pwm_config {
	PWM_Type *base;
	uint16_t prescaler;
};

struct imx_pwm_data {
	uint32_t period_cycles;
};


static bool imx_pwm_is_enabled(PWM_Type *base)
{
	return PWM_PWMCR_REG(base) & PWM_PWMCR_EN_MASK;
}

static int imx_pwm_get_cycles_per_sec(const struct device *dev, uint32_t pwm,
				       uint64_t *cycles)
{
	PWM_Type *base = DEV_BASE(dev);
	const struct imx_pwm_config *config = DEV_CFG(dev);

	*cycles = get_pwm_clock_freq(base) >> config->prescaler;

	return 0;
}

static int imx_pwm_pin_set(const struct device *dev, uint32_t pwm,
			   uint32_t period_cycles, uint32_t pulse_cycles,
			   pwm_flags_t flags)
{
	PWM_Type *base = DEV_BASE(dev);
	const struct imx_pwm_config *config = DEV_CFG(dev);
	struct imx_pwm_data *data = DEV_DATA(dev);
	unsigned int period_ms;
	bool enabled = imx_pwm_is_enabled(base);
	int wait_count = 0, fifoav;
	uint32_t cr, sr;


	if ((period_cycles == 0U) || (pulse_cycles > period_cycles)) {
		LOG_ERR("Invalid combination: period_cycles=%d, "
			    "pulse_cycles=%d", period_cycles, pulse_cycles);
		return -EINVAL;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	LOG_DBG("enabled=%d, pulse_cycles=%d, period_cycles=%d,"
		    " duty_cycle=%d\n", enabled, pulse_cycles, period_cycles,
		    (pulse_cycles * 100U / period_cycles));

	/*
	 * i.MX PWMv2 has a 4-word sample FIFO.
	 * In order to avoid FIFO overflow issue, we do software reset
	 * to clear all sample FIFO if the controller is disabled or
	 * wait for a full PWM cycle to get a relinquished FIFO slot
	 * when the controller is enabled and the FIFO is fully loaded.
	 */
	if (enabled) {
		sr = PWM_PWMSR_REG(base);
		fifoav = PWM_PWMSR_FIFOAV(sr);
		if (fifoav == PWM_PWMSR_FIFOAV_4WORDS) {
			period_ms = (get_pwm_clock_freq(base) >>
					config->prescaler) * MSEC_PER_SEC;
			k_sleep(K_MSEC(period_ms));

			sr = PWM_PWMSR_REG(base);
			if (fifoav == PWM_PWMSR_FIFOAV(sr)) {
				LOG_WRN("there is no free FIFO slot\n");
			}
		}
	} else {
		PWM_PWMCR_REG(base) = PWM_PWMCR_SWR(1);
		do {
			k_sleep(K_MSEC(1));
			cr = PWM_PWMCR_REG(base);
		} while ((PWM_PWMCR_SWR(cr)) &&
			 (++wait_count < CONFIG_PWM_PWMSWR_LOOP));

		if (PWM_PWMCR_SWR(cr)) {
			LOG_WRN("software reset timeout\n");
		}

	}

	/*
	 * according to imx pwm RM, the real period value should be
	 * PERIOD value in PWMPR plus 2.
	 */
	if (period_cycles > 2) {
		period_cycles -= 2U;
	} else {
		return -EINVAL;
	}

	PWM_PWMSAR_REG(base) = pulse_cycles;

	if (data->period_cycles != period_cycles) {
		LOG_WRN("Changing period cycles from %d to %d in %s",
			    data->period_cycles, period_cycles,
			    dev->name);

		data->period_cycles = period_cycles;
		PWM_PWMPR_REG(base) = period_cycles;
	}

	cr = PWM_PWMCR_EN_MASK | PWM_PWMCR_PRESCALER(config->prescaler) |
		PWM_PWMCR_DOZEN_MASK | PWM_PWMCR_WAITEN_MASK |
		PWM_PWMCR_DBGEN_MASK | PWM_PWMCR_CLKSRC(2);

	PWM_PWMCR_REG(base) = cr;

	return 0;
}

static int imx_pwm_init(const struct device *dev)
{
	struct imx_pwm_data *data = DEV_DATA(dev);
	PWM_Type *base = DEV_BASE(dev);

	PWM_PWMPR_REG(base) = data->period_cycles;

	return 0;
}

static const struct pwm_driver_api imx_pwm_driver_api = {
	.pin_set = imx_pwm_pin_set,
	.get_cycles_per_sec = imx_pwm_get_cycles_per_sec,
};

#define PWM_IMX_INIT(n)							\
	static const struct imx_pwm_config imx_pwm_config_##n = {	\
		.base = (PWM_Type *)DT_INST_REG_ADDR(n),		\
		.prescaler = DT_INST_PROP(n, prescaler),		\
	};								\
									\
	static struct imx_pwm_data imx_pwm_data_##n;			\
									\
	DEVICE_AND_API_INIT(imx_pwm_##n, DT_INST_LABEL(n),		\
			    &imx_pwm_init, &imx_pwm_data_##n,		\
			    &imx_pwm_config_##n, POST_KERNEL,		\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &imx_pwm_driver_api);

#if DT_HAS_COMPAT_STATUS_OKAY(fsl_imx7d_pwm)
#define DT_DRV_COMPAT fsl_imx7d_pwm
DT_INST_FOREACH_STATUS_OKAY(PWM_IMX_INIT)
#endif
