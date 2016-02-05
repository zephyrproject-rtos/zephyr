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
 * @file
 * @brief Public APIs for GPIO drivers
 */

#ifndef __GPIO_H__
#define __GPIO_H__
/**
 * @brief GPIO Driver APIs
 * @defgroup gpio_interface GPIO Driver APIs
 * @ingroup io_interfaces
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_ACCESS_BY_PIN 0
#define GPIO_ACCESS_BY_PORT 1

#include <stdint.h>
#include <stddef.h>
#include <device.h>

/** @brief Define flag/config bits. */
#define GPIO_DIR_IN		(0 << 0)
#define GPIO_DIR_OUT		(1 << 0)
#define GPIO_DIR_MASK		0x1

#define GPIO_INT		(1 << 1)

#define GPIO_INT_ACTIVE_LOW	(0 << 2)
#define GPIO_INT_ACTIVE_HIGH	(1 << 2)
#define GPIO_INT_CLOCK_SYNC     (1 << 3)
#define GPIO_INT_DEBOUNCE       (1 << 4)
#define GPIO_INT_LEVEL		(0 << 5)
#define GPIO_INT_EDGE		(1 << 5)
#define GPIO_INT_DOUBLE_EDGE	(1 << 6)

/** @brief Define the polarity of the GPIO (1 bit). */
#define GPIO_POL_POS		7
#define GPIO_POL_NORMAL		(0 << GPIO_POL_POS)
#define GPIO_POL_INV		(1 << GPIO_POL_POS)
#define GPIO_POL_MASK		(1 << GPIO_POL_POS)

#define GPIO_PUD_POS		8
#define GPIO_PUD_NORMAL		(0 << GPIO_PUD_POS)
#define GPIO_PUD_PULL_UP	(1 << GPIO_PUD_POS)
#define GPIO_PUD_PULL_DOWN	(2 << GPIO_PUD_POS)
#define GPIO_PUD_MASK		(3 << GPIO_PUD_POS)

/**
 * @brief Pin enable / disable.
 *
 * Individual pins can be enabled or disabled
 * if the controller supports this operation.
 */
#define GPIO_PIN_ENABLE		(1 << 10)
#define GPIO_PIN_DISABLE	(1 << 11)

/**
 * @brief Define the application callback function signature.
 *
 */
typedef void (*gpio_callback_t)(struct device *port, uint32_t pin);

/**
 * @brief GPIO driver API definition.
 */
typedef int (*gpio_config_t)(struct device *port, int access_op,
			     uint32_t pin, int flags);
typedef int (*gpio_write_t)(struct device *port, int access_op,
			    uint32_t pin, uint32_t value);
typedef int (*gpio_read_t)(struct device *port, int access_op,
			   uint32_t pin, uint32_t *value);
typedef int (*gpio_set_callback_t)(struct device *port,
				   gpio_callback_t callback);
typedef int (*gpio_enable_callback_t)(struct device *port,
				      int access_op,
				      uint32_t pin);
typedef int (*gpio_disable_callback_t)(struct device *port,
				       int access_op,
				       uint32_t pin);
typedef int (*gpio_suspend_port_t)(struct device *port);
typedef int (*gpio_resume_port_t)(struct device *port);

struct gpio_driver_api {
	gpio_config_t config;
	gpio_write_t write;
	gpio_read_t read;
	gpio_set_callback_t set_callback;
	gpio_enable_callback_t enable_callback;
	gpio_disable_callback_t disable_callback;
	gpio_suspend_port_t suspend;
	gpio_resume_port_t resume;
};

/**
 * @brief Configure a single pin.
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number to configure.
 * @param flags Flags for pin configuration. IN/OUT, interrupt ...
 *
 */
static inline int gpio_pin_configure(struct device *port, uint8_t pin,
				     int flags)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->config(port, GPIO_ACCESS_BY_PIN, pin, flags);
}

/**
 * @brief Write the data value to a single pin.
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where the data is written.
 * @param value Value set on the pin.
 */
static inline int gpio_pin_write(struct device *port, uint32_t pin,
				 uint32_t value)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->write(port, GPIO_ACCESS_BY_PIN, pin, value);

}

/**
 * @brief Read the data value of a single pin.
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where data is read.
 * @param value Integer pointer to receive the data vales from the pin.
 */
static inline int gpio_pin_read(struct device *port, uint32_t pin,
				uint32_t *value)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->read(port, GPIO_ACCESS_BY_PIN, pin, value);

}

/**
 * @brief Set the application's callback function.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback Application's callback function.
 */
static inline int gpio_set_callback(struct device *port,
				    gpio_callback_t callback)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->set_callback(port, callback);
}

/**
 * @brief Enable the callback function for a single pin.
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where the callback function is enabled.
 */
static inline int gpio_pin_enable_callback(struct device *port, uint32_t pin)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->enable_callback(port, GPIO_ACCESS_BY_PIN, pin);

}

/**
 * @brief Disable the callback function for a single pin.
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number where the callback function is disabled.
 */
static inline int gpio_pin_disable_callback(struct device *port, uint32_t pin)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->disable_callback(port, GPIO_ACCESS_BY_PIN, pin);
}


/**
 * @brief Configure all the pins in the port.
 *       List out all flags on the detailed description.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param flags Flags for the port configuration. IN/OUT, interrupt ...
 */
static inline int gpio_port_configure(struct device *port, int flags)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->config(port, GPIO_ACCESS_BY_PORT, 0, flags);
}

/**
 * @brief Write a data value to the port.
 * @param port Pointer to the device structure for the driver instance.
 * @param value Value to set on the port.
 */
static inline int gpio_port_write(struct device *port, uint32_t value)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->write(port, GPIO_ACCESS_BY_PORT, 0, value);

}

/**
 * @brief Read data value from the port.
 * @param port Pointer to the device structure for the driver instance.
 * @param value Integer pointer to receive the data value from the port.
 */
static inline int gpio_port_read(struct device *port, uint32_t *value)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->read(port, GPIO_ACCESS_BY_PORT, 0, value);

}

/**
 * @brief Enable port callback.
 * @param port Pointer to the device structure for the driver instance.
 */
static inline int gpio_port_enable_callback(struct device *port)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->enable_callback(port, GPIO_ACCESS_BY_PORT, 0);

}

/**
 * @brief Disable the callback function for the port.
 * @param port Pointer to the device structure for the driver instance.
 */
static inline int gpio_port_disable_callback(struct device *port)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->disable_callback(port, GPIO_ACCESS_BY_PORT, 0);
}

/**
 * @brief Save the state of the device and make it go to the
 * low power state.
 * @param port Pointer to the device structure for the driver instance.
 */
static inline int gpio_suspend(struct device *port)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->suspend(port);
}

/**
 * @brief Restore the state stored during suspend and resume operation.
 * @param port Pointer to the device structure for the driver instance.
 */
static inline int gpio_resume(struct device *port)
{
	struct gpio_driver_api *api;

	api = (struct gpio_driver_api *) port->driver_api;
	return api->resume(port);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __GPIO_H__ */
