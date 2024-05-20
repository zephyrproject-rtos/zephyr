/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <sys/types.h>

#include "ad.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(advertiser, LOG_LEVEL_INF);

static void create_adv(struct bt_le_ext_adv **adv)
{
	int err;
	struct bt_le_adv_param params;

	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_EXT_ADV;

	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_SLOW_INT_MIN;
	params.interval_max = BT_GAP_ADV_SLOW_INT_MAX;

	err = bt_le_ext_adv_create(&params, NULL, adv);
	if (err) {
		TEST_FAIL("Failed to create advertiser (%d)\n", err);
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
		TEST_FAIL("Failed to start advertiser (%d)\n", err);
	}
}

static size_t ad_deserialize(const uint8_t *input, size_t input_size, struct bt_data output[])
{
	uint8_t type;
	uint8_t data_len;
	const uint8_t *data;

	size_t idx = 0;
	size_t ad_len = 0;

	while (idx < input_size) {
		data_len = input[idx] - 1;
		type = input[idx + 1];
		data = &input[idx + 2];

		if (data_len + 2 > input_size - idx) {
			TEST_FAIL("malformed advertising data, expected %d bytes of data but got "
				  "only %d "
				  "bytes",
				  data_len + 1, input_size - idx);
			return -1;
		}

		output[ad_len].data = data;
		output[ad_len].type = type;
		output[ad_len].data_len = data_len;

		ad_len += 1;
		idx += data_len + 2;
	}

	return ad_len;
}

static int set_ad_data(struct bt_le_ext_adv *adv, const uint8_t *serialized_ad,
			size_t serialized_ad_size)
{
	int err;
	size_t ad_len = 0;
	uint8_t max_ad_len = 10;

	struct bt_data ad[max_ad_len];

	ad_len = ad_deserialize(serialized_ad, serialized_ad_size, ad);

	err = bt_le_ext_adv_set_data(adv, ad, ad_len, NULL, 0);
	if (err != 0 && err != -EDOM) {
		TEST_FAIL("Failed to set advertising data (%d)\n", err);
	}

	return err;
}

void entrypoint_advertiser(void)
{
	/* Test purpose:
	 *
	 * Verifies that we can send Advertising Data up to the size set in the
	 * Kconfig. And if we try to set data too large we get the correct
	 * error code.
	 *
	 * Two devices:
	 * - `advertiser`: tries to send the data
	 * - `scanner`: will receive the data and check that they match with the
	 *   data sent
	 *
	 * Procedure:
	 * - [advertiser] try to use `test_ad1` as advertising data
	 * - [advertiser] get the expected error (adv or scan resp too large)
	 * - [advertiser] try to use `test_ad2` as advertising data
	 * - [advertiser] get a success
	 * - [advertiser] start advertiser
	 * - [scanner] start scanner
	 * - [scanner] wait until receiving advertising data matching `test_ad2`
	 *
	 * [verdict]
	 * - advertiser receives the correct error code when trying to set
	 *   advertising data
	 * - scanner receives the correct data in advertising data
	 */
	int err;
	struct bt_le_ext_adv *adv = NULL;

	TEST_START("advertiser");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	create_adv(&adv);
	LOG_DBG("Advertiser created");

	err = set_ad_data(adv, test_ad1, ARRAY_SIZE(test_ad1));
	TEST_ASSERT(err == -EDOM,
		    "Tried to set Advertising Data larger than the controller can accept, "
		    "expected failure with error code %d but got %d",
		    -EDOM, err);

	err = set_ad_data(adv, test_ad2, ARRAY_SIZE(test_ad2));
	TEST_ASSERT(err == 0,
		    "Tried to set Advertising Data as large as the maximum advertising data size "
		    "the controller can accept, expected success but got error code %d",
		    err);
	LOG_DBG("AD set");

	start_adv(adv);
	LOG_DBG("Advertiser started");

	TEST_PASS("advertiser");
}
