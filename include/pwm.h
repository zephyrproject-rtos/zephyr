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
 * @file Public PWM Driver APIs
 */

#ifndef __PWM_H__
#define __PWM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_ACCESS_BY_PIN	0
#define PWM_ACCESS_ALL		1

#include <stdint.h>
#include <stddef.h>
#include <device.h>

/* driver API definition */
typedef int (*pwm_config_t)(struct device *dev, int access_op,
			    uint32_t pwm, int flags);
typedef int (*pwm_set_values_t)(struct device *dev, int access_op,
				uint32_t pwm, uint32_t on, uint32_t off);
typedef int (*pwm_set_duty_cycle_t)(struct device *dev, int access_op,
				    uint32_t pwm, uint8_t duty);
typedef int (*pwm_suspend_dev_t)(struct device *dev);
typedef int (*pwm_resume_dev_t)(struct device *dev);

struct pwm_driver_api {
	pwm_config_t config;
	pwm_set_values_t set_values;
	pwm_set_duty_cycle_t set_duty_cycle;
	pwm_suspend_dev_t suspend;
	pwm_resume_dev_t resume;
};

/**
 * @brief Configure a single PWM output.
 *
 * @param dev Pointer to device structure for driver instance
 * @param pwm PWM output
 * @param flags PWM configuration flags
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static inline int pwm_pin_configure(struct device *dev, uint8_t pwm,
				    int flags)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->config(dev, PWM_ACCESS_BY_PIN, pwm, flags);
}

/**
 * @brief Set the ON/OFF values for a single PWM output.
 *
 * @param dev Pointer to device structure for driver instance
 * @param pwm PWM output
 * @param on ON value to set the PWM to
 * @param off OFF value to set the PWM to
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static inline int pwm_pin_set_values(struct device *dev, uint32_t pwm,
				     uint32_t on, uint32_t off)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->set_values(dev, PWM_ACCESS_BY_PIN, pwm, on, off);
}

/**
 * @brief Set the duty cycle of a single PWM output.
 *
 * This overrides any ON/OFF values being set before.
 *
 * @param dev Pointer to device structure for driver instance.
 * @param pwm PWM output
 * @param duty Duty cycle to set the PWM to (in %, e.g. 50 => 50%)
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static inline int pwm_pin_set_duty_cycle(struct device *dev, uint32_t pwm,
					 uint8_t duty)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->set_duty_cycle(dev, PWM_ACCESS_BY_PIN, pwm, duty);
}

/**
 * @brief Configure all PWM outputs.
 *
 * @param dev Pointer to device structure for driver instance
 * @param flags PWM configuration flags
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static inline int pwm_all_configure(struct device *dev, int flags)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->config(dev, PWM_ACCESS_ALL, 0, flags);
}

/**
 * @brief Set the ON/OFF values for all PWM outputs.
 *
 * @param dev Pointer to device structure for driver instance
 * @param on ON value to set the PWM to
 * @param off OFF value to set the PWM to
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static inline int pwm_all_set_values(struct device *dev,
				     uint32_t on, uint32_t off)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->set_values(dev, PWM_ACCESS_ALL, 0, on, off);
}

/**
 * @brief Set the duty cycle of all PWM outputs.
 *
 * This overrides any ON/OFF values being set before.
 *
 * @param dev Pointer to device structure for driver instance.
 * @param pwm PWM output
 * @param duty Duty cycle to set the PWM to (in %, e.g. 50 => 50%)
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static inline int pwm_all_set_duty_cycle(struct device *dev, uint8_t duty)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->set_duty_cycle(dev, PWM_ACCESS_ALL, 0, duty);
}

/**
 * @brief Save the state of the device and go to low power state
 *
 * @param dev Pointer to device structure for driver instance.
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static inline int pwm_suspend(struct device *dev)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->suspend(dev);
}

/**
 * @brief Restore state stored during suspend and resume operation.
 *
 * @param dev Pointer to device structure for driver instance.
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static inline int pwm_resume(struct device *dev)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->resume(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* __PWM_H__ */
