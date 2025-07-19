/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>

 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kpp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <soc.h>
#include <autoconf.h>
#include <inttypes.h>
#include <zephyr/drivers/gpio.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(kpp))
#define KPP_NODE DT_ALIAS(kpp)
#else
#error "Could not find an KPP compatible device in DT"
#endif

#define TEST_KPP_ACTIVE_COLUMROWS (0xFF)

#define WAIT_FOR_IDLE_TIMEOUT_US (5 * USEC_PER_SEC)

ZTEST_SUITE(kpp_scan, NULL, NULL, NULL, NULL, NULL);

volatile bool g_kpp_interrupt;
uint8_t read_keys[INPUT_KPP_COLUMNNUM_MAX] = {0};

static const struct device *test_dev = DEVICE_DT_GET(KPP_NODE);

static void kpp_scan_wait_for_idle(void)
{
	k_busy_wait(1000);

	if	(!g_kpp_interrupt) {
		zassert_true((!g_kpp_interrupt), "timeout waiting for idle state");
	}
}

static void kpp_scan_data_check(uint8_t *data)
{
	int row_data_mask;

	for (int i = 0; i < INPUT_KPP_COLUMNNUM_MAX; i++) {
		if (data[i] != 0) {
			row_data_mask = data[i];
			for	(int j = 0; j < INPUT_KPP_ROWNUM_MAX; j++) {
				if ((row_data_mask & (1 << j)) == 0) {
					continue;
				}
				TC_PRINT("Key pressed at row %d, column %d", j, i);
			}
		}
	}

}

static void kpp_cb(const struct device *dev)
{
	input_kpp_scan(dev, &read_keys[0]);
	g_kpp_interrupt = true;
}

ZTEST(kpp_scan, test_kpp_keypress_interrupt)
{
	input_kpp_set_callback(test_dev, kpp_cb);
	input_kpp_config(test_dev, TEST_KPP_ACTIVE_COLUMROWS,
		TEST_KPP_ACTIVE_COLUMROWS, KPP_EVENT_KEY_DPRESS);

	kpp_scan_wait_for_idle();
	zexpect_true(g_kpp_interrupt);
	g_kpp_interrupt = false;

	kpp_scan_data_check(&read_keys[0]);
}

ZTEST(kpp_scan, test_kppkeyrelease_interrupt)
{
	input_kpp_set_callback(test_dev, kpp_cb);
	input_kpp_config(test_dev, TEST_KPP_ACTIVE_COLUMROWS,
		TEST_KPP_ACTIVE_COLUMROWS, KPP_EVENT_KEY_RELEASE);

	kpp_scan_wait_for_idle();
	zexpect_true(g_kpp_interrupt);
	g_kpp_interrupt = false;
}
