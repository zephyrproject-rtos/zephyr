/*
 * Copyright (c) 2018 SiFive Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifive_pwm0

#include <logging/log.h>

LOG_MODULE_REGISTER(pwm_sifive, CONFIG_PWM_LOG_LEVEL);

#include <sys/sys_io.h>
#include <device.h>
#include <drivers/pwm.h>

/* Macros */

#define PWM_REG(z_config, _offset) ((mem_addr_t) ((z_config)->base + _offset))

/* Register Offsets */
#define REG_PWMCFG		0x00
#define REG_PWMCOUNT		0x08
#define REG_PWMS		0x10
#define REG_PWMCMP0		0x20
#define REG_PWMCMP(_channel)	(REG_PWMCMP0 + ((_channel) * 0x4))

/* Number of PWM Channels */
#define SF_NUMCHANNELS		4

/* pwmcfg Bit Offsets */
#define SF_PWMSTICKY			8
#define SF_PWMZEROCMP			9
#define SF_PWMDEGLITCH			10
#define SF_PWMENALWAYS			12
#define SF_PWMENONESHOT			13
#define SF_PWMCMPCENTER(_channel)	(16 + (_channel))
#define SF_PWMCMPGANG(_channel)		(24 + (_channel))
#define SF_PWMCMPIP(_channel)		(28 + (_channel))

/* pwmcount scale factor */
#define SF_PWMSCALEMASK		0xF
#define SF_PWMSCALE(_val)	(SF_PWMSCALEMASK & (_val))

#define SF_PWMCOUNT_MIN_WIDTH	15

/* Structure Declarations */

struct pwm_sifive_data {};

struct pwm_sifive_cfg {
	uint32_t base;
	uint32_t f_sys;
	uint32_t cmpwidth;
};

/* Helper Functions */

static inline void sys_set_mask(mem_addr_t addr, uint32_t mask, uint32_t value)
{
	uint32_t temp = sys_read32(addr);

	temp &= ~(mask);
	temp |= value;

	sys_write32(temp, addr);
}

/* API Functions */

static int pwm_sifive_init(const struct device *dev)
{
	const struct pwm_sifive_cfg *config = dev->config;

	/* When pwms == pwmcmp0, reset the counter */
	sys_set_bit(PWM_REG(config, REG_PWMCFG), SF_PWMZEROCMP);

	/* Enable continuous operation */
	sys_set_bit(PWM_REG(config, REG_PWMCFG), SF_PWMENALWAYS);

	/* Clear IP config bits */
	sys_clear_bit(PWM_REG(config, REG_PWMCFG), SF_PWMSTICKY);
	sys_clear_bit(PWM_REG(config, REG_PWMCFG), SF_PWMDEGLITCH);

	/* Clear all channels */
	for (int i = 0; i < SF_NUMCHANNELS; i++) {
		/* Clear the channel comparator */
		sys_write32(0, PWM_REG(config, REG_PWMCMP(i)));

		/* Clear the compare center and compare gang bits */
		sys_clear_bit(PWM_REG(config, REG_PWMCFG), SF_PWMCMPCENTER(i));
		sys_clear_bit(PWM_REG(config, REG_PWMCFG), SF_PWMCMPGANG(i));
	}

	return 0;
}

static int pwm_sifive_pin_set(const struct device *dev,
			      uint32_t pwm,
			      uint32_t period_cycles,
			      uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	const struct pwm_sifive_cfg *config = NULL;
	uint32_t count_max = 0U;
	uint32_t max_cmp_val = 0U;
	uint32_t pwmscale = 0U;

	if (dev == NULL) {
		LOG_ERR("The device instance pointer was NULL\n");
		return -EFAULT;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	config = dev->config;
	if (config == NULL) {
		LOG_ERR("The device configuration is NULL\n");
		return -EFAULT;
	}

	if (pwm >= SF_NUMCHANNELS) {
		LOG_ERR("The requested PWM channel %d is invalid\n", pwm);
		return -EINVAL;
	}

	/* Channel 0 sets the period, we can't output PWM with it */
	if ((pwm == 0U)) {
		LOG_ERR("PWM channel 0 cannot be configured\n");
		return -ENOTSUP;
	}

	/* We can't support periods greater than we can store in pwmcount */
	count_max = (1 << (config->cmpwidth + SF_PWMCOUNT_MIN_WIDTH)) - 1;

	if (period_cycles > count_max) {
		LOG_ERR("Requested period is %d but maximum is %d\n",
			period_cycles, count_max);
		return -EIO;
	}

	/* Calculate the maximum value that pwmcmpX can be set to */
	max_cmp_val = ((1 << config->cmpwidth) - 1);

	/*
	 * Find the minimum value of pwmscale that will allow us to set the
	 * requested period
	 */
	while ((period_cycles >> pwmscale) > max_cmp_val) {
		pwmscale++;
	}

	/* Make sure that we can scale that much */
	if (pwmscale > SF_PWMSCALEMASK) {
		LOG_ERR("Requested period is %d but maximum is %d\n",
			period_cycles, max_cmp_val << pwmscale);
		return -EIO;
	}

	if (pulse_cycles > period_cycles) {
		LOG_ERR("Requested pulse %d is longer than period %d\n",
			pulse_cycles, period_cycles);
		return -EIO;
	}

	/* Set the pwmscale field */
	sys_set_mask(PWM_REG(config, REG_PWMCFG),
		     SF_PWMSCALEMASK,
		     SF_PWMSCALE(pwmscale));

	/* Set the period by setting pwmcmp0 */
	sys_write32((period_cycles >> pwmscale), PWM_REG(config, REG_PWMCMP0));

	/* Set the duty cycle by setting pwmcmpX */
	sys_write32((pulse_cycles >> pwmscale),
		    PWM_REG(config, REG_PWMCMP(pwm)));

	LOG_DBG("channel: %d, pwmscale: %d, pwmcmp0: %d, pwmcmp%d: %d",
		pwm,
		pwmscale,
		(period_cycles >> pwmscale),
		pwm,
		(pulse_cycles >> pwmscale));

	return 0;
}

static int pwm_sifive_get_cycles_per_sec(const struct device *dev,
					 uint32_t pwm,
					 uint64_t *cycles)
{
	const struct pwm_sifive_cfg *config;

	if (dev == NULL) {
		LOG_ERR("The device instance pointer was NULL\n");
		return -EFAULT;
	}

	config = dev->config;
	if (config == NULL) {
		LOG_ERR("The device configuration is NULL\n");
		return -EFAULT;
	}

	/* Fail if we don't have that channel */
	if (pwm >= SF_NUMCHANNELS) {
		return -EINVAL;
	}

	*cycles = config->f_sys;

	return 0;
}

/* Device Instantiation */

static const struct pwm_driver_api pwm_sifive_api = {
	.pin_set = pwm_sifive_pin_set,
	.get_cycles_per_sec = pwm_sifive_get_cycles_per_sec,
};

#define PWM_SIFIVE_INIT(n)	\
	static struct pwm_sifive_data pwm_sifive_data_##n;	\
	static const struct pwm_sifive_cfg pwm_sifive_cfg_##n = {	\
			.base = DT_INST_REG_ADDR(n),	\
			.f_sys = DT_INST_PROP(n, clock_frequency),  \
			.cmpwidth = DT_INST_PROP(n, sifive_compare_width), \
		};	\
	DEVICE_DT_INST_DEFINE(n,	\
			    pwm_sifive_init,	\
			    device_pm_control_nop,	\
			    &pwm_sifive_data_##n,	\
			    &pwm_sifive_cfg_##n,	\
			    POST_KERNEL,	\
			    CONFIG_PWM_SIFIVE_INIT_PRIORITY,	\
			    &pwm_sifive_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_SIFIVE_INIT)
