/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <host/hci_core.h>

#include "common.h"
#include "test_set_iq_sampling_enable.h"

struct ut_bt_df_scan_cte_rx_params {
	uint8_t slot_durations;
	uint8_t cte_count;
	uint8_t num_ant_ids;
	uint8_t *ant_ids;
};

static uint8_t g_ant_ids[] = { 0x1, 0x2, 0x3, 0x4, 0x5 };

static struct ut_bt_df_scan_cte_rx_params g_params = {
	.slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US,
	.cte_count = BT_HCI_LE_SAMPLE_CTE_ALL,
	.num_ant_ids = ARRAY_SIZE(g_ant_ids),
	.ant_ids = &g_ant_ids[0]
};

/* Macros delivering common values for unit tests */
#define SYNC_HANDLE_INVALID (CONFIG_BT_PER_ADV_SYNC_MAX + 1)
#define ANTENNA_SWITCHING_SLOT_INVALID                                         \
	(BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US + 1)
#define CTE_COUNT_INVALID (BT_HCI_LE_SAMPLE_CTE_COUNT_MAX + 1)
#define SWITCH_PATTERN_LEN_INVALID                                             \
	(CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN + 1)
/* @brief Function sends HCI_LE_Set_Connectionless_CTE_Sampling_Enable
 *        to controller.
 *
 * @param sync_handle                 Handle of sync set.
 * @param sync_flags                  Flags related with sync set.
 * @param params                      CTE Sampling parameters.
 * @param enable                      Enable or disable CTE RX
 *
 * @return Zero if success, non-zero value in case of failure.
 */
int send_set_scan_cte_rx_enable(uint16_t sync_handle,
				struct ut_bt_df_scan_cte_rx_params *params,
				bool enable)
{
	struct bt_hci_cp_le_set_cl_cte_sampling_enable *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CL_CTE_SAMPLING_ENABLE,
				sizeof(*cp) + params->num_ant_ids);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));
	cp->sync_handle = sys_cpu_to_le16(sync_handle);
	cp->sampling_enable = enable ? 1 : 0;

	cp->slot_durations = params->slot_durations;
	cp->max_sampled_cte = params->cte_count;
	if (params->num_ant_ids) {
		if (params->ant_ids) {
			uint8_t *dest_ant_ids =
				net_buf_add(buf, params->num_ant_ids);

			memcpy(dest_ant_ids, params->ant_ids,
			       params->num_ant_ids);
		}
		cp->switch_pattern_len = params->num_ant_ids;

	} else {
		cp->switch_pattern_len = 0;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CL_CTE_SAMPLING_ENABLE,
				    buf, NULL);
}

void test_set_scan_cte_rx_enable_invalid_scan_set_handle(void)
{
	int err;

	err = send_set_scan_cte_rx_enable(SYNC_HANDLE_INVALID, &g_params, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for enable IQ sampling with wrong sync handle");
}

void test_set_scan_cte_rx_enable_invalid_antenna_slots_value(void)
{
	int err;

	uint8_t slot_durations_prev = g_params.slot_durations;

	g_params.slot_durations = ANTENNA_SWITCHING_SLOT_INVALID;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for enable IQ sampling with wrong antenna switching"
		      " slots value");

	g_params.slot_durations = slot_durations_prev;
}

void test_set_scan_cte_rx_enable_invalid_antenna_pattern_num(void)
{
	int err;

	uint8_t num_ant_ids_prev = g_params.num_ant_ids;
	uint8_t *ant_ids_prev = g_params.ant_ids;

	uint8_t ant_ids[SWITCH_PATTERN_LEN_INVALID] = { 0 };

	g_params.num_ant_ids = SWITCH_PATTERN_LEN_INVALID;
	g_params.ant_ids = &ant_ids[0];
	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for enable IQ sampling with wrong number"
		      " of antenna ids.");

	g_params.num_ant_ids = num_ant_ids_prev;
	g_params.ant_ids = ant_ids_prev;
}

void test_set_scan_cte_rx_enable_invalid_cte_count_value(void)
{
	int err;

	uint8_t cte_count_prev = g_params.cte_count;

	g_params.cte_count = CTE_COUNT_INVALID;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for enable IQ sampling with wrong number of CTEs"
		      " to sample.");

	g_params.cte_count = cte_count_prev;
}

void test_set_scan_cte_rx_enable_with_slot_duration_2us(void)
{
	int err;

	uint8_t slot_durations_prev = g_params.slot_durations;

	g_params.slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, 0, "Unexpected error value for enable IQ sampling");

	g_params.slot_durations = slot_durations_prev;
}

void test_set_scan_cte_rx_enable_with_slot_duration_1us(void)
{
	int err;

	uint8_t slot_durations_prev = g_params.slot_durations;

	g_params.slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, 0,
		      "Unexpected error value for enable IQ sampling with 1us slot durations");

	g_params.slot_durations = slot_durations_prev;
}

void test_set_scan_cte_rx_enable_with_sample_cte_count_min(void)
{
	int err;

	uint8_t cte_count_prev = g_params.cte_count;

	g_params.cte_count = BT_HCI_LE_SAMPLE_CTE_COUNT_MIN;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, 0,
		      "Unexpected error value for enable IQ sampling with CTEs count set"
		      " to min value.");

	g_params.cte_count = cte_count_prev;
}

void test_set_scan_cte_rx_enable_with_sample_cte_count_max(void)
{
	int err;

	uint8_t cte_count_prev = g_params.cte_count;

	g_params.cte_count = BT_HCI_LE_SAMPLE_CTE_COUNT_MAX;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, 0,
		      "Unexpected error value for enable IQ sampling with CTEs count set"
		      " to max value.");

	g_params.cte_count = cte_count_prev;
}

void test_set_scan_cte_rx_enable_with_antenna_switch_patterns_min(void)
{
	int err;

	uint8_t num_ant_ids_prev = g_params.num_ant_ids;
	uint8_t *ant_ids_prev = g_params.ant_ids;
	uint8_t ant_ids[BT_HCI_LE_MAX_SWITCH_PATTERN_LEN_MIN] = { 0 };

	g_params.num_ant_ids = BT_HCI_LE_MAX_SWITCH_PATTERN_LEN_MIN;
	g_params.ant_ids = &ant_ids[0];
	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, 0,
		      "Unexpected error value for enable IQ sampling with min number of"
		      " antenna ids.");

	g_params.num_ant_ids = num_ant_ids_prev;
	g_params.ant_ids = ant_ids_prev;
}

void test_set_scan_cte_rx_enable_with_antenna_switch_patterns_max(void)
{
	int err;

	uint8_t num_ant_ids_prev = g_params.num_ant_ids;
	uint8_t *ant_ids_prev = g_params.ant_ids;
	uint8_t ant_ids[CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN] = { 0 };

	g_params.num_ant_ids = CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN;
	g_params.ant_ids = &ant_ids[0];
	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, 0,
		      "Unexpected error value for enable IQ sampling with max number of antenna"
		      " ids.");

	g_params.num_ant_ids = num_ant_ids_prev;
	g_params.ant_ids = ant_ids_prev;
}

void test_set_scan_cte_rx_disable_with_correct_sampling_parameters(void)
{
	int err;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, false);
	zassert_equal(err, 0, "Unexpected error value for disable IQ sampling.");
}

void test_set_scan_cte_rx_disable_with_invalid_sampling_parameters(void)
{
	int err;

	static struct ut_bt_df_scan_cte_rx_params params_invalid = {
		.slot_durations = ANTENNA_SWITCHING_SLOT_INVALID,
		.cte_count = CTE_COUNT_INVALID,
		.num_ant_ids = 0,
		.ant_ids = NULL
	};

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &params_invalid,
					  false);
	zassert_equal(err, 0, "Unexpected error value for disable IQ sampling.");
}

void test_set_scan_cte_rx_disable_when_disabled(void)
{
	int err;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, false);
	zassert_equal(err, 0,
		      "Unexpected error value for disable IQ sampling when it is disabled.");
}

void set_scan_cte_rx_enable_teardown(void)
{
	int err;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, false);
	zassert_equal(err, 0, "Unexpected error value for disable IQ sampling.");
}

void set_scan_cte_rx_disable_setup(void)
{
	int err;

	err = send_set_scan_cte_rx_enable(g_per_sync->handle, &g_params, true);
	zassert_equal(err, 0, "Unexpected error value for enable IQ sampling.");
}

void run_set_scan_cte_rx_enable_tests(void)
{
	ztest_test_suite(
		test_hci_set_scan_cte_rx_enable,
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_invalid_scan_set_handle,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_invalid_antenna_slots_value,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_invalid_antenna_pattern_num,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_invalid_cte_count_value,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_with_slot_duration_2us,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_with_slot_duration_1us,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_with_sample_cte_count_min,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_with_sample_cte_count_max,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_with_antenna_switch_patterns_min,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_enable_with_antenna_switch_patterns_max,
			unit_test_noop, set_scan_cte_rx_enable_teardown),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_disable_with_correct_sampling_parameters,
			set_scan_cte_rx_disable_setup, unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_set_scan_cte_rx_disable_with_invalid_sampling_parameters,
			set_scan_cte_rx_disable_setup, unit_test_noop),
		ztest_unit_test(test_set_scan_cte_rx_disable_when_disabled));
	ztest_run_test_suite(test_hci_set_scan_cte_rx_enable);
}
