/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file Driver for PCA9685 I2C-based PWM driver.
 */

#include <nanokernel.h>

#include <i2c.h>
#include <pwm.h>

#include "pwm-pca9685.h"

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

//#define WAIT_10MS	(sys_clock_ticks_per_sec / 100)
//static void _wait_10ms()
//{
//	int64_t start = nano_tick_get();
//
//	(void)nano_tick_delta(&start);
//	while (nano_tick_delta(&start) < WAIT_10MS);
//}

static int pwm_pca9685_configure(struct device *dev, int access_op,
				 uint32_t pwm, int flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pwm);
	ARG_UNUSED(flags);

	return DEV_OK;
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
		return DEV_INVALID_CONF;
	}

	switch (access_op) {
		case PWM_ACCESS_BY_PIN:
			if (pwm > MAX_PWM_OUT) {
				return DEV_INVALID_CONF;
			}
			buf[0] = REG_LED_ON_L(pwm);
			break;
		case PWM_ACCESS_ALL:
			buf[0] = REG_ALL_LED_ON_L;
			break;
		default:
			return DEV_INVALID_OP;
	}

	/* If both ON and OFF > max ticks, treat PWM as 100%.
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
		buf[0] = (on & 0xFF);
		buf[1] = ((on >> 8) & 0x0F);
		buf[2] = (off & 0xFF);
		buf[3] = ((off >> 8) & 0x0F);
	}

	if (i2c_configure(i2c_master,
			  (I2C_MODE_MASTER | (I2C_SPEED_FAST << 1)))) {
		return DEV_FAIL;
	}

	return i2c_write(i2c_master, buf, sizeof(buf), i2c_addr);
}

static int pwm_pca9685_set_duty_cycle(struct device *dev, int access_op,
				      uint32_t pwm, uint8_t duty)
{
	uint32_t on, off;

	if (duty == 0) {
		/* Turn off PWM */
		on = 0;
		off = 0;
	} else if (duty >= 100) {
		/* Force PWM to be 100% */
		on = PWM_ONE_PERIOD_TICKS + 1;
		off = PWM_ONE_PERIOD_TICKS + 1;
	} else {
		on = PWM_ONE_PERIOD_TICKS * duty / 100;
		off = PWM_ONE_PERIOD_TICKS - 1;
	}

	return pwm_pca9685_set_values(dev, access_op, pwm, on, off);
}

static int pwm_pca9685_suspend(struct device *dev)
{
	if (!_has_i2c_master(dev)) {
		return DEV_INVALID_CONF;
	}

	return DEV_INVALID_OP;
}

static int pwm_pca9685_resume(struct device *dev)
{
	if (!_has_i2c_master(dev)) {
		return DEV_INVALID_CONF;
	}

	return DEV_INVALID_OP;
}

static struct pwm_driver_api pwm_pca9685_drv_api_funcs = {
	.config = pwm_pca9685_configure,
	.set_values = pwm_pca9685_set_values,
	.set_duty_cycle = pwm_pca9685_set_duty_cycle,
	.suspend = pwm_pca9685_suspend,
	.resume = pwm_pca9685_resume,
};

/**
 * @brief Initialization function of PCA9685
 *
 * @param dev Device struct
 * @return DEV_OK if successful, failed otherwise.
 */
int pwm_pca9685_init(struct device *dev)
{
	const struct pwm_pca9685_config * const config =
		dev->config->config_info;
	struct pwm_pca9685_drv_data * const drv_data =
		(struct pwm_pca9685_drv_data * const)dev->driver_data;
	struct device *i2c_master;
	uint8_t buf[] = { 0, 0};
	int ret;

	dev->driver_api = &pwm_pca9685_drv_api_funcs;

	/* Find out the device struct of the I2C master */
	i2c_master = device_get_binding((char *)config->i2c_master_dev_name);
	if (!i2c_master) {
		return DEV_INVALID_CONF;
	}
	drv_data->i2c_master = i2c_master;

	/* Initialize the chip */
	if (i2c_configure(i2c_master,
			  (I2C_MODE_MASTER | (I2C_SPEED_FAST << 1)))) {
		return DEV_NOT_CONFIG;
	}

	/* MODE1 register */
	buf[0] = REG_MODE1;
	buf[1] = (1 << 5); /* register addr auto increment */
	ret = i2c_write(i2c_master, buf, 2, config->i2c_slave_addr);
	if (!ret) {
		return DEV_NOT_CONFIG;
	}

	return DEV_OK;
}

/* Initialization for PWM_PCA9685_0 */
#ifdef CONFIG_PWM_PCA9685_0
#include <device.h>
#include <init.h>

static struct pwm_pca9685_config pwm_pca9685_0_cfg = {
	.i2c_master_dev_name = CONFIG_PWM_PCA9685_0_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = CONFIG_PWM_PCA9685_0_I2C_ADDR,
};

static struct pwm_pca9685_drv_data pwm_pca9685_0_drvdata;

DECLARE_DEVICE_INIT_CONFIG(pwm_pca9685_0,
			   CONFIG_PWM_PCA9685_0_DEV_NAME,
			   pwm_pca9685_init, &pwm_pca9685_0_cfg);

/* This has to init after I2C master */
nano_early_init(pwm_pca9685_0, &pwm_pca9685_0_drvdata);

#endif /* CONFIG_PWM_PCA9685_0 */
