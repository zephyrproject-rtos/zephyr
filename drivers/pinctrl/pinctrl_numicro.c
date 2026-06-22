/*
 * Copyright (c) 2022 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numicro_pinctrl

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/numicro-pinctrl.h>
#include <NuMicro.h>

#define MODE_PIN_SHIFT(pin)	((pin) * 2)
#define MODE_MASK(pin)		(3 << MODE_PIN_SHIFT(pin))
#define DINOFF_PIN_SHIFT(pin)	((pin) + 16)
#define DINOFF_MASK(pin)	(1 << DINOFF_PIN_SHIFT(pin))
#define PUSEL_PIN_SHIFT(pin)	((pin) * 2)
#define PUSEL_MASK(pin)		(3 << PUSEL_PIN_SHIFT(pin))
#define SLEWCTL_PIN_SHIFT(pin)	((pin) * 2)
#define SLEWCTL_MASK(pin)	(3 << SLEWCTL_PIN_SHIFT(pin))

#define PORT_PIN_MASK		0xFFFF

#define REG_MFP(port, pin) (*(volatile uint32_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, mfp) + \
						   ((port) * 8) + \
						   ((pin) > 7 ? 4 : 0)))

#define REG_MFOS(port) (*(volatile uint32_t *)((uint32_t)DT_INST_REG_ADDR_BY_NAME(0, mfos) + \
					       ((port) * 4)))

#define MFP_CTL(pin, mfp) ((mfp) << (((pin) % 8) * 4))

/** Utility macro that expands to the GPIO port address if it exists */
#define NUMICRO_PORT_ADDR_OR_NONE(nodelabel)			\
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),	\
		   (DT_REG_ADDR(DT_NODELABEL(nodelabel)),))

/** Port addresses */
static const uint32_t gpio_port_addrs[] = {
	NUMICRO_PORT_ADDR_OR_NONE(gpioa)
	NUMICRO_PORT_ADDR_OR_NONE(gpiob)
	NUMICRO_PORT_ADDR_OR_NONE(gpioc)
	NUMICRO_PORT_ADDR_OR_NONE(gpiod)
	NUMICRO_PORT_ADDR_OR_NONE(gpioe)
	NUMICRO_PORT_ADDR_OR_NONE(gpiof)
	NUMICRO_PORT_ADDR_OR_NONE(gpiog)
	NUMICRO_PORT_ADDR_OR_NONE(gpioh)
};

static int gpio_configure(const pinctrl_soc_pin_t *pin)
{
	uint8_t port_idx, pin_idx;
	GPIO_T *port;
	uint32_t bias = GPIO_PUSEL_DISABLE;

	port_idx = NUMICRO_PORT(pin->pinmux);
	if (port_idx >= ARRAY_SIZE(gpio_port_addrs)) {
		return -EINVAL;
	}

	pin_idx = NUMICRO_PIN(pin->pinmux);

	port = (GPIO_T *)gpio_port_addrs[port_idx];

	if (pin->pull_up != 0) {
		bias = GPIO_PUSEL_PULL_UP;
	} else if (pin->pull_down != 0) {
		bias = GPIO_PUSEL_PULL_DOWN;
	}

	port->MODE = (port->MODE & ~MODE_MASK(pin_idx)) |
		     ((pin->open_drain ? 2 : 0) << MODE_PIN_SHIFT(pin_idx));
	port->DBEN = (port->DBEN & ~BIT(pin_idx)) |
		     ((pin->input_debounce ? 1 : 0) << pin_idx);
	port->SMTEN = (port->SMTEN & ~BIT(pin_idx)) |
		      ((pin->schmitt_trigger ? 1 : 0) << pin_idx);
	port->DINOFF = (port->SMTEN & ~DINOFF_MASK(pin_idx)) |
		       ((pin->input_disable ? 1 : 0) << DINOFF_PIN_SHIFT(pin_idx));
	port->PUSEL = (port->PUSEL & ~PUSEL_MASK(pin_idx)) |
		      (bias << PUSEL_PIN_SHIFT(pin_idx));
	port->SLEWCTL = (port->SLEWCTL & ~SLEWCTL_MASK(pin_idx)) |
			(pin->slew_rate << SLEWCTL_PIN_SHIFT(pin_idx));

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	int ret = 0;

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		uint32_t port = NUMICRO_PORT(pins[i].pinmux);
		uint32_t pin = NUMICRO_PIN(pins[i].pinmux);
		uint32_t mfp = NUMICRO_MFP(pins[i].pinmux);

		REG_MFP(port, pin) = (REG_MFP(port, pin) & ~MFP_CTL(pin, 0xf)) |
				     MFP_CTL(pin, mfp);

		if (pins[i].open_drain != 0) {
			REG_MFOS(port) |= BIT(pin);
		} else {
			REG_MFOS(port) &= ~BIT(pin);
		}

		ret = gpio_configure(&pins[i]);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}
