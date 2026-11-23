/*
 * Copyright (c) 2020 Jefferson Lee.
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>

/*
 * enable internal I2C pull-up and user led
 *
 * The associated GPIOs pins are defined in the board's devicetree by
 * the node "zephyr,user" and alias "led4".
 *
 * The original board.c used SYS_INIT() and the Zephyr's GPIO driver to
 * configure the pins to GPIO_OUTPUT_HIGH.
 *
 * We need to guarantee that the I2C pull-ups are enabled before any I2C
 * sensor drivers. This implies a board early init hook.
 * As such, we must use the nordic's nrfx HAL to configure the pins.
 *
 * The information needed is retrieved from the devicetree, to keep the
 * spirit of the former board init code and avoid information duplication.
 *
 */

/* helper to retrieve the GPIO port number given a node gpios property */
#define DT_GPIO_PORT(node, gpios) DT_PROP(DT_GPIO_CTLR(node, gpios), port)

/*  internal pull-up -> node:zephyr,user prop:pull-up-gpios */
#define PULLUP_NODE      DT_PATH(zephyr_user)
#define PULLUP_GPIO_PORT DT_GPIO_PORT(PULLUP_NODE, pull_up_gpios)
#define PULLUP_GPIO_PIN  DT_GPIO_PIN(PULLUP_NODE, pull_up_gpios)

/* user led -> alias:led4  prop: gpios */
#define USERLED_NODE      DT_ALIAS(led4)
#define USERLED_GPIO_PORT DT_GPIO_PORT(USERLED_NODE, gpios)
#define USERLED_GPIO_PIN  DT_GPIO_PIN(USERLED_NODE, gpios)

/* maps DT port/pin to absolute pin number as needed by nrfx HAL */
#define PULLUP_NRF_PIN  NRF_GPIO_PIN_MAP(PULLUP_GPIO_PORT, PULLUP_GPIO_PIN)
#define USERLED_NRF_PIN NRF_GPIO_PIN_MAP(USERLED_GPIO_PORT, USERLED_GPIO_PIN)

void board_early_init_hook(void)
{
	/* set pin high in the output latch first to prevent initial low glitch */
	nrf_gpio_pin_set(PULLUP_NRF_PIN);
	nrf_gpio_pin_set(USERLED_NRF_PIN);

	/* configure GPIO pins as output (high) */
	nrf_gpio_cfg_output(PULLUP_NRF_PIN);
	nrf_gpio_cfg_output(USERLED_NRF_PIN);
}
