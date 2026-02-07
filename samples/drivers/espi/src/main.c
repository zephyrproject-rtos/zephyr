/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include "espi_oob_handler.h"

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#include "espi_flash_caf_sample.h"
#endif

#ifdef CONFIG_ESPI_TAF
#include "espi_taf_sample.h"
#endif

LOG_MODULE_DECLARE(espi, CONFIG_ESPI_LOG_LEVEL);

/* eSPI flash parameters */
#define MAX_TEST_BUF_SIZE   1024u
#define MAX_FLASH_REQUEST   64u
#define TARGET_FLASH_REGION 0x72000ul

#define ESPI_FREQ_20MHZ 20u
#define ESPI_FREQ_25MHZ 25u
#define ESPI_FREQ_66MHZ 66u

#define K_WAIT_DELAY 100u

/* eSPI event */
#define EVENT_MASK         0x0000FFFFu
#define EVENT_DETAILS_MASK 0xFFFF0000u
#define EVENT_DETAILS_POS  16u
#define EVENT_TYPE(x)      (x & EVENT_MASK)
#define EVENT_DETAILS(x)   ((x & EVENT_DETAILS_MASK) >> EVENT_DETAILS_POS)

#define PWR_SEQ_TIMEOUT 3000u

/* The devicetree node identifier for the board power rails pins. */
#define BRD_PWR_NODE DT_NODELABEL(board_power)

#ifdef CONFIG_ESPI_USE_BOARD_POWER
static const struct gpio_dt_spec pwrgd_gpio = GPIO_DT_SPEC_GET(BRD_PWR_NODE, pwrg_gpios);
static const struct gpio_dt_spec rsm_gpio = GPIO_DT_SPEC_GET(BRD_PWR_NODE, rsm_gpios);
#endif

static const struct device *const espi_dev = DEVICE_DT_GET(DT_NODELABEL(espi0));
static struct espi_callback espi_bus_cb;
static struct espi_callback vw_rdy_cb;
static struct espi_callback vw_cb;
static struct espi_callback p80_cb;
static uint8_t espi_rst_sts;
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
static struct espi_callback oob_cb;
#endif

static void host_warn_handler(uint32_t signal, uint32_t status)
{
	switch (signal) {
	case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
		LOG_INF("Host reset warning %d", status);
		if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
			LOG_INF("HOST RST ACK %d", status);
			espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_HOST_RST_ACK, status);
		}
		break;
	case ESPI_VWIRE_SIGNAL_SUS_WARN:
		LOG_INF("Host suspend warning %d", status);
		if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
			LOG_INF("SUS ACK %d", status);
			espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_ACK, status);
		}
		break;
	default:
		break;
	}
}

/* eSPI bus event handler */
static void espi_reset_handler(const struct device *dev, struct espi_callback *cb,
			       struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_RESET) {
		espi_rst_sts = event.evt_data;
		LOG_INF("eSPI BUS reset %d", event.evt_data);
	}
}

/* eSPI logical channels enable/disable event handler */
static void espi_ch_handler(const struct device *dev, struct espi_callback *cb,
			    struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_CHANNEL_READY) {
		switch (event.evt_details) {
		case ESPI_CHANNEL_VWIRE:
			LOG_INF("VW channel event %x", event.evt_data);
			break;
		case ESPI_CHANNEL_FLASH:
			LOG_INF("Flash channel event %d", event.evt_data);
			break;
		case ESPI_CHANNEL_OOB:
			LOG_INF("OOB channel event %d", event.evt_data);
			break;
		default:
			LOG_ERR("Unknown channel event");
		}
	}
}

/* eSPI vwire received event handler */
static void vwire_handler(const struct device *dev, struct espi_callback *cb,
			  struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_VWIRE_RECEIVED) {
		switch (event.evt_details) {
		case ESPI_VWIRE_SIGNAL_PLTRST:
			LOG_INF("PLT_RST changed %d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SLP_S3:
		case ESPI_VWIRE_SIGNAL_SLP_S4:
		case ESPI_VWIRE_SIGNAL_SLP_S5:
			LOG_INF("SLP signal changed %d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SUS_WARN:
		case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
			host_warn_handler(event.evt_details, event.evt_data);
			break;
		}
	}
}

/* eSPI peripheral channel notifications handler */
static void periph_handler(const struct device *dev, struct espi_callback *cb,
			   struct espi_event event)
{
	uint8_t periph_type;
	uint8_t periph_index;

	periph_type = EVENT_TYPE(event.evt_details);
	periph_index = EVENT_DETAILS(event.evt_details);

	switch (periph_type) {
	case ESPI_PERIPHERAL_DEBUG_PORT80:
		LOG_INF("Postcode %x", event.evt_data);
		break;
	case ESPI_PERIPHERAL_HOST_IO:
		LOG_INF("ACPI %x", event.evt_data);
		espi_remove_callback(espi_dev, &p80_cb);
		break;
	default:
		LOG_INF("%s periph 0x%x [%x]", __func__, periph_type, event.evt_data);
	}
}

int espi_init(void)
{
	int ret;
	/* Indicate to eSPI master simplest configuration: Single line,
	 * 20MHz frequency and only logical channel 0 and 1 are supported
	 */
	struct espi_cfg cfg = {
		.io_caps = ESPI_IO_MODE_SINGLE_LINE,
		.channel_caps = ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_PERIPHERAL,
		.max_freq = ESPI_FREQ_20MHZ,
	};

	/* If eSPI driver supports additional capabilities use them */
#ifdef CONFIG_ESPI_OOB_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_OOB;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_FLASH;
	cfg.io_caps |= ESPI_IO_MODE_QUAD_LINES;
	cfg.max_freq = ESPI_FREQ_25MHZ;
#endif

	ret = espi_config(espi_dev, &cfg);
	if (ret) {
		LOG_ERR("Failed to configure eSPI target channels:%x err: %d", cfg.channel_caps,
			ret);
		return ret;
	} else {
		LOG_INF("eSPI target configured successfully!");
	}

	LOG_INF("eSPI test - callbacks initialization... ");
	espi_init_callback(&espi_bus_cb, espi_reset_handler, ESPI_BUS_RESET);
	espi_init_callback(&vw_rdy_cb, espi_ch_handler, ESPI_BUS_EVENT_CHANNEL_READY);
	espi_init_callback(&vw_cb, vwire_handler, ESPI_BUS_EVENT_VWIRE_RECEIVED);
	espi_init_callback(&p80_cb, periph_handler, ESPI_BUS_PERIPHERAL_NOTIFICATION);
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	espi_init_callback(&oob_cb, oob_rx_handler, ESPI_BUS_EVENT_OOB_RECEIVED);
#endif
	LOG_INF("complete");

	LOG_INF("eSPI test - callbacks registration... ");
	espi_add_callback(espi_dev, &espi_bus_cb);
	espi_add_callback(espi_dev, &vw_rdy_cb);
	espi_add_callback(espi_dev, &vw_cb);
	espi_add_callback(espi_dev, &p80_cb);
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	espi_add_callback(espi_dev, &oob_cb);
#endif
	LOG_INF("complete");

	return ret;
}

#ifdef CONFIG_ESPI_USE_BOARD_POWER
static int wait_for_pin(const struct gpio_dt_spec *gpio, uint16_t timeout, int exp_level)
{
	uint16_t loop_cnt = timeout;
	int level;

	do {
		level = gpio_pin_get_dt(gpio);
		if (level < 0) {
			LOG_ERR("Failed to read %x %d", gpio->pin, level);
			return -EIO;
		}

		if (exp_level == level) {
			LOG_DBG("PIN %x = %x", gpio->pin, exp_level);
			break;
		}

		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		LOG_ERR("Timeout for %x %x", gpio->pin, level);
		return -ETIMEDOUT;
	}

	return 0;
}
#endif

static int wait_for_vwire(const struct device *espi_dev, enum espi_vwire_signal signal,
			  uint16_t timeout, uint8_t exp_level)
{
	int ret;
	uint8_t level;
	uint16_t loop_cnt = timeout;

	do {
		ret = espi_receive_vwire(espi_dev, signal, &level);
		if (ret) {
			LOG_ERR("Failed to read %x %d", signal, ret);
			return -EIO;
		}

		if (exp_level == level) {
			break;
		}

		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		LOG_ERR("VWIRE %d is %x", signal, level);
		return -ETIMEDOUT;
	}

	return 0;
}

static int wait_for_espi_reset(uint8_t exp_sts)
{
	uint16_t loop_cnt = CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT;

	do {
		if (exp_sts == espi_rst_sts) {
			break;
		}
		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		return -ETIMEDOUT;
	}

	return 0;
}

int espi_handshake(void)
{
	int ret;

	LOG_INF("eSPI test - Handshake with eSPI master...");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_WARN, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("SUS_WARN Timeout");
		return ret;
	}

	LOG_INF("1st phase completed");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S5, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("SLP_S5 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S4, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("SLP_S4 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S3, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("SLP_S3 Timeout");
		return ret;
	}

	LOG_INF("2nd phase completed");

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_PLTRST, CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT,
			     1);
	if (ret) {
		LOG_ERR("PLT_RST Timeout");
		return ret;
	}

	LOG_INF("3rd phase completed");

	return 0;
}


#ifndef CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE
static void send_target_bootdone(void)
{
	int ret;
	uint8_t boot_done;

	ret = espi_receive_vwire(espi_dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, &boot_done);
	LOG_INF("%s boot_done: %d", __func__, boot_done);
	if (ret) {
		LOG_WRN("Fail to retrieve target boot done");
	} else if (!boot_done) {
		/* TARGET_BOOT_DONE & TARGET_LOAD_STS have to be sent together */
		espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS, 1);
		espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, 1);
	}
}
#endif

int espi_test(void)
{
	int ret;

	/* Account for the time serial port is detected so log messages can
	 * be seen
	 */
	k_sleep(K_SECONDS(1));

#ifdef CONFIG_ESPI_USE_BOARD_POWER
	if (!gpio_is_ready_dt(&pwrgd_gpio)) {
		LOG_ERR("%s: device not ready.", pwrgd_gpio.port->name);
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&rsm_gpio)) {
		LOG_ERR("%s: device not ready.", rsm_gpio.port->name);
		return -ENODEV;
	}
#endif
	if (!device_is_ready(espi_dev)) {
		LOG_ERR("%s: device not ready.", espi_dev->name);
		return -ENODEV;
	}

#ifdef CONFIG_ESPI_TAF
	const struct device *const qspi_dev = DEVICE_DT_GET(DT_NODELABEL(qspi0));
	const struct device *const espi_saf_dev = DEVICE_DT_GET(DT_NODELABEL(espi_saf0));

	if (!device_is_ready(qspi_dev)) {
		LOG_ERR("%s: device not ready.", qspi_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(espi_saf_dev)) {
		LOG_ERR("%s: device not ready.", espi_saf_dev->name);
		return -ENODEV;
	}
#endif

	LOG_INF("Hello eSPI test %s", CONFIG_BOARD);

#ifdef CONFIG_ESPI_USE_BOARD_POWER
	ret = gpio_pin_configure_dt(&pwrgd_gpio, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Unable to configure %d:%d", pwrgd_gpio.pin, ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&rsm_gpio, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("Unable to config %d: %d", rsm_gpio.pin, ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&rsm_gpio, 0);
	if (ret) {
		LOG_ERR("Unable to initialize %d", rsm_gpio.pin);
		return -1;
	}
#endif

	espi_init();

#ifdef CONFIG_ESPI_TAF
	/*
	 * eSPI SAF configuration must be after eSPI configuration.
	 * eSPI SAF EC portal flash tests before EC releases RSMRST# and
	 * Host de-asserts ESPI_RESET#.
	 */
	ret = spi_saf_init();
	if (ret) {
		LOG_ERR("Unable to configure %d:%s", ret, qspi_dev->name);
		return ret;
	}

	ret = espi_saf_init();
	if (ret) {
		LOG_ERR("Unable to configure %d:%s", ret, espi_saf_dev->name);
		return ret;
	}

	ret = espi_saf_test1(SAF_SPI_TEST_ADDRESS);
	if (ret) {
		LOG_INF("eSPI SAF test1 returned error %d", ret);
	}
#endif

#ifdef CONFIG_ESPI_USE_BOARD_POWER
	ret = wait_for_pin(&pwrgd_gpio, PWR_SEQ_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("RSMRST_PWRGD timeout");
		return ret;
	}

	ret = gpio_pin_set_dt(&rsm_gpio, 1);
	if (ret) {
		LOG_ERR("Failed to set rsm err: %d", ret);
		return ret;
	}
#endif
	ret = wait_for_espi_reset(1);
	if (ret) {
		LOG_INF("ESPI_RESET timeout");
		return ret;
	}

#ifndef CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE
	/* When automatic acknowledge is disabled to perform lengthy operations
	 * in the eSPI target, need to explicitly send target boot virtual wires
	 */
	bool vw_ch_sts;

	/* Simulate lengthy operation during boot */
	k_sleep(K_SECONDS(2));

	do {
		vw_ch_sts = espi_get_channel_status(espi_dev, ESPI_CHANNEL_VWIRE);
		k_busy_wait(100);
	} while (!vw_ch_sts);

	send_target_bootdone();
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	/* Flash operation need to be perform before VW handshake or
	 * after eSPI host completes full initialization.
	 * This sample code can't assume a full initialized eSPI host
	 * so flash operations are perform here.
	 */
	bool flash_sts;

	do {
		flash_sts = espi_get_channel_status(espi_dev, ESPI_CHANNEL_FLASH);
		k_busy_wait(100);
	} while (!flash_sts);

	/* eSPI flash test can fail and rest of operation can continue */
	ret = espi_flash_test(TARGET_FLASH_REGION, 1);
	if (ret) {
		LOG_INF("eSPI flash test failed %d", ret);
	}
#endif

	/* Showcase VW channel by exchanging virtual wires with eSPI host */
	ret = espi_handshake();
	if (ret) {
		LOG_ERR("eSPI VW handshake failed %d", ret);
		return ret;
	}

	/*  Attempt to use OOB channel to read temperature, regardless of
	 * if is enabled or not.
	 */
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	/* System without host-initiated OOB Rx traffic */
	get_pch_temp_sync(espi_dev);
#else
	/* System with host-initiated OOB Rx traffic */
	get_pch_temp_async(espi_dev);
#endif

	/* Cleanup */
	k_sleep(K_SECONDS(1));
	espi_remove_callback(espi_dev, &espi_bus_cb);
	espi_remove_callback(espi_dev, &vw_rdy_cb);
	espi_remove_callback(espi_dev, &vw_cb);

	LOG_INF("eSPI sample completed err: %d", ret);

	return ret;
}

int main(void)
{
	espi_test();
	return 0;
}
