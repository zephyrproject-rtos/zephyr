/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>
#include <host/hci_core.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
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

void test_remove_cte_from_chain_extended_to_tx_all_cte(void)
{
	struct ll_adv_set *adv;
	uint8_t handle;
	int err;

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	common_prepare_df_cfg(adv, TEST_CTE_COUNT);
	common_create_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);

	handle = ull_adv_handle_get(adv);

	ll_df_set_cl_cte_tx_enable(handle, true);

	err = ll_df_set_cl_cte_tx_enable(handle, false);
	zassert_equal(err, 0,
		      "Unexpected error while disabling CTE for periodic avertising chain, err: %d",
		      err);
	/* Validate result */
	common_validate_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);

	/* Teardown */
	common_release_per_adv_chain(adv);
	common_release_adv_set(adv);
}

void test_remove_cte_from_chain_where_each_pdu_includes_cte(void)
{
	struct ll_adv_set *adv;
	uint8_t handle;
	int err;

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	/* Use the same number for PDUs in a chain as for CTE request */
	common_prepare_df_cfg(adv, TEST_CTE_COUNT);
	common_create_per_adv_chain(adv, TEST_CTE_COUNT);

	handle = ull_adv_handle_get(adv);

	err = ll_df_set_cl_cte_tx_enable(handle, true);
	zassert_equal(err, 0,
		      "Unexpected error while enabling CTE for periodic avertising chain, err: %d",
		      err);

	err = ll_df_set_cl_cte_tx_enable(handle, false);
	zassert_equal(err, 0,
		      "Unexpected error while disabling CTE for periodic avertising chain, err: %d",
		      err);
	/* Validate result */
	common_validate_per_adv_chain(adv, TEST_CTE_COUNT);

	/* Teardown */
	common_release_per_adv_chain(adv);
	common_release_adv_set(adv);
}

void test_remove_cte_from_chain_with_more_pdu_than_cte(void)
{
	struct ll_adv_set *adv;
	uint8_t handle;
	int err;

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	/* Use the same number for PDUs in a chain as for CTE request */
	common_prepare_df_cfg(adv, TEST_CTE_COUNT);
	common_create_per_adv_chain(adv, TEST_PER_ADV_CHAIN_LENGTH);

	handle = ull_adv_handle_get(adv);

	ll_df_set_cl_cte_tx_enable(handle, true);

	err = ll_df_set_cl_cte_tx_enable(handle, false);
	zassert_equal(err, 0,
		      "Unexpected error while disabling CTE for periodic avertising chain, err: %d",
		      err);
	/* Validate result */
	common_validate_per_adv_chain(adv, TEST_PER_ADV_CHAIN_LENGTH);

	/* Teardown */
	common_release_per_adv_chain(adv);
	common_release_adv_set(adv);
}

void test_remove_cte_from_single_pdu_chain(void)
{
	struct ll_adv_set *adv;
	uint8_t handle;
	int err;

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	/* Use the same number for PDUs in a chain as for CTE request */
	common_prepare_df_cfg(adv, TEST_CTE_SINGLE);
	common_create_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);

	handle = ull_adv_handle_get(adv);

	ll_df_set_cl_cte_tx_enable(handle, true);

	err = ll_df_set_cl_cte_tx_enable(handle, false);
	zassert_equal(err, 0,
		      "Unexpected error while disabling CTE for periodic avertising chain, err: %d",
		      err);
	/* Validate result */
	common_validate_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);

	/* Teardown */
	common_release_per_adv_chain(adv);
	common_release_adv_set(adv);
}

void run_remove_cte_to_per_adv_chain_tests(void)
{
	ztest_test_suite(test_add_cte_to_per_adv_chain,
			 ztest_unit_test(test_remove_cte_from_chain_extended_to_tx_all_cte),
			 ztest_unit_test(test_remove_cte_from_chain_where_each_pdu_includes_cte),
			 ztest_unit_test(test_remove_cte_from_chain_with_more_pdu_than_cte),
			 ztest_unit_test(test_remove_cte_from_single_pdu_chain));
	ztest_run_test_suite(test_add_cte_to_per_adv_chain);
}
