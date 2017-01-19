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

#include <i2c.h>
#include <pwm.h>

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
static inline int _has_i2c_master(struct device *dev)
{
	struct pwm_pca9685_drv_data * const drv_data =
		(struct pwm_pca9685_drv_data * const)dev->driver_data;
	struct device * const i2c_master = drv_data->i2c_master;

	if (i2c_master)
		return 1;
	else
		return 0;
}

static int pwm_pca9685_configure(struct device *dev, int access_op,
				 uint32_t pwm, int flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pwm);
	ARG_UNUSED(flags);

	return 0;
}

static int pwm_pca9685_set_values(struct device *dev, int access_op,
				  uint32_t pwm, uint32_t on, uint32_t off)
{
	const struct pwm_pca9685_config * const config =
		dev->config->config_info;
	struct pwm_pca9685_drv_data * const drv_data =
		(struct pwm_pca9685_drv_data * const)dev->driver_data;
	struct device * const i2c_master = drv_data->i2c_master;
	uint16_t i2c_addr = config->i2c_slave_addr;
	uint8_t buf[] = { 0, 0, 0, 0, 0};

	if (!_has_i2c_master(dev)) {
		return -EINVAL;
	}

	switch (access_op) {
	case PWM_ACCESS_BY_PIN:
		if (pwm > MAX_PWM_OUT) {
			return -EINVAL;
		}
		buf[0] = REG_LED_ON_L(pwm);
		break;
	case PWM_ACCESS_ALL:
		buf[0] = REG_ALL_LED_ON_L;
		break;
	default:
		return -ENOTSUP;
	}

	/* If either ON and/or OFF > max ticks, treat PWM as 100%.
	 * If OFF value == 0, treat it as 0%.
	 * Otherwise, populate registers accordingly.
	 */
	if ((on >= PWM_ONE_PERIOD_TICKS) || (off >= PWM_ONE_PERIOD_TICKS)) {
		buf[1] = 0x0;
		buf[2] = (1 << 4);
		buf[3] = 0x0;
		buf[4] = 0x0;
	} else if (off == 0) {
		buf[1] = 0x0;
		buf[2] = 0x0;
		buf[3] = 0x0;
		buf[4] = (1 << 4);
	} else {
		buf[1] = (on & 0xFF);
		buf[2] = ((on >> 8) & 0x0F);
		buf[3] = (off & 0xFF);
		buf[4] = ((off >> 8) & 0x0F);
	}

	return i2c_write(i2c_master, buf, sizeof(buf), i2c_addr);
}

/**
 * Duty cycle describes the percentage of time a signal is turned
 * to the ON state.
 */
static int pwm_pca9685_set_duty_cycle(struct device *dev, int access_op,
				      uint32_t pwm, uint8_t duty)
{
	uint32_t on, off, phase;

	phase = 0;     /* Hard coded until API changes */

	if (duty == 0) {
		/* Turn off PWM */
		on = 0;
		off = 0;
	} else if (duty >= 100) {
		/* Force PWM to be 100% */
		on = PWM_ONE_PERIOD_TICKS + 1;
		off = PWM_ONE_PERIOD_TICKS + 1;
	} else {
		off = PWM_ONE_PERIOD_TICKS * duty / 100;
		on = phase;
	}

	return pwm_pca9685_set_values(dev, access_op, pwm, on, off);
}

static const struct pwm_driver_api pwm_pca9685_drv_api_funcs = {
	.config = pwm_pca9685_configure,
	.set_values = pwm_pca9685_set_values,
	.set_duty_cycle = pwm_pca9685_set_duty_cycle,
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
		dev->config->config_info;
	struct pwm_pca9685_drv_data * const drv_data =
		(struct pwm_pca9685_drv_data * const)dev->driver_data;
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

	dev->driver_api = &pwm_pca9685_drv_api_funcs;

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
DEVICE_INIT(pwm_pca9685_0, CONFIG_PWM_PCA9685_0_DEV_NAME,
			pwm_pca9685_init,
			&pwm_pca9685_0_drvdata, &pwm_pca9685_0_cfg,
			POST_KERNEL, CONFIG_PWM_PCA9685_INIT_PRIORITY);

#endif /* CONFIG_PWM_PCA9685_0 */
