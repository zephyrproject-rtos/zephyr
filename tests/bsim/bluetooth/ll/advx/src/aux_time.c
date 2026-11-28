/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "mem.h"
#include "memq.h"
#include "lll.h"
#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"
#include "ccm.h"
#include "util.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "ull_adv_types.h"
#include "ull_adv_internal.h"
#include "ll.h"

#include "babblekit/testcase.h"

#define FILLER_BYTE(idx, val) (val)

static const uint8_t small_filler[10] = { LISTIFY(10, FILLER_BYTE, (,), 0xAA) };

static const uint8_t med_filler[180] = { LISTIFY(180, FILLER_BYTE, (,), 0xAA) };

static const struct bt_data ad_small[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, small_filler, sizeof(small_filler)),
};

static const struct bt_data ad_medium[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, med_filler, sizeof(med_filler)),
};

static struct ll_adv_aux_set *aux_set_get(struct bt_le_ext_adv *adv)
{
	struct ll_adv_set *adv_set;
	uint8_t handle;
	uint8_t err;

	err = ll_adv_set_by_hci_handle_get(bt_le_ext_adv_get_index(adv), &handle);
	if (err != 0U) {
		return NULL;
	}

	adv_set = ull_adv_set_get(handle);
	if ((adv_set == NULL) || (adv_set->lll.aux == NULL)) {
		return NULL;
	}

	return HDR_LLL2ULL(adv_set->lll.aux);
}

static void aux_time_check_conn(struct bt_le_ext_adv *adv)
{
	struct ll_adv_aux_set *aux;
	int err;
	uint32_t initial_ticks, ticks_1, ticks_2;

	aux = aux_set_get(adv);

	if (aux == NULL) {
		TEST_FAIL("No auxiliary set");
	}

	if (aux->is_started == 0U) {
		TEST_FAIL("Auxiliary set not started");
	}

	initial_ticks = aux->ull.ticks_slot;

	/* set adv_data then record how many ticks slots are reserved for it
	 */
	err = bt_le_ext_adv_set_data(adv, ad_small, ARRAY_SIZE(ad_small), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Failed to set small adv data");
	}
	ticks_1 = aux->ull.ticks_slot;

	/* set a new adv_data PDU, this time larger
	 * verify if reserved tick slots increase
	 */
	err = bt_le_ext_adv_set_data(adv, ad_medium, ARRAY_SIZE(ad_medium), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Failed to set medium adv data");
	}
	ticks_2 = aux->ull.ticks_slot;

	printk("%u, %u, %u\n", initial_ticks, ticks_1, ticks_2);

	if (initial_ticks >= ticks_1 || initial_ticks >= ticks_2 || ticks_1 >= ticks_2) {
		TEST_FAIL("Reserved tick slots for conn adv incorrectly computed");
	}
}

static void test_conn_adv_aux_time(void)
{
	struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.sid = 0U,
		.secondary_max_skip = 0U,
		.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_NO_2M | BT_LE_ADV_OPT_CONN,
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
		.peer = NULL,
	};
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
	}

	err = bt_le_ext_adv_create(&adv_param, NULL, &adv);
	if (err != 0) {
		TEST_FAIL("Failed to create advertising set (err %d)", err);
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		TEST_FAIL("Failed to start advertising set (err %d)", err);
	}

	aux_time_check_conn(adv);

	err = bt_le_ext_adv_stop(adv);
	if (err != 0) {
		TEST_FAIL("Failed to stop advertising set (err %d)", err);
	}

	err = bt_le_ext_adv_delete(adv);
	if (err != 0) {
		TEST_FAIL("Failed to delete advertising set (err %d)", err);
	}

	TEST_PASS_AND_EXIT("Conn adv aux time reservation tests passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "conn_adv_aux_time",
		.test_descr = "Auxiliary event time reservation for connectable advertising",
		.test_main_f = test_conn_adv_aux_time
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_adv_aux_time_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
