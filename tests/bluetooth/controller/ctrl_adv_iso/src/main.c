/*
 * Copyright (c) 2026 Yago Fontoura do Rosario
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/hci_types.h>

#include "util/memq.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "ull_adv_iso_internal.h"

static uint32_t interval_calc(uint32_t sdu_interval_us, uint16_t max_sdu, uint8_t max_pdu,
			      bool framed, uint8_t exp_bn_event, uint8_t exp_bn)
{
	uint8_t bn_event = 0U;
	uint8_t bn = 0U;
	uint32_t iso_interval_us;

	iso_interval_us = ull_adv_iso_interval_calc(sdu_interval_us, max_sdu, max_pdu, framed,
						    &bn_event, &bn);
	if (iso_interval_us != 0U) {
		zassert_equal(bn_event, exp_bn_event);
		zassert_equal(bn, exp_bn);
	}

	return iso_interval_us;
}

ZTEST(test_adv_iso_interval_calc, test_one_sdu_per_event)
{
	/* ISO_Interval equal to SDU_Interval, whatever the maximum latency.
	 * With 10 ms SDUs a 65 ms Max_Transport_Latency used to give 5 SDUs per
	 * event, i.e. a 50 ms ISO_Interval and 125 ms of transport latency.
	 */
	zassert_equal(interval_calc(10000U, 100U, 100U, false, 1U, 1U), 10000U);
	zassert_equal(interval_calc(7500U, 100U, 100U, false, 1U, 1U), 7500U);
	zassert_equal(interval_calc(10000U, 100U, 105U, true, 1U, 1U), 10000U);
	zassert_equal(interval_calc(0xFFFFFU, 100U, 105U, true, 1U, 1U), 1047500U);
}

ZTEST(test_adv_iso_interval_calc, test_minimum_iso_interval)
{
	/* An SDU_Interval of exactly the minimum ISO_Interval (5 ms) fits */
	zassert_equal(interval_calc(5000U, 100U, 100U, false, 1U, 1U), 5000U);

	/* Shorter SDU_Intervals pack just enough SDUs to reach 5 ms */
	zassert_equal(interval_calc(2500U, 100U, 100U, false, 2U, 2U), 5000U);
	zassert_equal(interval_calc(1250U, 100U, 100U, false, 4U, 4U), 5000U);
	zassert_equal(interval_calc(4999U, 100U, 105U, true, 2U, 2U), 8750U);
}

ZTEST(test_adv_iso_interval_calc, test_unframed_interval_multiple)
{
	/* 6 ms does not round to 1.25 ms units: 5 SDUs give 30 ms */
	zassert_equal(interval_calc(6000U, 100U, 100U, false, 5U, 5U), 30000U);

	/* No multiple of 4.999 ms within BN rounds to 1.25 ms units */
	zassert_equal(interval_calc(4999U, 100U, 100U, false, 0U, 0U), 0U);
}

ZTEST(test_adv_iso_interval_calc, test_capped_bn)
{
	/* 8 PDUs per SDU exceed BN 7: one SDU would give a 3.75 ms
	 * ISO_Interval, two SDUs span three BIG events of 5 ms.
	 */
	zassert_equal(interval_calc(7500U, 200U, 27U, true, 7U, 21U), 5000U);

	/* An unframed SDU cannot span BIG events */
	zassert_equal(interval_calc(7500U, 200U, 27U, false, 0U, 0U), 0U);
}

ZTEST_SUITE(test_adv_iso_interval_calc, NULL, NULL, NULL, NULL, NULL);
