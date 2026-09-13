/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/canbus/j1939.h>

#include "J1939Tp.h"

/* A broadcast (PDU2) test PGN, distinct from PGNs used by the tp/ and ac/ test suites. */
#define TEST_PGN ((j1939_pgn_t)0xFEE0U)
#define TEST_REQUEST_PGN ((j1939_pgn_t)0xFEE1U)

/* Background j1939_task thread runs every CONFIG_J1939_TASK_INTERVAL_MS; give it several
 * ticks to drain the loopback CAN queue when verifying our own transmitted frames.
 */
#define TX_SETTLE_TIME K_MSEC(50)

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

J1939_NODE_DEFINE(test_node,
	can_dev,
	0x21,  /* source address */
	200,   /* ID number */
	1,     /* manufacturer code */
	0,     /* ECU instance */
	0,     /* function instance */
	0,     /* function */
	0,     /* vehicle system */
	0,     /* vehicle system instance */
	0,     /* industry group */
	false  /* arbitrary address capable */
);

/* Captures the most recent frame transmitted by the node under test matching test_capture_pf. */
static struct can_frame captured_frame;
static volatile bool captured_frame_valid;
static j1939_pdu_format_t captured_pf_filter;

static void capture_tx_frame(const struct device *dev, struct can_frame *frame, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (j1939_get_pdu_format(frame->id) == captured_pf_filter) {
		captured_frame = *frame;
		captured_frame_valid = true;
	}
}

static int install_capture_filter(j1939_pdu_format_t pf)
{
	struct can_filter filter = {.mask = 0, .flags = CAN_FILTER_IDE};

	captured_frame_valid = false;
	captured_pf_filter = pf;

	return can_add_rx_filter(can_dev, capture_tx_frame, NULL, &filter);
}

/* j1939_task() unconditionally calls into these app-provided hooks (see J1939.c); the background
 * task thread started by j1939_init() reaches them regardless of whether a test exercises ECU/SW
 * ID reporting, so the test binary must supply them to link.
 */
void j1939_app_get_device_info(j1939_device_info_t *device_info)
{
	static const char serial[] = "core-test";
	static const char revision[] = "1.0";
	static const char part_number[] = "CORE-TEST";

	*device_info = (j1939_device_info_t){
		.hardware_serial_number = serial,
		.hardware_revision = revision,
		.hardware_part_number = part_number,
	};
}

void j1939_get_software_id(uint8_t **software_id_ptr)
{
	ARG_UNUSED(software_id_ptr);
}

static void *core_suite_setup(void)
{
	zassert_true(device_is_ready(can_dev), "CAN device not ready");
	/* can_loopback.c only dispatches transmitted frames to installed RX filters when the
	 * device mode includes CAN_MODE_LOOPBACK.
	 */
	zassert_equal(can_set_mode(can_dev, CAN_MODE_LOOPBACK), 0, "Failed to set loopback mode");
	can_start(can_dev);
	/* j1939_init() spawns a background thread that periodically calls j1939_task(); it must
	 * only be called once for the whole binary, never per-test, or successive calls leak
	 * threads that race on shared state (message queues, node struct) and crash.
	 */
	j1939_init();
	k_sleep(TX_SETTLE_TIME);
	return NULL;
}

/* Resets per-node request-tracking state between tests without re-running j1939_init(). */
static void reset_pgn_request_list(void)
{
	test_node.j1939_requested_pgn_count = 0;
	for (uint8_t index = 0U; index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES; index++) {
		test_node.j1939_pgn_request_list[index].isUsed = false;
		test_node.j1939_pgn_request_list[index].source = J1939_GLOBAL_ADDRESS;
		test_node.j1939_pgn_request_list[index].pgn = 0;
		test_node.j1939_pgn_request_list[index].isRequested = false;
	}
}

static void core_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_pgn_request_list();
}

ZTEST_SUITE(j1939_core, NULL, core_suite_setup, core_before, NULL, NULL);

/* ── PGN/arbitration math (no CAN traffic needed) ── */

ZTEST(j1939_core, test_is_pgn_valid_pdu2_any_group_extension)
{
	zassert_true(j1939_is_pgn_valid(0xFECAU), "PDU2 (PF>=240) PGNs are always valid");
}

ZTEST(j1939_core, test_is_pgn_valid_pdu1_zero_group_extension)
{
	zassert_true(j1939_is_pgn_valid(0x1200U),
		     "PDU1 PGN with zero group extension byte should be valid");
}

ZTEST(j1939_core, test_is_pgn_valid_pdu1_nonzero_group_extension)
{
	zassert_false(j1939_is_pgn_valid(0x1234U),
		      "PDU1 PGN with a nonzero group extension byte should be invalid");
}

ZTEST(j1939_core, test_get_pgn_truncates_pdu2_group_extension)
{
	/* j1939_get_pgn() has a known bug: it always zeroes the low (group extension) byte, even
	 * for PDU2/broadcast PGNs where that byte is significant data, not a destination address.
	 * See repo memory J1939_core_dispatch_notes.md. This characterizes the current (buggy)
	 * behavior; update if the bug is ever fixed upstream.
	 */
	j1939_pgn_t built = j1939_build_pgn(false, false, 0xFEU, 0xCAU);
	j1939_arbitration_t id =
		j1939_build_message_id(false, false, J1939_Priority_6, built, 0x20U);

	zassert_equal(j1939_get_pgn(id), built & 0x3FF00U,
		      "j1939_get_pgn() is expected to truncate the PDU2 group extension to zero");
}

ZTEST(j1939_core, test_build_and_get_pgn_round_trip_pdu1_ignores_destination)
{
	j1939_pgn_t built = j1939_build_pgn(false, false, 0x12U, 0U);
	j1939_arbitration_t id =
		j1939_build_message_id(false, false, J1939_Priority_6, built + 0x34U, 0x20U);

	zassert_equal(j1939_get_pgn(id), built,
		      "PDU1 PGN extraction should zero out the destination-address byte");
}

ZTEST(j1939_core, test_is_msg_for_me_broadcast)
{
	struct can_frame frame = {0};

	frame.flags = CAN_FRAME_IDE;
	frame.id = j1939_build_message_id(false, false, J1939_Priority_6,
					  j1939_build_pgn(false, false, 0xFEU, 0xCAU), 0x40U);

	zassert_true(j1939_is_msg_for_me(&frame, &test_node),
		     "Broadcast (PF>=240) messages are for every node");
}

ZTEST(j1939_core, test_is_msg_for_me_unicast_to_other_address)
{
	struct can_frame frame = {0};
	/* For PDU1 (PF<240), j1939_build_pgn() zeroes the low (destination) byte; the sender
	 * adds the destination address into the PGN's low byte separately, as j1939_transmit_pgn()
	 * does.
	 */
	j1939_pgn_t pgn_with_destination = j1939_build_pgn(false, false, 0x12U, 0U) + 0x99U;

	frame.flags = CAN_FRAME_IDE;
	frame.id = j1939_build_message_id(false, false, J1939_Priority_6, pgn_with_destination,
					  0x40U);

	zassert_false(j1939_is_msg_for_me(&frame, &test_node),
		      "Unicast message addressed to a different node should not be for us");
}

/* ── Transmit paths, verified via real CAN loopback capture ── */

ZTEST(j1939_core, test_transmit_pgn_broadcast_single_frame)
{
	uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	int filter_id = install_capture_filter((j1939_pdu_format_t)(TEST_PGN >> 8));

	zassert_true(filter_id >= 0, "Failed to install capture filter");

	zassert_true(j1939_transmit_pgn(J1939_Priority_6, TEST_PGN, J1939_GLOBAL_ADDRESS, data,
					sizeof(data), &test_node),
		     "Broadcast single-frame transmit should succeed");
	k_sleep(TX_SETTLE_TIME);

	zassert_true(captured_frame_valid, "Expected to capture the transmitted frame");
	zassert_equal(captured_frame.dlc, sizeof(data), "Captured frame length should match");
	zassert_mem_equal(captured_frame.data, data, sizeof(data),
			   "Captured frame payload should match");

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(j1939_core, test_transmit_pgn_over_8_bytes_uses_transport_protocol)
{
	/* j1939_tp_transmit_session_exists() only tracks unicast (RTS/CTS) destinations by
	 * design — it returns false immediately for J1939_GLOBAL_ADDRESS (BAM), so use a real
	 * unicast destination here, matching the tp/ test suite's own pattern.
	 */
	const j1939_destination_address_t destination = 0x40U;
	uint8_t data[J1939TP_MIN_BYTES] = {0xAA};

	zassert_false(j1939_tp_transmit_session_exists(destination, &test_node),
		      "No transport session should exist before transmit");

	zassert_true(j1939_transmit_pgn(J1939_Priority_6, TEST_PGN, destination, data,
					sizeof(data), &test_node),
		     "Multi-packet transmit should be accepted by the transport layer");

	zassert_true(j1939_tp_transmit_session_exists(destination, &test_node),
		     "A transport session should exist after a >8 byte transmit");
}

ZTEST(j1939_core, test_build_and_queue_message_round_trip)
{
	uint8_t data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
	j1939_arbitration_t id =
		j1939_build_message_id(false, false, J1939_Priority_3,
				       j1939_build_pgn(false, false, 0xFDU, 0x11U), 0x21U);
	int filter_id = install_capture_filter(0xFDU);

	zassert_true(filter_id >= 0, "Failed to install capture filter");

	zassert_true(j1939_build_and_queue_message(&test_node, id, sizeof(data), true, data),
		     "build_and_queue_message should succeed");
	k_sleep(TX_SETTLE_TIME);

	zassert_true(captured_frame_valid, "Expected to capture the transmitted frame");
	zassert_equal(captured_frame.id, id, "Captured arbitration ID should match");
	zassert_mem_equal(captured_frame.data, data, sizeof(data),
			   "Captured frame payload should match");

	can_remove_rx_filter(can_dev, filter_id);
}

ZTEST(j1939_core, test_acknowledge_sends_ack_frame)
{
	int filter_id = install_capture_filter(J1939_PGN_ACK_PF);

	zassert_true(filter_id >= 0, "Failed to install capture filter");

	j1939_acknowledge(TEST_PGN, J1939_Response_Ack, 0x40U, &test_node);
	k_sleep(TX_SETTLE_TIME);

	zassert_true(captured_frame_valid, "Expected to capture the ACK frame");
	zassert_equal(captured_frame.data[0], (uint8_t)J1939_Response_Ack,
		      "First byte of ACK payload should be the control code");
	zassert_equal(captured_frame.data[5], LOBYTE(LOWORD(TEST_PGN)),
		      "ACK payload should embed the acknowledged PGN");
	zassert_equal(captured_frame.data[6], HIBYTE(LOWORD(TEST_PGN)),
		      "ACK payload should embed the acknowledged PGN");

	can_remove_rx_filter(can_dev, filter_id);
}

/* ── PGN request tracking ── */

ZTEST(j1939_core, test_flag_and_is_pgn_requested_round_trip)
{
	j1939_source_address_t source = J1939_NULL_ADDRESS;

	zassert_true(j1939_register_request_pgn(TEST_REQUEST_PGN, &test_node),
		     "Registering a request PGN should succeed");
	zassert_false(j1939_is_pgn_requested(TEST_REQUEST_PGN, &source, &test_node),
		      "PGN should not be flagged as requested until flagged");

	zassert_true(j1939_flag_pgn_request(TEST_REQUEST_PGN, 0x40U, &test_node),
		     "Flagging a registered PGN request should succeed");
	zassert_true(j1939_is_pgn_requested(TEST_REQUEST_PGN, &source, &test_node),
		     "PGN should be flagged as requested after flag_pgn_request");
	zassert_equal(source, 0x40U, "Requesting source address should be reported");

	zassert_false(j1939_is_pgn_requested(TEST_REQUEST_PGN, &source, &test_node),
		      "Request flag should clear after being read once");
}

ZTEST(j1939_core, test_request_pgn_frame_flags_registered_pgn)
{
	struct can_frame frame = {0};
	j1939_source_address_t source = J1939_NULL_ADDRESS;

	zassert_true(j1939_register_request_pgn(TEST_REQUEST_PGN, &test_node),
		     "Registering a request PGN should succeed");

	frame.flags = CAN_FRAME_IDE;
	frame.id = j1939_build_message_id(false, false, J1939_Priority_6, J1939_REQUEST_PGN, 0x40U);
	frame.dlc = 3;
	frame.data[0] = LOBYTE(LOWORD(TEST_REQUEST_PGN));
	frame.data[1] = HIBYTE(LOWORD(TEST_REQUEST_PGN));
	frame.data[2] = LOBYTE(HIWORD(TEST_REQUEST_PGN));

	(void)j1939_request_pgn(&frame, &test_node);

	zassert_true(j1939_is_pgn_requested(TEST_REQUEST_PGN, &source, &test_node),
		     "Receiving a Request PGN frame should flag the registered PGN");
	zassert_equal(source, 0x40U, "Requesting source address should be recorded");
}

ZTEST(j1939_core, test_register_request_pgn_capacity_exhaustion)
{
	bool ok = true;

	for (int i = 0; i < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES && ok; i++) {
		ok = j1939_register_request_pgn((j1939_pgn_t)(0xFD00U + i), &test_node);
		zassert_true(ok, "Registration %d unexpectedly failed", i);
	}

	zassert_false(j1939_register_request_pgn(0xFDFFU, &test_node),
		      "Registration beyond capacity should return false");
}

/* ── Virtual transmit-mode flag (round-trip only; not currently wired to any transmit path) ── */

ZTEST(j1939_core, test_virtual_mode_transmit_flag_round_trip)
{
	j1939_enable_virtual_mode_transmit(&test_node);
	zassert_true(j1939_is_virtual_node_transmit_enabled(&test_node),
		     "Flag should read back true after enabling");

	j1939_disable_virtual_mode_transmit(&test_node);
	zassert_false(j1939_is_virtual_node_transmit_enabled(&test_node),
		      "Flag should read back false after disabling");
}
