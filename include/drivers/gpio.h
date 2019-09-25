/*
 * Copyright (c) 2017 ARM Ltd
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for GPIO drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_H_

#include <sys/__assert.h>
#include <sys/slist.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <dt-bindings/gpio/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO Driver APIs
 * @defgroup gpio_interface GPIO Driver APIs
 * @ingroup io_interfaces
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define GPIO_ACCESS_BY_PIN 0
#define GPIO_ACCESS_BY_PORT 1
/**
 * @endcond
 */

struct gpio_callback;

/**
 * @typedef gpio_callback_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param "struct device *port" Device struct for the GPIO device.
 * @param "struct gpio_callback *cb" Original struct gpio_callback
 *        owning this handler
 * @param "u32_t pins" Mask of pins that triggers the callback handler
 *
 * Note: cb pointer can be used to retrieve private data through
 * CONTAINER_OF() if original struct gpio_callback is stored in
 * another private structure.
 */
typedef void (*gpio_callback_handler_t)(struct device *port,
					struct gpio_callback *cb,
					u32_t pins);

/**
 * @brief GPIO callback structure
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct gpio_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see gpio_init_callback() below
 */
struct gpio_callback {
	/** This is meant to be used in the driver and the user should not
	 * mess with it (see drivers/gpio/gpio_utils.h)
	 */
	sys_snode_t node;

	/** Actual callback function being called when relevant. */
	gpio_callback_handler_t handler;

	/** A mask of pins the callback is interested in, if 0 the callback
	 * will never be called. Such pin_mask can be modified whenever
	 * necessary by the owner, and thus will affect the handler being
	 * called or not. The selected pins must be configured to trigger
	 * an interrupt.
	 */
	u32_t pin_mask;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */
struct gpio_driver_api {
	int (*config)(struct device *port, int access_op, u32_t pin, int flags);
	int (*write)(struct device *port, int access_op, u32_t pin,
		     u32_t value);
	int (*read)(struct device *port, int access_op, u32_t pin,
		    u32_t *value);
	int (*manage_callback)(struct device *port, struct gpio_callback *cb,
			       bool set);
	int (*enable_callback)(struct device *port, int access_op, u32_t pin);
	int (*disable_callback)(struct device *port, int access_op, u32_t pin);
	u32_t (*get_pending_int)(struct device *dev);
};

__syscall int gpio_config(struct device *port, int access_op, u32_t pin,
			  int flags);

static inline int z_impl_gpio_config(struct device *port, int access_op,
				    u32_t pin, int flags)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	return api->config(port, access_op, pin, flags);
}

__syscall int gpio_write(struct device *port, int access_op, u32_t pin,
			 u32_t value);

static inline int z_impl_gpio_write(struct device *port, int access_op,
				   u32_t pin, u32_t value)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	return api->write(port, access_op, pin, value);
}

__syscall int gpio_read(struct device *port, int access_op, u32_t pin,
			u32_t *value);

static inline int z_impl_gpio_read(struct device *port, int access_op,
				  u32_t pin, u32_t *value)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	return api->read(port, access_op, pin, value);
}

__syscall int gpio_enable_callback(struct device *port, int access_op,
				   u32_t pin);

static inline int z_impl_gpio_enable_callback(struct device *port,
					     int access_op, u32_t pin)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	if (api->enable_callback == NULL) {
		return -ENOTSUP;
	}

	return api->enable_callback(port, access_op, pin);
}

__syscall int gpio_disable_callback(struct device *port, int access_op,
				    u32_t pin);

static inline int z_impl_gpio_disable_callback(struct device *port,
					      int access_op, u32_t pin)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	if (api->disable_callback == NULL) {
		return -ENOTSUP;
	}

	return api->disable_callback(port, access_op, pin);
}
/**
 * @endcond
 */

/**
 * @brief Configure a single pin.
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number to configure.
 * @param flags Flags for pin configuration. IN/OUT, interrupt ...
 * @return 0 if successful, negative errno code on failure.
 */
static inline int gpio_pin_configure(struct device *port, u32_t pin,
				     int flags)
{
	return gpio_config(port, GPIO_ACCESS_BY_PIN, pin, flags);
}

/**
 * @brief Write the data value to a single pin.
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where the data is written.
 * @param value Value set on the pin.
 * @return 0 if successful, negative errno code on failure.
 */
static inline int gpio_pin_write(struct device *port, u32_t pin,
				 u32_t value)
{
	return gpio_write(port, GPIO_ACCESS_BY_PIN, pin, value);
}

/**
 * @brief Read the data value of a single pin.
 *
 * Read the input state of a pin, returning the value 0 or 1.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where data is read.
 * @param value Integer pointer to receive the data values from the pin.
 * @return 0 if successful, negative errno code on failure.
 */
static inline int gpio_pin_read(struct device *port, u32_t pin,
				u32_t *value)
{
	return gpio_read(port, GPIO_ACCESS_BY_PIN, pin, value);
}

/**
 * @brief Helper to initialize a struct gpio_callback properly
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param pin_mask A bit mask of relevant pins for the handler
 */
static inline void gpio_init_callback(struct gpio_callback *callback,
				      gpio_callback_handler_t handler,
				      u32_t pin_mask)
{
	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");

	callback->handler = handler;
	callback->pin_mask = pin_mask;
}

/**
 * @brief Add an application callback.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback A valid Application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 *
 * @note Callbacks may be added to the device from within a callback
 * handler invocation, but whether they are invoked for the current
 * GPIO event is not specified.
 *
 * Note: enables to add as many callback as needed on the same port.
 */
static inline int gpio_add_callback(struct device *port,
				    struct gpio_callback *callback)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	if (api->manage_callback == NULL) {
		return -ENOTSUP;
	}

	return api->manage_callback(port, callback, true);
}

/**
 * @brief Remove an application callback.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback A valid application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 *
 * @warning It is explicitly permitted, within a callback handler, to
 * remove the registration for the callback that is running, i.e. @p
 * callback.  Attempts to remove other registrations on the same
 * device may result in undefined behavior, including failure to
 * invoke callbacks that remain registered and unintended invocation
 * of removed callbacks.
 *
 * Note: enables to remove as many callbacks as added through
 *       gpio_add_callback().
 */
static inline int gpio_remove_callback(struct device *port,
				       struct gpio_callback *callback)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->driver_api;

	if (api->manage_callback == NULL) {
		return -ENOTSUP;
	}

	return api->manage_callback(port, callback, false);
}

/**
 * @brief Enable callback(s) for a single pin.
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where the callback function is enabled.
 * @return 0 if successful, negative errno code on failure.
 *
 * Note: Depending on the driver implementation, this function will enable
 *       the pin to trigger an interruption. So as a semantic detail, if no
 *       callback is registered, of course none will be called.
 */
static inline int gpio_pin_enable_callback(struct device *port, u32_t pin)
{
	return gpio_enable_callback(port, GPIO_ACCESS_BY_PIN, pin);
}

/**
 * @brief Disable callback(s) for a single pin.
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where the callback function is disabled.
 * @return 0 if successful, negative errno code on failure.
 */
static inline int gpio_pin_disable_callback(struct device *port, u32_t pin)
{
	return gpio_disable_callback(port, GPIO_ACCESS_BY_PIN, pin);
}

/**
 * @brief Function to get pending interrupts
 *
 * The purpose of this function is to return the interrupt
 * status register for the device.
 * This is especially useful when waking up from
 * low power states to check the wake up source.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval status != 0 if at least one gpio interrupt is pending.
 * @retval 0 if no gpio interrupt is pending.
 */
__syscall int gpio_get_pending_int(struct device *dev);

static inline int z_impl_gpio_get_pending_int(struct device *dev)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)dev->driver_api;

	if (api->get_pending_int == NULL) {
		return -ENOTSUP;
	}

	return api->get_pending_int(dev);
}

/** @cond INTERNAL_HIDDEN */
struct gpio_pin_config {
	char *gpio_controller;
	u32_t gpio_pin;
};

#define GPIO_DECLARE_PIN_CONFIG_IDX(_idx)	\
	struct gpio_pin_config gpio_pin_ ##_idx
#define GPIO_DECLARE_PIN_CONFIG		\
	GPIO_DECLARE_PIN_CONFIG_IDX()

#define GPIO_PIN_IDX(_idx, _controller, _pin)	\
	.gpio_pin_ ##_idx = {			\
		.gpio_controller = (_controller),\
		.gpio_pin = (_pin),		\
	}
#define GPIO_PIN(_controller, _pin)		\
	GPIO_PIN_IDX(, _controller, _pin)

#define GPIO_GET_CONTROLLER_IDX(_idx, _conf)	\
	((_conf)->gpio_pin_ ##_idx.gpio_controller)
#define GPIO_GET_PIN_IDX(_idx, _conf)	\
	((_conf)->gpio_pin_ ##_idx.gpio_pin)

#define GPIO_GET_CONTROLLER(_conf)	GPIO_GET_CONTROLLER_IDX(, _conf)
#define GPIO_GET_PIN(_conf)		GPIO_GET_PIN_IDX(, _conf)
/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/gpio.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_H_ */
