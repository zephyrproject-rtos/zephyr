/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
