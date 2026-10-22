/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_test.h>
#include <zephyr/ztest_assert.h>

ZTEST_SUITE(gap_test_suite, NULL, NULL, NULL, NULL, NULL);

static ZTEST(gap_test_suite, test_bt_gap_conversion_macros)
{
	zassert_equal(BT_GAP_ADV_INTERVAL_TO_US(0x0020U), 20000U);
	zassert_equal(BT_GAP_ADV_INTERVAL_TO_US(0x0021U), 20625U);
	zassert_equal(BT_GAP_ADV_INTERVAL_TO_US(0x0022U), 21250U);

	zassert_equal(BT_GAP_ADV_INTERVAL_TO_MS(0x0020U), 20U);
	/* Round down expected from 20.625 */
	zassert_equal(BT_GAP_ADV_INTERVAL_TO_MS(0x0021U), 20U);
	/* Round down expected from 21.250 */
	zassert_equal(BT_GAP_ADV_INTERVAL_TO_MS(0x0022U), 21U);

	zassert_equal(BT_GAP_ISO_INTERVAL_TO_US(0x0004U), 5000U);
	zassert_equal(BT_GAP_ISO_INTERVAL_TO_US(0x0005U), 6250U);
	zassert_equal(BT_GAP_ISO_INTERVAL_TO_US(0x0006U), 7500U);

	zassert_equal(BT_GAP_ISO_INTERVAL_TO_MS(0x0004U), 5U);
	/* Round down expected from 6.25 */
	zassert_equal(BT_GAP_ISO_INTERVAL_TO_MS(0x0005U), 6U);
	/* Round down expected from 7.50 */
	zassert_equal(BT_GAP_ISO_INTERVAL_TO_MS(0x0006U), 7U);

	zassert_equal(BT_GAP_PER_ADV_INTERVAL_TO_US(0x0008U), 10000U);
	zassert_equal(BT_GAP_PER_ADV_INTERVAL_TO_US(0x0009U), 11250U);
	zassert_equal(BT_GAP_PER_ADV_INTERVAL_TO_US(0x000aU), 12500U);

	zassert_equal(BT_GAP_PER_ADV_INTERVAL_TO_MS(0x0008U), 10U);
	/* Round down expected from 11.25 */
	zassert_equal(BT_GAP_PER_ADV_INTERVAL_TO_MS(0x0009U), 11U);
	/* Round down expected from 12.50 */
	zassert_equal(BT_GAP_PER_ADV_INTERVAL_TO_MS(0x000aU), 12U);

	zassert_equal(BT_GAP_US_TO_ADV_INTERVAL(20000U), 0x0020U);
	/* Round down expected from 33.60 */
	zassert_equal(BT_GAP_US_TO_ADV_INTERVAL(21000U), 0x0021U);
	/* Round down expected from 35.20 */
	zassert_equal(BT_GAP_US_TO_ADV_INTERVAL(22000U), 0x0023U);

	zassert_equal(BT_GAP_MS_TO_ADV_INTERVAL(20U), 0x0020U);
	/* Round down expected from 33.60 */
	zassert_equal(BT_GAP_MS_TO_ADV_INTERVAL(21U), 0x0021U);
	/* Round down expected from 35.20 */
	zassert_equal(BT_GAP_MS_TO_ADV_INTERVAL(22U), 0x0023U);

	zassert_equal(BT_GAP_US_TO_PER_ADV_INTERVAL(10000U), 0x0008U);
	/* Round down expected from 8.8 */
	zassert_equal(BT_GAP_US_TO_PER_ADV_INTERVAL(11000U), 0x0008U);
	/* Round down expected from 9.6 */
	zassert_equal(BT_GAP_US_TO_PER_ADV_INTERVAL(12000U), 0x0009U);

	zassert_equal(BT_GAP_MS_TO_PER_ADV_INTERVAL(10U), 0x0008U);
	/* Round down expected from 8.8 */
	zassert_equal(BT_GAP_MS_TO_PER_ADV_INTERVAL(11U), 0x0008U);
	/* Round down expected from 9.6 */
	zassert_equal(BT_GAP_MS_TO_PER_ADV_INTERVAL(12U), 0x0009U);

	zassert_equal(BT_GAP_MS_TO_PER_ADV_SYNC_TIMEOUT(4000U), 0x0190U);
	/* Round down expected from 400.5 */
	zassert_equal(BT_GAP_MS_TO_PER_ADV_SYNC_TIMEOUT(4005U), 0x0190U);

	zassert_equal(BT_GAP_US_TO_PER_ADV_SYNC_TIMEOUT(4000000U), 0x0190U);
	/* Round down expected from 400.5 */
	zassert_equal(BT_GAP_US_TO_PER_ADV_SYNC_TIMEOUT(4005000U), 0x0190U);

	zassert_equal(BT_GAP_US_TO_SCAN_INTERVAL(20000U), 0x0020U);
	/* Round down expected from 33.60 */
	zassert_equal(BT_GAP_US_TO_SCAN_INTERVAL(21000U), 0x0021U);
	/* Round down expected from 35.20 */
	zassert_equal(BT_GAP_US_TO_SCAN_INTERVAL(22000U), 0x0023U);

	zassert_equal(BT_GAP_MS_TO_SCAN_INTERVAL(20U), 0x0020U);
	/* Round down expected from 33.60 */
	zassert_equal(BT_GAP_MS_TO_SCAN_INTERVAL(21U), 0x0021U);
	/* Round down expected from 35.20 */
	zassert_equal(BT_GAP_MS_TO_SCAN_INTERVAL(22U), 0x0023U);

	zassert_equal(BT_GAP_US_TO_SCAN_WINDOW(20000U), 0x0020U);
	/* Round down expected from 33.60 */
	zassert_equal(BT_GAP_US_TO_SCAN_WINDOW(21000U), 0x0021U);
	/* Round down expected from 35.20 */
	zassert_equal(BT_GAP_US_TO_SCAN_WINDOW(22000U), 0x0023U);

	zassert_equal(BT_GAP_MS_TO_SCAN_WINDOW(20U), 0x0020U);
	/* Round down expected from 33.60 */
	zassert_equal(BT_GAP_MS_TO_SCAN_WINDOW(21U), 0x0021U);
	/* Round down expected from 35.20 */
	zassert_equal(BT_GAP_MS_TO_SCAN_WINDOW(22U), 0x0023U);

	zassert_equal(BT_GAP_US_TO_CONN_INTERVAL(10000U), 0x0008U);
	/* Round down expected from 8.8 */
	zassert_equal(BT_GAP_US_TO_CONN_INTERVAL(11000U), 0x0008U);
	/* Round down expected from 9.6 */
	zassert_equal(BT_GAP_US_TO_CONN_INTERVAL(12000U), 0x0009U);

	zassert_equal(BT_GAP_MS_TO_CONN_INTERVAL(10U), 0x0008U);
	/* Round down expected from 8.8 */
	zassert_equal(BT_GAP_MS_TO_CONN_INTERVAL(11U), 0x0008U);
	/* Round down expected from 9.6 */
	zassert_equal(BT_GAP_MS_TO_CONN_INTERVAL(12U), 0x0009U);

	zassert_equal(BT_GAP_MS_TO_CONN_TIMEOUT(4000U), 0x0190U);
	/* Round down expected from 400.5 */
	zassert_equal(BT_GAP_MS_TO_CONN_TIMEOUT(4005U), 0x0190U);

	zassert_equal(BT_GAP_US_TO_CONN_TIMEOUT(4000000U), 0x0190U);
	/* Round down expected from 400.5 */
	zassert_equal(BT_GAP_US_TO_CONN_TIMEOUT(4005000U), 0x0190U);

	zassert_equal(BT_GAP_US_TO_CONN_EVENT_LEN(20000U), 0x0020U);
	/* Round down expected from 33.60 */
	zassert_equal(BT_GAP_US_TO_CONN_EVENT_LEN(21000U), 0x0021U);
	/* Round down expected from 35.20 */
	zassert_equal(BT_GAP_US_TO_CONN_EVENT_LEN(22000U), 0x0023U);

	zassert_equal(BT_GAP_MS_TO_CONN_EVENT_LEN(20U), 0x0020U);
	/* Round down expected from 33.60 */
	zassert_equal(BT_GAP_MS_TO_CONN_EVENT_LEN(21U), 0x0021U);
	/* Round down expected from 35.20 */
	zassert_equal(BT_GAP_MS_TO_CONN_EVENT_LEN(22U), 0x0023U);
}
