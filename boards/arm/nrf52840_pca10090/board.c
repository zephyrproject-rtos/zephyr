/*
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <drivers/gpio.h>
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

/* The following tables specify the configuration of each pin based on the
 * Kconfig options that drive it.
 * The switches have active-low logic, so when writing to the port we will
 * need to invert the value to match the IS_ENABLED() logic.
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

static int pins_configure(struct device *port, const struct pin_config cfg[],
			  size_t pins)
{
	int err;

	/* Write to the pins before configuring them as output,
	 * to make sure we are driving them to the correct level
	 * right after they are configured.
	 */
	for (size_t i = 0; i < pins; i++) {
		/* The swiches on the board are active low, so we need
		 * to negate the IS_ENABLED() value from the tables.
		 */
		err = gpio_pin_write(port, cfg[i].pin, !cfg[i].val);
		if (err) {
			return cfg[i].pin;
		}

		err = gpio_pin_configure(port, cfg[i].pin, GPIO_DIR_OUT);
		if (err) {
			return cfg[i].pin;
		}

		LOG_DBG("port %p, pin %u -> %u",
			port, cfg[i].pin, !cfg[i].val);
	}

	return 0;
}

static void chip_reset(struct device *gpio,
		       struct gpio_callback *cb, u32_t pins)
{
	const u32_t stamp = k_cycle_get_32();

	printk("GPIO reset line asserted, device reset.\n");
	printk("Bye @ cycle32 %u\n", stamp);

	NVIC_SystemReset();
}

static void reset_pin_wait_low(struct device *port, u32_t pin)
{
	int err;
	u32_t val;

	/* Wait until the pin is pulled low */
	do {
		err = gpio_pin_read(port, pin, &val);
	} while (err == 0 && val != 0);
}

static int reset_pin_configure(struct device *p0, struct device *p1)
{
	int err;
	u32_t pin = 0;
	struct device *port = NULL;

	static struct gpio_callback gpio_ctx;

	/* MCU interface pins 0-2 */
	if (IS_ENABLED(CONFIG_BOARD_PCA10090_NRF52840_RESET_P0_17)) {
		port = p0;
		pin = 17;
	}
	if (IS_ENABLED(CONFIG_BOARD_PCA10090_NRF52840_RESET_P0_20)) {
		port = p0;
		pin = 20;
	}
	if (IS_ENABLED(CONFIG_BOARD_PCA10090_NRF52840_RESET_P0_15)) {
		port = p0;
		pin = 15;
	}
	/* MCU interface pins 3-6 */
	if (IS_ENABLED(CONFIG_BOARD_PCA10090_NRF52840_RESET_P0_22)) {
		port = p0;
		pin = 22;
	}
	if (IS_ENABLED(CONFIG_BOARD_PCA10090_NRF52840_RESET_P1_04)) {
		port = p1;
		pin = 4;
	}
	if (IS_ENABLED(CONFIG_BOARD_PCA10090_NRF52840_RESET_P1_02)) {
		port = p1;
		pin = 2;
	}

	__ASSERT_NO_MSG(port != NULL);

	err = gpio_pin_configure(port, pin,
				 GPIO_DIR_IN | GPIO_INT | GPIO_PUD_PULL_DOWN |
				 GPIO_INT_ACTIVE_HIGH | GPIO_INT_EDGE);
	if (err) {
		return err;
	}

	gpio_init_callback(&gpio_ctx, chip_reset, BIT(pin));

	err = gpio_add_callback(port, &gpio_ctx);
	if (err) {
		return err;
	}

	err = gpio_pin_enable_callback(port, pin);
	if (err) {
		return err;
	}

	/* Wait until the pin is pulled low before continuing.
	 * This lets the other side ensure that they are ready.
	 */
	LOG_INF("GPIO reset line enabled on pin %s.%02u, holding..",
		port == p0 ? "P0" : "P1", pin);

	reset_pin_wait_low(port, pin);

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

	/* Configure pins on each port */
	rc = pins_configure(p0, pins_on_p0, ARRAY_SIZE(pins_on_p0));
	if (rc) {
		LOG_ERR("Error while configuring pin P0.%02d", rc);
		return -EIO;
	}
	rc = pins_configure(p1, pins_on_p1, ARRAY_SIZE(pins_on_p1));
	if (rc) {
		LOG_ERR("Error while configuring pin P1.%02d", rc);
		return -EIO;
	}

	config_print();

	/* Make sure to configure the switches before initializing
	 * the GPIO reset pin, so that we are connected to
	 * the nRF9160 before enabling our interrupt.
	 */
	if (IS_ENABLED(CONFIG_BOARD_PCA10090_NRF52840_RESET)) {
		rc = reset_pin_configure(p0, p1);
		if (rc) {
			LOG_ERR("Unable to configure reset pin, err %d", rc);
			return -EIO;
		}
	}

	LOG_INF("Board configured.");

	return 0;
}

SYS_INIT(init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
