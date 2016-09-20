/*
 * Copyright (c) 2016 Intel Corporation.
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
 * @file
 * @brief Public PWM Driver APIs
 */

#ifndef __PWM_H__
#define __PWM_H__

/**
 * @brief PWM Interface
 * @defgroup pwm_interface PWM Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_ACCESS_BY_PIN	0
#define PWM_ACCESS_ALL		1

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <device.h>

/**
 * @typedef pwm_config_t
 * @brief Callback API upon configuration
 * See @a pwm_pin_configure() for argument description
 */
typedef int (*pwm_config_t)(struct device *dev, int access_op,
			    uint32_t pwm, int flags);

/**
 * @typedef pwm_set_values_t
 * @brief Callback API upon setting PIN values
 * See @a pwm_pin_set_values() for argument description
 */
typedef int (*pwm_set_values_t)(struct device *dev, int access_op,
				uint32_t pwm, uint32_t on, uint32_t off);

/**
 * @typedef pwm_set_duty_cycle_t
 * @brief Callback API upon setting the duty cycle
 * See @a pwm_pin_set_duty_cycle() for argument description
 */
typedef int (*pwm_set_duty_cycle_t)(struct device *dev, int access_op,
				    uint32_t pwm, uint8_t duty);

/**
 * @typedef pwm_set_phase_t
 * @brief Callback API upon setting the phase
 * See @a pwm_pin_set_phase() for argument description
 */
typedef int (*pwm_set_phase_t)(struct device *dev, int access_op,
			       uint32_t pwm, uint8_t phase);

/**
 * @typedef pwm_set_period_t
 * @brief Callback API upon setting the period
 * See @a pwm_pin_set_period() for argument description
 */
typedef int (*pwm_set_period_t)(struct device *dev, int access_op,
				uint32_t pwm, uint32_t period);

/**
 * @typedef pwm_pin_set_t
 * @brief Callback API upon setting the pin
 * See @a pwm_pin_set_cycles() for argument description
 */
typedef int (*pwm_pin_set_t)(struct device *dev, uint32_t pwm,
			     uint32_t period_cycles, uint32_t pulse_cycles);

/**
 * @typedef pwm_get_cycles_per_sec_t
 * @brief Callback API upon getting cycles per second
 * See @a pwm_get_cycles_per_sec() for argument description
 */
typedef int (*pwm_get_cycles_per_sec_t)(struct device *dev, uint32_t pwm,
					uint64_t *cycles);

/** @brief PWM driver API definition. */
struct pwm_driver_api {
	pwm_config_t config;
	pwm_set_values_t set_values;
	pwm_set_period_t set_period;
	pwm_set_duty_cycle_t set_duty_cycle;
	pwm_set_phase_t set_phase;
	pwm_pin_set_t pin_set;
	pwm_get_cycles_per_sec_t get_cycles_per_sec;
};

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in clock cycle) set to the PWM. HW specific.
 * @param pulse Pulse width (in clock cycle) set to the PWM. HW specific.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_cycles(struct device *dev, uint32_t pwm,
				     uint32_t period, uint32_t pulse)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->pin_set(dev, pwm, period, pulse);
}

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param period Period (in micro second) set to the PWM.
 * @param pulse Pulse width (in micro second) set to the PWM.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_pin_set_usec(struct device *dev, uint32_t pwm,
				   uint32_t period, uint32_t pulse)
{
	struct pwm_driver_api *api;
	uint64_t period_cycles, pulse_cycles, cycles_per_sec;

	api = (struct pwm_driver_api *)dev->driver_api;

	if (api->get_cycles_per_sec(dev, pwm, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / USEC_PER_SEC;
	if (period_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / USEC_PER_SEC;
	if (pulse_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return api->pin_set(dev, pwm, (uint32_t)period_cycles,
			    (uint32_t)pulse_cycles);
}

/**
 * @brief Get the clock rate (cycles per second) for a single PWM output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM pin.
 * @param cycles Pointer to the memory to store clock rate (cycles per sec).
 *		 HW specific.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int pwm_get_cycles_per_sec(struct device *dev, uint32_t pwm,
					 uint64_t *cycles)
{
	struct pwm_driver_api *api;

	api = (struct pwm_driver_api *)dev->driver_api;
	return api->get_cycles_per_sec(dev, pwm, cycles);
}

/**
 * @brief Configure a single PWM output.
 *
 * @deprecated This API will be deprecated.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM output.
 * @param flags PWM configuration flags.
 *
 * @retval 0 If successful,
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_pin_configure(struct device *dev,
						 uint8_t pwm, int flags)
{
	const struct pwm_driver_api *api = dev->driver_api;

	return api->config(dev, PWM_ACCESS_BY_PIN, pwm, flags);
}

/**
 * @brief Set the ON/OFF values for a single PWM output.
 *
 * @deprecated This API will be deprecated. Please use the new API
 *	       pwm_pin_set_cycles.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM output.
 * @param on ON value (number of timer count) set to the PWM. HW specific.
 *	     How far from the beginning of a PWM cycle the PWM pulse starts.
 * @param off OFF value (number of timer count) set to the PWM. HW specific.
 *	      How far from the beginning of a PWM cycle the PWM pulse stops.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_pin_set_values(struct device *dev,
						  uint32_t pwm, uint32_t on,
						  uint32_t off)
{
	const struct pwm_driver_api *api = dev->driver_api;

	return api->set_values(dev, PWM_ACCESS_BY_PIN, pwm, on, off);
}

/**
 * @brief Set the period of a single PWM output.
 *
 * @deprecated This API will be deprecated. Please use the new API
 *	       pwm_pin_set_usec.
 *
 * It is optional to call this API. If not called, there is a default
 * period.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM output.
 * @param period Period duration of the cycle to set in microseconds
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_pin_set_period(struct device *dev,
						  uint32_t pwm,
						  uint32_t period)
{
	const struct pwm_driver_api *api = dev->driver_api;

	return api->set_period(dev, PWM_ACCESS_BY_PIN, pwm, period);
}

/**
 * @brief Set the duty cycle of a single PWM output.
 *
 * @deprecated This API will be deprecated. Please use the new API
 *	       pwm_pin_set_cycles or pwm_pin_set_usec.
 *
 * This routine overrides any ON/OFF values set before.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM output.
 * @param duty Duty cycle to set to the PWM in %, for example,
 *        50 sets to 50%.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_pin_set_duty_cycle(struct device *dev,
						      uint32_t pwm,
						      uint8_t duty)
{
	const struct pwm_driver_api *api = dev->driver_api;

	return api->set_duty_cycle(dev, PWM_ACCESS_BY_PIN, pwm, duty);
}

/**
 * @brief Set the phase of a single PWM output.
 *
 * @deprecated This API will be deprecated. Set_phase will not be supported
 *	       any more.
 *
 * This routine sets the delay before pulses.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pwm PWM output.
 * @param phase The number of clock ticks to delay before the start of pulses.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_pin_set_phase(struct device *dev,
						 uint32_t pwm, uint8_t phase)
{
	const struct pwm_driver_api *api = dev->driver_api;

	if ((api != NULL) && (api->set_phase != NULL)) {
		return api->set_phase(dev, PWM_ACCESS_BY_PIN, pwm, phase);
	}

	return -ENOTSUP;
}

/**
 * @brief Configure all the PWM outputs.
 *
 * @deprecated This API will be deprecated.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param flags PWM configuration flags.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_all_configure(struct device *dev, int flags)
{
	const struct pwm_driver_api *api = dev->driver_api;

	return api->config(dev, PWM_ACCESS_ALL, 0, flags);
}

/**
 * @brief Set the ON/OFF values for all PWM outputs.
 *
 * @deprecated This API will be deprecated. Please use the new API
 *	       pwm_pin_set_cycles or pwm_pin_set_usec to set the pins
 *	       individually.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param on ON value (number of timer count) set to the PWM. HW specific.
 *	     How far from the beginning of a PWM cycle the PWM pulse starts.
 * @param off OFF value (number of timer count) set to the PWM. HW specific.
 *	      How far from the beginning of a PWM cycle the PWM pulse stops.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_all_set_values(struct device *dev,
						  uint32_t on, uint32_t off)
{
	const struct pwm_driver_api *api = dev->driver_api;

	return api->set_values(dev, PWM_ACCESS_ALL, 0, on, off);
}

/**
 * @brief Set the period of all PWM outputs.
 *
 * @deprecated This API will be deprecated. Please use the new API
 *	       pwm_pin_set_cycles or pwm_pin_set_usec to set the pins
 *	       individually.
 *
 * It is optional to call this API. If not called, there is a default
 * period.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param period Period duration of the cycle to set in microseconds
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_all_period(struct device *dev,
					      uint32_t period)
{
	const struct pwm_driver_api *api = dev->driver_api;

	return api->set_period(dev, PWM_ACCESS_ALL, 0, period);
}

/**
 * @brief Set the duty cycle of all PWM outputs.
 *
 * @deprecated This API will be deprecated. Please use the new API
 *	       pwm_pin_set_cycles or pwm_pin_set_usec to set the pins
 *	       individually.
 *
 * This overrides any ON/OFF values being set before.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param duty Duty cycle to set to the PWM in %, for example,
 *        50 sets to 50%.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_all_set_duty_cycle(struct device *dev,
						      uint8_t duty)
{
	const struct pwm_driver_api *api = dev->driver_api;

	return api->set_duty_cycle(dev, PWM_ACCESS_ALL, 0, duty);
}

/**
 * @brief Set the phase of all PWM outputs.
 *
 * @deprecated This API will be deprecated. Set_phase will not be supported
 *	       any more.
 *
 * This routine sets the delay before pulses.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param phase The number of clock ticks to delay before the start of pulses.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int __deprecated pwm_all_set_phase(struct device *dev,
						 uint8_t phase)
{
	const struct pwm_driver_api *api = dev->driver_api;

	if ((api != NULL) && (api->set_phase != NULL)) {
		return api->set_phase(dev, PWM_ACCESS_ALL, 0, phase);
	}

	return -ENOTSUP;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __PWM_H__ */
