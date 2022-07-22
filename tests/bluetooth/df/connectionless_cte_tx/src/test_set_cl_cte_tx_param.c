/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
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
#include "common.h"

/* Macros delivering common values for unit tests */
#define CTE_LEN_VALID                   (BT_HCI_LE_CTE_LEN_MIN + 5)
#define CTE_LEN_INVALID                 (BT_HCI_LE_CTE_LEN_MAX + 1)
#define CTE_COUNT_VALID                 (BT_HCI_LE_CTE_COUNT_MIN + 5)
#define CTE_COUNT_INVALID               (BT_HCI_LE_CTE_COUNT_MAX + 1)
#define ADV_HANDLE_INVALID              (CONFIG_BT_CTLR_ADV_AUX_SET + 1)
#define CTE_TYPE_INVALID                0xFF
#define ANT_SW_PATTERN_LEN_INVALID      \
	(CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN + 1)

/* Antenna identifiers */
static uint8_t g_ant_ids[] = { 0x1, 0x2, 0x3, 0x4, 0x5 };

/* @brief Function sends HCI_LE_Set_Connectionless_CTE_Transmit_Parameters
 *        to controller.
 *
 * @param[in]adv_handle                 Handle of advertising set.
 * @param[in]cte_len                    Length of CTE in 8us units.
 * @param[in]cte_type                   Type of CTE to be used for transmission.
 * @param[in]cte_count                  Number of CTE that should be transmitted
 *                                      during each periodic advertising
 *                                      interval.
 * @param[in]num_ant_ids                Number of antenna IDs in switching
 *                                      pattern. May be zero if CTE type is
 *                                      AoA.
 * @param[in]ant_ids                    Array of antenna IDs in a switching
 *                                      pattern. May be NULL if CTE type is AoA.
 *
 * @return Zero if success, non-zero value in case of failure.
 */
int send_set_cl_cte_tx_params(uint8_t adv_handle, uint8_t cte_len,
					  uint8_t cte_type, uint8_t cte_count,
					  uint8_t switch_pattern_len,
					  uint8_t *ant_ids)
{
	struct bt_hci_cp_le_set_cl_cte_tx_params *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CL_CTE_TX_PARAMS,
				sizeof(*cp) + switch_pattern_len);
	zassert_not_null(buf, "Failed to create HCI cmd object");

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = adv_handle;
	cp->cte_len = cte_len;
	cp->cte_type = cte_type;
	cp->cte_count = cte_count;

	uint8_t *dest_ant_ids = net_buf_add(buf, switch_pattern_len);

	if (switch_pattern_len > 0) {
		memcpy(dest_ant_ids, ant_ids, switch_pattern_len);
	}
	cp->switch_pattern_len = switch_pattern_len;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CL_CTE_TX_PARAMS,
				    buf, NULL);
}

ZTEST(test_set_cl_cte_tx_param, test_set_cl_cte_tx_params_with_correct_aod_2us)
{
	int err;

	err = send_set_cl_cte_tx_params(g_adv->handle,
					CTE_LEN_VALID,
					BT_HCI_LE_AOD_CTE_2US,
					CTE_COUNT_VALID,
					ARRAY_SIZE(g_ant_ids),
					g_ant_ids);
	zassert_equal(err, 0, "Set AoD 2us CTE parameters failed");
}

ZTEST(test_set_cl_cte_tx_param, test_set_cl_cte_tx_params_with_correct_aod_1us)
{
	int err;

	err = send_set_cl_cte_tx_params(g_adv->handle,
					CTE_LEN_VALID,
					BT_HCI_LE_AOD_CTE_1US,
					CTE_COUNT_VALID,
					ARRAY_SIZE(g_ant_ids),
					g_ant_ids);
	zassert_equal(err, 0, "Set AoD 1us CTE parameters failed");
}

ZTEST(test_set_cl_cte_tx_param, test_set_ct_cte_tx_params_correct_aoa)
{
	int err;

	err = send_set_cl_cte_tx_params(ADV_HANDLE_INVALID,
					CTE_LEN_VALID,
					BT_HCI_LE_AOA_CTE,
					CTE_COUNT_VALID,
					ARRAY_SIZE(g_ant_ids),
					g_ant_ids);
	zassert_equal(err, -EIO, "Set AoA CTE parameters failed");
}

ZTEST(test_set_cl_cte_tx_param, test_set_ct_cte_tx_params_correct_aoa_without_ant_pattern)
{
	int err;

	err = send_set_cl_cte_tx_params(ADV_HANDLE_INVALID,
					CTE_LEN_VALID,
					BT_HCI_LE_AOA_CTE,
					CTE_COUNT_VALID,
					0,
					NULL);
	zassert_equal(err, -EIO, "Set AoA CTE parameters failed");
}

ZTEST(test_set_cl_cte_tx_param, test_set_ct_cte_tx_params_wrong_adv_handle)
{
	int err;

	err = send_set_cl_cte_tx_params(ADV_HANDLE_INVALID,
					CTE_LEN_VALID,
					BT_HCI_LE_AOD_CTE_2US,
					CTE_COUNT_VALID,
					ARRAY_SIZE(g_ant_ids),
					g_ant_ids);
	zassert_equal(err, -EIO,
		      "Unexpected error value for invalid adv handle");
}

ZTEST(test_set_cl_cte_tx_param, test_set_ct_cte_tx_params_invalid_cte_len)
{
	int err;

	err = send_set_cl_cte_tx_params(g_adv->handle,
					CTE_LEN_INVALID,
					BT_HCI_LE_AOD_CTE_2US,
					CTE_COUNT_VALID,
					ARRAY_SIZE(g_ant_ids),
					g_ant_ids);
	zassert_equal(err, -EIO,
		      "Unexpected error value for invalid CTE length");
}

ZTEST(test_set_cl_cte_tx_param, test_set_ct_cte_tx_params_invalid_cte_type)
{
	int err;

	err = send_set_cl_cte_tx_params(g_adv->handle,
					CTE_LEN_VALID,
					CTE_TYPE_INVALID,
					CTE_COUNT_VALID,
					ARRAY_SIZE(g_ant_ids),
					g_ant_ids);
	zassert_equal(err, -EIO,
		      "Unexpected error value for invalid CTE type");
}

ZTEST(test_set_cl_cte_tx_param, test_set_ct_cte_tx_params_invalid_cte_count)
{
	int err;

	err = send_set_cl_cte_tx_params(g_adv->handle,
					CTE_LEN_VALID,
					BT_HCI_LE_AOD_CTE_2US,
					CTE_COUNT_INVALID,
					ARRAY_SIZE(g_ant_ids),
					g_ant_ids);
	zassert_equal(err, -EIO,
		      "Unexpected error value for invalid CTE count");
}

ZTEST(test_set_cl_cte_tx_param, test_set_ct_cte_tx_params_aod_2us_invalid_pattern_len)
{
	int err;

	err = send_set_cl_cte_tx_params(g_adv->handle,
					CTE_LEN_VALID,
					BT_HCI_LE_AOD_CTE_2US,
					CTE_COUNT_VALID,
					ANT_SW_PATTERN_LEN_INVALID,
					g_ant_ids);
	zassert_equal(err, -EIO,
		      "Unexpected error value for invalid switch pattern len");
}

ZTEST(test_set_cl_cte_tx_param, test_set_ct_cte_tx_params_aod_1us_invalid_pattern_len)
{
	int err;

	err = send_set_cl_cte_tx_params(g_adv->handle,
					CTE_LEN_VALID,
					BT_HCI_LE_AOD_CTE_1US,
					CTE_COUNT_VALID,
					ANT_SW_PATTERN_LEN_INVALID,
					g_ant_ids);
	zassert_equal(err, -EIO,
		      "Unexpected error value for invalid switch pattern len");
}

ZTEST(test_set_cl_cte_tx_param, test_set_ct_cte_tx_params_aoa_invalid_pattern_len)
{
	int err;

	err = send_set_cl_cte_tx_params(g_adv->handle,
					CTE_LEN_VALID,
					BT_HCI_LE_AOA_CTE,
					CTE_COUNT_VALID,
					ANT_SW_PATTERN_LEN_INVALID,
					g_ant_ids);
	zassert_equal(err, 0, "Unexpected error value for AoA");
}

static void *common_adv_set_setup(void)
{
	ut_bt_setup();

	common_create_adv_set();

	return NULL;
}

static void common_adv_set_teardown(void *data)
{
	common_delete_adv_set();

	ut_bt_teardown(data);
}

ZTEST_SUITE(test_set_cl_cte_tx_param, NULL, common_adv_set_setup, NULL, NULL,
	    common_adv_set_teardown);
