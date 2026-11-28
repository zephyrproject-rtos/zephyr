/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "babblekit/testcase.h"

/* The static random address of the periodic advertising that the playback
 * device of sync_pdu_type.sh sends, C0:00:00:00:00:05, and its SID, as in
 * test_data/gen_sync_pdu_type.py
 */
static const bt_addr_le_t adv_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x05, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

#define ADV_SID 1U

/* The kinds of AD data in the PDUs of the playback device: an AUX_SYNC_IND
 * without AuxPtr, an AUX_SYNC_IND with an AuxPtr, and a PDU of another type in
 * place of an AUX_SYNC_IND or AUX_CHAIN_IND
 */
#define AD_KIND_VALID   0x01U
#define AD_KIND_PARENT  0x02U
#define AD_KIND_INVALID 0x03U

/* The paEventCounter of the first periodic advertising event of each phase of
 * the playback, as in test_data/gen_sync_pdu_type.py:
 * - [0, EVT_VALID): PDUs of another type in place of the AUX_SYNC_INDs
 * - [EVT_VALID, EVT_CHAIN): AUX_SYNC_INDs
 * - [EVT_CHAIN, EVT_CHAIN_ALT): AUX_SYNC_INDs with an AuxPtr to a PDU of
 *   another type in place of the AUX_CHAIN_IND
 * - [EVT_CHAIN_ALT, EVT_LOST): as the phase before in the even events, as
 *   [EVT_VALID, EVT_CHAIN) in the odd ones
 * - [EVT_LOST, EVT_END): as [0, EVT_VALID)
 */
#define EVT_VALID     30U
#define EVT_CHAIN     60U
#define EVT_CHAIN_ALT 80U
#define EVT_LOST      100U
#define EVT_END       130U

/* The periodic advertising event with paEventCounter 0 starts 100 ms into the
 * simulation, and the events are 100 ms apart. From EVT_CHAIN to EVT_LOST,
 * each event starts 30 us later than that after the one before it, a drift
 * that only the drift compensation of every event keeps in the receive window,
 * and that adds up to less than 2 ms.
 */
#define EVT_START_MS(counter) (100U + ((counter) * 100U))

/* Synchronization timeout of 1 s, 10 periodic advertising events, in 10 ms
 * units
 */
#define SYNC_TIMEOUT    100U
#define SYNC_TIMEOUT_MS (SYNC_TIMEOUT * 10U)

static K_SEM_DEFINE(sem_sync, 0, 1);
static int64_t volatile synced_ms;
static int64_t volatile term_ms;
static uint8_t volatile term_reason;
static bool volatile is_synced;

static ATOMIC_DEFINE(reported, EVT_END);
static atomic_t reports_unexpected;

static void synced(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	synced_ms = k_uptime_get();
	is_synced = true;

	k_sem_give(&sem_sync);
}

static void term(struct bt_le_per_adv_sync *sync,
		 const struct bt_le_per_adv_sync_term_info *info)
{
	term_ms = k_uptime_get();
	term_reason = info->reason;

	k_sem_give(&sem_sync);
}

static void recv(struct bt_le_per_adv_sync *sync,
		 const struct bt_le_per_adv_sync_recv_info *info, struct net_buf_simple *buf)
{
	uint16_t counter;

	/* A single Manufacturer Specific Data structure with the kind of the
	 * PDU and its paEventCounter. Anything else is decoded from a PDU that
	 * is not part of the train, or is reassembled with data from one.
	 */
	if ((buf->len != 5U) || (buf->data[0] != 4U) ||
	    (buf->data[1] != BT_DATA_MANUFACTURER_DATA) || (buf->data[2] != AD_KIND_VALID)) {
		(void)atomic_inc(&reports_unexpected);

		return;
	}

	counter = sys_get_le16(&buf->data[3]);
	if (counter >= EVT_END) {
		(void)atomic_inc(&reports_unexpected);

		return;
	}

	atomic_set_bit(reported, counter);
}

static struct bt_le_per_adv_sync_cb sync_cb = {
	.synced = synced,
	.term = term,
	.recv = recv,
};

static void sync_create(void)
{
	struct bt_le_per_adv_sync_param param = {
		.options = BT_LE_PER_ADV_SYNC_OPT_NONE,
		.sid = ADV_SID,
		.skip = 0U,
		.timeout = SYNC_TIMEOUT,
	};
	struct bt_le_per_adv_sync *sync;
	int err;

	bt_addr_le_copy(&param.addr, &adv_addr);

	err = bt_le_per_adv_sync_create(&param, &sync);
	TEST_ASSERT(err == 0, "Periodic sync create failed (err %d)", err);
}

static void check_reported(uint16_t first, uint16_t last, uint16_t step)
{
	for (uint16_t counter = first; counter < last; counter += step) {
		TEST_ASSERT(atomic_test_bit(reported, counter),
			    "No report of periodic advertising event %u", counter);
	}
}

static void test_sync_pdu_type(void)
{
	struct bt_le_scan_param scan_param = BT_LE_SCAN_PARAM_INIT(
		BT_LE_SCAN_TYPE_PASSIVE, BT_LE_SCAN_OPT_NONE, BT_GAP_SCAN_FAST_INTERVAL_MIN,
		BT_GAP_SCAN_FAST_INTERVAL_MIN);
	uint32_t estab_failed = 0U;
	int err;

	TEST_START("sync_pdu_type");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	bt_le_per_adv_sync_cb_register(&sync_cb);

	err = bt_le_scan_start(&scan_param, NULL);
	TEST_ASSERT(err == 0, "Scan start failed (err %d)", err);

	/* The PDUs of other types that the train has in place of the
	 * AUX_SYNC_INDs at first may not establish the synchronization. The
	 * Controller fails to establish it after six periodic advertising
	 * events, then the scanner tries again.
	 */
	sync_create();
	while (true) {
		(void)k_sem_take(&sem_sync, K_FOREVER);

		if (is_synced) {
			break;
		}

		TEST_ASSERT(term_reason == BT_HCI_ERR_CONN_FAIL_TO_ESTAB,
			    "Periodic sync establishment ended with reason 0x%02x", term_reason);
		estab_failed++;

		sync_create();
	}

	TEST_ASSERT(estab_failed > 0U, "No periodic sync establishment failed");
	TEST_ASSERT(synced_ms >= EVT_START_MS(EVT_VALID),
		    "Periodic sync established at %lld ms, before the first AUX_SYNC_IND",
		    synced_ms);

	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "Scan stop failed (err %d)", err);

	/* The PDUs of other types in place of the AUX_CHAIN_INDs leave the
	 * parent AUX_SYNC_INDs to compensate the drift of the train, which
	 * keeps the synchronization up. Those in place of the AUX_SYNC_INDs at
	 * the end do not: the synchronization is lost with the timeout after
	 * the last AUX_SYNC_IND. The LE Periodic Advertising Sync Lost event
	 * has no reason, which the Host reports as unspecified.
	 */
	(void)k_sem_take(&sem_sync, K_FOREVER);
	TEST_ASSERT(term_reason == BT_HCI_ERR_UNSPECIFIED,
		    "Periodic sync terminated with reason 0x%02x", term_reason);
	TEST_ASSERT(term_ms > EVT_START_MS(EVT_LOST - 1U),
		    "Periodic sync lost at %lld ms, before the last AUX_SYNC_IND", term_ms);
	TEST_ASSERT(term_ms < EVT_START_MS(EVT_LOST + 1U) + SYNC_TIMEOUT_MS,
		    "Periodic sync lost at %lld ms, after the timeout", term_ms);

	/* Every AUX_SYNC_IND without AuxPtr is reported. One that follows an
	 * AUX_SYNC_IND whose AUX_CHAIN_IND was not received is reported only
	 * when the Controller has ended the report of that one as incomplete.
	 */
	check_reported(EVT_VALID, EVT_CHAIN, 1U);
	check_reported(EVT_CHAIN_ALT + 1U, EVT_LOST, 2U);

	/* No report has data of an AUX_SYNC_IND with an AuxPtr, or of a PDU of
	 * another type
	 */
	TEST_ASSERT(atomic_get(&reports_unexpected) == 0, "%ld unexpected reports",
		    (long)atomic_get(&reports_unexpected));

	TEST_PASS("sync_pdu_type");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "sync_pdu_type",
		.test_descr = "Periodic sync ignores PDUs of types other than AUX_SYNC_IND",
		.test_main_f = test_sync_pdu_type,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_sync_pdu_type_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
