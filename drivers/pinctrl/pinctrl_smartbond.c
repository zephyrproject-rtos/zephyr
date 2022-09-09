/*
 * Copyright (c) 2022, Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <DA1469xAB.h>

/** Utility macro to retrieve starting mode register and pin count for GPIO port from DT */
#define GPIO_PORT_ENTRY(nodelabel)							\
		{ DT_REG_ADDR_BY_IDX(DT_NODELABEL(nodelabel), 1),			\
		  DT_PROP(DT_NODELABEL(nodelabel), ngpios) }

struct gpio_port {
	uint32_t p0_mode_addr;
	uint8_t pin_count;
};

static const struct gpio_port smartbond_gpio_ports[] = {
	GPIO_PORT_ENTRY(gpio0),
	GPIO_PORT_ENTRY(gpio1),
};

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	volatile uint32_t *reg;
	uint32_t reg_val;

	__ASSERT_NO_MSG(pin->port < ARRAY_SIZE(smartbond_gpio_ports));
	__ASSERT_NO_MSG(pin->pin < smartbond_gpio_ports[pin->port].pin_count);

	reg = (volatile uint32_t *)smartbond_gpio_ports[pin->port].p0_mode_addr;
	reg += pin->pin;

	reg_val = pin->func << GPIO_P0_00_MODE_REG_PID_Pos;
	if (pin->bias_pull_up) {
		reg_val |= 0x01 << GPIO_P0_00_MODE_REG_PUPD_Pos;
	} else if (pin->bias_pull_down) {
		reg_val |= 0x02 << GPIO_P0_00_MODE_REG_PUPD_Pos;
	}

	*reg = reg_val;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins++);
	}

	return 0;
}
