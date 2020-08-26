/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for PCA9685 I2C-based PWM driver.
 */

#include <errno.h>

#include <kernel.h>

#include <drivers/i2c.h>
#include <drivers/pwm.h>

#include "pwm_pca9685.h"

#define REG_MODE1		0x00
#define REG_MODE2		0x01

#define REG_LED_ON_L(n)		((4 * n) + 0x06)
#define	REG_LED_ON_H(n)		((4 * n) + 0x07)
#define REG_LED_OFF_L(n)	((4 * n) + 0x08)
#define REG_LED_OFF_H(n)	((4 * n) + 0x09)

#define REG_ALL_LED_ON_L	0xFA
#define REG_ALL_LED_ON_H	0xFB
#define REG_ALL_LED_OFF_L	0xFC
#define REG_ALL_LED_OFF_H	0xFD
#define REG_PRE_SCALE		0xFE

/* Maximum PWM outputs */
#define MAX_PWM_OUT		16

/* How many ticks per one period */
#define PWM_ONE_PERIOD_TICKS	4096

/**
 * @brief Check to see if a I2C master is identified for communication.
 *
 * @param dev Device struct.
 * @return 1 if I2C master is identified, 0 if not.
 */
static inline int has_i2c_master(struct device *dev)
{
	struct pwm_pca9685_drv_data * const drv_data =
		(struct pwm_pca9685_drv_data * const)dev->data;
	struct device * const i2c_master = drv_data->i2c_master;

	if (i2c_master) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * period_count is always taken as 4095. To control the on period send
 * value to pulse_count
 */
static int pwm_pca9685_pin_set_cycles(struct device *dev, uint32_t pwm,
				      uint32_t period_count, uint32_t pulse_count,
				      pwm_flags_t flags)
{
	const struct pwm_pca9685_config * const config =
		dev->config;
	struct pwm_pca9685_drv_data * const drv_data =
		(struct pwm_pca9685_drv_data * const)dev->data;
	struct device * const i2c_master = drv_data->i2c_master;
	uint16_t i2c_addr = config->i2c_slave_addr;
	uint8_t buf[] = { 0, 0, 0, 0, 0};

	ARG_UNUSED(period_count);
	if (!has_i2c_master(dev)) {
		return -EINVAL;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	if (pwm > MAX_PWM_OUT) {
		return -EINVAL;
	}
	buf[0] = REG_LED_ON_L(pwm);

	/* If either pulse_count > max ticks, treat PWM as 100%.
	 * If pulse_count value == 0, treat it as 0%.
	 * Otherwise, populate registers accordingly.
	 */
	if (pulse_count >= PWM_ONE_PERIOD_TICKS) {
		buf[1] = 0x0;
		buf[2] = (1 << 4);
		buf[3] = 0x0;
		buf[4] = 0x0;
	} else if (pulse_count == 0U) {
		buf[1] = 0x0;
		buf[2] = 0x0;
		buf[3] = 0x0;
		buf[4] = (1 << 4);
	} else {
		/* No start delay given. When the count is 0 the
		 * pwm will be high .
		 */
		buf[0] = 0x0;
		buf[1] = 0x0;
		buf[2] = (pulse_count & 0xFF);
		buf[3] = ((pulse_count >> 8) & 0x0F);
	}

	return i2c_write(i2c_master, buf, sizeof(buf), i2c_addr);
}


static const struct pwm_driver_api pwm_pca9685_drv_api_funcs = {
	.pin_set =  pwm_pca9685_pin_set_cycles
};

/**
 * @brief Initialization function of PCA9685
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
int pwm_pca9685_init(struct device *dev)
{
	const struct pwm_pca9685_config * const config =
		dev->config;
	struct pwm_pca9685_drv_data * const drv_data =
		(struct pwm_pca9685_drv_data * const)dev->data;
	struct device *i2c_master;
	uint8_t buf[] = {0, 0};
	int ret;

	/* Find out the device struct of the I2C master */
	i2c_master = device_get_binding((char *)config->i2c_master_dev_name);
	if (!i2c_master) {
		return -EINVAL;
	}
	drv_data->i2c_master = i2c_master;

	/* MODE1 register */

	buf[0] = REG_MODE1;
	buf[1] = (1 << 5); /* register addr auto increment */

	ret = i2c_write(i2c_master, buf, 2, config->i2c_slave_addr);
	if (ret != 0) {
		return -EPERM;
	}

	return 0;
}

/* Initialization for PWM_PCA9685_0 */
#ifdef CONFIG_PWM_PCA9685_0
#include <device.h>
#include <init.h>

static const struct pwm_pca9685_config pwm_pca9685_0_cfg = {
	.i2c_master_dev_name = CONFIG_PWM_PCA9685_0_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = CONFIG_PWM_PCA9685_0_I2C_ADDR,
};

static struct pwm_pca9685_drv_data pwm_pca9685_0_drvdata;

/* This has to init after I2C master */
DEVICE_AND_API_INIT(pwm_pca9685_0, CONFIG_PWM_PCA9685_0_DEV_NAME,
			pwm_pca9685_init,
			&pwm_pca9685_0_drvdata, &pwm_pca9685_0_cfg,
			POST_KERNEL, CONFIG_PWM_PCA9685_INIT_PRIORITY,
			&pwm_pca9685_drv_api_funcs);

#endif /* CONFIG_PWM_PCA9685_0 */
