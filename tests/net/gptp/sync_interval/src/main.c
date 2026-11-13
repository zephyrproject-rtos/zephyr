/*
 * Copyright (c) 2026 Axonne Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/ztest.h>

#include "gptp_messages.h"
#include "gptp_data_set.h"

bool gptp_itv_request_check_valid(int8_t log_itv);
void gptp_mi_setup_sync_send_time(uint64_t current_time);

static void setup_sync_send_time(uint16_t nb_ports, uint64_t current_time)
{
	gptp_domain.default_ds.nb_ports = nb_ports;
	gptp_mi_setup_sync_send_time(current_time);
	gptp_domain.default_ds.nb_ports = 0U;
}

ZTEST(gptp_sync_interval, test_supported_requests_are_accepted)
{
	zassert_true(gptp_itv_request_check_valid(GPTP_ITV_LOG_MIN));
	zassert_true(gptp_itv_request_check_valid(0));
	zassert_true(gptp_itv_request_check_valid(GPTP_ITV_LOG_MAX));
	zassert_true(gptp_itv_request_check_valid(GPTP_ITV_SET_TO_INIT));
}

ZTEST(gptp_sync_interval, test_reserved_requests_are_ignored)
{
	zassert_false(gptp_itv_request_check_valid(-127));
	zassert_false(gptp_itv_request_check_valid(GPTP_ITV_LOG_MIN - 1));
	zassert_false(gptp_itv_request_check_valid(GPTP_ITV_LOG_MAX + 1));
	zassert_false(gptp_itv_request_check_valid(125));
}

ZTEST(gptp_sync_interval, test_keep_and_stop_requests_are_ignored)
{
	zassert_false(gptp_itv_request_check_valid(GPTP_ITV_KEEP));
	zassert_false(gptp_itv_request_check_valid(GPTP_ITV_STOP));
}

ZTEST(gptp_sync_interval, test_sync_send_time_starts_from_current_time)
{
	struct gptp_clk_master_sync_snd_state *state = &GPTP_STATE()->clk_master_sync_send;

	GPTP_PORT_DS(1)->cur_log_half_sync_itv = 0;
	state->sync_send_time.low = 0U;

	setup_sync_send_time(1U, 5U * NSEC_PER_SEC);

	zassert_equal(GPTP_GLOBAL_DS()->clk_master_sync_itv, 2U * NSEC_PER_SEC);
	zassert_equal(state->sync_send_time.low, 5U * NSEC_PER_SEC + 2U * NSEC_PER_SEC);
}

ZTEST(gptp_sync_interval, test_sync_interval_is_port_minimum)
{
	struct gptp_clk_master_sync_snd_state *state = &GPTP_STATE()->clk_master_sync_send;

	GPTP_PORT_DS(1)->cur_log_half_sync_itv = -1;
	GPTP_PORT_DS(2)->cur_log_half_sync_itv = -3;
	state->sync_send_time.low = 0U;

	setup_sync_send_time(2U, 5U * NSEC_PER_SEC);

	zassert_equal(GPTP_GLOBAL_DS()->clk_master_sync_itv, NSEC_PER_SEC / 4U);
	zassert_equal(state->sync_send_time.low, 5U * NSEC_PER_SEC + NSEC_PER_SEC / 4U);
}

ZTEST(gptp_sync_interval, test_sync_interval_without_ports)
{
	setup_sync_send_time(0U, 0U);

	zassert_equal(GPTP_GLOBAL_DS()->clk_master_sync_itv,
		      BIT64(GPTP_ITV_LOG_MAX) * NSEC_PER_SEC);
}

ZTEST_SUITE(gptp_sync_interval, NULL, NULL, NULL, NULL, NULL);
