/* pinmux.h - the private pinmux driver header */

/*
 * Copyright (c) 2015 Intel Corporation
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
#ifndef __INCLUDE_PINMUX_H
#define __INCLUDE_PINMUX_H

/**
 * @brief Pinmux Interface
 * @defgroup pinmux_interface Pinmux Interface
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <device.h>

#define PINMUX_NAME		"pinmux"

#define PINMUX_FUNC_A		0
#define PINMUX_FUNC_B		1
#define PINMUX_FUNC_C		2
#define PINMUX_FUNC_D		3

#define PINMUX_PULLUP_ENABLE	(0x1)
#define PINMUX_PULLUP_DISABLE	(0x0)

#define PINMUX_INPUT_ENABLED	(0x1)
#define PINMUX_OUTPUT_ENABLED	(0x0)

typedef uint32_t (*pmux_set)(struct device *dev, uint32_t pin, uint8_t func);
typedef uint32_t (*pmux_get)(struct device *dev, uint32_t pin, uint8_t *func);
typedef uint32_t (*pmux_pullup)(struct device *dev, uint32_t pin, uint8_t func);
typedef uint32_t (*pmux_input)(struct device *dev, uint32_t pin, uint8_t func);

struct pinmux_driver_api {
	pmux_set set;
	pmux_get get;
	pmux_pullup pullup;
	pmux_input input;
};


static inline uint32_t pinmux_pin_set(struct device *dev,
				      uint32_t pin,
				      uint8_t func)
{
	struct pinmux_driver_api *api;

	api = (struct pinmux_driver_api *) dev->driver_api;
	return api->set(dev, pin, func);
}

static inline uint32_t pinmux_pin_get(struct device *dev,
				      uint32_t pin,
				      uint8_t *func)
{
	struct pinmux_driver_api *api;

	api = (struct pinmux_driver_api *) dev->driver_api;
	return api->get(dev, pin, func);
}

static inline uint32_t pinmux_pin_pullup(struct device *dev,
					 uint32_t pin,
					 uint8_t func)
{
	struct pinmux_driver_api *api;

	api = (struct pinmux_driver_api *) dev->driver_api;
	return api->pullup(dev, pin, func);
}

static inline uint32_t pinmux_pin_input_enable(struct device *dev,
					       uint32_t pin,
					       uint8_t func)
{
	struct pinmux_driver_api *api;

	api = (struct pinmux_driver_api *) dev->driver_api;
	return api->input(dev, pin, func);
}

/**
 *
 * @}
 */
#endif /* __INCLUDE_PINMUX_H */
