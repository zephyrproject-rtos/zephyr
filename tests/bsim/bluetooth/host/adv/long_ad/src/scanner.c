/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>

#include "ad.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(scanner, LOG_LEVEL_INF);

DEFINE_FLAG(scan_received);

static bool data_parse_cb(struct bt_data *data, void *user_data)
{
	LOG_DBG("Type: %02x (size: %d)", data->type, data->data_len);
	LOG_HEXDUMP_DBG(data->data, data->data_len, "Data:");

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	LOG_DBG("Device found: %s (RSSI %d)", addr_str, rssi);

	if (ad->len > 0) {
		LOG_INF("Received AD of size %d", ad->len);

		TEST_ASSERT(ad->len == ARRAY_SIZE(test_ad2),
			    "Received %d bytes of Advertising Data %d bytes were expected", ad->len,
			    ARRAY_SIZE(test_ad2));

		if (memcmp(ad->data, test_ad2, ad->len) != 0) {
			LOG_HEXDUMP_ERR(ad->data, ad->len, "Received AD:");
			LOG_HEXDUMP_ERR(test_ad2, ARRAY_SIZE(test_ad2), "Expected AD:");
			TEST_FAIL("Received Advertising Data doesn't match expected ones");
		}
	}

	bt_data_parse(ad, data_parse_cb, NULL);

	SET_FLAG(scan_received);
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	LOG_DBG("Scanning successfully started");
}

void entrypoint_scanner(void)
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

	TEST_START("scanner");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	start_scan();

	LOG_DBG("Wait until we receive at least one AD");
	WAIT_FOR_FLAG(scan_received);

	TEST_PASS_AND_EXIT("scanner");
}
