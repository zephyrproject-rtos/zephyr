/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/at32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <at32_gpio.h>

/** Utility macro that expands to the GPIO port address if it exists */
#define AT32_PORT_ADDR_OR_NONE(nodelabel)				       \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),		       \
		   (DT_REG_ADDR(DT_NODELABEL(nodelabel)),), ())

/** Utility macro that expands to the GPIO clock id if it exists */
#define AT32_PORT_CLOCK_ID_OR_NONE(nodelabel)				       \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),		       \
		   (DT_CLOCKS_CELL(DT_NODELABEL(nodelabel), id),), ())
#define GPIO_PIN_OFFSET(n)     (1 << n)
/** GD32 port addresses */
static const uint32_t at32_port_addrs[] = {
	AT32_PORT_ADDR_OR_NONE(gpioa)
	AT32_PORT_ADDR_OR_NONE(gpiob)
	AT32_PORT_ADDR_OR_NONE(gpioc)
	AT32_PORT_ADDR_OR_NONE(gpiod)
	AT32_PORT_ADDR_OR_NONE(gpioe)
	AT32_PORT_ADDR_OR_NONE(gpiof)
	AT32_PORT_ADDR_OR_NONE(gpiog)
	AT32_PORT_ADDR_OR_NONE(gpioh)
	AT32_PORT_ADDR_OR_NONE(gpioi)
};

/** GD32 port clock identifiers */
static const uint16_t at32_port_clkids[] = {
	AT32_PORT_CLOCK_ID_OR_NONE(gpioa)
	AT32_PORT_CLOCK_ID_OR_NONE(gpiob)
	AT32_PORT_CLOCK_ID_OR_NONE(gpioc)
	AT32_PORT_CLOCK_ID_OR_NONE(gpiod)
	AT32_PORT_CLOCK_ID_OR_NONE(gpioe)
	AT32_PORT_CLOCK_ID_OR_NONE(gpiof)
	AT32_PORT_CLOCK_ID_OR_NONE(gpiog)
	AT32_PORT_CLOCK_ID_OR_NONE(gpioh)
	AT32_PORT_CLOCK_ID_OR_NONE(gpioi)
};

/**
 * @brief Configure a pin.
 *
 * @param pin The pin to configure.
 */
static void pinctrl_configure_pin(pinctrl_soc_pin_t pin)
{
	uint8_t port_idx;
	uint32_t port, pin_num, mux;
	uint16_t clkid;
	gpio_init_type init;
	gpio_type *gpio;
	uint32_t gpio_pins = 0;

	port_idx = AT32_PORT_GET(pin);
	__ASSERT_NO_MSG(port_idx < ARRAY_SIZE(at32_port_addrs));

	clkid = at32_port_clkids[port_idx];
	port = at32_port_addrs[port_idx];
	pin_num = AT32_PIN_GET(pin);
	gpio_pins = GPIO_PIN_OFFSET(pin_num);
	mux = AT32_MUX_GET(pin);

	gpio = (gpio_type *)port;

	init.gpio_pins = gpio_pins;
	if (mux != AT32_ANALOG) {
		init.gpio_mode = GPIO_MODE_MUX;
	} else {
		init.gpio_mode = GPIO_MODE_ANALOG;
	}

	init.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;

	(void)clock_control_on(AT32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t)&clkid);

	gpio_init(gpio, &init);
	gpio_pin_mux_config(gpio, pin_num, mux);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(pins[i]);
	}

	return 0;
}
