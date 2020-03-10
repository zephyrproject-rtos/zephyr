/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <device.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <drivers/espi.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(espi, CONFIG_ESPI_LOG_LEVEL);

#define DEST_SLV_ADDR  0x02
#define SRC_SLV_ADDR   0x21
#define OOB_CMDCODE    0x01
#define MAX_RESP_SIZE  20

struct oob_header {
	u8_t dest_slave_addr;
	u8_t oob_cmd_code;
	u8_t byte_cnt;
	u8_t src_slave_addr;
};

struct oob_response {
	struct oob_header hdr;
	u8_t buf[MAX_RESP_SIZE];
};


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
		LOG_INF("\neSPI BUS reset %d", event.evt_data);
	}
}

/* eSPI logical channels enable/disable event handler */
static void espi_ch_handler(struct device *dev, struct espi_callback *cb,
			    struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_CHANNEL_READY) {
		if (event.evt_details == ESPI_CHANNEL_VWIRE) {
			LOG_INF("\nVW channel is ready");
		}
	}
}

/* eSPI vwire received event handler */
static void vwire_handler(struct device *dev, struct espi_callback *cb,
			  struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_VWIRE_RECEIVED) {
		if (event.evt_details == ESPI_VWIRE_SIGNAL_PLTRST) {
			LOG_INF("\nPLT_RST changed %d\n", event.evt_data);
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
			LOG_INF("Postcode %x\n", event.evt_data);
			break;
		case ESPI_PERIPHERAL_HOST_IO:
			LOG_INF("ACPI %x\n", event.evt_data);
			espi_remove_callback(espi_dev, &p80_cb);
			break;
		default:
			LOG_INF("\n%s periph 0x%x [%x]\n", __func__, peripheral,
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
		LOG_INF("Failed to configure eSPI slave! error (%d)\n", ret);
	} else {
		LOG_INF("eSPI slave configured successfully!");
	}

	LOG_INF("eSPI test - callbacks initialization... ");
	espi_init_callback(&espi_bus_cb, espi_reset_handler, ESPI_BUS_RESET);
	espi_init_callback(&vw_rdy_cb, espi_ch_handler,
			   ESPI_BUS_EVENT_CHANNEL_READY);
	espi_init_callback(&vw_cb, vwire_handler,
			   ESPI_BUS_EVENT_VWIRE_RECEIVED);
	espi_init_callback(&p80_cb, periph_handler,
			   ESPI_BUS_PERIPHERAL_NOTIFICATION);
	LOG_INF("complete");

	LOG_INF("eSPI test - callbacks registration... ");
	espi_add_callback(espi_dev, &espi_bus_cb);
	espi_add_callback(espi_dev, &vw_rdy_cb);
	espi_add_callback(espi_dev, &vw_cb);
	espi_add_callback(espi_dev, &p80_cb);
	LOG_INF("complete");

	return ret;
}

static int wait_for_pin(struct device *dev, u8_t pin, u16_t timeout,
			u32_t exp_level)
{
	int level;
	u16_t loop_cnt = timeout;

	do {
		level = gpio_pin_get(dev, pin);
		if (level < 0) {
			printk("Failed to read %x %d\n", pin, level);
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
			LOG_WRN("Failed to read %x %d", signal, ret);
			return -EIO;
		}

		if (exp_level == level) {
			break;
		}

		k_busy_wait(50);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		LOG_WRN("VWIRE %d! is %x\n", signal, level);
		return -ETIMEDOUT;
	}

	return 0;
}

int espi_handshake(void)
{
	int ret;

	LOG_INF("eSPI test - Handshake with eSPI master...");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_WARN,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_WRN("SUS_WARN Timeout");
		return ret;
	}

	LOG_INF("\t1st phase completed");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S5,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_WRN("SLP_S5 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S4,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_WRN("SLP_S4 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S3,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_WRN("SLP_S3 Timeout");
		return ret;
	}

	LOG_INF("\t2nd phase completed");
	return 0;
}

int get_pch_temp(struct device *dev)
{
	struct espi_oob_packet req_pckt;
	struct espi_oob_packet resp_pckt;
	struct oob_header oob_hdr;
	struct oob_response rsp;
	int ret;

	LOG_INF("%s", __func__);

	oob_hdr.dest_slave_addr = DEST_SLV_ADDR;
	oob_hdr.oob_cmd_code = OOB_CMDCODE;
	oob_hdr.byte_cnt = 1;
	oob_hdr.src_slave_addr = SRC_SLV_ADDR;

	/* Packetize OOB request */
	req_pckt.buf = (u8_t *)&oob_hdr;
	req_pckt.len = sizeof(struct oob_header);
	resp_pckt.buf = (u8_t *)&rsp;
	resp_pckt.len = MAX_RESP_SIZE;

	ret = espi_send_oob(dev, req_pckt);
	if (ret) {
		LOG_ERR("OOB Tx failed %d", ret);
		return ret;
	}

	ret = espi_receive_oob(dev, resp_pckt);
	if (ret) {
		LOG_ERR("OOB Rx failed %d", ret);
		return ret;
	}

	for (int i = 0; i < resp_pckt.len; i++) {
		LOG_INF("%x ", rsp.buf[i]);
	}

	return 0;
}

int espi_test(void)
{
	int ret;

#ifdef CONFIG_ESPI_GPIO_DEV_NEEDED
	gpio_dev0 = device_get_binding(CONFIG_ESPI_GPIO_DEV0);
	if (!gpio_dev0) {
		LOG_WRN("Fail to find: %s!\n", CONFIG_ESPI_GPIO_DEV0);
		return -1;
	}

	gpio_dev1 = device_get_binding(CONFIG_ESPI_GPIO_DEV1);
	if (!gpio_dev1) {
		LOG_WRN("Fail to find: %s!\n", CONFIG_ESPI_GPIO_DEV1);
		return -1;
	}

#endif
	espi_dev = device_get_binding(CONFIG_ESPI_DEV);
	if (!espi_dev) {
		LOG_WRN("Fail to find %s\n", CONFIG_ESPI_DEV);
		return 1;
	}

	LOG_INF("Hello eSPI test! %s\n", CONFIG_BOARD);

#ifdef CONFIG_ESPI_GPIO_DEV_NEEDED
	ret = gpio_pin_configure(gpio_dev0, CONFIG_PWRGD_PIN,
				 GPIO_INPUT | GPIO_ACTIVE_HIGH);
	if (ret) {
		printk("Unable to configure %d:%d\n", CONFIG_PWRGD_PIN, ret);
		return ret;
	}

	ret = gpio_pin_configure(gpio_dev1, CONFIG_ESPI_INIT_PIN,
				 GPIO_OUTPUT | GPIO_ACTIVE_HIGH);
	if (ret) {
		LOG_WRN("Unable to config %d: %d\n", CONFIG_ESPI_INIT_PIN, ret);
		return ret;
	}

	ret = gpio_pin_set(gpio_dev1, CONFIG_ESPI_INIT_PIN, 0);
	if (ret) {
		LOG_WRN("Unable to initialize %d\n", CONFIG_ESPI_INIT_PIN);
		return -1;
	}
#endif

	espi_init();

#ifdef CONFIG_ESPI_GPIO_DEV_NEEDED
	ret = wait_for_pin(gpio_dev0, CONFIG_PWRGD_PIN, PWR_SEQ_TIMEOUT, 1);
	if (ret) {
		printk("RSMRST_PWRGD timeout!");
		return ret;
	}

	ret = gpio_pin_set(gpio_dev1, CONFIG_ESPI_INIT_PIN, 1);
	if (ret) {
		printk("Failed to write %x %d\n", CONFIG_ESPI_INIT_PIN, ret);
		return ret;
	}
#endif

	ret = espi_handshake();
	if (ret) {
		LOG_PANIC();
		return ret;
	}

	/* Attempt anyways to test failure case */
	get_pch_temp(espi_dev);

	return 0;
}

void main(void)
{
	espi_test();
}
