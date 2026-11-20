/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "ticker.h"
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
#include "lll_adv_pdu.h"
#include "ull_adv_types.h"
#include "ull_adv_internal.h"
#include "ll.h"

#include "babblekit/testcase.h"

#define FILLER_BYTE(idx, val) val

#if defined(CONFIG_BT_CTLR_ADV_DATA_CHAIN)
/* AD data length larger than PDU_AC_EXT_AD_DATA_LEN_MAX, so that the
 * auxiliary PDU is fragmented into chain PDUs, and with a maximum common
 * extended header size of 63 octets would wrap an 8-bit length.
 */
#define AUX_TIME_AD_DATA_LEN  200U
#define AUX_TIME_MFG_DATA_LEN (AUX_TIME_AD_DATA_LEN - 2U)
#define AUX_TIME_UPDATE_COUNT 2U

BUILD_ASSERT(AUX_TIME_AD_DATA_LEN > PDU_AC_EXT_AD_DATA_LEN_MAX);
BUILD_ASSERT(AUX_TIME_AD_DATA_LEN <= CONFIG_BT_CTLR_ADV_DATA_LEN_MAX);

static uint8_t mfg_data[AUX_TIME_MFG_DATA_LEN] = { 0xFF, 0xFF, };

const struct bt_data ad_long[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
};
const struct bt_data ad_short[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1U),
};
#endif

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
	int err = 0;
	uint32_t initial_ticks, ticks_1, ticks_2;

	aux = aux_set_get(adv);

	if (aux == NULL) {
		TEST_FAIL("No auxiliary set");
	}

	if (aux->is_started == 0U) {
		TEST_FAIL("Auxiliary set not started");
	}

	initial_ticks = aux->ull.ticks_slot;

	/* set adv_data, wait for the next advertising event so that adv data
	 * gets updated. then record how many ticks slots are reserved for it
	 */
	err = bt_le_ext_adv_set_data(adv, ad_small, ARRAY_SIZE(ad_small), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Failed to set small adv data");
	}
	(void)k_sleep(K_MSEC(100));
	ticks_1 = aux->ull.ticks_slot;

	/* set a new adv_data PDU, this time larger
	 * verify if reserved tick slots increase
	 */
	err = bt_le_ext_adv_set_data(adv, ad_medium, ARRAY_SIZE(ad_medium), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Failed to set medium adv data");
	}
	(void)k_sleep(K_MSEC(100));
	ticks_2 = aux->ull.ticks_slot;

	printk("%u, %u, %u\n", initial_ticks, ticks_1, ticks_2);

	if (initial_ticks >= ticks_1 || initial_ticks >= ticks_2 || ticks_1 >= ticks_2) {
		TEST_FAIL("Reserved tick slots for conn adv incorrectly computed");
	}
}

#if defined(CONFIG_BT_CTLR_ADV_DATA_CHAIN)
static void aux_time_check_chained(const struct ll_adv_aux_set *aux)
{
	const struct lll_adv_aux *lll_aux;
	const struct pdu_adv *pdu_chain;
	const struct lll_adv *lll;
	const struct pdu_adv *pdu;
	uint32_t slot_us;
	uint32_t time_us;
	uint8_t chain_count;

	lll_aux = &aux->lll;
	lll = lll_aux->adv;

	/* On-air time of the auxiliary PDU and its chain PDUs */
	pdu = lll_adv_aux_data_peek(lll_aux);
	time_us = PDU_AC_US(pdu->len, lll->phy_s, lll->phy_flags);

	chain_count = 0U;
	pdu_chain = lll_adv_pdu_linked_next_get(pdu);
	while (pdu_chain != NULL) {
		time_us += EVENT_B2B_MAFS_US;
		time_us += PDU_AC_US(pdu_chain->len, lll->phy_s, lll->phy_flags);
		chain_count++;
		pdu_chain = lll_adv_pdu_linked_next_get(pdu_chain);
	}

	if (chain_count == 0U) {
		TEST_FAIL("No chain PDU");
	}

	slot_us = HAL_TICKER_TICKS_TO_US(aux->ull.ticks_slot);
	printk("Aux PDU len %u, chain PDUs %u, on-air %u us, reserved %u us\n",
		   pdu->len, chain_count, time_us, slot_us);

	if (slot_us < time_us) {
		TEST_FAIL("Aux event time reservation %u us less than on-air %u us",
			  slot_us, time_us);
	}
}
#endif


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

#if defined(CONFIG_BT_CTLR_ADV_DATA_CHAIN)
static void test_chained_adv_aux_time(void)
{
	struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.sid = 0U,
		.secondary_max_skip = 0U,
		.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_NO_2M,
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
		.peer = NULL,
	};
	struct ll_adv_aux_set *aux;
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed");
	}

	err = bt_le_ext_adv_create(&adv_param, NULL, &adv);
	if (err != 0) {
		TEST_FAIL("Failed to create advertising set");
	}

	err = bt_le_ext_adv_set_data(adv, ad_short, ARRAY_SIZE(ad_short), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Failed to update advertising data");
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		TEST_FAIL("Failed to start advertising set");
	}

	aux = aux_set_get(adv);
	if (aux == NULL) {
		TEST_FAIL("No auxiliary set");
	}

	if (aux->is_started == 0U) {
		TEST_FAIL("Auxiliary set not started");
	}

	/* Update the AD data of the enabled advertising set, repeat with the
	 * same AD data.
	 */
	for (uint8_t i = 0U; i < AUX_TIME_UPDATE_COUNT; i++) {
		err = bt_le_ext_adv_set_data(adv, ad_long, ARRAY_SIZE(ad_long), NULL, 0);
		if (err != 0) {
			TEST_FAIL("Failed to update advertising data");
		}

		aux_time_check_chained(aux);

		k_sleep(K_MSEC(100));
	}

	err = bt_le_ext_adv_stop(adv);
	if (err != 0) {
		TEST_FAIL("Failed to stop advertising set");
	}

	err = bt_le_ext_adv_delete(adv);
	if (err != 0) {
		TEST_FAIL("Failed to delete advertising set");
	}

	TEST_PASS_AND_EXIT("Chained adv aux time reservation tests passed");
}
#endif

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "conn_adv_aux_time",
		.test_descr = "Auxiliary event time reservation for connectable advertising",
		.test_main_f = test_conn_adv_aux_time
	},
	#if defined(CONFIG_BT_CTLR_ADV_DATA_CHAIN)
	{
		.test_id = "chained_adv_aux_time",
		.test_descr = "Auxiliary event time reservation for chained advertising",
		.test_main_f = test_chained_adv_aux_time
	},
	#endif
	BSTEST_END_MARKER
};

struct bst_test_list *test_adv_aux_time_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
