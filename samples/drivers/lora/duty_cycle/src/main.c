/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * LoRa RX duty cycle (wake-on-radio) sample.
 *
 * Two boards are needed: a sender and a receiver. Both must be built
 * from this sample. Hold button 0 during boot to select the sender
 * role; the default is receiver.
 *
 * Receiver: calls lora_recv_duty_cycle_async() so the radio alternates
 *           between a short RX window and sleep, waking the MCU
 *           only when a packet is received.
 *
 * Sender:   transmits with a long preamble (100 symbols) so the
 *           receiver is guaranteed to catch the preamble during
 *           one of its RX windows.
 *
 * RX preamble length and StopTimerOnPreamble
 * -------------------------------------------
 * The driver sets StopTimerOnPreamble = true before entering duty
 * cycle mode. This makes the radio stay in RX once a preamble is
 * detected, instead of going back to sleep when the RX window
 * expires. The radio then searches for the sync word within a
 * window sized by the receiver's preamble_len. This value must
 * match the sender's: a mismatch causes the demodulator to stop
 * searching before the sync word arrives.
 *
 * Matching preamble_len — OK (works even if RX joins mid-preamble)
 *
 *   TX  |#########< 100 preamble symbols >###########|SW|HDR| payload |
 *                       ^                            ^
 *   RX  ..sleep..|==RX==|<<< stays in RX >>>>>>>>>>>>|
 *                   ^                                |
 *                   preamble detected                |
 *                   timer stops (StopTimerOnPreamble)|
 *                   search window = 100 symbols      |
 *                   sync word found -----------------+
 *                   --> full packet received, RX_DONE
 *
 * Mismatched preamble_len (RX = 8, TX = 100) — FAIL
 *
 *   TX  |#########< 100 preamble symbols >###########|SW|HDR| payload |
 *                       ^                            ^
 *   RX  ..sleep..|==RX==|<<< stays in RX             |
 *                   ^        but gives up here       |
 *                   |        |<-8->|                 |
 *                   preamble detected                |
 *                   search window = only 8 symbols   |
 *                   sync word NOT found within window
 *                   --> demodulator fails, packet lost
 *
 * Preamble sizing rule (SX126x datasheet 13.1.7):
 *   preamble_time > 2 * rx_period + sleep_period
 *   100 symbols * 8.19 ms = 819 ms > 2 * 50 + 450 = 550 ms  OK
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_duty_cycle);

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
	     "No default LoRa radio specified in DT");

#define FREQUENCY 865100000
#define BANDWIDTH BW_125_KHZ
#define SPREADING_FACTOR SF_10
#define CODING_RATE CR_4_5

/* Shared preamble length used by both sender and receiver. */
#define TX_PREAMBLE_LEN 100
#define RX_PREAMBLE_LEN TX_PREAMBLE_LEN

/*
 * Duty cycle timing.
 *
 * The RX window must be long enough for the radio to detect a preamble.
 * At SF10 / BW125 one symbol is ~8.19 ms and the detector needs at
 * least ~2 symbols, so 50 ms (~6 symbols) gives comfortable margin.
 *
 * The sender preamble must satisfy (SX126x datasheet 13.1.7):
 *   preamble_time > 2 * rx_period + sleep_period
 *   100 symbols * 8.19 ms = 819 ms > 2 * 50 + 450 = 550 ms  OK
 */
#define RX_PERIOD_MS 50
#define SLEEP_PERIOD_MS 450

#define MAX_DATA_LEN 255
#define TX_DATA_LEN 12

static void duty_cycle_recv_cb(const struct device *dev, uint8_t *data,
			       uint16_t size, int16_t rssi, int8_t snr,
			       void *user_data)
{
	static int cnt;

	ARG_UNUSED(user_data);

	LOG_INF("RX %d bytes, RSSI: %d dBm, SNR: %d dB", size, rssi, snr);
	LOG_HEXDUMP_INF(data, size, "payload");

	if (++cnt == 10) {
		LOG_INF("Stopping duty cycle reception");
		lora_recv_duty_cycle_async(dev, K_NO_WAIT, K_NO_WAIT, NULL, NULL);
	}
}

static int run_receiver(const struct device *lora_dev)
{
	struct lora_modem_config config = {
		.frequency = FREQUENCY,
		.bandwidth = BANDWIDTH,
		.datarate = SPREADING_FACTOR,
		.coding_rate = CODING_RATE,
		.preamble_len = RX_PREAMBLE_LEN,
		.public_network = false,
		.tx_power = 4,
		.tx = false,
	};
	int ret;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed: %d", ret);
		return ret;
	}

	LOG_INF("RX duty cycle started (rx=%d ms, sleep=%d ms)",
		RX_PERIOD_MS, SLEEP_PERIOD_MS);
	LOG_INF("RX preamble configured to %u symbols", RX_PREAMBLE_LEN);

	ret = lora_recv_duty_cycle_async(lora_dev,
				   K_MSEC(RX_PERIOD_MS),
				   K_MSEC(SLEEP_PERIOD_MS),
				   duty_cycle_recv_cb, NULL);
	if (ret < 0) {
		LOG_ERR("lora_recv_duty_cycle_async failed: %d", ret);
		return ret;
	}

	k_sleep(K_FOREVER);
	return 0;
}

static int run_sender(const struct device *lora_dev)
{
	struct lora_modem_config config = {
		.frequency = FREQUENCY,
		.bandwidth = BANDWIDTH,
		.datarate = SPREADING_FACTOR,
		.coding_rate = CODING_RATE,
		.preamble_len = TX_PREAMBLE_LEN,
		.public_network = false,
		.tx_power = 4,
		.tx = true,
	};
	char data[TX_DATA_LEN] = {'d', 'u', 't', 'y', 'c', 'y',
				   'c', 'l', 'e', ' ', ' ', '0'};
	int ret;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed: %d", ret);
		return ret;
	}

	LOG_INF("Sender mode (preamble=%d symbols)", TX_PREAMBLE_LEN);
	LOG_INF("Expected packet airtime: %u ms",
		lora_airtime(lora_dev, TX_DATA_LEN));

	while (1) {
		ret = lora_send(lora_dev, data, TX_DATA_LEN);
		if (ret < 0) {
			LOG_ERR("LoRa send failed: %d", ret);
			return ret;
		}

		LOG_INF("Sent packet %c", data[TX_DATA_LEN - 1]);

		k_sleep(K_SECONDS(4));

		if (data[TX_DATA_LEN - 1] == '9') {
			data[TX_DATA_LEN - 1] = '0';
		} else {
			data[TX_DATA_LEN - 1] += 1;
		}
	}

	return 0;
}

static bool is_sender_mode(void)
{
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0),
							 gpios);

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

	return run_receiver(lora_dev);
}
