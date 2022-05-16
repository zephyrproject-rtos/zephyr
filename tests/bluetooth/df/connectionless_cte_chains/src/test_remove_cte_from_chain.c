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

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"
#include "util/dbuf.h"

#include "pdu.h"

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
#define TEST_CTE_COUNT 3
#define TEST_PER_ADV_CHAIN_LENGTH 5
#define TEST_PER_ADV_CHAIN_INCREASED_LENGTH 7
#define TEST_PER_ADV_CHAIN_DECREASED_LENGTH (TEST_CTE_COUNT - 1)
#define TEST_PER_ADV_SINGLE_PDU 1
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
		      "Unexpected error while disabling CTE for periodic advertising chain, err: %d",
		      err);
	/* Validate result */
	common_validate_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);

	common_teardown(adv);
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
		      "Unexpected error while enabling CTE for periodic advertising chain, err: %d",
		      err);

	err = ll_df_set_cl_cte_tx_enable(handle, false);
	zassert_equal(err, 0,
		      "Unexpected error while disabling CTE for periodic advertising chain, err: %d",
		      err);
	/* Validate result */
	common_validate_per_adv_chain(adv, TEST_CTE_COUNT);

	common_teardown(adv);
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
		      "Unexpected error while disabling CTE for periodic advertising chain, err: %d",
		      err);
	/* Validate result */
	common_validate_per_adv_chain(adv, TEST_PER_ADV_CHAIN_LENGTH);

	common_teardown(adv);
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
		      "Unexpected error while disabling CTE for periodic advertising chain, err: %d",
		      err);
	/* Validate result */
	common_validate_per_adv_chain(adv, TEST_PER_ADV_SINGLE_PDU);

	common_teardown(adv);
}

void remove_cte_from_chain_after_enqueue_to_lll(uint8_t cte_count, uint8_t init_chain_length,
						uint8_t expected_mem_pdu_used_count_for_enable,
						uint8_t expected_mem_pdu_used_count_for_disable,
						uint8_t expected_pdu_in_chain_after_cte_disable,
						uint8_t updated_chain_length,
						uint8_t expected_end_fifo_free_pdu_count,
						bool new_chain_before_cte_disable)
{
	uint32_t pdu_mem_cnt_expected, pdu_mem_cnt;
	struct pdu_adv *pdu_prev, *pdu_new;
	struct ll_adv_set *adv;
	uint32_t pdu_fifo_cnt;
	uint8_t handle;
	uint8_t upd;
	int err;

	pdu_mem_cnt_expected = lll_adv_pdu_mem_free_count_get();

	/* Setup for test */
	adv = common_create_adv_set(TEST_ADV_SET_HANDLE);
	/* Use smaller number of CTE for request than number of PDUs in an initial chain.
	 * In such situation chain should not be extended and PDUs pool should be not affected.
	 */
	common_prepare_df_cfg(adv, cte_count);
	common_create_per_adv_chain(adv, init_chain_length);

	handle = ull_adv_handle_get(adv);

	err = ll_df_set_cl_cte_tx_enable(handle, true);
	zassert_equal(err, 0,
		      "Unexpected error while enabling CTE for periodic advertising chain, err: %d",
		      err);

	/* Swap PDU double buffer and get new latest PDU data */
	pdu_new = lll_adv_sync_data_latest_get(adv->lll.sync, NULL, &upd);
	zassert_not_equal(pdu_new, NULL,
			  "Unexpected value of new PDU pointer after PDU double buffer swap");

	pdu_prev = lll_adv_sync_data_peek(adv->lll.sync, NULL);
	zassert_equal(pdu_prev, pdu_new,
		      "Unexpected value of previous PDU pointer after PDU double buffer swap");

	/* Free PDUs fifo should hold single PDU released during double buffer swap. The PDU
	 * was allocated during advertising set creation.
	 */
	pdu_fifo_cnt = lll_adv_free_pdu_fifo_count_get();
	zassert_equal(pdu_fifo_cnt, TEST_PER_ADV_SINGLE_PDU,
		      "Unexpected number of free PDUs in a fifo: %d", pdu_fifo_cnt);

	/* Expected free PDUs count is decreased by:
	 * - single PDU allocated during advertising set creation,
	 * - number of PDUs allocated for per. adv. chain to Tx CTE
	 */
	pdu_mem_cnt_expected -= expected_mem_pdu_used_count_for_enable;
	pdu_mem_cnt = lll_adv_pdu_mem_free_count_get();
	zassert_equal(pdu_mem_cnt, pdu_mem_cnt_expected,
		      "Unexpected amount of free PDUs memory: %d, expected %d", pdu_mem_cnt,
		      pdu_mem_cnt_expected);

	if (new_chain_before_cte_disable) {
		common_create_per_adv_chain(adv, updated_chain_length);
	}

	err = ll_df_set_cl_cte_tx_enable(handle, false);
	zassert_equal(err, 0,
		      "Unexpected error while disabling CTE for periodic advertising chain, err: %d",
		      err);
	/* Validate result */
	common_validate_per_adv_chain(adv, expected_pdu_in_chain_after_cte_disable);

	/* Swap PDU double buffer to check correctness of release former PDUs */
	pdu_new = lll_adv_sync_data_latest_get(adv->lll.sync, NULL, &upd);
	zassert_not_equal(pdu_new, NULL,
			  "Unexpected value of PDU pointer after PDU double buffer swap");

	/* Validate number of released PDUs */

	/* Number of free PDUs in a fifo is a number of released PDUs from former periodic
	 * advertising chain. One free PDU that had been in a fifo was used for allocation of
	 * the new PDU without CTE.
	 */
	pdu_fifo_cnt = lll_adv_free_pdu_fifo_count_get();
	zassert_equal(pdu_fifo_cnt, expected_end_fifo_free_pdu_count,
		      "Unexpected number of free PDUs in a fifo: %d", pdu_fifo_cnt);

	/* Number of free PDUs in memory pool may decreased. Single PDU for AUX_SYNC_IND was
	 * acquired from free PDUs fifo. The memory pool will decrease by number of non empty PDUs
	 * in a chain subtracted by 1 (PDU taken from free PDUs fifo).
	 */
	pdu_mem_cnt_expected -= expected_mem_pdu_used_count_for_disable;

	pdu_mem_cnt = lll_adv_pdu_mem_free_count_get();
	zassert_equal(pdu_mem_cnt, pdu_mem_cnt_expected,
		      "Unexpected amount of free PDUs memory: %d, expected %d", pdu_mem_cnt,
		      pdu_mem_cnt_expected);

	common_teardown(adv);
}

void test_remove_cte_from_chain_extended_to_tx_all_cte_after_enqueue_to_lll(void)
{
	uint8_t cte_count = TEST_CTE_COUNT;
	uint8_t init_chain_length = TEST_PER_ADV_SINGLE_PDU;
	uint8_t expected_mem_pdu_used_count_for_enable = TEST_CTE_COUNT + TEST_PER_ADV_SINGLE_PDU;
	uint8_t expected_mem_pdu_used_count_for_disable = 0;
	uint8_t expected_pdu_in_chain_after_cte_disable = TEST_PER_ADV_SINGLE_PDU;
	uint8_t updated_chain_length = 0;
	uint8_t expected_end_fifo_free_pdu_count = TEST_CTE_COUNT;
	bool new_chain_before_cte_disable = false;

	remove_cte_from_chain_after_enqueue_to_lll(
		cte_count, init_chain_length, expected_mem_pdu_used_count_for_enable,
		expected_mem_pdu_used_count_for_disable, expected_pdu_in_chain_after_cte_disable,
		updated_chain_length, expected_end_fifo_free_pdu_count,
		new_chain_before_cte_disable);
}

void test_remove_cte_from_chain_with_more_pdu_than_cte_after_enqueue_to_lll(void)
{
	uint8_t cte_count = TEST_CTE_COUNT;
	uint8_t init_chain_length = TEST_PER_ADV_CHAIN_LENGTH;
	uint8_t expected_mem_pdu_used_count_for_enable =
		TEST_PER_ADV_CHAIN_LENGTH + TEST_PER_ADV_SINGLE_PDU;
	uint8_t expected_mem_pdu_used_count_for_disable =
		TEST_PER_ADV_CHAIN_LENGTH - TEST_PER_ADV_SINGLE_PDU;
	uint8_t expected_pdu_in_chain_after_cte_disable = TEST_PER_ADV_CHAIN_LENGTH;
	uint8_t updated_chain_length = 0;
	uint8_t expected_end_fifo_free_pdu_count = TEST_PER_ADV_CHAIN_LENGTH;
	bool new_chain_before_cte_disable = false;

	remove_cte_from_chain_after_enqueue_to_lll(
		cte_count, init_chain_length, expected_mem_pdu_used_count_for_enable,
		expected_mem_pdu_used_count_for_disable, expected_pdu_in_chain_after_cte_disable,
		updated_chain_length, expected_end_fifo_free_pdu_count,
		new_chain_before_cte_disable);
}

void test_remove_cte_from_chain_length_increased_after_enqueue_to_lll(void)
{
	uint8_t cte_count = TEST_CTE_COUNT;
	uint8_t init_chain_length = TEST_PER_ADV_CHAIN_LENGTH;
	uint8_t expected_mem_pdu_used_count_for_enable =
		TEST_PER_ADV_CHAIN_LENGTH + TEST_PER_ADV_SINGLE_PDU;
	uint8_t expected_mem_pdu_used_count_for_disable =
		TEST_PER_ADV_CHAIN_INCREASED_LENGTH - TEST_PER_ADV_SINGLE_PDU;
	uint8_t expected_pdu_in_chain_after_cte_disable = TEST_PER_ADV_CHAIN_INCREASED_LENGTH;
	uint8_t updated_chain_length = TEST_PER_ADV_CHAIN_INCREASED_LENGTH;
	uint8_t expected_end_fifo_free_pdu_count = TEST_PER_ADV_CHAIN_LENGTH;
	bool new_chain_before_cte_disable = true;

	remove_cte_from_chain_after_enqueue_to_lll(
		cte_count, init_chain_length, expected_mem_pdu_used_count_for_enable,
		expected_mem_pdu_used_count_for_disable, expected_pdu_in_chain_after_cte_disable,
		updated_chain_length, expected_end_fifo_free_pdu_count,
		new_chain_before_cte_disable);
}

void test_remove_cte_from_chain_length_decreased_after_enqueue_to_lll(void)
{
	uint8_t cte_count = TEST_CTE_COUNT;
	uint8_t init_chain_length = TEST_PER_ADV_CHAIN_LENGTH;
	uint8_t expected_mem_pdu_used_count_for_enable =
		TEST_PER_ADV_CHAIN_LENGTH + TEST_PER_ADV_SINGLE_PDU;
	uint8_t expected_mem_pdu_used_count_for_disable =
		TEST_PER_ADV_CHAIN_DECREASED_LENGTH - TEST_PER_ADV_SINGLE_PDU;
	uint8_t expected_pdu_in_chain_after_cte_disable = TEST_PER_ADV_CHAIN_DECREASED_LENGTH;
	uint8_t updated_chain_length = TEST_PER_ADV_CHAIN_DECREASED_LENGTH;
	uint8_t expected_end_fifo_free_pdu_count = TEST_PER_ADV_CHAIN_LENGTH;
	bool new_chain_before_cte_disable = true;

	remove_cte_from_chain_after_enqueue_to_lll(
		cte_count, init_chain_length, expected_mem_pdu_used_count_for_enable,
		expected_mem_pdu_used_count_for_disable, expected_pdu_in_chain_after_cte_disable,
		updated_chain_length, expected_end_fifo_free_pdu_count,
		new_chain_before_cte_disable);
}

void run_remove_cte_to_per_adv_chain_tests(void)
{
	ztest_test_suite(
		test_add_cte_to_per_adv_chain,
		ztest_unit_test(test_remove_cte_from_chain_extended_to_tx_all_cte),
		ztest_unit_test(test_remove_cte_from_chain_where_each_pdu_includes_cte),
		ztest_unit_test(test_remove_cte_from_chain_with_more_pdu_than_cte),
		ztest_unit_test(test_remove_cte_from_single_pdu_chain),
		ztest_unit_test(
			test_remove_cte_from_chain_extended_to_tx_all_cte_after_enqueue_to_lll),
		ztest_unit_test(
			test_remove_cte_from_chain_with_more_pdu_than_cte_after_enqueue_to_lll),
		ztest_unit_test(test_remove_cte_from_chain_length_increased_after_enqueue_to_lll),
		ztest_unit_test(test_remove_cte_from_chain_length_decreased_after_enqueue_to_lll));
	ztest_run_test_suite(test_add_cte_to_per_adv_chain);
}
