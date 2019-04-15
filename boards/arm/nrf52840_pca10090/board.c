/*
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <gpio.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(board_control, CONFIG_BOARD_PCA10090_LOG_LEVEL);

/* The following pins on the nRF52840 control the routing of certain
 * components/lines on the nRF9160 DK. They are specified as follows:
 *
 * COMPONENT_SWITCH : ROUTING PIN
 *
 * NOTE: UART1_VCOM_U7 is on pin 12 of both P0 and P1.
 * Both P0.12 -and- P1.12 need to be toggled to route UART1 to VCOM2.
 */

/* GPIO pins on Port 0 */
#define INTERFACE0_U5 13 /* MCU interface pins 0 - 2 */
#define INTERFACE1_U6 24 /* MCU interface pins 3 - 5 */
#define UART1_VCOM_U7 12 /* Route nRF9160 UART1 to VCOM2 */
#define BUTTON1_U12 6
#define BUTTON2_U12 26
#define SWITCH2_U9 8

/* GPIO pins on Port 1 */
#define INTERFACE2_U21 10 /* COEX interface pins 6 - 8 */
#define UART0_VCOM_U14 14 /* Route nRF9160 UART0 to VCOM0 */
#define UART1_VCOM_U7 12  /* Route nRF9160 UART1 to VCOM2 */
#define LED1_U8 5
#define LED2_U8 7
#define LED3_U11 1
#define LED4_U11 3
#define SWITCH1_U9 9

/* MCU interface pins
 * These pins can be used for inter-SoC communication.
 *
 * | nRF9160 |				 | nRF52840 | nRF9160 DK |
 * | P0.17   | -- MCU Interface Pin 0 -- | P0.17    | Arduino 4  |
 * | P0.18   | -- MCU Interface Pin 1 -- | P0.20    | Arduino 5  |
 * | P0.19   | -- MCU Interface Pin 2 -- | P0.15    | Arduino 6  |
 * | P0.21   | -- MCU Interface Pin 3 -- | P0.22    | TRACECLK   |
 * | P0.22   | -- MCU Interface Pin 4 -- | P1.04    | TRACEDATA0 |
 * | P0.23   | -- MCU Interface Pin 5 -- | P1.02    | TRACEDATA1 |
 * | COEX0   | -- MCU Interface Pin 6 -- | P1.13    | COEX0_PH   |
 * | COEX1   | -- MCU Interface Pin 7 -- | P1.11    | COEX1_PH   |
 * | COEX2   | -- MCU Interface Pin 8 -- | P1.15    | COEX2_PH   |
 */

__packed struct pin_config {
	u8_t pin;
	u8_t val;
};

/* The following tables specify the default configuration of each pin.
 * The switches have active-low logic, so when a pin is set to 1 in
 * these tables, pull it low to leave it set to its default.
 * When it is set to 0, pull it high to activate the non-default setting.
 */

static const struct pin_config pins_on_p0[] = {
	{ INTERFACE0_U5, IS_ENABLED(CONFIG_BOARD_PCA10090_INTERFACE0_ARDUINO) },
	{ INTERFACE1_U6, IS_ENABLED(CONFIG_BOARD_PCA10090_INTERFACE1_TRACE) },
	{ UART1_VCOM_U7, IS_ENABLED(CONFIG_BOARD_PCA10090_UART1_ARDUINO) },
	{ BUTTON1_U12,   IS_ENABLED(CONFIG_BOARD_PCA10090_BUTTON0_PHY) },
	{ BUTTON2_U12,   IS_ENABLED(CONFIG_BOARD_PCA10090_BUTTON1_PHY) },
	{ SWITCH2_U9,    IS_ENABLED(CONFIG_BOARD_PCA10090_SWITCH1_PHY) },
};

static const struct pin_config pins_on_p1[] = {
	{ INTERFACE2_U21, IS_ENABLED(CONFIG_BOARD_PCA10090_INTERFACE2_COEX) },
	{ UART0_VCOM_U14, IS_ENABLED(CONFIG_BOARD_PCA10090_UART0_VCOM) },
	{ UART1_VCOM_U7,  IS_ENABLED(CONFIG_BOARD_PCA10090_UART1_ARDUINO) },
	{ LED1_U8,        IS_ENABLED(CONFIG_BOARD_PCA10090_LED0_PHY) },
	{ LED2_U8,        IS_ENABLED(CONFIG_BOARD_PCA10090_LED1_PHY) },
	{ LED3_U11,       IS_ENABLED(CONFIG_BOARD_PCA10090_LED2_PHY) },
	{ LED4_U11,       IS_ENABLED(CONFIG_BOARD_PCA10090_LED3_PHY) },
	{ SWITCH1_U9,     IS_ENABLED(CONFIG_BOARD_PCA10090_SWITCH0_PHY) },
};

static void config_print(void)
{
	/* Interface pins 0-2 */
	LOG_INF("Routing interface pins 0-2 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_INTERFACE0_MCU) ?
			"nRF52840" :
			"Arduino headers",
		IS_ENABLED(CONFIG_BOARD_PCA10090_INTERFACE0_MCU));

	/* Interface pins 3-5 */
	LOG_INF("Routing interface pins 3-5 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_INTERFACE1_MCU) ?
			"nRF52840" :
			"TRACE header",
		IS_ENABLED(CONFIG_BOARD_PCA10090_INTERFACE1_MCU));

	/* Interface pins 6-8 */
	LOG_INF("Routing interface pins 6-8 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_INTERFACE2_MCU) ?
			"nRF52840" :
			"COEX header",
		IS_ENABLED(CONFIG_BOARD_PCA10090_INTERFACE2_MCU));

	LOG_INF("Routing nRF9160 UART0 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_UART0_ARDUINO) ?
			"Arduino pin headers" :
			"VCOM0",
		IS_ENABLED(CONFIG_BOARD_PCA10090_UART0_ARDUINO));

	LOG_INF("Routing nRF9160 UART1 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_UART1_ARDUINO) ?
			"Arduino pin headers" :
			"VCOM2",
		/* defaults to arduino pins */
		IS_ENABLED(CONFIG_BOARD_PCA10090_UART1_VCOM));

	LOG_INF("Routing nRF9160 LED 1 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_LED0_ARDUINO) ?
			"Arduino pin headers" :
			"physical LED",
		IS_ENABLED(CONFIG_BOARD_PCA10090_LED0_ARDUINO));

	LOG_INF("Routing nRF9160 LED 2 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_LED1_ARDUINO) ?
			"Arduino pin headers" :
			"physical LED",
		IS_ENABLED(CONFIG_BOARD_PCA10090_LED1_ARDUINO));

	LOG_INF("Routing nRF9160 LED 3 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_LED2_ARDUINO) ?
			"Arduino pin headers" :
			"physical LED",
		IS_ENABLED(CONFIG_BOARD_PCA10090_LED2_ARDUINO));

	LOG_INF("Routing nRF9160 LED 4 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_LED3_ARDUINO) ?
			"Arduino pin headers" :
			"physical LED",
		IS_ENABLED(CONFIG_BOARD_PCA10090_LED3_ARDUINO));

	LOG_INF("Routing nRF9160 button 1 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_BUTTON0_ARDUINO) ?
			"Arduino pin headers" :
			"physical button",
		IS_ENABLED(CONFIG_BOARD_PCA10090_BUTTON0_ARDUINO));

	LOG_INF("Routing nRF9160 button 2 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_BUTTON1_ARDUINO) ?
			"Arduino pin headers" :
			"physical button",
		IS_ENABLED(CONFIG_BOARD_PCA10090_BUTTON1_ARDUINO));

	LOG_INF("Routing nRF9160 switch 1 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_SWITCH0_ARDUINO) ?
			"Arduino pin headers" :
			"physical switch",
		IS_ENABLED(CONFIG_BOARD_PCA10090_SWITCH0_ARDUINO));

	LOG_INF("Routing nRF9160 switch 2 to %s (pin -> %d)",
		IS_ENABLED(CONFIG_BOARD_PCA10090_SWITCH1_ARDUINO) ?
			"Arduino pin headers" :
			"physical switch",
		IS_ENABLED(CONFIG_BOARD_PCA10090_SWITCH1_ARDUINO));
}

static int configure_pins(struct device *port, const struct pin_config cfg[],
			  size_t size)
{
	int err;

	for (size_t i = 0; i < size; i++) {
		err = gpio_pin_configure(port, cfg[i].pin, GPIO_DIR_OUT);
		if (err) {
			return cfg[i].pin;
		}

		/* The tables specify the default configuration of each pin.
		 * The switches have active-low logic, so when a pin is
		 * set to 1 in these tables, we pull it to low to leave
		 * it set to its default. If it's set to 0, we to set it
		 * to high to activate the non-default setting.
		 */
		err = gpio_pin_write(port, cfg[i].pin, !cfg[i].val);
		if (err) {
			return cfg[i].pin;
		}

		LOG_DBG("port %p, pin %u -> %u",
			port, cfg[i].pin, !cfg[i].val);
	}

	return 0;
}

static int init(struct device *dev)
{
	int rc;
	struct device *p0;
	struct device *p1;

	p0 = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!p0) {
		LOG_ERR("GPIO device " DT_GPIO_P0_DEV_NAME "not found!");
		return -EIO;
	}

	p1 = device_get_binding(DT_GPIO_P1_DEV_NAME);
	if (!p1) {
		LOG_ERR("GPIO device " DT_GPIO_P1_DEV_NAME " not found!");
		return -EIO;
	}

	rc = configure_pins(p0, pins_on_p0, ARRAY_SIZE(pins_on_p0));
	if (rc) {
		LOG_ERR("Error while configuring pin P0.%02d", rc);
		return -EIO;
	}

	rc = configure_pins(p1, pins_on_p1, ARRAY_SIZE(pins_on_p1));
	if (rc) {
		LOG_ERR("Error while configuring pin P1.%02d", rc);
		return -EIO;
	}

	config_print();

	LOG_INF("Board configured.");

	return 0;
}

SYS_INIT(init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
