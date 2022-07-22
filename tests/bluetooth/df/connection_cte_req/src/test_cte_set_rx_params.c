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

#include <bt_common.h>
#include <bt_conn_common.h>
#include "test_cte_set_rx_params.h"

static uint16_t g_conn_handle;

static uint8_t g_ant_ids[] = { 0x1, 0x2, 0x3, 0x4, 0x5 };

static struct ut_bt_df_conn_cte_rx_params g_params;

/* Macros delivering common values for unit tests */
#define CONN_HANDLE_INVALID (CONFIG_BT_MAX_CONN + 1)
#define ANTENNA_SWITCHING_SLOT_INVALID 0x3 /* BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US + 1 */

/* Antenna switch pattern length is stored in 1 octet. If BT Core spec. extends the max value to
 * UINT8_MAX expected failures may not be checked. If storage size is increased, tests shall be
 * updated.
 */
BUILD_ASSERT(CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN < UINT8_MAX,
	     "Can't test expected failures for switch pattern length longer or "
	     "equal to UINT8_MAX.");
#define SWITCH_PATTERN_LEN_TOO_LONG (CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN + 1)

BUILD_ASSERT(BT_HCI_LE_SWITCH_PATTERN_LEN_MIN > 0x0,
	     "Can't test expected failures for switch pattern length smaller or equal to zero.");
#define SWITCH_PATTERN_LEN_TOO_SHORT (BT_HCI_LE_SWITCH_PATTERN_LEN_MIN - 1)

int send_set_conn_cte_rx_params(uint16_t conn_handle,
				const struct ut_bt_df_conn_cte_rx_params *params, bool enable)
{
	struct bt_hci_cp_le_set_conn_cte_rx_params *cp;
	struct net_buf *buf;

	uint8_t ant_ids_num = (params != NULL ? params->switch_pattern_len : 0);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CONN_CTE_RX_PARAMS, sizeof(*cp) + ant_ids_num);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn_handle);
	cp->sampling_enable = enable ? 1 : 0;

	if (params != NULL) {
		cp->slot_durations = params->slot_durations;

		if (ant_ids_num) {
			uint8_t *dest_ant_ids = net_buf_add(buf, ant_ids_num);

			if (params->ant_ids) {
				(void)memcpy(dest_ant_ids, params->ant_ids, ant_ids_num);
			}
		}
		cp->switch_pattern_len = params->switch_pattern_len;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CONN_CTE_RX_PARAMS, buf, NULL);
}

ZTEST(test_hci_set_conn_cte_rx_params, test_set_conn_cte_rx_params_enable_with_invalid_conn_handle)
{
	int err;

	err = send_set_conn_cte_rx_params(CONN_HANDLE_INVALID, &g_params, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for set iq sampling params with wrong conn handle");
}

ZTEST(test_hci_set_conn_cte_rx_params, test_set_conn_cte_rx_params_enable_invalid_slot_durations)
{
	int err;

	g_params.slot_durations = ANTENNA_SWITCHING_SLOT_INVALID;

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for set iq sampling params with invalid slot "
		      "durations");
}

ZTEST(test_hci_set_conn_cte_rx_params,
	test_set_conn_cte_rx_params_enable_with_too_long_switch_pattern_len)
{
	int err;
	uint8_t ant_ids[SWITCH_PATTERN_LEN_TOO_LONG] = { 0 };

	g_params.switch_pattern_len = SWITCH_PATTERN_LEN_TOO_LONG;
	g_params.ant_ids = ant_ids;

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for set iq sampling params with switch pattern set "
		      "length beyond max value");
}

ZTEST(test_hci_set_conn_cte_rx_params,
	test_set_conn_cte_rx_params_enable_with_too_short_switch_pattern_len)
{
	int err;
	uint8_t ant_ids[SWITCH_PATTERN_LEN_TOO_SHORT] = { 0 };

	g_params.switch_pattern_len = SWITCH_PATTERN_LEN_TOO_SHORT;
	g_params.ant_ids = &ant_ids[0];

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for set iq sampling params with switch pattern set "
		      "length below min value");
}

ZTEST(test_hci_set_conn_cte_rx_params, test_set_conn_cte_rx_params_enable_with_ant_ids_ptr_null)
{
	int err;

	g_params.ant_ids = NULL;

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, true);
	/* If size of the command buffer equals to expected value, controller is not able to
	 * identify wrong or missing antenna IDs. It will use provided values as if they were
	 * valid antenna IDs.
	 */
	zassert_equal(err, 0,
		      "Unexpected error value for set iq sampling params with antenna ids "
		      "pointing NULL");
}

ZTEST(test_hci_set_conn_cte_rx_params, test_set_conn_cte_rx_params_enable_with_correct_params)
{
	int err;

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, true);
	zassert_equal(err, 0,
		      "Unexpected error value for set iq sampling params enabled with "
		      "correct params");
}

ZTEST(test_hci_set_conn_cte_rx_params, test_set_conn_cte_rx_params_disable_with_correct_params)
{
	int err;

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, false);
	zassert_equal(err, 0,
		      "Unexpected error value for set iq sampling params disable with "
		      "correct params");
}

ZTEST(test_hci_set_conn_cte_rx_params,
	test_set_conn_cte_rx_params_disable_with_invalid_slot_duration)
{
	int err;

	g_params.slot_durations = ANTENNA_SWITCHING_SLOT_INVALID;

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, false);
	zassert_equal(err, 0,
		      "Unexpected error value for set iq sampling params disable with "
		      "invalid slot durations");
}

ZTEST(test_hci_set_conn_cte_rx_params,
	test_set_conn_cte_rx_params_disable_with_too_long_switch_pattern_len)
{
	int err;
	uint8_t ant_ids[SWITCH_PATTERN_LEN_TOO_LONG] = { 0 };

	g_params.switch_pattern_len = SWITCH_PATTERN_LEN_TOO_LONG;
	g_params.ant_ids = ant_ids;

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, false);
	zassert_equal(err, 0,
		      "Unexpected error value for set iq sampling params disable with "
		      "switch pattern length above max value");
}

ZTEST(test_hci_set_conn_cte_rx_params,
	test_set_conn_cte_rx_params_disable_with_too_short_switch_pattern_len)
{
	int err;
	uint8_t ant_ids[SWITCH_PATTERN_LEN_TOO_SHORT] = { 0 };

	g_params.switch_pattern_len = SWITCH_PATTERN_LEN_TOO_SHORT;
	g_params.ant_ids = ant_ids;

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, false);
	zassert_equal(err, 0,
		      "Unexpected error value for set iq sampling params disable with "
		      "switch pattern length below min value");
}

ZTEST(test_hci_set_conn_cte_rx_params, test_set_conn_cte_rx_params_disable_with_ant_ids_ptr_null)
{
	int err;

	g_params.ant_ids = NULL;

	err = send_set_conn_cte_rx_params(g_conn_handle, &g_params, false);
	zassert_equal(err, 0,
		      "Unexpected error value for set iq sampling params disable with "
		      "antenna ids pointing NULL");
}

static void connection_setup(void *data)
{
	g_params.slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US;
	g_params.switch_pattern_len = ARRAY_SIZE(g_ant_ids);
	g_params.ant_ids = g_ant_ids;

	g_conn_handle = ut_bt_create_connection();
}

static void connection_teardown(void *data) { ut_bt_destroy_connection(g_conn_handle); }

ZTEST_SUITE(test_hci_set_conn_cte_rx_params, NULL, ut_bt_setup, connection_setup,
	    connection_teardown, ut_bt_teardown);
