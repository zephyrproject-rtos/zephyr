/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LoRa Channel Activity Detection (CAD) sample.
 *
 * Hold button 0 at boot for the sender role; default is scanner. The
 * scanner reports how many of each window of CAD probes found the channel
 * busy; the sender loads the channel with LBT transmissions.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_cad);

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
	     "No default LoRa radio specified in DT");

#define FREQUENCY 865100000
#define BANDWIDTH BW_125_KHZ
#define SPREADING_FACTOR SF_10
#define CODING_RATE CR_4_5

/* CAD detects preamble chirps, so a long preamble is easier to catch. */
#define PREAMBLE_LEN 64

#define TX_DATA_LEN 12
#define SCAN_WINDOW 20
#define TX_GAP_MS 50

static struct k_poll_signal cad_done_signal;

static void cad_cb(const struct device *dev, bool activity_detected,
		   void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	LOG_INF("Async CAD result: channel %s",
		activity_detected ? "busy" : "clear");
	k_poll_signal_raise(&cad_done_signal, activity_detected);
}

static int run_scanner(const struct device *lora_dev)
{
	struct lora_modem_config config = {
		.frequency = FREQUENCY,
		.bandwidth = BANDWIDTH,
		.datarate = SPREADING_FACTOR,
		.coding_rate = CODING_RATE,
		.preamble_len = PREAMBLE_LEN,
		.public_network = false,
		.tx_power = 4,
		.tx = false,
		.cad = {
			.symbol_num = LORA_CAD_SYMB_4,
		},
	};
	struct k_poll_event event;
	int busy = 0;
	int probes = 0;
	int ret;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed: %d", ret);
		return ret;
	}

	LOG_INF("Scanner mode: probing channel with CAD");

	LOG_INF("Asynchronous CAD probe");
	k_poll_signal_init(&cad_done_signal);

	ret = lora_cad_async(lora_dev, cad_cb, NULL);
	if (ret < 0) {
		LOG_ERR("lora_cad_async failed: %d", ret);
		return ret;
	}

	k_poll_event_init(&event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &cad_done_signal);
	k_poll(&event, 1, K_FOREVER);

	LOG_INF("Continuous CAD scan");
	for (;;) {
		ret = lora_cad(lora_dev, K_SECONDS(1));
		if (ret < 0) {
			LOG_ERR("lora_cad failed: %d", ret);
			return ret;
		}

		if (ret == 1) {
			busy++;
		}

		if (++probes == SCAN_WINDOW) {
			LOG_INF("channel busy on %d of last %d probes",
				busy, SCAN_WINDOW);
			busy = 0;
			probes = 0;
		}
	}
}

static int run_sender(const struct device *lora_dev)
{
	struct lora_modem_config config = {
		.frequency = FREQUENCY,
		.bandwidth = BANDWIDTH,
		.datarate = SPREADING_FACTOR,
		.coding_rate = CODING_RATE,
		.preamble_len = PREAMBLE_LEN,
		.public_network = false,
		.tx_power = 4,
		.tx = true,
		.cad = {
			.mode = LORA_CAD_MODE_LBT,
			.symbol_num = LORA_CAD_SYMB_4,
		},
	};
	char data[TX_DATA_LEN] = {'l', 'o', 'r', 'a', ' ', 'c',
				   'a', 'd', ' ', ' ', ' ', '0'};
	int ret;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed: %d", ret);
		return ret;
	}

	LOG_INF("Sender mode: Listen Before Talk enabled");

	while (1) {
		ret = lora_send(lora_dev, data, TX_DATA_LEN);
		if (ret < 0 && ret != -EBUSY) {
			LOG_ERR("LoRa send failed: %d", ret);
			return ret;
		}

		if (ret == -EBUSY) {
			LOG_WRN("Channel busy (LBT), skipping transmission");
		} else {
			LOG_INF("Sent packet %c", data[TX_DATA_LEN - 1]);

			if (data[TX_DATA_LEN - 1] == '9') {
				data[TX_DATA_LEN - 1] = '0';
			} else {
				data[TX_DATA_LEN - 1] += 1;
			}
		}

		k_sleep(K_MSEC(TX_GAP_MS));
	}

	return 0;
}

static bool is_sender_mode(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

	if (!gpio_is_ready_dt(&btn)) {
		return false;
	}

	gpio_pin_configure_dt(&btn, GPIO_INPUT);
	k_sleep(K_MSEC(10));

	return gpio_pin_get_dt(&btn) != 0;
#else
	return false;
#endif
}

int main(void)
{
	const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s not ready", lora_dev->name);
		return 0;
	}

	if (is_sender_mode()) {
		return run_sender(lora_dev);
	}

	return run_scanner(lora_dev);
}
