/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_KEYS_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_KEYS_H_

#include <stdint.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpio_keys_callback {
	struct gpio_callback gpio_cb;
	uint32_t zephyr_code;
	int8_t   pin_state;
};

typedef void (*gpio_keys_callback_handler_t)(const struct device *port,
					     struct gpio_keys_callback *cb, gpio_port_pins_t pins);
/**
 * @brief GPIO Keys Driver APIs
 * @defgroup gpio_keys_interface GPIO KeysDriver APIs
 * @ingroup io_interfaces
 * @{
 */

__subsystem struct gpio_keys_api {
	int (*enable_interrupt)(const struct device *dev, gpio_keys_callback_handler_t cb);
	int (*disable_interrupt)(const struct device *dev);
	int (*get_pin)(const struct device *dev, uint32_t idx);
};

/**
 * @brief Enable interrupt
 *
 * @param dev Pointer to device structure for the driver instance.
 * @param cb Function callback to be invoked after GPIO key has been debounced
 *
 * @return 0 If successful
 */
__syscall int gpio_keys_enable_interrupt(const struct device *dev, gpio_keys_callback_handler_t cb);

static inline int z_impl_gpio_keys_enable_interrupt(const struct device *dev,
						    gpio_keys_callback_handler_t cb)
{
	struct gpio_keys_api *api;

	api = (struct gpio_keys_api *)dev->api;
	return api->enable_interrupt(dev, cb);
}

/**
 * @brief Disable interrupt
 *
 * @param dev Pointer to device structure for the driver instance.
 *
 * @return 0 If successful
 */
__syscall int gpio_keys_disable_interrupt(const struct device *dev);

static inline int z_impl_gpio_keys_disable_interrupt(const struct device *dev)
{
	struct gpio_keys_api *api;

	api = (struct gpio_keys_api *)dev->api;
	return api->disable_interrupt(dev);
}

/**
 * @brief Get the logical level of GPIO Key
 *
 * @param dev Pointer to device structure for the driver instance.
 * @param idx GPIO Key index in device tree
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_keys_get_pin(const struct device *dev, uint32_t idx);

static inline int z_impl_gpio_keys_get_pin(const struct device *dev, uint32_t idx)
{
	struct gpio_keys_api *api;

	api = (struct gpio_keys_api *)dev->api;
	return api->get_pin(dev, idx);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/gpio_keys.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_KEYS_H_ */
