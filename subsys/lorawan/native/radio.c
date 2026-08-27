/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>

#include "radio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native_radio, CONFIG_LORAWAN_LOG_LEVEL);

static const struct device *radio_dev;

#define LORA_PREAMBLE_LEN	8

static struct lora_modem_config tx_config = {
	.coding_rate = CR_4_5,
	.preamble_len = LORA_PREAMBLE_LEN,
	.public_network = IS_ENABLED(CONFIG_LORAWAN_NATIVE_PUBLIC_NETWORK),
	.tx = true,
	.iq_inverted = false,
	.packet_crc_disable = false,
};

static struct lora_modem_config rx_config = {
	.coding_rate = CR_4_5,
	.preamble_len = LORA_PREAMBLE_LEN,
	.public_network = IS_ENABLED(CONFIG_LORAWAN_NATIVE_PUBLIC_NETWORK),
	.tx = false,
	.iq_inverted = true,
	.packet_crc_disable = true,
};

int radio_init(const struct device *lora_dev)
{
	if (!device_is_ready(lora_dev)) {
		LOG_ERR("LoRa device not ready");
		return -ENODEV;
	}

	radio_dev = lora_dev;

	return 0;
}

int radio_tx(const uint8_t *data, size_t len,
	     uint32_t freq, const struct lwan_dr_params *dr,
	     int8_t power)
{
	struct k_poll_signal tx_done = K_POLL_SIGNAL_INITIALIZER(tx_done);
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &tx_done),
	};
	int ret;

	tx_config.frequency = freq;
	tx_config.bandwidth = dr->bw;
	tx_config.datarate = dr->sf;
	tx_config.tx_power = power;

	ret = lora_config(radio_dev, &tx_config);
	if (ret != 0) {
		LOG_ERR("TX config failed: %d", ret);
		return -EIO;
	}

	ret = lora_send_async(radio_dev, (uint8_t *)data, len, &tx_done);
	if (ret != 0) {
		LOG_ERR("lora_send_async failed: %d", ret);
		return -EIO;
	}

	/* Wait for TX complete with a generous timeout */
	ret = k_poll(events, 1, K_SECONDS(10));
	if (ret != 0) {
		LOG_ERR("TX poll timeout: %d", ret);
		return -EIO;
	}

	if (tx_done.result != 0) {
		LOG_ERR("TX failed: %d", tx_done.result);
		return -EIO;
	}

	LOG_DBG("TX done: freq=%u sf=%u bw=%u len=%zu",
		freq, dr->sf, dr->bw, len);

	return 0;
}

uint32_t radio_airtime(uint32_t data_len)
{
	return lora_airtime(radio_dev, data_len);
}

uint32_t radio_airtime_params(uint8_t sf, uint16_t bw_khz, uint8_t frame_len)
{
	uint32_t t_sym_us = ((uint32_t)1 << sf) * 1000 / bw_khz;
	/* Low data-rate optimise when symbol time >= 16.384 ms */
	uint8_t de = (t_sym_us >= 16384) ? 1 : 0;
	uint32_t preamble_us = t_sym_us * 12 + t_sym_us / 4;
	uint32_t payload_us;
	int payload_syms;
	int denom;
	int num;

	/*
	 * Payload symbol count (LoRa modem formula):
	 *   8 + ceil(max(8*PL - 4*SF + 44, 0) / (4*(SF - 2*DE))) * (CR+4)
	 * With CR=1 -> (CR+4)=5, and 44 = 28 + 16 (CRC enabled).
	 */
	num = 8 * (int)frame_len - 4 * (int)sf + 44;

	if (num < 0) {
		num = 0;
	}

	denom = 4 * ((int)sf - 2 * (int)de);
	payload_syms = 8 + ((num + denom - 1) / denom) * 5;

	payload_us = (uint32_t)payload_syms * t_sym_us;

	return (preamble_us + payload_us + 500) / 1000;
}

int radio_rx(uint32_t freq, const struct lwan_dr_params *dr,
	     uint32_t timeout_ms,
	     uint8_t *buf, uint8_t buf_size,
	     int16_t *rssi, int8_t *snr)
{
	int ret;

	rx_config.frequency = freq;
	rx_config.bandwidth = dr->bw;
	rx_config.datarate = dr->sf;

	ret = lora_config(radio_dev, &rx_config);
	if (ret != 0) {
		LOG_ERR("RX config failed: %d", ret);
		return -EIO;
	}

	ret = lora_recv(radio_dev, buf, buf_size,
			K_MSEC(timeout_ms), rssi, snr);
	if (ret < 0) {
		if (ret == -EAGAIN) {
			LOG_DBG("RX timeout: freq=%u sf=%u", freq, dr->sf);
			return 0;
		}
		LOG_ERR("lora_recv failed: %d", ret);
		return -EIO;
	}

	LOG_DBG("RX done: freq=%u sf=%u len=%d rssi=%d snr=%d",
		freq, dr->sf, ret, *rssi, *snr);

	return ret;
}
