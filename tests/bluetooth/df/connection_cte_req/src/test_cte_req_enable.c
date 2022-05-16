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

#include <bt_conn_common.h>
#include "test_cte_req_enable.h"
#include "test_cte_set_rx_params.h"

struct ut_bt_df_conn_cte_request_data {
	uint8_t cte_request_interval;
	uint8_t requested_cte_length;
	uint8_t requested_cte_type;
};

static uint16_t g_conn_handle;

static struct ut_bt_df_conn_cte_request_data g_data;

/* Macros delivering common values for unit tests */
#define CONN_HANDLE_INVALID (CONFIG_BT_MAX_CONN + 1)
#define CONN_PERIPH_LATENCY 7 /* arbitrary latency value */
#define REQUEST_INTERVAL_OK (CONN_PERIPH_LATENCY)
#define REQUEST_INTERVAL_TOO_LOW (CONN_PERIPH_LATENCY - 1)

/* CTE length is stored in 1 octet. If BT Core spec. extends the max value to UINT8_MAX
 * expected failures may not be checked. If storage size is increased, tests shall be updated.
 */
BUILD_ASSERT(BT_HCI_LE_CTE_LEN_MAX < UINT8_MAX,
	     "Can't test expected failures for CTE length value longer than or "
	     "equal to UINT8_MAX.");
#define REQUEST_CTE_LEN_TOO_LONG (BT_HCI_LE_CTE_LEN_MAX + 1)

BUILD_ASSERT(BT_HCI_LE_CTE_LEN_MIN > 0x0,
	     "Can't test expected failures for CTE length value smaller or equal to zero");
#define REQUEST_CTE_LEN_TOO_SHORT (BT_HCI_LE_CTE_LEN_MIN - 1)

/* Arbitrary value different than values allowed BT Core spec. */
#define REQUEST_CTE_TYPE_INVALID 0xFF

/* @brief Function sends HCI_LE_Set_Connectionless_CTE_Sampling_Enable
 *        to controller.
 *
 * @param conn_handle                 Connection instance handle.
 * @param data                        CTE request data.
 * @param enable                      Enable or disable CTE request.
 *
 * @return Zero if success, non-zero value in case of failure.
 */
int send_conn_cte_req_enable(uint16_t conn_handle,
			     const struct ut_bt_df_conn_cte_request_data *data, bool enable)
{
	struct bt_hci_cp_le_conn_cte_req_enable *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_CTE_REQ_ENABLE, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn_handle);
	cp->enable = enable ? 1 : 0;
	if (data != NULL) {
		cp->cte_request_interval = data->cte_request_interval;
		cp->requested_cte_length = data->requested_cte_length;
		cp->requested_cte_type = data->requested_cte_type;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CONN_CTE_REQ_ENABLE, buf, NULL);
}

void test_set_conn_cte_req_enable_invalid_conn_handle(void)
{
	int err;

	err = send_conn_cte_req_enable(CONN_HANDLE_INVALID, &g_data, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for CTE request enable with wrong conn handle");
}

void test_set_conn_cte_req_enable_before_set_rx_params(void)
{
	int err;

	err = send_conn_cte_req_enable(g_conn_handle, &g_data, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for CTE request enable before set rx params");
}

void test_set_conn_cte_req_enable_with_too_short_interval(void)
{
	int err;

	g_data.cte_request_interval = REQUEST_INTERVAL_TOO_LOW;

	err = send_conn_cte_req_enable(g_conn_handle, &g_data, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for CTE request enable with too short request"
		      " interval");
}

void test_set_conn_cte_req_enable_with_too_long_requested_length(void)
{
	int err;

	g_data.requested_cte_length = REQUEST_CTE_LEN_TOO_LONG;

	err = send_conn_cte_req_enable(g_conn_handle, &g_data, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for CTE request enable with too long requested CTE"
		      " length");
}

void test_set_conn_cte_req_enable_with_too_short_requested_length(void)
{
	int err;

	g_data.requested_cte_length = REQUEST_CTE_LEN_TOO_SHORT;

	err = send_conn_cte_req_enable(g_conn_handle, &g_data, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for CTE request enable with too short requested CTE"
		      " length");
}

void test_set_conn_cte_req_enable_with_invalid_cte_type(void)
{
	int err;

	g_data.requested_cte_type = REQUEST_CTE_LEN_TOO_LONG;

	err = send_conn_cte_req_enable(g_conn_handle, &g_data, true);
	zassert_equal(err, -EIO,
		      "Unexpected error value for CTE request enable with invalid CTE type");
}

void test_set_conn_cte_req_enable(void)
{
	int err;

	err = send_conn_cte_req_enable(g_conn_handle, &g_data, true);
	zassert_equal(err, 0, "Unexpected error value for CTE request enable");
}

void test_set_conn_cte_req_disable(void)
{
	int err;

	err = send_conn_cte_req_enable(g_conn_handle, NULL, false);
	zassert_equal(err, 0, "Unexpected error value for CTE request disable");
}
static void cte_req_params_set(void)
{
	g_data.cte_request_interval = REQUEST_INTERVAL_OK;
	g_data.requested_cte_length = BT_HCI_LE_CTE_LEN_MAX;
	g_data.requested_cte_type = BT_HCI_LE_AOD_CTE_2US;
}

static void connection_setup(void)
{
	cte_req_params_set();

	g_conn_handle = ut_bt_create_connection();
}

static void connection_teardown(void)
{
	ut_bt_destroy_connection(g_conn_handle);
}

static void cte_rx_param_setup(void)
{
	/* Arbitrary antenna IDs. May be random for test purposes. */
	static uint8_t ant_ids[] = { 0x1, 0x2, 0x3, 0x4, 0x5 };

	/* Use arbitrary values that allow enable CTE receive and sampling. */
	struct ut_bt_df_conn_cte_rx_params cte_rx_params = {
		.slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US,
		.switch_pattern_len = ARRAY_SIZE(ant_ids),
		.ant_ids = ant_ids
	};

	cte_req_params_set();

	g_conn_handle = ut_bt_create_connection();
	ut_bt_set_periph_latency(g_conn_handle, CONN_PERIPH_LATENCY);

	send_set_conn_cte_rx_params(g_conn_handle, &cte_rx_params, true);
}

static void cte_req_setup(void)
{
	cte_rx_param_setup();

	send_conn_cte_req_enable(g_conn_handle, &g_data, true);
}

static void cte_rx_param_teardown(void)
{
	connection_teardown();

	send_set_conn_cte_rx_params(g_conn_handle, NULL, false);
}

static void cte_req_teardown(void)
{
	send_conn_cte_req_enable(g_conn_handle, NULL, false);

	cte_rx_param_teardown();
}

void run_cte_request_enable_tests(void)
{
	ztest_test_suite(
		test_hci_set_conn_cte_rx_params,
		ztest_unit_test(test_set_conn_cte_req_enable_invalid_conn_handle),
		ztest_unit_test_setup_teardown(test_set_conn_cte_req_enable_before_set_rx_params,
					       connection_setup, connection_teardown),
		ztest_unit_test_setup_teardown(test_set_conn_cte_req_enable_with_too_short_interval,
					       cte_rx_param_setup, cte_rx_param_teardown),
		ztest_unit_test_setup_teardown(
			test_set_conn_cte_req_enable_with_too_short_requested_length,
			cte_rx_param_setup, cte_rx_param_teardown),
		ztest_unit_test_setup_teardown(
			test_set_conn_cte_req_enable_with_too_long_requested_length,
			cte_rx_param_setup, cte_rx_param_teardown),
		ztest_unit_test_setup_teardown(test_set_conn_cte_req_enable, cte_rx_param_setup,
					       cte_rx_param_teardown),
		ztest_unit_test_setup_teardown(test_set_conn_cte_req_disable, cte_req_setup,
					       cte_req_teardown));
	ztest_run_test_suite(test_hci_set_conn_cte_rx_params);

	/* TODO: Add tests cases that verify positive behavior of the function
	 * HCI_LE_Connection_CTE_Request_Enable.
	 */
}
