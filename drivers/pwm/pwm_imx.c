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
	((const struct imx_pwm_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct imx_pwm_data * const)(dev)->driver_data)
#define DEV_BASE(dev) \
	((PWM_Type *)(DEV_CFG(dev))->base)

struct imx_pwm_config {
	PWM_Type *base;
	u16_t prescaler;
};

struct imx_pwm_data {
	u32_t period_cycles;
};


static bool imx_pwm_is_enabled(PWM_Type *base)
{
	return PWM_PWMCR_REG(base) & PWM_PWMCR_EN_MASK;
}

static int imx_pwm_get_cycles_per_sec(struct device *dev, u32_t pwm,
				       u64_t *cycles)
{
	PWM_Type *base = DEV_BASE(dev);
	const struct imx_pwm_config *config = DEV_CFG(dev);

	*cycles = get_pwm_clock_freq(base) >> config->prescaler;

	return 0;
}

static int imx_pwm_pin_set(struct device *dev, u32_t pwm,
			    u32_t period_cycles, u32_t pulse_cycles)
{
	PWM_Type *base = DEV_BASE(dev);
	const struct imx_pwm_config *config = DEV_CFG(dev);
	struct imx_pwm_data *data = DEV_DATA(dev);
	unsigned int period_ms;
	bool enabled = imx_pwm_is_enabled(base);
	int wait_count = 0, fifoav;
	u32_t cr, sr;


	if ((period_cycles == 0U) || (pulse_cycles > period_cycles)) {
		LOG_ERR("Invalid combination: period_cycles=%d, "
			    "pulse_cycles=%d", period_cycles, pulse_cycles);
		return -EINVAL;
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
			    dev->config->name);

		data->period_cycles = period_cycles;
		PWM_PWMPR_REG(base) = period_cycles;
	}

	cr = PWM_PWMCR_EN_MASK | PWM_PWMCR_PRESCALER(config->prescaler) |
		PWM_PWMCR_DOZEN_MASK | PWM_PWMCR_WAITEN_MASK |
		PWM_PWMCR_DBGEN_MASK | PWM_PWMCR_CLKSRC(2);

	PWM_PWMCR_REG(base) = cr;

	return 0;
}

static int imx_pwm_init(struct device *dev)
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

#ifdef CONFIG_PWM_1
static const struct imx_pwm_config imx_pwm_config_1 = {
	.base = (PWM_Type *)DT_ALIAS_PWM_1_BASE_ADDRESS,
	.prescaler = DT_ALIAS_PWM_1_PRESCALER,
};

static struct imx_pwm_data imx_pwm_data_1;

DEVICE_AND_API_INIT(imx_pwm_1, DT_ALIAS_PWM_1_LABEL, &imx_pwm_init,
		    &imx_pwm_data_1, &imx_pwm_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &imx_pwm_driver_api);
#endif /* CONFIG_PWM_1 */

#ifdef CONFIG_PWM_2
static const struct imx_pwm_config imx_pwm_config_2 = {
	.base = (PWM_Type *)DT_ALIAS_PWM_2_BASE_ADDRESS,
	.prescaler = DT_ALIAS_PWM_2_PRESCALER,
};

static struct imx_pwm_data imx_pwm_data_2;

DEVICE_AND_API_INIT(imx_pwm_2, DT_ALIAS_PWM_2_LABEL, &imx_pwm_init,
		    &imx_pwm_data_2, &imx_pwm_config_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &imx_pwm_driver_api);
#endif /* CONFIG_PWM_2 */

#ifdef CONFIG_PWM_3
static const struct imx_pwm_config imx_pwm_config_3 = {
	.base = (PWM_Type *)DT_ALIAS_PWM_3_BASE_ADDRESS,
	.prescaler = DT_ALIAS_PWM_3_PRESCALER,
};

static struct imx_pwm_data imx_pwm_data_3;

DEVICE_AND_API_INIT(imx_pwm_3, DT_ALIAS_PWM_3_LABEL, &imx_pwm_init,
		    &imx_pwm_data_3, &imx_pwm_config_3,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &imx_pwm_driver_api);
#endif /* CONFIG_PWM_3 */

#ifdef CONFIG_PWM_4
static const struct imx_pwm_config imx_pwm_config_4 = {
	.base = (PWM_Type *)DT_ALIAS_PWM_4_BASE_ADDRESS
	.prescaler = DT_ALIAS_PWM_4_PRESCALER,
};

static struct imx_pwm_data imx_pwm_data_4;

DEVICE_AND_API_INIT(imx_pwm_4, DT_ALIAS_PWM_4_LABEL, &imx_pwm_init,
		    &imx_pwm_data_4, &imx_pwm_config_4,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &imx_pwm_driver_api);
#endif /* CONFIG_PWM_4 */
