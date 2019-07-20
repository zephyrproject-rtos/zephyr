/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <misc/util.h>
#include <net/lora.h>
#include <zephyr.h>

#define MAX_DATA_LEN 128

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lora_receive);

void main(void)
{
	struct device *lora_dev;
	struct lora_modem_config config;
	int i, ret, len;
	char data[MAX_DATA_LEN];

	lora_dev = device_get_binding(DT_INST_0_SEMTECH_SX1276_LABEL);
	if (!lora_dev) {
		LOG_ERR("%s Device not found", DT_INST_0_SEMTECH_SX1276_LABEL);
		return;
	}

	config.frequency = 868100000;
	config.bandwidth = BW_125_KHZ;
	config.spreading_factor = SF_10;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.tx_power = 14;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return;
	}

	while (1) {
		len = lora_recv(lora_dev, data);
		if (len < 0) {
			LOG_ERR("LoRa receive failed");
			return;
		}

		LOG_INF("Received data:");
		for (i = 0; i < len; i++)
			LOG_INF("%c", data[i]);

		k_sleep(2000);
	}
}
