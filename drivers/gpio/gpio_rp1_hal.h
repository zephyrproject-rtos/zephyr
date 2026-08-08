/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_RP1_HAL_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_RP1_HAL_H_

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_rp1.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

#include "gpio_rpi_pico.h"

#define RP1_ATOMIC_RAW_OFF 0x0000
#define RP1_ATOMIC_XOR_OFF 0x1000
#define RP1_ATOMIC_SET_OFF 0x2000
#define RP1_ATOMIC_CLR_OFF 0x3000

#define GPIO_STATUS_ADDR(port, n) (DEVICE_MMIO_NAMED_GET(port, gpio) + 0x8 * n)
#define GPIO_CTRL_ADDR(port, n)   (GPIO_STATUS_ADDR(port, n) + 0x4)
#define PADS_CTRL_ADDR(port, n)   (DEVICE_MMIO_NAMED_GET(port, pads) + 0x4 * (n))

/*
 * Interrupt summary registers of the PCIE destination, which is the
 * one wired towards the host on the Raspberry Pi 5. The registers at
 * 0x100-0x118 are the raw INTR and the PROC0/PROC1 destinations.
 */
#define GPIO_INTE_ADDR(port) (DEVICE_MMIO_NAMED_GET(port, gpio) + 0x11c)
#define GPIO_INTF_ADDR(port) (DEVICE_MMIO_NAMED_GET(port, gpio) + 0x120)
#define GPIO_INTS_ADDR(port) (DEVICE_MMIO_NAMED_GET(port, gpio) + 0x124)
#define RIO_OUT_ADDR(port)   (DEVICE_MMIO_NAMED_GET(port, rio) + 0x0)
#define RIO_OE_ADDR(port)    (DEVICE_MMIO_NAMED_GET(port, rio) + 0x4)
#define RIO_IN_ADDR(port)    (DEVICE_MMIO_NAMED_GET(port, rio) + 0x8)

#define GPIO_CTRL(port, n)          sys_read32(GPIO_CTRL_ADDR(port, n))
#define GPIO_CTRL_RAW(port, n, val) sys_write32(val, GPIO_CTRL_ADDR(port, n) + RP1_ATOMIC_RAW_OFF)
#define GPIO_CTRL_XOR(port, n, val) sys_write32(val, GPIO_CTRL_ADDR(port, n) + RP1_ATOMIC_XOR_OFF)
#define GPIO_CTRL_SET(port, n, val) sys_write32(val, GPIO_CTRL_ADDR(port, n) + RP1_ATOMIC_SET_OFF)
#define GPIO_CTRL_CLR(port, n, val) sys_write32(val, GPIO_CTRL_ADDR(port, n) + RP1_ATOMIC_CLR_OFF)
#define PADS_CTRL(port, n)          sys_read32(PADS_CTRL_ADDR(port, n))
#define PADS_CTRL_RAW(port, n, val) sys_write32(val, PADS_CTRL_ADDR(port, n) + RP1_ATOMIC_RAW_OFF)
#define PADS_CTRL_XOR(port, n, val) sys_write32(val, PADS_CTRL_ADDR(port, n) + RP1_ATOMIC_XOR_OFF)
#define PADS_CTRL_SET(port, n, val) sys_write32(val, PADS_CTRL_ADDR(port, n) + RP1_ATOMIC_SET_OFF)
#define PADS_CTRL_CLR(port, n, val) sys_write32(val, PADS_CTRL_ADDR(port, n) + RP1_ATOMIC_CLR_OFF)

#define GPIO_INTE(port)          sys_read32(GPIO_INTE_ADDR(port))
#define GPIO_INTE_RAW(port, val) sys_write32(val, GPIO_INTE_ADDR(port) + RP1_ATOMIC_RAW_OFF)
#define GPIO_INTE_XOR(port, val) sys_write32(val, GPIO_INTE_ADDR(port) + RP1_ATOMIC_XOR_OFF)
#define GPIO_INTE_SET(port, val) sys_write32(val, GPIO_INTE_ADDR(port) + RP1_ATOMIC_SET_OFF)
#define GPIO_INTE_CLR(port, val) sys_write32(val, GPIO_INTE_ADDR(port) + RP1_ATOMIC_CLR_OFF)
#define GPIO_INTF(port)          sys_read32(GPIO_INTF_ADDR(port))
#define GPIO_INTF_RAW(port, val) sys_write32(val, GPIO_INTF_ADDR(port) + RP1_ATOMIC_RAW_OFF)
#define GPIO_INTF_XOR(port, val) sys_write32(val, GPIO_INTF_ADDR(port) + RP1_ATOMIC_XOR_OFF)
#define GPIO_INTF_SET(port, val) sys_write32(val, GPIO_INTF_ADDR(port) + RP1_ATOMIC_SET_OFF)
#define GPIO_INTF_CLR(port, val) sys_write32(val, GPIO_INTF_ADDR(port) + RP1_ATOMIC_CLR_OFF)
#define GPIO_INTS(port)          sys_read32(GPIO_INTS_ADDR(port))
#define RIO_OUT(port)            sys_read32(RIO_OUT_ADDR(port))
#define RIO_OUT_RAW(port, val)   sys_write32(val, RIO_OUT_ADDR(port) + RP1_ATOMIC_RAW_OFF)
#define RIO_OUT_XOR(port, val)   sys_write32(val, RIO_OUT_ADDR(port) + RP1_ATOMIC_XOR_OFF)
#define RIO_OUT_SET(port, val)   sys_write32(val, RIO_OUT_ADDR(port) + RP1_ATOMIC_SET_OFF)
#define RIO_OUT_CLR(port, val)   sys_write32(val, RIO_OUT_ADDR(port) + RP1_ATOMIC_CLR_OFF)
#define RIO_OE(port)             sys_read32(RIO_OE_ADDR(port))
#define RIO_OE_RAW(port, val)    sys_write32(val, RIO_OE_ADDR(port) + RP1_ATOMIC_RAW_OFF)
#define RIO_OE_XOR(port, val)    sys_write32(val, RIO_OE_ADDR(port) + RP1_ATOMIC_XOR_OFF)
#define RIO_OE_SET(port, val)    sys_write32(val, RIO_OE_ADDR(port) + RP1_ATOMIC_SET_OFF)
#define RIO_OE_CLR(port, val)    sys_write32(val, RIO_OE_ADDR(port) + RP1_ATOMIC_CLR_OFF)
#define RIO_IN(port)             sys_read32(RIO_IN_ADDR(port))

#define GPIO_CTRL_FUNCSEL_RIO 0x5
#define GPIO_FUNC_SIO         0x5
#define GPIO_IN               0
#define GPIO_OUT              1

#define NUM_BANK0_GPIOS        28
#define GPIO_RPI_PINS_PER_PORT 28
#define GPIO_CTRL_IRQMASK_ALL                                                                      \
	(GPIO_CTRL_IRQMASK_EDGE_LOW_MASK | GPIO_CTRL_IRQMASK_EDGE_HIGH_MASK |                      \
	 GPIO_CTRL_IRQMASK_LEVEL_LOW_MASK | GPIO_CTRL_IRQMASK_LEVEL_HIGH_MASK |                    \
	 GPIO_CTRL_IRQMASK_F_EDGE_LOW_MASK | GPIO_CTRL_IRQMASK_F_EDGE_HIGH_MASK |                  \
	 GPIO_CTRL_IRQMASK_DB_LEVEL_LOW_MASK | GPIO_CTRL_IRQMASK_DB_LEVEL_HIGH_MASK)

/* The leading bank (child index 0) is the low node; later banks are high nodes. */
#define GPIO_RPI_CHILD_IDX_0             1
#define IS_GPIO_RPI_LO_NODE(node_id) UTIL_CAT(GPIO_RPI_CHILD_IDX_, DT_NODE_CHILD_IDX(node_id))

enum {
	GPIO_IRQ_LEVEL_LOW = 0x1u,
	GPIO_IRQ_LEVEL_HIGH = 0x2u,
	GPIO_IRQ_EDGE_FALL = 0x4u,
	GPIO_IRQ_EDGE_RISE = 0x8u,
};

#define ALL_EVENTS                                                                                 \
	(GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE | GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH)

typedef unsigned int uint;

/*
 * Only a single RP1 GPIO bank can be enabled (enforced by BUILD_ASSERT in the
 * driver), so this HAL targets that one instance directly and ignores the bank
 * index ("n") and port arguments passed by the shared gpio_rpi_pico.c code.
 */
static const struct device *rp1_port0 =
	DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(raspberrypi_rp1_gpio));

static inline bool gpio_is_pulled_up(uint pin)
{
	return (PADS_CTRL(rp1_port0, pin) & GPIO_PADS_PULL_UP_ENABLE_MASK) != 0;
}

static inline bool gpio_is_pulled_down(uint pin)
{
	return (PADS_CTRL(rp1_port0, pin) & GPIO_PADS_PULL_DOWN_ENABLE_MASK) != 0;
}

static inline uint32_t gpio_get_irq_event_mask(uint pin)
{
	if (GPIO_INTS(rp1_port0) & BIT(pin)) {
		uint32_t ctrl = GPIO_CTRL(rp1_port0, pin);
		uint32_t events = 0;

		if (ctrl & GPIO_CTRL_IRQMASK_EDGE_LOW_MASK) {
			events |= GPIO_IRQ_EDGE_FALL;
		}
		if (ctrl & GPIO_CTRL_IRQMASK_EDGE_HIGH_MASK) {
			events |= GPIO_IRQ_EDGE_RISE;
		}
		if (ctrl & GPIO_CTRL_IRQMASK_LEVEL_LOW_MASK) {
			events |= GPIO_IRQ_LEVEL_LOW;
		}
		if (ctrl & GPIO_CTRL_IRQMASK_LEVEL_HIGH_MASK) {
			events |= GPIO_IRQ_LEVEL_HIGH;
		}

		return events;
	}

	return 0;
}

static inline void gpio_acknowledge_irq(uint pin, uint32_t event_mask)
{
	(void)event_mask;
	GPIO_CTRL_SET(rp1_port0, pin, GPIO_CTRL_IRQRESET_MASK);
}

static inline void gpio_set_mask_n(uint n, uint32_t mask)
{
	RIO_OUT_SET(rp1_port0, mask);
}

static inline void gpio_clr_mask_n(uint n, uint32_t mask)
{
	RIO_OUT_CLR(rp1_port0, mask);
}

static inline void gpio_xor_mask_n(uint n, uint32_t mask)
{
	RIO_OUT_XOR(rp1_port0, mask);
}

static inline void gpio_put_masked_n(uint n, uint32_t mask, uint32_t value)
{
	RIO_OUT_XOR(rp1_port0, (RIO_OUT(rp1_port0) ^ value) & mask);
}

static inline void gpio_put(uint pin, bool value)
{
	if (value) {
		RIO_OUT_SET(rp1_port0, BIT(pin));
	} else {
		RIO_OUT_CLR(rp1_port0, BIT(pin));
	}
}

static inline bool gpio_get_out_level(uint pin)
{
	return !!(RIO_OUT(rp1_port0) & BIT(pin));
}

static inline void gpio_set_dir(uint pin, bool value)
{
	if (value == GPIO_OUT) {
		RIO_OE_SET(rp1_port0, BIT(pin));
	} else {
		RIO_OE_CLR(rp1_port0, BIT(pin));
	}
}

static inline bool gpio_get_dir(uint pin)
{
	return !!(RIO_OE(rp1_port0) & BIT(pin));
}

static inline void gpio_set_function(uint pin, uint32_t fn)
{
	uint32_t ctrl = GPIO_CTRL(rp1_port0, pin) & GPIO_CTRL_RESERVED_MASK;

	PADS_CTRL_SET(rp1_port0, pin, GPIO_PADS_INPUT_ENABLE_MASK);
	PADS_CTRL_CLR(rp1_port0, pin, GPIO_PADS_OUTPUT_DISABLE_MASK);

	ctrl |= (fn << GPIO_CTRL_FUNCSEL_SHIFT) & GPIO_CTRL_FUNCSEL_MASK;
	GPIO_CTRL_RAW(rp1_port0, pin, ctrl);
}

static inline void gpio_set_pulls(uint pin, bool up, bool down)
{
	PADS_CTRL_XOR(rp1_port0, pin,
		      (PADS_CTRL(rp1_port0, pin) ^ ((up << GPIO_PADS_PULL_UP_ENABLE_SHIFT) |
						    (down << GPIO_PADS_PULL_DOWN_ENABLE_SHIFT))) &
			      (GPIO_PADS_PULL_UP_ENABLE_MASK | GPIO_PADS_PULL_DOWN_ENABLE_MASK));
}

static inline void gpio_set_input_enabled(uint pin, bool enabled)
{
	if (enabled) {
		PADS_CTRL_SET(rp1_port0, pin, GPIO_PADS_INPUT_ENABLE_MASK);
	} else {
		PADS_CTRL_CLR(rp1_port0, pin, GPIO_PADS_INPUT_ENABLE_MASK);
	}
}

static inline int gpio_set_irq_enabled(uint pin, uint32_t events, bool value)
{
	uint32_t ctrl_events = 0;

	if (events & GPIO_IRQ_EDGE_FALL) {
		ctrl_events |= GPIO_CTRL_IRQMASK_EDGE_LOW_MASK;
	}
	if (events & GPIO_IRQ_EDGE_RISE) {
		ctrl_events |= GPIO_CTRL_IRQMASK_EDGE_HIGH_MASK;
	}
	if (events & GPIO_IRQ_LEVEL_LOW) {
		ctrl_events |= GPIO_CTRL_IRQMASK_LEVEL_LOW_MASK;
	}
	if (events & GPIO_IRQ_LEVEL_HIGH) {
		ctrl_events |= GPIO_CTRL_IRQMASK_LEVEL_HIGH_MASK;
	}

	if (value) {
		GPIO_CTRL_SET(rp1_port0, pin, GPIO_CTRL_IRQRESET_MASK);
		GPIO_CTRL_SET(rp1_port0, pin, ctrl_events);
		GPIO_INTE_SET(rp1_port0, BIT(pin));
	} else {
		GPIO_INTE_CLR(rp1_port0, BIT(pin));
		GPIO_CTRL_CLR(rp1_port0, pin, GPIO_CTRL_IRQMASK_ALL);
		GPIO_CTRL_SET(rp1_port0, pin, GPIO_CTRL_IRQRESET_MASK);
	}

	return 0;
}

static inline void gpio_set_dir_out_masked_n(uint n, uint32_t mask)
{
	RIO_OE_SET(rp1_port0, mask);
}

static inline void gpio_set_dir_in_masked_n(uint n, uint32_t mask)
{
	RIO_OE_CLR(rp1_port0, mask);
}

static inline void gpio_set_dir_masked_n(uint n, uint32_t mask, uint32_t value)
{
	(void)n;
	RIO_OE_XOR(rp1_port0, (RIO_OE(rp1_port0) ^ value) & mask);
}

static inline uint32_t gpio_get_all_n(uint n)
{
	(void)n;
	return RIO_IN(rp1_port0);
}

static inline void gpio_toggle_dir_masked_n(uint n, uint32_t mask)
{
	(void)n;
	RIO_OE_XOR(rp1_port0, mask);
}

static inline uint32_t gpio_get_dir_all_bits_n(uint n)
{
	(void)n;
	return RIO_OE(rp1_port0);
}

static inline bool gpio_is_input_enabled(uint pin)
{
	return !!(PADS_CTRL(rp1_port0, pin) & GPIO_PADS_INPUT_ENABLE_MASK);
}

static inline bool gpio_is_output_disabled(uint pin)
{
	return !!(PADS_CTRL(rp1_port0, pin) & GPIO_PADS_OUTPUT_DISABLE_MASK);
}

static inline void gpio_set_input_enabled_output_disabled(uint pin, bool ie, bool od)
{
	PADS_CTRL_XOR(rp1_port0, pin,
		      (PADS_CTRL(rp1_port0, pin) ^ ((ie << GPIO_PADS_INPUT_ENABLE_SHIFT) |
						    (od << GPIO_PADS_OUTPUT_DISABLE_SHIFT))) &
			      (GPIO_PADS_INPUT_ENABLE_MASK | GPIO_PADS_OUTPUT_DISABLE_MASK));
}

static inline bool gpio_has_pending_irq(void)
{
	return !!GPIO_INTS(rp1_port0);
}

static inline int gpio_rpi_hal_irq_setup(void)
{
	return 0;
}

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_RP1_HAL_H_ */
