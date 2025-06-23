/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stddef.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <host/hci_core.h>

#include <bt_common.h>
#include <bt_conn_common.h>

static uint16_t g_conn_handle;

static const uint8_t g_ant_ids[] = { 0x1, 0x2, 0x3, 0x4, 0x5 };

/**
 * @brief Structure to store CTE receive parameters for unit tests setup.
 */
struct ut_bt_df_conn_cte_tx_params {
	uint8_t cte_types;
	uint8_t switch_pattern_len;
	const uint8_t *ant_ids;
};

static struct ut_bt_df_conn_cte_tx_params g_params;

/* Macros delivering common values for unit tests */
#define CONN_HANDLE_INVALID (CONFIG_BT_MAX_CONN + 1)
#define ANTENNA_SWITCHING_SLOT_INVALID 0x3 /* BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US + 1 */
#define CTE_TYPE_NONE_ALLOWED 0x0 /* Any bit is not set */
#define CTE_TYPE_INVALID                                                                           \
	(~(uint8_t)(BT_HCI_LE_AOA_CTE_RSP | BT_HCI_LE_AOD_CTE_RSP_1US |                            \
		    BT_HCI_LE_AOD_CTE_RSP_2US)) /* Any other than allowed bit is set */

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

static int send_set_conn_cte_tx_params(uint16_t conn_handle,
				       const struct ut_bt_df_conn_cte_tx_params *params)
{
	struct bt_hci_cp_le_set_conn_cte_tx_params *cp;
	uint8_t *dest_ant_ids;
	struct net_buf *buf;

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->handle = sys_cpu_to_le16(conn_handle);
	cp->cte_types = params->cte_types;

	dest_ant_ids = net_buf_add(buf, params->switch_pattern_len);

	if (params->ant_ids) {
		(void)memcpy(dest_ant_ids, params->ant_ids, params->switch_pattern_len);
	}
	cp->switch_pattern_len = params->switch_pattern_len;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CONN_CTE_TX_PARAMS, buf, NULL);
}

ZTEST(test_set_conn_cte_tx_params, test_set_conn_cte_tx_params_with_invalid_conn_handle)
{
	int err;

	err = send_set_conn_cte_tx_params(CONN_HANDLE_INVALID, &g_params);
	zassert_equal(err, -EIO,
		      "Unexpected error value for set conn CTE tx params with wrong conn handle");
}

ZTEST(test_set_conn_cte_tx_params, test_set_conn_cte_tx_params_with_cte_type_none)
{
	int err;

	g_params.cte_types = CTE_TYPE_NONE_ALLOWED;

	err = send_set_conn_cte_tx_params(g_conn_handle, &g_params);
	zassert_equal(err, -EIO,
		      "Unexpected error value for set conn CTE TX params with invalid slot "
		      "durations");
}

ZTEST(test_set_conn_cte_tx_params, test_set_conn_cte_tx_params_with_cte_type_invalid)
{
	int err;

	g_params.cte_types = CTE_TYPE_INVALID;

	err = send_set_conn_cte_tx_params(g_conn_handle, &g_params);
	zassert_equal(err, -EIO,
		      "Unexpected error value for set conn CTE TX params with invalid slot "
		      "durations");
}

ZTEST(test_set_conn_cte_tx_params, test_set_conn_cte_tx_params_with_too_long_switch_pattern_len)
{
	int err;
	uint8_t ant_ids[SWITCH_PATTERN_LEN_TOO_LONG] = { 0 };

	g_params.switch_pattern_len = SWITCH_PATTERN_LEN_TOO_LONG;
	g_params.ant_ids = ant_ids;

	err = send_set_conn_cte_tx_params(g_conn_handle, &g_params);
	zassert_equal(err, -EIO,
		      "Unexpected error value for set conn CTE TX params with switch pattern set "
		      "length beyond max value");
}

ZTEST(test_set_conn_cte_tx_params, test_set_conn_cte_tx_params_with_too_short_switch_pattern_len)
{
	int err;
	uint8_t ant_ids[SWITCH_PATTERN_LEN_TOO_SHORT] = { 0 };

	g_params.switch_pattern_len = SWITCH_PATTERN_LEN_TOO_SHORT;
	g_params.ant_ids = &ant_ids[0];

	err = send_set_conn_cte_tx_params(g_conn_handle, &g_params);
	zassert_equal(err, -EIO,
		      "Unexpected error value for set conn CTE TX params with switch pattern set "
		      "length below min value");
}

ZTEST(test_set_conn_cte_tx_params, test_set_conn_cte_tx_params_with_ant_ids_ptr_null)
{
	int err;

	g_params.ant_ids = NULL;

	err = send_set_conn_cte_tx_params(g_conn_handle, &g_params);
	/* If size of the command buffer equals to expected value, controller is not able to
	 * identify wrong or missing antenna IDs. It will use provided values as if they were
	 * valid antenna IDs.
	 */
	zassert_equal(err, 0,
		      "Unexpected error value for set conn CTE TX params with antenna ids "
		      "pointing NULL");
}

ZTEST(test_set_conn_cte_tx_params, test_set_conn_cte_tx_params_with_correct_params)
{
	int err;

	err = send_set_conn_cte_tx_params(g_conn_handle, &g_params);
	zassert_equal(err, 0,
		      "Unexpected error value for set conn CTE TX params enabled with correct "
		      "params");
}

static void connection_setup(void *data)
{
	g_params.cte_types =
		BT_HCI_LE_AOA_CTE_RSP | BT_HCI_LE_AOD_CTE_RSP_1US | BT_HCI_LE_AOD_CTE_RSP_2US;
	g_params.switch_pattern_len = ARRAY_SIZE(g_ant_ids);
	g_params.ant_ids = g_ant_ids;

	g_conn_handle = ut_bt_create_connection();
}

static void connection_teardown(void *data) { ut_bt_destroy_connection(g_conn_handle); }

ZTEST_SUITE(test_set_conn_cte_tx_params, NULL, ut_bt_setup, connection_setup, connection_teardown,
	    ut_bt_teardown);
