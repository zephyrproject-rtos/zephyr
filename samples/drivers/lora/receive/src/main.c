/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
	     "No default LoRa radio specified in DT");

#define MAX_DATA_LEN 255

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_receive);

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
		     int16_t rssi, int8_t snr, void *user_data)
{
	static int cnt;

	ARG_UNUSED(dev);
	ARG_UNUSED(size);
	ARG_UNUSED(user_data);

	LOG_INF("LoRa RX RSSI: %d dBm, SNR: %d dB", rssi, snr);
	LOG_HEXDUMP_INF(data, size, "LoRa RX payload");

	/* Stop receiving after 10 packets */
	if (++cnt == 10) {
		LOG_INF("Stopping packet receptions");
		lora_recv_async(dev, NULL, NULL);
	}
}

int main(void)
{
	const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
#ifdef CONFIG_SAMPLE_LORA_GFSK
	struct lora_modem_config_gfsk config = {0};
#else
	struct lora_modem_config config = {0};
#endif
	int ret, len;
	uint8_t data[MAX_DATA_LEN] = {0};
	int16_t rssi;
	int8_t snr;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

#ifdef CONFIG_SAMPLE_LORA_GFSK
	/* The 50 kbps FSK that LoRaWAN defines as DR7. A backend whose library
	 * writes the sync word, the pulse shape and the whitening itself takes
	 * these and nothing else. GFSK reports no signal-to-noise ratio, so
	 * the SNR below reads zero.
	 */
	config.frequency = 865100000;
	config.bitrate = 50000;
	config.freq_deviation = 25000;
	config.bandwidth = 100000;
	config.pulse_shape = LORA_GFSK_PULSE_SHAPE_BT_1_0;
	config.sync_word[0] = 0xC1;
	config.sync_word[1] = 0x94;
	config.sync_word[2] = 0xC1;
	config.sync_word_len = 3;
	config.preamble_len = 5;
	config.whitening = true;
	config.tx_power = 14;
	config.tx = false;

	ret = lora_config_gfsk(lora_dev, &config);
#else
	config.frequency = 865100000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_10;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 14;
	config.tx = false;

	ret = lora_config(lora_dev, &config);
#endif
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return 0;
	}

	/* Receive 4 packets synchronously */
	LOG_INF("Synchronous reception");
	for (int i = 0; i < 4; i++) {
		/* Block until data arrives */
		len = lora_recv(lora_dev, data, MAX_DATA_LEN, K_FOREVER,
				&rssi, &snr);
		if (len < 0) {
			LOG_ERR("LoRa receive failed");
			return 0;
		}

		LOG_INF("LoRa RX RSSI: %d dBm, SNR: %d dB", rssi, snr);
		LOG_HEXDUMP_INF(data, len, "LoRa RX payload");
	}

	/* Enable asynchronous reception */
	LOG_INF("Asynchronous reception");
	lora_recv_async(lora_dev, lora_receive_cb, NULL);
	k_sleep(K_FOREVER);
	return 0;
}
