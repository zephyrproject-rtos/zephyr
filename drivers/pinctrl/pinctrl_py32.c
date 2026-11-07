/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/py32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <py32_gpio.h>

struct py32_pin_port {
	GPIO_TypeDef *gpio;
	struct py32_pclken pclken;
};

#define PY32_PIN_PORT(node_id)                                                                     \
	[(DT_REG_ADDR(node_id) - DT_REG_ADDR(DT_NODELABEL(pinctrl))) / 0x400U] = {                 \
		.gpio = (GPIO_TypeDef *)DT_REG_ADDR(node_id),                                      \
		.pclken = {DT_CLOCKS_CELL(node_id, bus), DT_CLOCKS_CELL(node_id, bits)},           \
	},

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	static const struct py32_pin_port ports[] = {
		DT_FOREACH_STATUS_OKAY(puya_py32_gpio, PY32_PIN_PORT)};
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(rcc));

	ARG_UNUSED(reg);
	if (!device_is_ready(clk_dev)) {
		return -ENODEV;
	}
	for (uint8_t i = 0U; i < pin_cnt; i++) {
		const pinctrl_soc_pin_t *cfg = &pins[i];
		uint32_t port = (cfg->pinmux >> PY32_PORT_SHIFT) & PY32_PORT_MASK;
		uint32_t pin = (cfg->pinmux >> PY32_LINE_SHIFT) & PY32_LINE_MASK;
		uint32_t func = (cfg->pinmux >> PY32_MODE_SHIFT) & PY32_MODE_MASK;
		uint32_t mode;
		int ret;

		if ((port >= ARRAY_SIZE(ports)) || (ports[port].gpio == NULL)) {
			return -EINVAL;
		}
		ret = clock_control_on(clk_dev, (clock_control_subsys_t)&ports[port].pclken);
		if (ret < 0) {
			return ret;
		}
		if (func == PY32_ANALOG) {
			mode = LL_GPIO_MODE_ANALOG;
		} else if (func == PY32_GPIO) {
			mode = (cfg->output_low || cfg->output_high) ? LL_GPIO_MODE_OUTPUT
								     : LL_GPIO_MODE_INPUT;
		} else {
			mode = LL_GPIO_MODE_ALTERNATE;
		}
		py32_gpio_configure(ports[port].gpio, pin, mode, cfg->pull, cfg->speed,
				    cfg->open_drain, func & 0xfU, cfg->output_low,
				    cfg->output_high);
	}
	return 0;
}
