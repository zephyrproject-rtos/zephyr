/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for Time-aware GPIO driver APIs
 */

#ifndef ZEPHYR_DRIVERS_MISC_TIMEAWARE_GPIO_TIMEAWARE_GPIO
#error "Should only be included by zephyr/drivers/misc/timeaware_gpio/timeaware_gpio.h"
#endif

#ifndef ZEPHYR_DRIVERS_MISC_TIMEAWARE_GPIO_TIMEAWARE_GPIO_INTERNAL_IMPL_H_
#define ZEPHYR_DRIVERS_MISC_TIMEAWARE_GPIO_TIMEAWARE_GPIO_INTERNAL_IMPL_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/internal/syscall_handler.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_tgpio_port_get_time(const struct device *dev, uint64_t *current_time)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->get_time(dev, current_time);
}

static inline int z_impl_tgpio_port_get_cycles_per_second(const struct device *dev,
							   uint32_t *cycles)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->cyc_per_sec(dev, cycles);
}

static inline int z_impl_tgpio_pin_disable(const struct device *dev, uint32_t pin)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->pin_disable(dev, pin);
}

static inline int z_impl_tgpio_pin_config_ext_timestamp(const struct device *dev, uint32_t pin,
							 uint32_t event_polarity)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->config_ext_ts(dev, pin, event_polarity);
}

static inline int z_impl_tgpio_pin_periodic_output(const struct device *dev, uint32_t pin,
						    uint64_t start_time, uint64_t repeat_interval,
						    bool periodic_enable)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->set_perout(dev, pin, start_time, repeat_interval, periodic_enable);
}

static inline int z_impl_tgpio_pin_read_ts_ec(const struct device *dev, uint32_t pin,
					       uint64_t *timestamp, uint64_t *event_count)
{
	const struct tgpio_driver_api *api = (const struct tgpio_driver_api *)dev->api;

	return api->read_ts_ec(dev, pin, timestamp, event_count);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_TIMEAWARE_GPIO_TIMEAWARE_GPIO_INTERNAL_IMPL_H_ */
