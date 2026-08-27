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

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "hal/ccm.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll/lll_adv_internal.h"
#include "lll_adv_sync.h"
#include "lll/lll_df_types.h"

#include "ull_adv_types.h"
#include "ull_adv_internal.h"

#include "ll.h"
#include "common.h"

#define TEST_ADV_SET_HANDLE 0
#define TEST_PER_ADV_CHAIN_LENGTH 5
#define TEST_PER_ADV_SINGLE_PDU 1
#define TEST_CTE_COUNT 3
#define TEST_CTE_SINGLE 1
/* It does not matter for purpose of this tests what is the type or length of CTE used. */
#define TEST_CTE_TYPE BT_HCI_LE_AOD_CTE_2US

ZTEST(test_add_cte_to_per_adv_chain, test_add_number_of_cte_to_sigle_pdu_chain)
{
	struct ll_adv_set *adv;
	uint8_t handle;
	int err;

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	common_prepare_df_cfg(adv, TEST_CTE_COUNT);
	common_create_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);
	common_validate_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);

	handle = ull_adv_handle_get(adv);

	err = ll_df_set_cl_cte_tx_enable(handle, true);
	zassert_equal(err, 0,
		      "Unexpected error while enabling CTE for periodic advertising chain, err: %d",
		      err);

	/* Validate result */
	common_validate_chain_with_cte(adv, TEST_CTE_COUNT, TEST_PER_ADV_SINGLE_PDU);

	common_teardown(adv);
}

ZTEST(test_add_cte_to_per_adv_chain, test_add_cte_for_each_pdu_in_chain)
{
	struct ll_adv_set *adv;
	uint8_t handle;
	int err;

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	/* Use the same number for PDUs in a chain as for CTE request */
	common_prepare_df_cfg(adv, TEST_CTE_COUNT);
	common_create_per_adv_chain(adv, TEST_CTE_COUNT);
	common_validate_per_adv_chain(adv, TEST_CTE_COUNT);

	handle = ull_adv_handle_get(adv);

	err = ll_df_set_cl_cte_tx_enable(handle, true);
	zassert_equal(err, 0,
		      "Unexpected error while enabling CTE for periodic advertising chain, err: %d",
		      err);

	/* Validate result */
	common_validate_chain_with_cte(adv, TEST_CTE_COUNT, TEST_CTE_COUNT);

	common_teardown(adv);
}

ZTEST(test_add_cte_to_per_adv_chain, test_add_cte_for_not_all_pdu_in_chain)
{
	struct ll_adv_set *adv;
	uint8_t handle;
	int err;

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	/* Use the same number for PDUs in a chain as for CTE request */
	common_prepare_df_cfg(adv, TEST_CTE_COUNT);
	common_create_per_adv_chain(adv, TEST_PER_ADV_CHAIN_LENGTH);
	common_validate_per_adv_chain(adv, TEST_PER_ADV_CHAIN_LENGTH);

	handle = ull_adv_handle_get(adv);

	err = ll_df_set_cl_cte_tx_enable(handle, true);
	zassert_equal(err, 0,
		      "Unexpected error while enabling CTE for periodic advertising chain, err: %d",
		      err);

	/* Validate result */
	common_validate_chain_with_cte(adv, TEST_CTE_COUNT, TEST_PER_ADV_CHAIN_LENGTH);

	common_teardown(adv);
}

ZTEST(test_add_cte_to_per_adv_chain, test_add_cte_to_not_all_pdus_in_chain_enqueued_to_lll)
{
	struct pdu_adv *pdu_prev, *pdu_new;
	struct ll_adv_set *adv;
	uint8_t handle;
	uint8_t upd;
	int err;

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	/* Use the same number for PDUs in a chain as for CTE request */
	common_prepare_df_cfg(adv, TEST_CTE_COUNT);
	common_create_per_adv_chain(adv, TEST_PER_ADV_CHAIN_LENGTH);
	common_validate_per_adv_chain(adv, TEST_PER_ADV_CHAIN_LENGTH);

	/* Swap PDU double buffer and get new latest PDU data */
	pdu_new = lll_adv_sync_data_latest_get(adv->lll.sync, NULL, &upd);
	zassert_not_equal(pdu_new, NULL,
			  "Unexpected value of new PDU pointer after PDU double buffer swap");

	pdu_prev = lll_adv_sync_data_peek(adv->lll.sync, NULL);
	zassert_equal(pdu_prev, pdu_new,
		      "Unexpected value of previous PDU pointer after PDU double buffer swap");

	handle = ull_adv_handle_get(adv);

	err = ll_df_set_cl_cte_tx_enable(handle, true);
	zassert_equal(err, 0,
		      "Unexpected error while enabling CTE for periodic advertising chain, err: %d",
		      err);

	/* Validate result */
	common_validate_chain_with_cte(adv, TEST_CTE_COUNT, TEST_PER_ADV_CHAIN_LENGTH);

	common_teardown(adv);
}

ZTEST(test_add_cte_to_per_adv_chain, test_add_cte_for_single_pdu_chain)
{
	struct ll_adv_set *adv;
	uint8_t handle;
	int err;

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	/* Use the same number for PDUs in a chain as for CTE request */
	common_prepare_df_cfg(adv, TEST_CTE_SINGLE);
	common_create_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);
	common_validate_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);

	handle = ull_adv_handle_get(adv);

	err = ll_df_set_cl_cte_tx_enable(handle, true);
	zassert_equal(err, 0,
		      "Unexpected error while enabling CTE for periodic advertising chain, err: %d",
		      err);

	/* Validate result */
	common_validate_chain_with_cte(adv, TEST_CTE_SINGLE, TEST_PER_ADV_SINGLE_PDU);

	common_teardown(adv);
}

ZTEST_SUITE(test_add_cte_to_per_adv_chain, NULL, NULL, NULL, NULL, NULL);
