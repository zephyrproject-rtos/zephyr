/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2021 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <string.h>
#include <soc.h>
#include <device.h>
#include <string.h>
#include <stdio.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/printk.h>

const struct device *gpio_bn, *gpio_led;

/* in msec */
#define WAIT_TIME 500
/* change the value of SLEEP_TIME to check the residency */
#define SLEEP_TIME 4000
/* setting to K_TICKS_FOREVER will activate the deepsleep mode */

#define DEMO_DESCRIPTION	\
	"Demo Description\n"	\
	"Application is demonstrating the PM on device when transferring on SPI\n"\
	"The system goes to sleep mode on k_sleep() and wakes-up\n" \
	"(led on) by the lptim timeout expiration\n" \
	"or when pressing the user push button (irq mode on EXTI)\n\n"

/* The devicetree node identifier for the "sw0" alias. */
#define SW0_NODE DT_ALIAS(sw0)
/* GPIO for the User push button */
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW	DT_GPIO_LABEL(SW0_NODE, gpios)
#define PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#if DT_PHA_HAS_CELL(SW0_NODE, gpios, flags)
#define SW0_FLAGS DT_GPIO_FLAGS(SW0_NODE, gpios)
#else
#define SW0_FLAGS 0
#endif
#else
/* A build error here means your board isn't set up to input a button. */
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
/* GPIO for the Led */
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN0	DT_GPIO_PIN(LED0_NODE, gpios)
#define LED0_FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)

#define SPI_DRV_NAME	CONFIG_SPI_LOOPBACK_DRV_NAME
#define SPI_SLAVE	CONFIG_SPI_LOOPBACK_SLAVE_NUMBER
#define SLOW_FREQ	CONFIG_SPI_LOOPBACK_SLOW_FREQ
#define FAST_FREQ	CONFIG_SPI_LOOPBACK_FAST_FREQ

#if defined(CONFIG_SPI_LOOPBACK_CS_GPIO)
#define CS_CTRL_GPIO_DRV_NAME CONFIG_SPI_LOOPBACK_CS_CTRL_GPIO_DRV_NAME
struct spi_cs_control spi_cs = {
	.gpio_pin = CONFIG_SPI_LOOPBACK_CS_CTRL_GPIO_PIN,
	.gpio_dt_flags = GPIO_ACTIVE_LOW,
	.delay = 0,
};
#define SPI_CS (&spi_cs)
#else
#define SPI_CS NULL
#define CS_CTRL_GPIO_DRV_NAME ""
#endif

static struct gpio_callback gpio_cb;
/* 2 spi config for testing */
struct spi_config spi_cfg_slow = {
	.frequency = SLOW_FREQ,
#if CONFIG_SPI_LOOPBACK_MODE_LOOP
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_LOOP |
#else
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
#endif
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = SPI_SLAVE,
	.cs = SPI_CS,
};

struct spi_config spi_cfg_fast = {
	.frequency = FAST_FREQ,
#if CONFIG_SPI_LOOPBACK_MODE_LOOP
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_LOOP |
#else
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
#endif
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = SPI_SLAVE,
	.cs = SPI_CS,
};

#define BUF_SIZE 36
uint8_t buffer_tx[] = "Thequickbrownfoxjumpsoverthelazydog\0";
uint8_t buffer_rx[BUF_SIZE] = {};

/* We need 5x(buffer size) + 1 to print a comma-separated list of each
 * byte in hex, plus a null.
 */
uint8_t buffer_print_tx[BUF_SIZE * 5 + 1];
uint8_t buffer_print_rx[BUF_SIZE * 5 + 1];

static void to_display_format(const uint8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

#if defined(CONFIG_SPI_LOOPBACK_CS_GPIO)
static int cs_ctrl_gpio_config(void)
{
	spi_cs.gpio_dev = device_get_binding(CS_CTRL_GPIO_DRV_NAME);
	if (!spi_cs.gpio_dev) {
		printk("Cannot find %s!\n", CS_CTRL_GPIO_DRV_NAME);
		return -1;
	}

	return 0;
}
#endif /* CONFIG_SPI_LOOPBACK_CS_GPIO */

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);
	ARG_UNUSED(cb);
	/* indicates a signal wakeUp from pin */
	gpio_pin_set(gpio_led, PIN0, 1);
}

/* test transferring a buffer on the same dma channels */
static int spi_complete_loop(const struct device *dev,
			     struct spi_config *spi_conf)
{
	const struct spi_buf tx_bufs[] = {
		{
			.buf = buffer_tx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = buffer_rx,
			.len = BUF_SIZE,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs)
	};

	int ret;

	/* clean the Rx buffer before reception */
	(void)memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(dev, spi_conf, &tx, &rx);
	if (ret) {
	printk("Error starting complete spi loop (%d)\n", ret);
		return ret;
	}
	printk("Starting complete spi loop\n");

	if (memcmp(buffer_tx, buffer_rx, BUF_SIZE)) {
		to_display_format(buffer_tx, BUF_SIZE, buffer_print_tx);
		to_display_format(buffer_rx, BUF_SIZE, buffer_print_rx);
		printk("Buffer contents are different: %s\n",
			    buffer_print_tx);
		printk("                           vs: %s\n",
			    buffer_print_rx);
		return -1;
	}

	printk("Passed\n");

	return 0;
}

/* Application main Thread */
void main(void)
{
	const struct device *spi_slow;

	printk("\n\n*** Low power Management Demo on %s ***\n", CONFIG_BOARD);
	printk(DEMO_DESCRIPTION);

	gpio_bn = device_get_binding(SW);

	/* Configure Button 1 to wakeup from sleep mode */
	gpio_pin_configure(gpio_bn, PIN,
			SW0_FLAGS | GPIO_INPUT);
	gpio_pin_interrupt_configure(gpio_bn,
				   PIN,
				   GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&gpio_cb, button_pressed, BIT(PIN));
	gpio_add_callback(gpio_bn, &gpio_cb);

	/* Configure LEDs */
	gpio_led = device_get_binding(LED0);
	if (gpio_led == NULL) {
		return;
	}

	if (gpio_pin_configure(gpio_led, PIN0,
			GPIO_OUTPUT_ACTIVE | LED0_FLAGS) < 0) {
		return;
	}

#if defined(CONFIG_SPI_LOOPBACK_CS_GPIO)
	if (cs_ctrl_gpio_config()) {
		return;
	}
#endif /* CONFIG_SPI_LOOPBACK_CS_GPIO */

	spi_slow = device_get_binding(SPI_DRV_NAME);
	if (!spi_slow) {
		printk("Cannot find %s!\n", SPI_DRV_NAME);
		return;
	}

	/*
	 * Start the demo.
	 */
	while (1) {
		/* Set pin to HIGH/LOW */
		gpio_pin_set(gpio_led, PIN0, 1);
		/* transfer some data over spi with config slow */
		spi_complete_loop(spi_slow, &spi_cfg_slow);

		/* wait for while before going to low power mode */
		k_busy_wait(WAIT_TIME * 1000);
		printk(" sleeping...");

		gpio_pin_set(gpio_led, PIN0, 0);
		/* now going to low power mode */
		k_msleep(SLEEP_TIME);
		printk("...wakeUp\n");
	}
}
