/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Header file for the PCA9685 PWM driver.
 */

#ifndef __PWM_PCA9685_H__
#define __PWM_PCA9685_H__

#include <gpio.h>
#include <i2c.h>

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
	uint16_t i2c_slave_addr;
	uint8_t stride[2];
};

/** Runtime driver data */
struct pwm_pca9685_drv_data {
	/** Master I2C device */
	struct device *i2c_master;
};

#ifdef __cplusplus
}
#endif

#endif /* __PWM_PCA9685_H__ */
