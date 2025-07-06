/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/check.h>

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
LOG_MODULE_REGISTER(bt_data);

void bt_data_parse(struct net_buf_simple *ad,
		   bool (*func)(struct bt_data *data, void *user_data),
		   void *user_data)
{
	while (ad->len > 1) {
		struct bt_data data;
		uint8_t len;

		len = net_buf_simple_pull_u8(ad);
		if (len == 0U) {
			/* Early termination */
			return;
		}

		if (len > ad->len) {
			LOG_WRN("malformed advertising data %u / %u",
				len, ad->len);
			return;
		}

		data.type = net_buf_simple_pull_u8(ad);
		data.data_len = len - 1;
		data.data = ad->data;

		if (!func(&data, user_data)) {
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

size_t bt_data_get_len(const struct bt_data data[], size_t data_count)
{
	size_t total_len = 0;

	for (size_t i = 0; i < data_count; i++) {
		total_len += sizeof(data[i].data_len) + sizeof(data[i].type) + data[i].data_len;
	}

	return total_len;
}

size_t bt_data_serialize(const struct bt_data *input, uint8_t *output)
{
	CHECKIF(input == NULL) {
		LOG_DBG("input is NULL");
		return 0;
	}

	CHECKIF(output == NULL) {
		LOG_DBG("output is NULL");
		return 0;
	}

	uint8_t ad_data_len = input->data_len;
	uint8_t data_len = ad_data_len + 1;

	output[0] = data_len;
	output[1] = input->type;

	memcpy(&output[2], input->data, ad_data_len);

	return data_len + 1;
}
