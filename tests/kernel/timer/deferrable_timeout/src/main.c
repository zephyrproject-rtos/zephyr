/*
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util_macro.h>

static struct k_timer lpi_non_def_timer;
static struct k_timer non_def_timer;
static struct k_timer def_timer;

uint32_t lpi_non_def_timer_expiry_cnt;
uint32_t non_def_timer_expiry_cnt;
uint32_t def_timer_expiry_cnt;

static void lpi_non_def_timer_expiry_cb(struct k_timer *timer)
{
	lpi_non_def_timer_expiry_cnt++;
}

static void non_def_timer_expiry_cb(struct k_timer *timer)
{
	non_def_timer_expiry_cnt++;
}

static void def_timer_expiry_cb(struct k_timer *timer)
{
	def_timer_expiry_cnt++;
}

ZTEST(deferrable_timeout, test_lpi_non_deferrable_timeout)
{
	const int lpi_non_def_time = 10;
	const int non_def_time = 5;
	const int def_time = 3;
	int32_t timeout_expiry = 0;
	k_ticks_t remaining_ticks = 0;
	uint8_t flags = 0;

	/* Initialize the timers */
	k_timer_init(&lpi_non_def_timer, lpi_non_def_timer_expiry_cb, NULL);
	k_timer_init(&non_def_timer, non_def_timer_expiry_cb, NULL);
	k_timer_init(&def_timer, def_timer_expiry_cb, NULL);

	/* Set the timers as LPI mode timers */
	k_timer_timeout_flags_set(&lpi_non_def_timer, K_TIMEOUT_LPI);
	k_timer_timeout_flags_set(&def_timer, K_TIMEOUT_DEFERRABLE | K_TIMEOUT_LPI);

	flags = k_timer_timeout_flags_get(&lpi_non_def_timer);

	/* Check LPI bit for non-deferrable timer*/
	zassert_true(IS_BIT_SET(flags, K_TIMEOUT_LPI_BIT), "LPI bit not set");

	flags = k_timer_timeout_flags_get(&def_timer);

	/* Check flag bits for deferrable timer */
	zassert_true(IS_BIT_SET(flags, K_TIMEOUT_DEFERRABLE_BIT), "Deferrable bit not set");
	zassert_true(IS_BIT_SET(flags, K_TIMEOUT_LPI_BIT), "LPI bit not set for def_timer");

	/* Start timers */
	k_timer_start(&lpi_non_def_timer, K_SECONDS(lpi_non_def_time), K_NO_WAIT);
	k_timer_start(&non_def_timer, K_SECONDS(non_def_time), K_NO_WAIT);
	k_timer_start(&def_timer, K_SECONDS(def_time), K_NO_WAIT);

	remaining_ticks = k_timer_remaining_ticks(&non_def_timer);
	timeout_expiry = k_get_next_non_deferrable_timeout_expiry(K_TIMEOUT_NON_DEFERRABLE);

	/* timeout_expiry should have expiry details of non_def_timer,
	 * as normal mode is selected to get next expiry.
	 */
	zassert_true((remaining_ticks >= timeout_expiry), "Incorrect Normal mode expiry");

	remaining_ticks = k_timer_remaining_ticks(&lpi_non_def_timer);
	timeout_expiry = k_get_next_non_deferrable_timeout_expiry(K_TIMEOUT_LPI);

	/* timeout_expiry should have expiry details of lpi_non_def_timer,
	 * as LPI mode is selected to get next expiry. And this might be the only
	 * LPI mode timer present in the timeout_list.
	 */
	zassert_true((remaining_ticks >= timeout_expiry), "Incorrect LPI mode expiry");

	/* Clear the bit */
	k_timer_timeout_flags_clear(&lpi_non_def_timer, K_TIMEOUT_LPI);
	flags = k_timer_timeout_flags_get(&lpi_non_def_timer);
	zassert_true(flags == 0, "Flag bit are not cleared");

	k_timer_stop(&lpi_non_def_timer);
	k_timer_stop(&non_def_timer);
	k_timer_stop(&def_timer);
}

ZTEST_SUITE(deferrable_timeout, NULL, NULL, NULL, NULL, NULL);
