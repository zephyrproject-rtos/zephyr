/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Unit tests for the fragmented PAwR (Periodic Advertising with Responses)
 * response report reassembly logic in bt_hci_le_per_adv_response_report()
 * (subsys/bluetooth/host/adv.c).
 *
 * The tests drive the HCI event handler directly with crafted LE Periodic
 * Advertising Response Report events and observe the reassembled data that is
 * delivered to the application through the pawr_response callback.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>

#define TEST_SUBEVENT      3
#define TEST_RESPONSE_SLOT 5

/* A single PAwR response report HCI event fits comfortably in this buffer. */
NET_BUF_SIMPLE_DEFINE_STATIC(evt_buf, 512);
static struct net_buf evt_net_buf;

/* Captured state from the last pawr_response callback invocation. */
struct captured_response {
	bool called;
	bool data_null;
	uint8_t subevent;
	uint8_t response_slot;
	uint16_t data_len;
	uint8_t data[BT_PER_ADV_RSP_REASSEMBLY_BUF_SIZE];
};

static struct captured_response last_response;
static unsigned int response_call_count;

static struct bt_le_ext_adv *test_adv;

static void pawr_response_cb(struct bt_le_ext_adv *adv, struct bt_le_per_adv_response_info *info,
			     struct net_buf_simple *buf)
{
	ARG_UNUSED(adv);

	response_call_count++;

	last_response.called = true;
	last_response.subevent = info->subevent;
	last_response.response_slot = info->response_slot;

	if (buf == NULL) {
		last_response.data_null = true;
		last_response.data_len = 0;
	} else {
		last_response.data_null = false;
		last_response.data_len = buf->len;
		zassert_true(buf->len <= sizeof(last_response.data),
			     "Reported data larger than reassembly buffer");
		memcpy(last_response.data, buf->data, buf->len);
	}
}

static const struct bt_le_ext_adv_cb adv_cb = {
	.pawr_response = pawr_response_cb,
};

/*
 * Append a single response entry (bt_hci_evt_le_per_adv_response header plus
 * data) to the event buffer currently being built.
 */
static void add_response_slot(struct net_buf_simple *buf, uint8_t data_status,
			      const uint8_t *data, uint8_t data_length, uint8_t response_slot)
{
	struct bt_hci_evt_le_per_adv_response response = {
		.tx_power = 0,
		.rssi = 0,
		.cte_type = 0,
		.response_slot = response_slot,
		.data_status = data_status,
		.data_length = data_length,
	};

	net_buf_simple_add_mem(buf, &response, sizeof(response));
	if (data_length > 0) {
		net_buf_simple_add_mem(buf, data, data_length);
	}
}

/*
 * Build a full LE Periodic Advertising Response Report event carrying a single
 * response entry for the given response slot, then feed it to the handler
 * under test.
 */
static void deliver_single_response_slot(uint8_t data_status, const uint8_t *data,
					 uint8_t data_length, uint8_t response_slot)
{
	struct bt_hci_evt_le_per_adv_response_report report = {
		.adv_handle = 0,
		.subevent = TEST_SUBEVENT,
		.tx_status = 0,
		.num_responses = 1,
	};

	net_buf_simple_reset(&evt_buf);
	net_buf_simple_add_mem(&evt_buf, &report, sizeof(report));
	add_response_slot(&evt_buf, data_status, data, data_length, response_slot);

	evt_net_buf.b = evt_buf;
	bt_hci_le_per_adv_response_report(&evt_net_buf);

	/* Reflect any consumed data back to the standalone simple buffer. */
	evt_buf = evt_net_buf.b;
}

/*
 * Build a full LE Periodic Advertising Response Report event carrying a single
 * response entry, then feed it to the handler under test.
 */
static void deliver_single_response(uint8_t data_status, const uint8_t *data, uint8_t data_length)
{
	deliver_single_response_slot(data_status, data, data_length, TEST_RESPONSE_SLOT);
}

/*
 * Begin building a LE Periodic Advertising Response Report event that may carry
 * more than one response entry. Append entries with add_response_slot() and
 * feed the event to the handler with deliver_event().
 */
static void start_event(void)
{
	struct bt_hci_evt_le_per_adv_response_report report = {
		.adv_handle = 0,
		.subevent = TEST_SUBEVENT,
		.tx_status = 0,
		.num_responses = 0,
	};

	net_buf_simple_reset(&evt_buf);
	net_buf_simple_add_mem(&evt_buf, &report, sizeof(report));
}

/*
 * Patch the number of responses into the event header and feed the built event
 * to the handler under test.
 */
static void deliver_event(uint8_t num_responses)
{
	struct bt_hci_evt_le_per_adv_response_report *report =
		(struct bt_hci_evt_le_per_adv_response_report *)evt_buf.data;

	report->num_responses = num_responses;

	evt_net_buf.b = evt_buf;
	bt_hci_le_per_adv_response_report(&evt_net_buf);

	/* Reflect any consumed data back to the standalone simple buffer. */
	evt_buf = evt_net_buf.b;
}

static void fill_pattern(uint8_t *data, uint16_t len, uint8_t seed)
{
	for (uint16_t i = 0; i < len; i++) {
		data[i] = (uint8_t)(seed + i);
	}
}

static void *pawr_setup(void)
{
	int err;

	/* Provide a valid identity so bt_le_ext_adv_create() accepts params. */
	memset(&bt_dev, 0, sizeof(bt_dev));
	bt_dev.id_count = 1;
	bt_dev.hci_version = BT_HCI_VERSION_5_0;
	/* A non-zero identity address is required; BT_ADDR_LE_ANY is rejected. */
	bt_dev.id_addr[0].type = BT_ADDR_LE_RANDOM;
	bt_dev.id_addr[0].a.val[0] = 0x01;
	bt_dev.id_addr[0].a.val[5] = 0xC0;
	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, &adv_cb, &test_adv);

	zassert_ok(err, "Failed to create ext adv set (%d)", err);
	zassert_not_null(test_adv, "adv set is NULL");

	return NULL;
}

static void pawr_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&last_response, 0, sizeof(last_response));
	response_call_count = 0;
}

ZTEST_SUITE(pawr_rsp_reassembly, NULL, pawr_setup, pawr_before, NULL, NULL);

/*
 * A single COMPLETE response with no preceding fragments must be reported
 * verbatim to the application.
 */
ZTEST(pawr_rsp_reassembly, test_single_complete_report)
{
	uint8_t data[100];

	fill_pattern(data, sizeof(data), 0x10);

	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, data, sizeof(data));

	zassert_equal(response_call_count, 1, "Expected exactly one response");
	zassert_true(last_response.called, "Callback not invoked");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.subevent, TEST_SUBEVENT, "Wrong subevent");
	zassert_equal(last_response.response_slot, TEST_RESPONSE_SLOT, "Wrong response slot");
	zassert_equal(last_response.data_len, sizeof(data), "Wrong data length");
	zassert_mem_equal(last_response.data, data, sizeof(data), "Data mismatch");
}

/*
 * Two PARTIAL fragments followed by a COMPLETE fragment must be reassembled
 * into a single contiguous report delivered on the COMPLETE fragment only.
 */
ZTEST(pawr_rsp_reassembly, test_reassemble_partial_then_complete)
{
	uint8_t frag1[80];
	uint8_t frag2[80];
	uint8_t frag3[80];
	uint8_t expected[240];

	fill_pattern(frag1, sizeof(frag1), 0x00);
	fill_pattern(frag2, sizeof(frag2), 0x50);
	fill_pattern(frag3, sizeof(frag3), 0xA0);
	memcpy(&expected[0], frag1, sizeof(frag1));
	memcpy(&expected[80], frag2, sizeof(frag2));
	memcpy(&expected[160], frag3, sizeof(frag3));

	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, frag1, sizeof(frag1));
	zassert_equal(response_call_count, 0, "Partial fragment should not report");

	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, frag2, sizeof(frag2));
	zassert_equal(response_call_count, 0, "Partial fragment should not report");

	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, frag3, sizeof(frag3));

	zassert_equal(response_call_count, 1, "Expected exactly one report on completion");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.data_len, sizeof(expected), "Wrong reassembled length");
	zassert_mem_equal(last_response.data, expected, sizeof(expected), "Reassembly mismatch");
}

/*
 * RX_FAILED must be reported to the application with a NULL buffer and must
 * drop any partial chain currently being reassembled.
 */
ZTEST(pawr_rsp_reassembly, test_rx_failed_reports_null)
{
	uint8_t frag1[80];

	fill_pattern(frag1, sizeof(frag1), 0x00);

	/* Start a chain, then have reception fail. */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, frag1, sizeof(frag1));
	zassert_equal(response_call_count, 0, "Partial fragment should not report");

	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_RX_FAILED, NULL, 0);

	zassert_equal(response_call_count, 1, "RX_FAILED should report once");
	zassert_true(last_response.data_null, "RX_FAILED must report NULL data");
}

/*
 * Regression test for the truncation bug: if an intermediate fragment overflows
 * the reassembly buffer, the chain is marked truncated and the terminating
 * COMPLETE fragment must be dropped instead of being reported as a valid
 * (silently truncated) response.
 */
ZTEST(pawr_rsp_reassembly, test_overflow_drops_until_complete)
{
	uint8_t big_frag[200];
	uint8_t last_frag[100];

	fill_pattern(big_frag, sizeof(big_frag), 0x00);
	fill_pattern(last_frag, sizeof(last_frag), 0x80);

	/* First partial fits (200 <= 254). */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, big_frag,
				sizeof(big_frag));
	zassert_equal(response_call_count, 0, "Partial fragment should not report");

	/*
	 * Second partial (another 200 bytes) does not fit in the remaining
	 * tailroom, so the chain is marked truncated.
	 */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, big_frag,
				sizeof(big_frag));
	zassert_equal(response_call_count, 0, "Overflowing partial must not report");

	/*
	 * The terminating COMPLETE fragment matches the truncated chain and must
	 * be dropped rather than reported as if it were a complete response.
	 */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, last_frag,
				sizeof(last_frag));
	zassert_equal(response_call_count, 0, "Truncated chain must not be reported");
}

/*
 * After a truncated/dropped chain, a fresh COMPLETE response for the same slot
 * must again be reported normally (state is reset).
 */
ZTEST(pawr_rsp_reassembly, test_recovers_after_truncation)
{
	uint8_t big_frag[200];
	uint8_t fresh[50];

	fill_pattern(big_frag, sizeof(big_frag), 0x00);
	fill_pattern(fresh, sizeof(fresh), 0x33);

	/* Overflow and drop a chain. */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, big_frag,
				sizeof(big_frag));
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, big_frag,
				sizeof(big_frag));
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, big_frag,
				sizeof(big_frag));
	zassert_equal(response_call_count, 0, "Truncated chain must not be reported");

	/* A brand new, self-contained COMPLETE response must report normally. */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, fresh, sizeof(fresh));

	zassert_equal(response_call_count, 1, "Fresh response after truncation must report");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.data_len, sizeof(fresh), "Wrong data length");
	zassert_mem_equal(last_response.data, fresh, sizeof(fresh), "Data mismatch");
}

/*
 * Regression test: a truncated chain for one response slot must not leak into a
 * different response slot. A chain for slot A overflows and is marked
 * truncated, then a completely separate response for slot B arrives. Slot B
 * must be reported normally instead of being dropped by the stale truncation
 * state from slot A.
 */
ZTEST(pawr_rsp_reassembly, test_truncation_does_not_leak_to_next_slot)
{
	uint8_t big_frag[200];
	uint8_t slot_b_frag[80];
	uint8_t slot_b_complete[80];
	uint8_t expected[160];

	fill_pattern(big_frag, sizeof(big_frag), 0x00);
	fill_pattern(slot_b_frag, sizeof(slot_b_frag), 0x11);
	fill_pattern(slot_b_complete, sizeof(slot_b_complete), 0x77);
	memcpy(&expected[0], slot_b_frag, sizeof(slot_b_frag));
	memcpy(&expected[80], slot_b_complete, sizeof(slot_b_complete));

	/* Slot A: two partial fragments overflow the buffer (marks truncated). */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, big_frag,
				     sizeof(big_frag), TEST_RESPONSE_SLOT);
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, big_frag,
				     sizeof(big_frag), TEST_RESPONSE_SLOT);
	zassert_equal(response_call_count, 0, "Overflowing partial must not report");

	/* Slot B: a fresh partial + complete for a different slot. The stale
	 * truncation state from slot A must not cause slot B to be dropped.
	 */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, slot_b_frag,
				     sizeof(slot_b_frag), TEST_RESPONSE_SLOT + 1);
	zassert_equal(response_call_count, 0, "Partial fragment should not report");

	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, slot_b_complete,
				     sizeof(slot_b_complete), TEST_RESPONSE_SLOT + 1);

	zassert_equal(response_call_count, 1, "Slot B must be reported, not dropped");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.response_slot, TEST_RESPONSE_SLOT + 1, "Wrong response slot");
	zassert_equal(last_response.data_len, sizeof(expected), "Wrong reassembled length");
	zassert_mem_equal(last_response.data, expected, sizeof(expected), "Reassembly mismatch");
}

/*
 * Regression test: an overflowing partial chain followed by RX_FAILED must
 * fully reset the reassembly state (including the truncated flag). A fresh
 * COMPLETE response for the same slot afterwards must be reported normally
 * rather than being dropped by leftover truncation state.
 */
ZTEST(pawr_rsp_reassembly, test_rx_failed_clears_truncation)
{
	uint8_t big_frag[200];
	uint8_t fresh[60];

	fill_pattern(big_frag, sizeof(big_frag), 0x00);
	fill_pattern(fresh, sizeof(fresh), 0x44);

	/* Overflow the chain so it is marked truncated. */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, big_frag,
				sizeof(big_frag));
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, big_frag,
				sizeof(big_frag));
	zassert_equal(response_call_count, 0, "Overflowing partial must not report");

	/* RX_FAILED must reset the truncated flag as well as the buffer. */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_RX_FAILED, NULL, 0);
	zassert_equal(response_call_count, 1, "RX_FAILED should report once");
	zassert_true(last_response.data_null, "RX_FAILED must report NULL data");

	/* A fresh self-contained COMPLETE response must report normally. */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, fresh, sizeof(fresh));

	zassert_equal(response_call_count, 2, "Fresh response after RX_FAILED must report");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.data_len, sizeof(fresh), "Wrong data length");
	zassert_mem_equal(last_response.data, fresh, sizeof(fresh), "Data mismatch");
}

/*
 * Realistic fragmentation shape: a single HCI event carries several response
 * entries, and the last entry in the event is a PARTIAL fragment of a chain
 * that is terminated by the COMPLETE report delivered in the next event.
 *
 * The event carries:
 *   - a self-contained COMPLETE response for one slot (reported immediately),
 *   - an RX_FAILED response for another slot (reported with NULL), and
 *   - the first PARTIAL fragment of the chain slot.
 * The following event delivers the COMPLETE fragment of the chain slot, which
 * must trigger reassembly of just the two chain fragments.
 */
ZTEST(pawr_rsp_reassembly, test_multi_response_event_partial_tail_then_complete)
{
	uint8_t self_contained[40];
	uint8_t chain_frag1[80];
	uint8_t chain_frag2[60];
	uint8_t expected[140];

	fill_pattern(self_contained, sizeof(self_contained), 0x10);
	fill_pattern(chain_frag1, sizeof(chain_frag1), 0x40);
	fill_pattern(chain_frag2, sizeof(chain_frag2), 0x90);
	memcpy(&expected[0], chain_frag1, sizeof(chain_frag1));
	memcpy(&expected[80], chain_frag2, sizeof(chain_frag2));

	/* First event: three response entries, ending in a PARTIAL fragment. */
	start_event();
	add_response_slot(&evt_buf, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, self_contained,
			  sizeof(self_contained), TEST_RESPONSE_SLOT);
	add_response_slot(&evt_buf, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_RX_FAILED, NULL, 0,
			  TEST_RESPONSE_SLOT + 1);
	add_response_slot(&evt_buf, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, chain_frag1,
			  sizeof(chain_frag1), TEST_RESPONSE_SLOT + 2);
	deliver_event(3);

	/* The self-contained COMPLETE and the RX_FAILED are reported; the
	 * trailing PARTIAL fragment is only buffered.
	 */
	zassert_equal(response_call_count, 2, "Expected COMPLETE and RX_FAILED to report");

	/* Second event: the COMPLETE fragment terminating the buffered chain. */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, chain_frag2,
				     sizeof(chain_frag2), TEST_RESPONSE_SLOT + 2);

	zassert_equal(response_call_count, 3, "Chain completion must report exactly once");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.response_slot, TEST_RESPONSE_SLOT + 2, "Wrong response slot");
	zassert_equal(last_response.data_len, sizeof(expected), "Wrong reassembled length");
	zassert_mem_equal(last_response.data, expected, sizeof(expected), "Reassembly mismatch");
}

/*
 * A COMPLETE report for a different slot arriving while a chain is buffered must
 * discard the stale (mismatched) chain, then be reported as a self-contained
 * response. The interrupted chain must not leak into the new slot's report.
 */
ZTEST(pawr_rsp_reassembly, test_complete_other_slot_discards_buffered_chain)
{
	uint8_t chain_frag[80];
	uint8_t other_complete[50];

	fill_pattern(chain_frag, sizeof(chain_frag), 0x20);
	fill_pattern(other_complete, sizeof(other_complete), 0x60);

	/* Buffer a partial chain for slot A. */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, chain_frag,
				     sizeof(chain_frag), TEST_RESPONSE_SLOT);
	zassert_equal(response_call_count, 0, "Partial fragment should not report");

	/* A COMPLETE for slot B interrupts the buffered chain. It must be
	 * reported as a self-contained response, carrying only slot B's data.
	 */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, other_complete,
				     sizeof(other_complete), TEST_RESPONSE_SLOT + 1);

	zassert_equal(response_call_count, 1, "Slot B COMPLETE must report once");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.response_slot, TEST_RESPONSE_SLOT + 1, "Wrong response slot");
	zassert_equal(last_response.data_len, sizeof(other_complete), "Wrong data length");
	zassert_mem_equal(last_response.data, other_complete, sizeof(other_complete),
			  "Slot B data must not include slot A chain");
}

/*
 * Second overflow-drop sequence from the COMPLETE branch in adv.c: the buffered
 * fragments all fit, but the terminating COMPLETE fragment itself does not fit
 * in the remaining tailroom. The chain must be dropped rather than reported as
 * a silently truncated response. This complements test_overflow_drops_until_
 * complete(), which exercises the report_truncated path instead.
 */
ZTEST(pawr_rsp_reassembly, test_complete_fragment_overflow_drops_chain)
{
	uint8_t frag1[200];
	uint8_t final_frag[100];
	uint8_t fresh[50];

	fill_pattern(frag1, sizeof(frag1), 0x00);
	fill_pattern(final_frag, sizeof(final_frag), 0x80);
	fill_pattern(fresh, sizeof(fresh), 0x33);

	/* A single PARTIAL fragment fits (200 <= 254), so the chain is buffered
	 * without being marked truncated.
	 */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, frag1, sizeof(frag1));
	zassert_equal(response_call_count, 0, "Partial fragment should not report");

	/* The COMPLETE fragment (100 bytes) does not fit in the remaining 54
	 * bytes of tailroom, so the whole chain must be dropped.
	 */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, final_frag,
				sizeof(final_frag));
	zassert_equal(response_call_count, 0, "Overflowing COMPLETE must not report");

	/* State must be reset: a fresh self-contained COMPLETE reports normally. */
	deliver_single_response(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, fresh, sizeof(fresh));

	zassert_equal(response_call_count, 1, "Fresh response after drop must report");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.data_len, sizeof(fresh), "Wrong data length");
	zassert_mem_equal(last_response.data, fresh, sizeof(fresh), "Data mismatch");
}

/*
 * The advertiser keeps a single reassembly buffer per advertising set, so only
 * one response chain can be reassembled at a time. If a PARTIAL fragment for
 * slot 5 arrives while a chain for slot 4 is still buffered, the slot 4 chain
 * is discarded (the controller only ever fragments one response at a time) and
 * reassembly restarts for slot 5. The slot 5 chain must then complete normally,
 * carrying only slot 5 data.
 */
ZTEST(pawr_rsp_reassembly, test_interleaved_partial_slots_keeps_latest)
{
	uint8_t slot4_frag[80];
	uint8_t slot5_frag1[80];
	uint8_t slot5_frag2[60];
	uint8_t expected[140];

	fill_pattern(slot4_frag, sizeof(slot4_frag), 0x10);
	fill_pattern(slot5_frag1, sizeof(slot5_frag1), 0x50);
	fill_pattern(slot5_frag2, sizeof(slot5_frag2), 0x90);
	memcpy(&expected[0], slot5_frag1, sizeof(slot5_frag1));
	memcpy(&expected[80], slot5_frag2, sizeof(slot5_frag2));

	/* Buffer a PARTIAL fragment for slot 4. */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, slot4_frag,
				     sizeof(slot4_frag), 4);
	zassert_equal(response_call_count, 0, "Partial fragment should not report");

	/* A PARTIAL fragment for slot 5 interrupts and replaces the slot 4
	 * chain. Neither reports yet.
	 */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, slot5_frag1,
				     sizeof(slot5_frag1), 5);
	zassert_equal(response_call_count, 0, "Partial fragment should not report");

	/* The terminating COMPLETE for slot 5 must reassemble only slot 5's
	 * fragments, with no bytes leaking from the dropped slot 4 chain.
	 */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, slot5_frag2,
				     sizeof(slot5_frag2), 5);

	zassert_equal(response_call_count, 1, "Slot 5 completion must report once");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.response_slot, 5, "Wrong response slot");
	zassert_equal(last_response.data_len, sizeof(expected), "Wrong reassembled length");
	zassert_mem_equal(last_response.data, expected, sizeof(expected),
			  "Slot 5 data must not include slot 4 chain");
}

/*
 * Interleaving within a single event: one HCI event carries a PARTIAL fragment
 * for slot 4 immediately followed by a PARTIAL fragment for slot 5. Because
 * only one chain can be buffered, the slot 4 fragment is dropped in favour of
 * slot 5. A COMPLETE for slot 4 arriving afterwards no longer has a buffered
 * chain (slot 5 is the buffered one), so it interrupts the slot 5 chain and is
 * reported as a self-contained response carrying only its own COMPLETE data.
 */
ZTEST(pawr_rsp_reassembly, test_two_partial_slots_in_one_event_then_stale_complete)
{
	uint8_t slot4_frag[80];
	uint8_t slot5_frag[80];
	uint8_t slot4_complete[40];

	fill_pattern(slot4_frag, sizeof(slot4_frag), 0x10);
	fill_pattern(slot5_frag, sizeof(slot5_frag), 0x50);
	fill_pattern(slot4_complete, sizeof(slot4_complete), 0xC0);

	/* One event with two PARTIAL fragments for different slots. */
	start_event();
	add_response_slot(&evt_buf, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, slot4_frag,
			  sizeof(slot4_frag), 4);
	add_response_slot(&evt_buf, BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, slot5_frag,
			  sizeof(slot5_frag), 5);
	deliver_event(2);
	zassert_equal(response_call_count, 0, "Partial fragments should not report");

	/* A COMPLETE for slot 4 no longer matches the buffered slot 5 chain, so
	 * the slot 5 chain is discarded and slot 4 is reported self-contained.
	 */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, slot4_complete,
				     sizeof(slot4_complete), 4);

	zassert_equal(response_call_count, 1, "Slot 4 COMPLETE must report once");
	zassert_false(last_response.data_null, "Data unexpectedly NULL");
	zassert_equal(last_response.response_slot, 4, "Wrong response slot");
	zassert_equal(last_response.data_len, sizeof(slot4_complete), "Wrong data length");
	zassert_mem_equal(last_response.data, slot4_complete, sizeof(slot4_complete),
			  "Slot 4 COMPLETE must carry only its own data");
}

/*
 * Two independent chains reassembled back to back on different slots. A
 * complete PARTIAL+COMPLETE sequence for slot 4 is reported, then a separate
 * PARTIAL+COMPLETE sequence for slot 5 is reported. Each chain must be
 * delivered with the correct slot and its own data only.
 */
ZTEST(pawr_rsp_reassembly, test_sequential_chains_on_different_slots)
{
	uint8_t slot4_frag1[70];
	uint8_t slot4_frag2[50];
	uint8_t slot5_frag1[90];
	uint8_t slot5_frag2[40];
	uint8_t expected4[120];
	uint8_t expected5[130];

	fill_pattern(slot4_frag1, sizeof(slot4_frag1), 0x00);
	fill_pattern(slot4_frag2, sizeof(slot4_frag2), 0x40);
	fill_pattern(slot5_frag1, sizeof(slot5_frag1), 0x80);
	fill_pattern(slot5_frag2, sizeof(slot5_frag2), 0xC0);
	memcpy(&expected4[0], slot4_frag1, sizeof(slot4_frag1));
	memcpy(&expected4[70], slot4_frag2, sizeof(slot4_frag2));
	memcpy(&expected5[0], slot5_frag1, sizeof(slot5_frag1));
	memcpy(&expected5[90], slot5_frag2, sizeof(slot5_frag2));

	/* Slot 4: full PARTIAL + COMPLETE sequence. */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, slot4_frag1,
				     sizeof(slot4_frag1), 4);
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, slot4_frag2,
				     sizeof(slot4_frag2), 4);

	zassert_equal(response_call_count, 1, "Slot 4 chain must report once");
	zassert_equal(last_response.response_slot, 4, "Wrong response slot");
	zassert_equal(last_response.data_len, sizeof(expected4), "Wrong slot 4 length");
	zassert_mem_equal(last_response.data, expected4, sizeof(expected4), "Slot 4 mismatch");

	/* Slot 5: a separate PARTIAL + COMPLETE sequence reassembles cleanly. */
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL, slot5_frag1,
				     sizeof(slot5_frag1), 5);
	deliver_single_response_slot(BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE, slot5_frag2,
				     sizeof(slot5_frag2), 5);

	zassert_equal(response_call_count, 2, "Slot 5 chain must report once");
	zassert_equal(last_response.response_slot, 5, "Wrong response slot");
	zassert_equal(last_response.data_len, sizeof(expected5), "Wrong slot 5 length");
	zassert_mem_equal(last_response.data, expected5, sizeof(expected5), "Slot 5 mismatch");
}
