/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

extern const struct test_sample_data *sample_data;

extern int data_set;

static void create_adv(struct bt_le_ext_adv **adv)
{
	int err;
	struct bt_le_adv_param params;

	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_CONN;
	params.options |= BT_LE_ADV_OPT_EXT_ADV;

	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_SLOW_INT_MIN;
	params.interval_max = BT_GAP_ADV_SLOW_INT_MAX;

	err = bt_le_ext_adv_create(&params, NULL, adv);
	if (err) {
		FAIL("Failed to create advertiser (%d)\n", err);
	}
}

static void start_adv(struct bt_le_ext_adv *adv)
{
	int err;
	int32_t timeout = 0;
	uint8_t num_events = 0;

	struct bt_le_ext_adv_start_param start_params;

	start_params.timeout = timeout;
	start_params.num_events = num_events;

	err = bt_le_ext_adv_start(adv, &start_params);
	if (err) {
		FAIL("Failed to start advertiser (%d)\n", err);
	}

	LOG_DBG("Advertiser started");
}

static void set_ad_data(struct bt_le_ext_adv *adv)
{
	int err;

	uint8_t ead[sample_data->size_ead];
	struct bt_data ead_struct;
	size_t size_ad_data = sample_data->size_ad_data;
	size_t size_ead = BT_EAD_ENCRYPTED_PAYLOAD_SIZE(size_ad_data);

	if (size_ead != sample_data->size_ead) {
		LOG_ERR("Size of ead: %zu\n", size_ead);
		LOG_ERR("Size of sample_ead: %zu", sample_data->size_ead);
		FAIL("Computed size of encrypted data does not match the size of the encrypted "
		     "data from the sample. (data set %d)\n",
		     data_set);
	}

	err = bt_test_ead_encrypt(sample_data->session_key, sample_data->iv,
				  sample_data->randomizer_little_endian, sample_data->ad_data,
				  size_ad_data, ead);
	if (err != 0) {
		FAIL("Error during encryption.\n");
	} else if (memcmp(ead, sample_data->ead, sample_data->size_ead) != 0) {
		LOG_HEXDUMP_ERR(ead, size_ead, "Encrypted data from bt_ead_encrypt:");
		LOG_HEXDUMP_ERR(sample_data->ead, sample_data->size_ead,
				"Encrypted data from sample:");
		FAIL("Encrypted AD data does not match the ones provided in the sample. (data set "
		     "%d)\n",
		     data_set);
	}

	LOG_HEXDUMP_DBG(ead, size_ead, "Encrypted data:");

	ead_struct.data_len = size_ead;
	ead_struct.type = BT_DATA_ENCRYPTED_AD_DATA;
	ead_struct.data = ead;

	err = bt_le_ext_adv_set_data(adv, &ead_struct, 1, NULL, 0);
	if (err) {
		FAIL("Failed to set advertising data (%d)\n", err);
	}

	PASS("Peripheral test passed. (data set %d)\n", data_set);
}

void test_peripheral(void)
{
	int err;
	struct bt_le_ext_adv *adv = NULL;

	LOG_DBG("Peripheral device. (data set %d)", data_set);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
	}

	LOG_DBG("Bluetooth initialized");

	create_adv(&adv);
	start_adv(adv);

	set_ad_data(adv);
}
