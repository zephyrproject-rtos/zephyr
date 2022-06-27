/*
 * Copyright (c) 2022, Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <soc_gpio.h>

/** Utility macro that expands to the GPIO port address if it exists */
#define SAM_PORT_ADDR_OR_NONE(nodelabel)					\
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),			\
		   (DT_REG_ADDR(DT_NODELABEL(nodelabel)),))

/** Utility macro that expands to the GPIO Peripheral ID if it exists */
#define SAM_PORT_PERIPH_ID_OR_NONE(nodelabel)					\
	IF_ENABLED(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),			\
		   (DT_PROP(DT_NODELABEL(nodelabel), peripheral_id),))

/** SAM port addresses */
static const uint32_t sam_port_addrs[] = {
#ifdef ID_GPIO
	SAM_PORT_ADDR_OR_NONE(gpioa)
	SAM_PORT_ADDR_OR_NONE(gpiob)
	SAM_PORT_ADDR_OR_NONE(gpioc)
#else
	SAM_PORT_ADDR_OR_NONE(pioa)
	SAM_PORT_ADDR_OR_NONE(piob)
	SAM_PORT_ADDR_OR_NONE(pioc)
	SAM_PORT_ADDR_OR_NONE(piod)
	SAM_PORT_ADDR_OR_NONE(pioe)
	SAM_PORT_ADDR_OR_NONE(piof)
#endif
};

/** SAM port peripheral id */
static const uint32_t sam_port_periph_id[] = {
#ifdef ID_GPIO
	SAM_PORT_PERIPH_ID_OR_NONE(gpioa)
	SAM_PORT_PERIPH_ID_OR_NONE(gpiob)
	SAM_PORT_PERIPH_ID_OR_NONE(gpioc)
#else
	SAM_PORT_PERIPH_ID_OR_NONE(pioa)
	SAM_PORT_PERIPH_ID_OR_NONE(piob)
	SAM_PORT_PERIPH_ID_OR_NONE(pioc)
	SAM_PORT_PERIPH_ID_OR_NONE(piod)
	SAM_PORT_PERIPH_ID_OR_NONE(pioe)
	SAM_PORT_PERIPH_ID_OR_NONE(piof)
#endif
};

static void pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	struct soc_gpio_pin soc_pin;
	uint8_t  port_idx, port_func;

	port_idx = SAM_PINMUX_PORT_GET(pin);
	__ASSERT_NO_MSG(port_idx < ARRAY_SIZE(sam_port_addrs));
	port_func = SAM_PINMUX_FUNC_GET(pin);

#ifdef ID_GPIO
	soc_pin.regs = (Gpio *) sam_port_addrs[port_idx];
#else
	soc_pin.regs = (Pio *) sam_port_addrs[port_idx];
#endif
	soc_pin.periph_id = sam_port_periph_id[port_idx];
	soc_pin.mask = 1 << SAM_PINMUX_PIN_GET(pin);
	soc_pin.flags = SAM_PINCTRL_FLAGS_GET(pin) << SOC_GPIO_FLAGS_POS;

	if (port_func == SAM_PINMUX_FUNC_periph) {
		soc_pin.flags |= (SAM_PINMUX_PERIPH_GET(pin)
				  << SOC_GPIO_FUNC_POS);
	}

	soc_gpio_configure(&soc_pin);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(*pins++);
	}

	return 0;
}
