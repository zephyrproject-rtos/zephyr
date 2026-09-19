/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2711_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/bcm2711-pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

/* BCM2711 PINCTRL Base Address */
#define BCM2711_PINCTRL_BASE_ADDR DT_REG_ADDR(DT_DRV_INST(0))
#define BCM2711_NUM_GPIO          DT_INST_PROP(0, ngpios)

/* Function Select Registers (3 bits per pin, 10 pins per register) */
#define GPFSEL_OFFSET(pin) (((pin) / 10) * 4)
#define GPFSEL_SHIFT(pin)  (((pin) % 10) * 3)
#define GPFSEL_MASK        0x7

/* Pull-up/down Control Registers (2 bits per pin, 16 pins per register) */
#define GPIO_PUP_PDN_OFFSET(pin) (0xE4 + ((pin) / 16) * 4)
#define GPIO_PUP_PDN_SHIFT(pin)  (((pin) % 16) * 2)
#define GPIO_PUP_PDN_MASK        0x3

static uintptr_t bcm2711_pinctrl_base;

static inline uint32_t bcm2711_pinctrl_read(uintptr_t base, uint32_t offset)
{
	return sys_read32(base + offset);
}

static inline void bcm2711_pinctrl_write(uintptr_t base, uint32_t offset, uint32_t val)
{
	sys_write32(val, base + offset);
}

static void bcm2711_pinctrl_set_func(uintptr_t base, uint8_t pin, uint8_t func)
{
	uint32_t offset = GPFSEL_OFFSET(pin);
	uint32_t shift = GPFSEL_SHIFT(pin);
	uint32_t reg_val;

	reg_val = bcm2711_pinctrl_read(base, offset);
	reg_val &= ~(GPFSEL_MASK << shift);
	reg_val |= (func & GPFSEL_MASK) << shift;
	bcm2711_pinctrl_write(base, offset, reg_val);
}

static void bcm2711_pinctrl_set_pull(uintptr_t base, uint8_t pin, uint8_t pull)
{
	uint32_t offset = GPIO_PUP_PDN_OFFSET(pin);
	uint32_t shift = GPIO_PUP_PDN_SHIFT(pin);
	uint32_t reg_val;

	reg_val = bcm2711_pinctrl_read(base, offset);
	reg_val &= ~(GPIO_PUP_PDN_MASK << shift);
	reg_val |= (pull & GPIO_PUP_PDN_MASK) << shift;
	bcm2711_pinctrl_write(base, offset, reg_val);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	if (!pins || pin_cnt == 0) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < pin_cnt; i++) {
		uint8_t pin = pins[i].pin;

		if (pin >= BCM2711_NUM_GPIO) {
			return -EINVAL;
		}

		bcm2711_pinctrl_set_func(bcm2711_pinctrl_base, pin, pins[i].func);
		bcm2711_pinctrl_set_pull(bcm2711_pinctrl_base, pin, pins[i].pull);
	}

	return 0;
}

/**
 * @brief Map the pinctrl MMIO region into the virtual address space.
 *
 * device_map() hands out a new virtual mapping on every call and nothing
 * unmaps the pinctrl window, so mapping it from pinctrl_configure_pins()
 * leaks virtual address space on every runtime pin reconfiguration.
 *
 * @return 0 on success
 */
static int bcm2711_pinctrl_map_mmio(void)
{
	device_map(&bcm2711_pinctrl_base, BCM2711_PINCTRL_BASE_ADDR,
		   DT_REG_SIZE(DT_DRV_INST(0)), K_MEM_CACHE_NONE);
	return 0;
}

/*
 * Use the lowest init priority so that the pinctrl region is mapped before
 * any device that calls pinctrl_configure_pins() in its own PRE_KERNEL_1 init.
 */
SYS_INIT(bcm2711_pinctrl_map_mmio, PRE_KERNEL_1, 0);
