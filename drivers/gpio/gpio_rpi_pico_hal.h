/*
 * Copyright (c) 2021, Yonatan Schachter
 * Copyright (c) 2025, Andrew Featherstone
 * Copyright (c) 2025, TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_RPI_PICO_HAL_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_RPI_PICO_HAL_H_

/* pico-sdk includes */
#include <hardware/gpio.h>
#include <hardware/regs/intctrl.h>
#include <hardware/structs/iobank0.h>

#define GPIO_RPI_PINS_PER_PORT 32
#define ALL_EVENTS                                                                                 \
	(GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE | GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH)

#define GPIO_REG_0U                  1
#define IS_GPIO_RPI_LO_NODE(node_id) UTIL_CAT(GPIO_REG_, DT_REG_ADDR(node_id))

static inline void gpio_set_dir_out_masked_n(uint n, uint32_t mask)
{
	if (!n) {
		gpio_set_dir_out_masked(mask);
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_hi_oe_set(mask);
#else
		sio_hw->gpio_hi_oe_set = mask;
#endif
	}
}

static inline void gpio_set_dir_in_masked_n(uint n, uint32_t mask)
{
	if (!n) {
		gpio_set_dir_in_masked(mask);
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_hi_oe_clr(mask);
#else
		sio_hw->gpio_hi_oe_clr = mask;
#endif
	}
}

static inline void gpio_set_dir_masked_n(uint n, uint32_t mask, uint32_t value)
{
	if (!n) {
		gpio_set_dir_masked(mask, value);
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_hi_oe_xor((gpioc_hi_oe_get() ^ value) & mask);
#else
		sio_hw->gpio_hi_oe_togl = (sio_hw->gpio_hi_oe ^ value) & mask;
#endif
	}
}

static inline uint32_t gpio_get_all_n(uint n)
{
	if (!n) {
		return gpio_get_all();
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		return gpioc_hi_in_get();
#else
		return sio_hw->gpio_hi_in;
#endif
	}

	return 0;
}

static inline void gpio_toggle_dir_masked_n(uint n, uint32_t mask)
{
	if (!n) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_lo_oe_xor(mask);
#else
		sio_hw->gpio_oe_togl = mask;
#endif
	} else if (n == 1) {
#if PICO_USE_GPIO_COPROCESSOR
		gpioc_hi_oe_xor(mask);
#else
		sio_hw->gpio_hi_oe_togl = mask;
#endif
	}
}

static inline uint32_t gpio_get_dir_all_bits_n(uint n)
{
	if (!n) {
		return sio_hw->gpio_oe;
	} else {
		return sio_hw->gpio_hi_oe;
	}
}

static inline bool gpio_is_input_enabled(uint gpio)
{
	check_gpio_param(gpio);
	return (pads_bank0_hw->io[gpio] & PADS_BANK0_GPIO0_IE_BITS) != 0;
}

static inline bool gpio_is_output_disabled(uint gpio)
{
	check_gpio_param(gpio);
	return (pads_bank0_hw->io[gpio] & PADS_BANK0_GPIO0_OD_BITS) != 0;
}

static inline void gpio_set_input_enabled_output_disabled(uint gpio, bool ie, bool od)
{
	const uint32_t value =
		(ie ? PADS_BANK0_GPIO0_IE_BITS : 0) | (od ? PADS_BANK0_GPIO0_OD_BITS : 0);

	check_gpio_param(gpio);
	hw_write_masked(&pads_bank0_hw->io[gpio], value,
			PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS);
}

static inline bool gpio_has_pending_irq(void)
{
	io_bank0_irq_ctrl_hw_t *irq_ctrl_base =
		get_core_num() ? &io_bank0_hw->proc1_irq_ctrl : &io_bank0_hw->proc0_irq_ctrl;
	ARRAY_FOR_EACH_PTR(irq_ctrl_base->ints, p) {
		if (*p) {
			return 1;
		}
	}

	return 0;
}

static inline int gpio_rpi_hal_irq_setup(void)
{
	return 0;
}

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_RPI_PICO_HAL_H_ */
