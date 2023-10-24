/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch5xx_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#define GET_GPIO_CONTROLLER(node_id, prop, idx)                                                    \
	DEVICE_DT_GET_OR_NULL(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

const struct device *gpio[] = {DT_INST_FOREACH_PROP_ELEM(0, gpio_controllers, GET_GPIO_CONTROLLER)};

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t regval;

	if (pin->port >= ARRAY_SIZE(gpio) || !device_is_ready(gpio[pin->port])) {
		return;
	}

	if (pin->remap_bit) {
		regval = sys_read8(CH32V_SYS_R8_PIN_ALTERNATE_REG);
		if (pin->remap_en) {
			regval |= BIT(pin->remap_bit);
		} else {
			regval &= ~BIT(pin->remap_bit);
		}
		sys_write8(regval, CH32V_SYS_R8_PIN_ALTERNATE_REG);
	}

	gpio_pin_configure(gpio[pin->port], pin->pin, pin->flags);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		pinctrl_configure_pin(&pins[i]);
	}

	return 0;
}
