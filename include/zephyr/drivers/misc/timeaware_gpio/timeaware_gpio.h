/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for Time-aware GPIO drivers
 * @ingroup tgpio_interface
 */
#ifndef ZEPHYR_DRIVERS_MISC_TIMEAWARE_GPIO_TIMEAWARE_GPIO
#define ZEPHYR_DRIVERS_MISC_TIMEAWARE_GPIO_TIMEAWARE_GPIO

/**
 * @brief Interfaces for time-aware GPIO controllers.
 * @defgroup tgpio_interface Time-aware GPIO
 * @since 3.5
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/internal/syscall_handler.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Event polarity
 */
enum tgpio_pin_polarity {
	TGPIO_RISING_EDGE = 0,
	TGPIO_FALLING_EDGE,
	TGPIO_TOGGLE_EDGE,
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * TGPIO driver API definition and system call entry points
 *
 * (Internal use only.)
 */

__subsystem struct tgpio_driver_api {
	int (*pin_disable)(const struct device *dev, uint32_t pin);
	int (*get_time)(const struct device *dev, uint64_t *current_time);
	int (*cyc_per_sec)(const struct device *dev, uint32_t *cycles);
	int (*set_perout)(const struct device *dev, uint32_t pin, uint64_t start_time,
			  uint64_t repeat_interval, bool periodic_enable);
	int (*config_ext_ts)(const struct device *dev, uint32_t pin, uint32_t event_polarity);
	int (*read_ts_ec)(const struct device *dev, uint32_t pin, uint64_t *timestamp,
			  uint64_t *event_count);
};

/**
 * @endcond
 */

/**
 * @brief Get time from ART timer
 *
 * @param dev TGPIO device
 * @param current_time Pointer to store timer value in cycles
 *
 * @return 0 if successful
 * @return negative errno code on failure.
 */
__syscall int tgpio_port_get_time(const struct device *dev, uint64_t *current_time);

static inline int z_impl_tgpio_port_get_time(const struct device *dev, uint64_t *current_time)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->get_time(dev, current_time);
}

/**
 * @brief Get current running rate
 *
 * @param dev TGPIO device
 * @param cycles pointer to store current running frequency
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_port_get_cycles_per_second(const struct device *dev, uint32_t *cycles);

static inline int z_impl_tgpio_port_get_cycles_per_second(const struct device *dev,
							   uint32_t *cycles)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->cyc_per_sec(dev, cycles);
}

/**
 * @brief Disable operation on pin
 *
 * @param dev TGPIO device
 * @param pin TGPIO pin
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_pin_disable(const struct device *dev, uint32_t pin);

static inline int z_impl_tgpio_pin_disable(const struct device *dev, uint32_t pin)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->pin_disable(dev, pin);
}

/**
 * @brief Enable/Continue operation on pin
 *
 * @param dev TGPIO device
 * @param pin TGPIO pin
 * @param event_polarity TGPIO pin event polarity
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_pin_config_ext_timestamp(const struct device *dev, uint32_t pin,
					      uint32_t event_polarity);

static inline int z_impl_tgpio_pin_config_ext_timestamp(const struct device *dev, uint32_t pin,
							 uint32_t event_polarity)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->config_ext_ts(dev, pin, event_polarity);
}

/**
 * @brief Enable periodic pulse generation on a pin
 *
 * @param dev TGPIO device
 * @param pin TGPIO pin
 * @param start_time start_time of first pulse in hw cycles
 * @param repeat_interval repeat interval between two pulses in hw cycles
 * @param periodic_enable enables periodic mode if 'true' is passed.
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_pin_periodic_output(const struct device *dev, uint32_t pin,
					 uint64_t start_time, uint64_t repeat_interval,
					 bool periodic_enable);

static inline int z_impl_tgpio_pin_periodic_output(const struct device *dev, uint32_t pin,
						    uint64_t start_time, uint64_t repeat_interval,
						    bool periodic_enable)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->set_perout(dev, pin, start_time, repeat_interval, periodic_enable);
}

/**
 * @brief Read timestamp and event counter from TGPIO
 *
 * @param dev TGPIO device
 * @param pin TGPIO pin
 * @param timestamp timestamp of the last pulse received
 * @param event_count number of pulses received since the pin is enabled
 *
 * @return 0 if successful, negative errno code on failure.
 */
__syscall int tgpio_pin_read_ts_ec(const struct device *dev, uint32_t pin, uint64_t *timestamp,
				    uint64_t *event_count);

static inline int z_impl_tgpio_pin_read_ts_ec(const struct device *dev, uint32_t pin,
					       uint64_t *timestamp, uint64_t *event_count)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->read_ts_ec(dev, pin, timestamp, event_count);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/timeaware_gpio.h>

#endif /* ZEPHYR_DRIVERS_MISC_TIMEAWARE_GPIO_TIMEAWARE_GPIO */
