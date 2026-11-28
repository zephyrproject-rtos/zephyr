/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_ROCKCHIP_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_ROCKCHIP_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/slist.h>

/* Rockchip GPIO v2 register offsets (RK3588). */
#define RK_GPIO_SWPORT_DR_L    0x00U
#define RK_GPIO_SWPORT_DR_H    0x04U
#define RK_GPIO_SWPORT_DDR_L   0x08U
#define RK_GPIO_SWPORT_DDR_H   0x0cU
#define RK_GPIO_INT_EN_L       0x10U
#define RK_GPIO_INT_EN_H       0x14U
#define RK_GPIO_INT_MASK_L     0x18U
#define RK_GPIO_INT_MASK_H     0x1cU
#define RK_GPIO_INT_TYPE_L     0x20U
#define RK_GPIO_INT_TYPE_H     0x24U
#define RK_GPIO_INT_POLARITY_L 0x28U
#define RK_GPIO_INT_POLARITY_H 0x2cU
#define RK_GPIO_INT_BOTHEDGE_L 0x30U
#define RK_GPIO_INT_BOTHEDGE_H 0x34U
#define RK_GPIO_INT_STATUS     0x50U
#define RK_GPIO_PORT_EOI_L     0x60U
#define RK_GPIO_PORT_EOI_H     0x64U
#define RK_GPIO_EXT_PORT       0x70U
#define RK_GPIO_VER_ID         0x78U

#ifdef CONFIG_CLOCK_CONTROL
struct gpio_rockchip_clock {
	const struct device *dev;
	uint16_t id;
};
#endif

struct gpio_rockchip_config {
	/* common must be the first member of the driver config structure. */
	struct gpio_driver_config common;
	DEVICE_MMIO_NAMED_ROM(reg_base);
	void (*irq_config_func)(const struct device *port);
#ifdef CONFIG_CLOCK_CONTROL
	const struct gpio_rockchip_clock *clocks;
	size_t clock_count;
#endif
};

struct gpio_rockchip_data {
	/* common must be the first member of the driver data structure. */
	struct gpio_driver_data common;
	DEVICE_MMIO_NAMED_RAM(reg_base);
	sys_slist_t callbacks;
	struct k_spinlock lock;
};

#ifdef CONFIG_ZTEST
extern const struct gpio_driver_api gpio_rockchip_api;
void gpio_rockchip_isr(const struct device *port);
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_ROCKCHIP_H_ */
