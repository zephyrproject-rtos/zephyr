/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>

#include "babblekit/testcase.h"

LOG_MODULE_DECLARE(bt_bsim_id_settings, LOG_LEVEL_DBG);

void run_dut1(void)
{
	int err;
	size_t bt_id_count;

	LOG_DBG("Starting DUT 1");

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
	}

	LOG_DBG("Bluetooth initialised");

	err = settings_load();
	if (err) {
		TEST_FAIL("Failed to load settings (err %d)", err);
	}

	bt_id_get(NULL, &bt_id_count);
	LOG_DBG("Number of Bluetooth identities after settings load: %d", bt_id_count);

	err = bt_id_create(NULL, NULL);
	if (err < 0) {
		TEST_FAIL("Failed to create a new identity (err %d)", err);
	}

	bt_id_get(NULL, &bt_id_count);
	LOG_DBG("Number of Bluetooth identities after identity creation: %d", bt_id_count);

	/* Wait for the workqueue to complete before switching device */
	k_msleep(100);

	TEST_PASS("Test passed (DUT 1)");
}

void run_dut2(void)
{
	int err;
	size_t bt_id_count;
	size_t expected_id_count = 2;

	LOG_DBG("Starting DUT 2");

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
	}

	LOG_DBG("Bluetooth initialised");

	err = settings_load();
	if (err) {
		TEST_FAIL("Failed to load settings (err %d)", err);
	}

	LOG_DBG("Settings loaded");

	bt_id_get(NULL, &bt_id_count);
	if (bt_id_count != expected_id_count) {
		TEST_FAIL("Wrong ID count (got %d; expected %d)", bt_id_count, expected_id_count);
	}

	TEST_PASS("Test passed (DUT 2)");
}
