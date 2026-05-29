/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/arch/arm/cortex_a_r/sys_io.h>
#include <zephyr/sys/barrier.h>

#define BCM2835_GPIO_BASE	0x20200000U
#define BCM2835_GPFSEL(pin)	(BCM2835_GPIO_BASE + (((pin) / 10U) * 4U))
#define BCM2835_GPPUD		(BCM2835_GPIO_BASE + 0x94U)
#define BCM2835_GPPUDCLK(pin)	(BCM2835_GPIO_BASE + 0x98U + ((((pin) / 32U)) * 4U))

#define BCM2835_GPIO_FSEL_MASK	0x7U
#define BCM2835_GPIO_PIN_MAX	54U

#define BCM2835_GPPUD_OFF	0U
#define BCM2835_GPPUD_PULLDOWN	1U
#define BCM2835_GPPUD_PULLUP	2U

static inline void bcm2835_pinctrl_delay(void)
{
	for (volatile int i = 0; i < 150; i++) {
		compiler_barrier();
	}
}

static void bcm2835_configure_function(const pinctrl_soc_pin_t *pin)
{
	uintptr_t reg = BCM2835_GPFSEL(pin->pin);
	uint32_t shift = (pin->pin % 10U) * 3U;
	uint32_t val = sys_read32(reg);

	val &= ~(BCM2835_GPIO_FSEL_MASK << shift);
	val |= (pin->function & BCM2835_GPIO_FSEL_MASK) << shift;
	sys_write32(val, reg);
}

static void bcm2835_configure_bias(const pinctrl_soc_pin_t *pin)
{
	uint32_t pud = BCM2835_GPPUD_OFF;
	uint32_t clk = BIT(pin->pin % 32U);

	if (!pin->bias_disable) {
		if (pin->bias_pull_up) {
			pud = BCM2835_GPPUD_PULLUP;
		} else if (pin->bias_pull_down) {
			pud = BCM2835_GPPUD_PULLDOWN;
		} else {
			pud = BCM2835_GPPUD_OFF;
		}
	}

	sys_write32(pud, BCM2835_GPPUD);
	bcm2835_pinctrl_delay();
	sys_write32(clk, BCM2835_GPPUDCLK(pin->pin));
	bcm2835_pinctrl_delay();
	sys_write32(BCM2835_GPPUD_OFF, BCM2835_GPPUD);
	sys_write32(0U, BCM2835_GPPUDCLK(pin->pin));
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		if (pins[i].pin >= BCM2835_GPIO_PIN_MAX) {
			return -EINVAL;
		}

		bcm2835_configure_function(&pins[i]);
		bcm2835_configure_bias(&pins[i]);
	}

	return 0;
}
