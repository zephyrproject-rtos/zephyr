/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header file for the PCA9685 PWM driver.
 */

#ifndef ZEPHYR_DRIVERS_PWM_PWM_PCA9685_H_
#define ZEPHYR_DRIVERS_PWM_PWM_PCA9685_H_

#include <drivers/gpio.h>
#include <drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialization function for PCA9685
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise
 */
extern int pwm_pca9685_init(struct device *dev);

/** Configuration data */
struct pwm_pca9685_config {
	/** The master I2C device's name */
	const char * const i2c_master_dev_name;

	/** The slave address of the chip */
	u16_t i2c_slave_addr;
	u8_t stride[2];
};

/** Runtime driver data */
struct pwm_pca9685_drv_data {
	/** Master I2C device */
	struct device *i2c_master;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_PWM_PWM_PCA9685_H_ */
