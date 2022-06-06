/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for Pinmux drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINMUX_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINMUX_H_

/**
 * @brief Pinmux Interface
 * @defgroup pinmux_interface Pinmux Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PINMUX_FUNC_A		0
#define PINMUX_FUNC_B		1
#define PINMUX_FUNC_C		2
#define PINMUX_FUNC_D		3
#define PINMUX_FUNC_E		4
#define PINMUX_FUNC_F		5
#define PINMUX_FUNC_G		6
#define PINMUX_FUNC_H		7
#define PINMUX_FUNC_I		8
#define PINMUX_FUNC_J		9
#define PINMUX_FUNC_K		10
#define PINMUX_FUNC_L		11
#define PINMUX_FUNC_M		12
#define PINMUX_FUNC_N		13
#define PINMUX_FUNC_O		14
#define PINMUX_FUNC_P		15

#define PINMUX_PULLUP_ENABLE	(0x1)
#define PINMUX_PULLUP_DISABLE	(0x0)

#define PINMUX_INPUT_ENABLED	(0x1)
#define PINMUX_OUTPUT_ENABLED	(0x0)

/**
 * @typedef pmux_set
 * @brief Callback API upon setting a PIN's function
 * See pinmux_pin_set() for argument description
 */
typedef int (*pmux_set)(const struct device *dev, uint32_t pin, uint32_t func);
/**
 * @typedef pmux_get
 * @brief Callback API upon getting a PIN's function
 * See pinmux_pin_get() for argument description
 */
typedef int (*pmux_get)(const struct device *dev, uint32_t pin,
			uint32_t *func);
/**
 * @typedef pmux_pullup
 * @brief Callback API upon setting a PIN's pullup
 * See pinmix_pin_pullup() for argument description
 */
typedef int (*pmux_pullup)(const struct device *dev, uint32_t pin,
			   uint8_t func);
/**
 * @typedef pmux_input
 * @brief Callback API upon setting a PIN's input function
 * See pinmux_input() for argument description
 */
typedef int (*pmux_input)(const struct device *dev, uint32_t pin,
			  uint8_t func);

__subsystem struct pinmux_driver_api {
	pmux_set set;
	pmux_get get;
	pmux_pullup pullup;
	pmux_input input;
};

__deprecated static inline int pinmux_pin_set(const struct device *dev,
					      uint32_t pin, uint32_t func)
{
	const struct pinmux_driver_api *api =
		(const struct pinmux_driver_api *)dev->api;

	return api->set(dev, pin, func);
}

__deprecated static inline int pinmux_pin_get(const struct device *dev,
					      uint32_t pin, uint32_t *func)
{
	const struct pinmux_driver_api *api =
		(const struct pinmux_driver_api *)dev->api;

	return api->get(dev, pin, func);
}

__deprecated static inline int pinmux_pin_pullup(const struct device *dev,
						 uint32_t pin, uint8_t func)
{
	const struct pinmux_driver_api *api =
		(const struct pinmux_driver_api *)dev->api;

	return api->pullup(dev, pin, func);
}

__deprecated static inline int pinmux_pin_input_enable(const struct device *dev,
						       uint32_t pin,
						       uint8_t func)
{
	const struct pinmux_driver_api *api =
		(const struct pinmux_driver_api *)dev->api;

	return api->input(dev, pin, func);
}

#ifdef __cplusplus
}
#endif

/**
 *
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINMUX_H_ */
