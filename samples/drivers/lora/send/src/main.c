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

#define MAX_DATA_LEN 12

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_send);

char data[MAX_DATA_LEN] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', ' ', '0'};

int main(void)
{
	const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
#ifdef CONFIG_SAMPLE_LORA_GFSK
	struct lora_modem_config_gfsk config = {0};
#else
	struct lora_modem_config config = {0};
#endif
	int ret;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

#ifdef CONFIG_SAMPLE_LORA_GFSK
	/* The 50 kbps FSK that LoRaWAN defines as DR7. A backend whose library
	 * writes the sync word, the pulse shape and the whitening itself takes
	 * these and nothing else.
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
	config.tx_power = 4;
	config.tx = true;

	ret = lora_config_gfsk(lora_dev, &config);
#else
	config.frequency = 865100000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_10;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 4;
	config.tx = true;

	ret = lora_config(lora_dev, &config);
#endif
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return 0;
	}

	LOG_INF("Expected packet airtime: %u ms", lora_airtime(lora_dev, MAX_DATA_LEN));

	while (1) {
		ret = lora_send(lora_dev, data, MAX_DATA_LEN);
		if (ret < 0) {
			LOG_ERR("LoRa send failed");
			return 0;
		}

		LOG_INF("Data sent %c!", data[MAX_DATA_LEN - 1]);

		/* Send data at 1s interval */
		k_sleep(K_MSEC(1000));

		/* Increment final character to differentiate packets */
		if (data[MAX_DATA_LEN - 1] == '9') {
			data[MAX_DATA_LEN - 1] = '0';
		} else {
			data[MAX_DATA_LEN - 1] += 1;
		}
	}
	return 0;
}
