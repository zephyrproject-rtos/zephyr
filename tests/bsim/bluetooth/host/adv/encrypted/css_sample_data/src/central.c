/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

extern const struct test_sample_data *sample_data;

extern int data_set;

static bool data_parse_cb(struct bt_data *data, void *user_data)
{
	if (data->type == BT_DATA_ENCRYPTED_AD_DATA) {
		int err;
		uint8_t decrypted_payload[sample_data->size_ad_data];
		struct net_buf_simple decrypted_buf;
		size_t decrypted_data_size = BT_EAD_DECRYPTED_PAYLOAD_SIZE(data->data_len);

		if (decrypted_data_size != sample_data->size_ad_data) {
			LOG_ERR("Size of decrypted data: %d", decrypted_data_size);
			LOG_ERR("Size of sample data: %d", sample_data->size_ad_data);
			FAIL("Computed size of data does not match the size of the data from the "
			     "sample. (data set %d)\n",
			     data_set);
		}

		if (memcmp(sample_data->randomizer_little_endian, data->data,
			   BT_EAD_RANDOMIZER_SIZE) != 0) {
			LOG_ERR("Received Randomizer: %s",
				bt_hex(data->data, BT_EAD_RANDOMIZER_SIZE));
			LOG_ERR("Expected Randomizer from sample: %s",
				bt_hex(sample_data->randomizer_little_endian,
				       BT_EAD_RANDOMIZER_SIZE));
			FAIL("Received Randomizer does not match the expected one.\n");
		}

		net_buf_simple_init_with_data(&decrypted_buf, decrypted_payload,
					      decrypted_data_size);

		err = bt_ead_decrypt(sample_data->session_key, sample_data->iv, data->data,
				     data->data_len, decrypted_buf.data);
		if (err != 0) {
			FAIL("Error during decryption.\n");
		} else if (memcmp(decrypted_buf.data, sample_data->ad_data, decrypted_data_size)) {
			LOG_HEXDUMP_ERR(decrypted_buf.data, decrypted_data_size,
					"Decrypted data from bt_ead_decrypt:");
			LOG_HEXDUMP_ERR(sample_data->ad_data, sample_data->size_ad_data,
					"Expected data from sample:");
			FAIL("Decrypted AD data does not match expected sample data. (data set "
			     "%d)\n",
			     data_set);
		}

		LOG_HEXDUMP_DBG(decrypted_buf.data, decrypted_data_size, "Raw decrypted data: ");

		bt_data_parse(&decrypted_buf, &data_parse_cb, NULL);

		PASS("Central test passed. (data set %d)\n", data_set);

		return false;
	}

	LOG_DBG("Parsed data:");
	LOG_DBG("len : %d", data->data_len);
	LOG_DBG("type: 0x%02x", data->type);
	LOG_HEXDUMP_DBG(data->data, data->data_len, "data:");

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	LOG_DBG("Device found: %s (RSSI %d)", addr_str, rssi);

	bt_data_parse(ad, &data_parse_cb, NULL);
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
	}

	LOG_DBG("Scanning successfully started");
}

void test_central(void)
{
	int err;

	LOG_DBG("Central device. (data set %d)", data_set);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
	}

	LOG_DBG("Bluetooth initialized");

	start_scan();
}
