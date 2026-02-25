/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * FSK send sample using the public LoRa driver API.
 *
 * LoRa-capable modems (such as the SX126x family) support both LoRa and
 * (G)FSK modulations on the same hardware.  FSK is required for LoRaWAN
 * regional plans that mandate an FSK data rate (e.g. EU868 DR7).
 *
 * This sample shows how to switch to FSK by setting
 * lora_modem_config.modulation = LORA_MOD_FSK and filling the .fsk
 * sub-struct, without touching any driver-private symbols.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <errno.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
	     "No default LoRa radio specified in DT");

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_send_fsk);

#define MAX_DATA_LEN 16

static char data[MAX_DATA_LEN] = "FSK hello world!";

int main(void)
{
	const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
	struct lora_modem_config config = {0};
	int ret;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

	/*
	 * Common parameters shared by all modulations.
	 */
	config.frequency  = 868100000; /* EU868 center frequency */
	config.tx_power   = 14;        /* dBm */
	config.tx         = true;

	/*
	 * Select FSK modulation.  Because LORA_MOD_LORA == 0, any existing
	 * code that zero-initializes lora_modem_config and does not set this
	 * field continues to work unchanged.
	 */
	config.modulation = LORA_MOD_FSK;

	/*
	 * FSK-specific parameters.  These are ignored for LoRa mode.
	 *
	 * 50 kbps / ±25 kHz deviation is the standard LoRaWAN FSK data rate
	 * (EU868 DR7).  Carson bandwidth ≈ 2*(fdev + br/2) = 100 kHz, so a
	 * 117 kHz RX filter comfortably fits the signal.
	 */
	config.fsk.bitrate      = 50000;                    /* 50 kbps */
	config.fsk.fdev         = 25000;                    /* ±25 kHz */
	config.fsk.shaping      = LORA_FSK_SHAPING_GAUSS_BT_0_5;
	config.fsk.bandwidth    = FSK_BW_117_KHZ;
	config.fsk.preamble_len = 5;                        /* bytes */
	config.fsk.variable_len = true;
	config.fsk.crc_on       = true;
	config.fsk.whitening    = true;

	/*
	 * Sync word: LoRaWAN GFSK sync word (EU868 DR7).
	 * TX and RX must use the same sync word. Any 1-8 byte sequence
	 * can be used for private peer-to-peer links.
	 */
	config.fsk.sync_word[0] = 0xC1;
	config.fsk.sync_word[1] = 0x94;
	config.fsk.sync_word[2] = 0xC1;
	config.fsk.sync_word_len = 3;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa FSK config failed: %d", ret);
		return 0;
	}

	LOG_INF("FSK packet airtime: %u ms",
		lora_airtime(lora_dev, MAX_DATA_LEN));

	while (1) {
		ret = lora_send(lora_dev, data, MAX_DATA_LEN);
		if (ret < 0) {
			LOG_ERR("LoRa FSK send failed: %d", ret);
			return 0;
		}

		LOG_INF("FSK data sent");
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
