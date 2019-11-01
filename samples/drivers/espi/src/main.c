/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <soc.h>
#include <gpio.h>
#include <drivers/espi.h>

#ifdef CONFIG_ESPI_GPIO_DEV_NEEDED
static struct device *gpio_dev0;
static struct device *gpio_dev1;
#define PWR_SEQ_TIMEOUT    3000
#endif

static struct device *espi_dev;

static struct espi_callback espi_bus_cb;
static struct espi_callback vw_rdy_cb;
static struct espi_callback vw_cb;
static struct espi_callback p80_cb;

/* eSPI bus event handler */
static void espi_reset_handler(struct device *dev,
			       struct espi_callback *cb,
			       struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_RESET) {
		printk("\neSPI BUS reset %d", event.evt_data);
	}
}

/* eSPI logical channels enable/disable event handler */
static void espi_ch_handler(struct device *dev, struct espi_callback *cb,
			    struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_CHANNEL_READY) {
		if (event.evt_details == ESPI_CHANNEL_VWIRE) {
			printk("\nVW channel is ready\n");
		}
	}
}

/* eSPI vwire received event handler */
static void vwire_handler(struct device *dev, struct espi_callback *cb,
			  struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_VWIRE_RECEIVED) {
		if (event.evt_details == ESPI_VWIRE_SIGNAL_PLTRST) {
			printk("\nPLT_RST changed %d\n", event.evt_data);
		}
	}
}

/* eSPI peripheral channel notifications handler */
static void periph_handler(struct device *dev, struct espi_callback *cb,
			   struct espi_event event)
{
	u8_t peripheral;

	if (event.evt_type == ESPI_BUS_PERIPHERAL_NOTIFICATION) {
		peripheral = event.evt_details & 0x00FF;

		switch (peripheral) {
		case ESPI_PERIPHERAL_DEBUG_PORT80:
			printk("Postcode %x\n", event.evt_data);
			break;
		case ESPI_PERIPHERAL_HOST_IO:
			printk("ACPI %x\n", event.evt_data);
			break;
		default:
			printk("\n%s periph 0x%x [%x]\n", __func__, peripheral,
			       event.evt_data);
		}
	}
}

int espi_init(void)
{
	int ret;
	/* Indicate to eSPI master simplest configuration: Single line,
	 * 20MHz frequency and only logical channel 0 and 1 are supported
	 */
	struct espi_cfg cfg = {
		ESPI_IO_MODE_SINGLE_LINE,
		ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_PERIPHERAL,
		20,
	};

	ret = espi_config(espi_dev, &cfg);
	if (ret) {
		printk("Failed to configure eSPI slave! error (%d)\n", ret);
	} else {
		printk("eSPI slave configured successfully!\n");
	}

	printk("eSPI test - callbacks initialization... ");
	espi_init_callback(&espi_bus_cb, espi_reset_handler, ESPI_BUS_RESET);
	espi_init_callback(&vw_rdy_cb, espi_ch_handler,
			   ESPI_BUS_EVENT_CHANNEL_READY);
	espi_init_callback(&vw_cb, vwire_handler,
			   ESPI_BUS_EVENT_VWIRE_RECEIVED);
	espi_init_callback(&p80_cb, periph_handler,
			   ESPI_BUS_PERIPHERAL_NOTIFICATION);
	printk("complete\n");

	printk("eSPI test - callbacks registration... ");
	espi_add_callback(espi_dev, &espi_bus_cb);
	espi_add_callback(espi_dev, &vw_rdy_cb);
	espi_add_callback(espi_dev, &vw_cb);
	espi_add_callback(espi_dev, &p80_cb);
	printk("complete\n");

	return ret;
}

static int wait_for_pin(struct device *dev, u8_t pin, u16_t timeout,
			u32_t exp_level)
{
	int ret;
	u16_t loop_cnt = timeout;
	u32_t level;

	do {
		ret = gpio_pin_read(dev, pin, &level);
		if (ret) {
			printk("Failed to read %x %d\n", pin, ret);
			return -EIO;
		}

		if (exp_level == level) {
			printk("PIN %x = %x\n", pin, exp_level);
			break;
		}

		k_busy_wait(100);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		printk("PWRT! for %x %x\n", pin, level);
		return -ETIMEDOUT;
	}

	return 0;
}

int wait_for_vwire(struct device *espi_dev, enum espi_vwire_signal signal,
		   u16_t timeout, u8_t exp_level)
{
	int ret;
	u8_t level;
	u16_t loop_cnt = timeout;

	do {
		ret = espi_receive_vwire(espi_dev, signal, &level);
		if (ret) {
			printk("Failed to read %x %d", signal, ret);
			return -EIO;
		}

		if (exp_level == level) {
			break;
		}

		k_busy_wait(50);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		printk("VWIRE %d! is %x\n", signal, level);
		return -ETIMEDOUT;
	}

	return 0;
}

int espi_handshake(void)
{
	int ret;

	printk("eSPI test - Handshake with eSPI master...\n");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_WARN,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		printk("SUS_WARN Timeout!\n");
		return ret;
	}

	printk("\t1st phase completed\n");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S5,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		printk("SLP_S5 Timeout!\n");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S4,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		printk("SLP_S4 Timeout!\n");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S3,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		printk("SLP_S3 Timeout!\n");
		return ret;
	}

	printk("\t2nd phase completed\n");
	return 0;
}

void main(void)
{
	int ret;

	k_sleep(K_MSEC(500));

#ifdef CONFIG_ESPI_GPIO_DEV_NEEDED
	gpio_dev0 = device_get_binding(CONFIG_ESPI_GPIO_DEV0);
	if (gpio_dev0) {
		printk("%s FOUND!\n", CONFIG_ESPI_GPIO_DEV0);
	}

	gpio_dev1 = device_get_binding(CONFIG_ESPI_GPIO_DEV1);
	if (gpio_dev1) {
		printk("%s FOUND!\n", CONFIG_ESPI_GPIO_DEV1);
	}

#endif
	espi_dev = device_get_binding(CONFIG_ESPI_DEV);
	if (espi_dev) {
		printk("%s FOUND!\n", CONFIG_ESPI_DEV);
	}

	printk("Hello eSPI test! %s\n", CONFIG_BOARD);

#ifdef CONFIG_ESPI_GPIO_DEV_NEEDED
	ret = gpio_pin_configure(gpio_dev0, CONFIG_PWRGD_PIN, GPIO_DIR_IN);
	if (ret) {
		printk("Unable to configure PWRGD %d\n", CONFIG_PWRGD_PIN);
		return;
	}

	ret = gpio_pin_configure(gpio_dev1, CONFIG_ESPI_INIT_PIN, GPIO_DIR_OUT);
	if (ret) {
		printk("Unable to configure RSMRST %d\n", CONFIG_ESPI_INIT_PIN);
		return;
	}

	ret = gpio_pin_write(gpio_dev1, CONFIG_ESPI_INIT_PIN, 0);
	if (ret) {
		printk("Unable to initialize %d\n", CONFIG_ESPI_INIT_PIN);
		return;
	}
#endif

	espi_init();

#ifdef CONFIG_ESPI_GPIO_DEV_NEEDED
	ret = wait_for_pin(gpio_dev0, CONFIG_PWRGD_PIN, PWR_SEQ_TIMEOUT, 1);
	if (ret) {
		printk("RSMRST_PWRGD timeout!\n");
		return;
	}

	ret = gpio_pin_write(gpio_dev1, CONFIG_ESPI_INIT_PIN, 1);
	if (ret) {
		printk("Failed to write %x %d\n", CONFIG_ESPI_INIT_PIN, ret);
		return;
	}
#endif

	ret = espi_handshake();
	if (ret) {
		printk("Test failed %d\n", ret);
	}
}
