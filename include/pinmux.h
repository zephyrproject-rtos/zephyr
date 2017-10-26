/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for Pinmux drivers
 */

#ifndef __INCLUDE_PINMUX_H
#define __INCLUDE_PINMUX_H

/**
 * @brief Pinmux Interface
 * @defgroup pinmux_interface Pinmux Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PINMUX_FUNC_A		0
#define PINMUX_FUNC_B		1
#define PINMUX_FUNC_C		2
#define PINMUX_FUNC_D		3
#define PINMUX_FUNC_E		4
#define PINMUX_FUNC_F		5

#define PINMUX_PULLUP_ENABLE	(0x1)
#define PINMUX_PULLUP_DISABLE	(0x0)

#define PINMUX_INPUT_ENABLED	(0x1)
#define PINMUX_OUTPUT_ENABLED	(0x0)

/**
 * @typedef pmux_set
 * @brief Callback API upon setting a PIN's function
 * See pinmux_pin_set() for argument description
 */
typedef int (*pmux_set)(struct device *dev, u32_t pin, u32_t func);
/**
 * @typedef pmux_get
 * @brief Callback API upon getting a PIN's function
 * See pinmux_pin_get() for argument description
 */
typedef int (*pmux_get)(struct device *dev, u32_t pin, u32_t *func);
/**
 * @typedef pmux_pullup
 * @brief Callback API upon setting a PIN's pullup
 * See pinmix_pin_pullup() for argument description
 */
typedef int (*pmux_pullup)(struct device *dev, u32_t pin, u8_t func);
/**
 * @typedef pmux_input
 * @brief Callback API upon setting a PIN's input function
 * See pinmux_input() for argument description
 */
typedef int (*pmux_input)(struct device *dev, u32_t pin, u8_t func);

struct pinmux_driver_api {
	pmux_set set;
	pmux_get get;
	pmux_pullup pullup;
	pmux_input input;
};

__syscall int pinmux_pin_set(struct device *dev, u32_t pin, u32_t func);

static inline int _impl_pinmux_pin_set(struct device *dev, u32_t pin,
				       u32_t func)
{
	const struct pinmux_driver_api *api = dev->driver_api;

	return api->set(dev, pin, func);
}

__syscall int pinmux_pin_get(struct device *dev, u32_t pin, u32_t *func);

static inline int _impl_pinmux_pin_get(struct device *dev, u32_t pin,
				       u32_t *func)
{
	const struct pinmux_driver_api *api = dev->driver_api;

	return api->get(dev, pin, func);
}

__syscall int pinmux_pin_pullup(struct device *dev, u32_t pin, u8_t func);

static inline int _impl_pinmux_pin_pullup(struct device *dev, u32_t pin,
					  u8_t func)
{
	const struct pinmux_driver_api *api = dev->driver_api;

	return api->pullup(dev, pin, func);
}

__syscall int pinmux_pin_input_enable(struct device *dev, u32_t pin, u8_t func);

static inline int _impl_pinmux_pin_input_enable(struct device *dev,
						u32_t pin, u8_t func)
{
	const struct pinmux_driver_api *api = dev->driver_api;

	return api->input(dev, pin, func);
}

#ifdef __cplusplus
}
#endif

/**
 *
 * @}
 */

#include <syscalls/pinmux.h>

#endif /* __INCLUDE_PINMUX_H */
