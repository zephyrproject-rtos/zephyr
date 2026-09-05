/*
 * Copyright (c) 2026 Khai Do
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/spinlock.h>

#define SUNXI_PIO_BASE_PHYS DT_REG_ADDR(DT_NODELABEL(pio))
#define SUNXI_PIO_BASE sunxi_pio_base_virt
#define SUNXI_CFG_PINS_PER_REG           8
#define SUNXI_CFG_BITS_PER_PIN           4
#define SUNXI_DRV_PINS_PER_REG           16
#define SUNXI_DRV_BITS_PER_PIN           2
#define SUNXI_PULL_PINS_PER_REG          16
#define SUNXI_PULL_BITS_PER_PIN          2

#define SUNXI_BANK_SIZE                  0x24
#define SUNXI_DRV_OFFSET                 0x14
#define SUNXI_PULL_OFFSET                0x1C

#define SUNXI_CFG_OFFSET(pin) \
	(((pin) / SUNXI_CFG_PINS_PER_REG) * sizeof(uint32_t))
#define SUNXI_DRV_OFFSET_BY_PIN(pin) \
	(SUNXI_DRV_OFFSET + ((pin) / SUNXI_DRV_PINS_PER_REG) * sizeof(uint32_t))
#define SUNXI_PULL_OFFSET_BY_PIN(pin) \
	(SUNXI_PULL_OFFSET + ((pin) / SUNXI_PULL_PINS_PER_REG) * sizeof(uint32_t))

#define SUNXI_CFG_PIN_SHIFT(pin) \
	(((pin) % SUNXI_CFG_PINS_PER_REG) * SUNXI_CFG_BITS_PER_PIN)
#define SUNXI_DRV_PIN_SHIFT(pin) \
	(((pin) % SUNXI_DRV_PINS_PER_REG) * SUNXI_DRV_BITS_PER_PIN)
#define SUNXI_PULL_PIN_SHIFT(pin) \
	(((pin) % SUNXI_PULL_PINS_PER_REG) * SUNXI_PULL_BITS_PER_PIN)

#define SUNXI_CFG_PIN_MASK(pin)          (GENMASK(3, 0) << SUNXI_CFG_PIN_SHIFT(pin))
#define SUNXI_DRV_PIN_MASK(pin)          (GENMASK(1, 0) << SUNXI_DRV_PIN_SHIFT(pin))
#define SUNXI_PULL_PIN_MASK(pin)         (GENMASK(1, 0) << SUNXI_PULL_PIN_SHIFT(pin))

static uintptr_t sunxi_pio_base_virt;
static struct k_spinlock pinctrl_lock;

static void sunxi_pinctrl_set_mux(uintptr_t port_base, uint8_t pin, uint8_t mux)
{
	uint32_t val;
	uint32_t reg_addr = port_base + SUNXI_CFG_OFFSET(pin);
	k_spinlock_key_t key;

	key = k_spin_lock(&pinctrl_lock);
	val = sys_read32(reg_addr);
	val &= ~SUNXI_CFG_PIN_MASK(pin);
	val |= (mux << SUNXI_CFG_PIN_SHIFT(pin));
	sys_write32(val, reg_addr);
	k_spin_unlock(&pinctrl_lock, key);
}

static void sunxi_pinctrl_set_drive(uintptr_t port_base, uint8_t pin, uint8_t drv)
{
	uint32_t val;
	uint32_t reg_addr = port_base + SUNXI_DRV_OFFSET_BY_PIN(pin);
	k_spinlock_key_t key;

	key = k_spin_lock(&pinctrl_lock);
	val = sys_read32(reg_addr);
	val &= ~SUNXI_DRV_PIN_MASK(pin);
	val |= (drv << SUNXI_DRV_PIN_SHIFT(pin));
	sys_write32(val, reg_addr);
	k_spin_unlock(&pinctrl_lock, key);
}

static void sunxi_pinctrl_set_pull(uintptr_t port_base, uint8_t pin, uint8_t pull)
{
	uint32_t val;
	uint32_t reg_addr = port_base + SUNXI_PULL_OFFSET_BY_PIN(pin);
	k_spinlock_key_t key;

	key = k_spin_lock(&pinctrl_lock);
	val = sys_read32(reg_addr);
	val &= ~SUNXI_PULL_PIN_MASK(pin);
	val |= (pull << SUNXI_PULL_PIN_SHIFT(pin));
	sys_write32(val, reg_addr);
	k_spin_unlock(&pinctrl_lock, key);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		pinctrl_soc_pin_t pin_info = pins[i];

		uint8_t bank = SUNXI_GET_BANK(pin_info);
		uint8_t pin  = SUNXI_GET_PIN(pin_info);
		uint8_t mux  = SUNXI_GET_MUX(pin_info);
		uint8_t pull = SUNXI_GET_PULL(pin_info);
		uint8_t drv  = SUNXI_GET_DRV(pin_info);

		uintptr_t port_base = SUNXI_PIO_BASE + (bank * SUNXI_BANK_SIZE);

		sunxi_pinctrl_set_mux(port_base, pin, mux);
		if (drv != SUNXI_DRV_KEEP) {
			sunxi_pinctrl_set_drive(port_base, pin, drv);
		}

		if (pull != SUNXI_PULL_KEEP) {
			sunxi_pinctrl_set_pull(port_base, pin, pull);
		}
	}

	return 0;
}

static int sunxi_pinctrl_init(void)
{
	device_map(&sunxi_pio_base_virt, SUNXI_PIO_BASE_PHYS,
		   DT_REG_SIZE(DT_NODELABEL(pio)), K_MEM_CACHE_NONE);
	return 0;
}
SYS_INIT(sunxi_pinctrl_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
