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
 * @file Header file for the MMIO-based GPIO driver.
 */

#ifndef _GPIO_MMIO_H_
#define _GPIO_MMIO_H_

#include <nanokernel.h>

#include <gpio.h>

/* For enable register:
 * NORMAL: 0 - disable, 1 - enable
 * INV: 0 - enable, 1 - disable
 */
#define GPIO_MMIO_CFG_EN_NORMAL		(0 << 0)
#define GPIO_MMIO_CFG_EN_INV		(1 << 0)
#define GPIO_MMIO_CFG_EN_MASK		(1 << 0)

/* For direction register:
 * NORMAL: 0 - pin is output, 1 - pin is input
 * INV: 0 - pin is input, 1 - pin is output
 */
#define GPIO_MMIO_CFG_DIR_NORMAL	(0 << 1)
#define GPIO_MMIO_CFG_DIR_INV		(1 << 1)
#define GPIO_MMIO_CFG_DIR_MASK		(1 << 1)

/**
 * @brief Initialization function for GPIO driver
 *
 * @param dev Device struct
 * @return DEV_OK if successful, failed otherwise
 */
extern int gpio_mmio_init(struct device *dev);

/* internal use only for register access function */
typedef uint32_t (*__gpio_mmio_access_t)(uint32_t addr, uint32_t bit,
					 uint32_t value);

/** Configuration data */
struct gpio_mmio_config {
	/* config flags */
	uint32_t cfg_flags;

	struct {
		/* enable register */
		uint32_t en;

		/* direction register */
		uint32_t dir;

		/* pin level register for input*/
		uint32_t input;

		/* pin level register for output */
		uint32_t output;
	} reg;

	struct {
		__gpio_mmio_access_t set_bit;
		__gpio_mmio_access_t read;
		__gpio_mmio_access_t write;
	} access;
};

#endif /* _GPIO_MMIO_H_ */
