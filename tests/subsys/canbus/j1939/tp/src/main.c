/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/ztest.h>
#include <zephyr/canbus/j1939.h>

#include "J1939Tp.h"

/* J1939TP_NUM_ALL_BUFFERS is a plain C macro computed inside J1939Tp.c (sum of the tier buffer
 * counts), not a Kconfig symbol, so there is no CONFIG_J1939TP_NUM_ALL_BUFFERS to set via
 * prj.conf. Recompute it here from the real Kconfig symbols used below.
 */
#define CONFIG_J1939TP_NUM_ALL_BUFFERS                                                           \
	(CONFIG_J1939TP_NUM_SMALL_BUFFERS + CONFIG_J1939TP_NUM_MEDIUM_BUFFERS +                   \
	 CONFIG_J1939TP_NUM_LARGE_BUFFERS + CONFIG_J1939TP_NUM_MAX_BUFFERS)

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

J1939_NODE_DEFINE(test_node,
	can_dev,
	0x20,  /* source address */
	100,   /* ID number */
	1,     /* manufacturer code */
	0,     /* ECU instance */
	0,     /* function instance */
	0,     /* function */
	0,     /* vehicle system */
	0,     /* vehicle system instance */
	0,     /* industry group */
	false  /* arbitrary address capable */
);

static bool tp_rx_callback(uint16_t pgn, uint8_t *data, uint32_t length,
			   uint8_t sender, j1939_node_t node)
{
	ARG_UNUSED(pgn);
	ARG_UNUSED(data);
	ARG_UNUSED(length);
	ARG_UNUSED(sender);
	ARG_UNUSED(node);
	return true;
}

static void *tp_suite_setup(void)
{
	zassert_true(device_is_ready(can_dev), "CAN device not ready");
	can_start(can_dev);
	return NULL;
}

static void tp_before(void *fixture)
{
	ARG_UNUSED(fixture);
	test_node.j1939_tp_transmit_bam = false;
	j1939_tp_init();
}

ZTEST_SUITE(j1939_tp, NULL, tp_suite_setup, tp_before, NULL, NULL);

/* ── Buffer pool ── */

ZTEST(j1939_tp, test_tp_get_buffer_min_size)
{
	uint8_t *buf = j1939_tp_get_buffer(J1939TP_MIN_BYTES);

	zassert_not_null(buf, "Buffer allocation for minimum size failed");
	j1939_tp_free_buffer(buf);
}

ZTEST(j1939_tp, test_tp_get_buffer_free_reuse)
{
	uint8_t *buf = j1939_tp_get_buffer(J1939TP_MIN_BYTES);

	zassert_not_null(buf, NULL);
	j1939_tp_free_buffer(buf);

	buf = j1939_tp_get_buffer(J1939TP_MIN_BYTES);
	zassert_not_null(buf, "Buffer not available after free");
	j1939_tp_free_buffer(buf);
}

ZTEST(j1939_tp, test_tp_buffer_exhaust_returns_null)
{
	uint8_t *bufs[CONFIG_J1939TP_NUM_ALL_BUFFERS];

	for (int i = 0; i < CONFIG_J1939TP_NUM_ALL_BUFFERS; i++) {
		bufs[i] = j1939_tp_get_buffer(J1939TP_MIN_BYTES);
		zassert_not_null(bufs[i], "Unexpected NULL at allocation %d", i);
	}

	uint8_t *extra = j1939_tp_get_buffer(J1939TP_MIN_BYTES);

	zassert_is_null(extra, "Expected NULL when pool is exhausted");

	for (int i = 0; i < CONFIG_J1939TP_NUM_ALL_BUFFERS; i++) {
		j1939_tp_free_buffer(bufs[i]);
	}
}

ZTEST(j1939_tp, test_tp_max_available_buffer_size)
{
	j1939_counter_t max = j1939_tp_get_max_available_buffer_size();

	zassert_true(max >= J1939TP_MIN_BYTES, "Max available buffer size %u below minimum", max);
}

ZTEST(j1939_tp, test_tp_max_buffer_size_zero_when_exhausted)
{
	uint8_t *bufs[CONFIG_J1939TP_NUM_ALL_BUFFERS];

	for (int i = 0; i < CONFIG_J1939TP_NUM_ALL_BUFFERS; i++) {
		bufs[i] = j1939_tp_get_buffer(J1939TP_MIN_BYTES);
	}

	j1939_counter_t max = j1939_tp_get_max_available_buffer_size();

	zassert_equal(max, 0, "Expected 0 when all buffers are in use, got %u", max);

	for (int i = 0; i < CONFIG_J1939TP_NUM_ALL_BUFFERS; i++) {
		j1939_tp_free_buffer(bufs[i]);
	}
}

/* ── Registration ── */

ZTEST(j1939_tp, test_tp_register_callback_first)
{
	bool ok = j1939_tp_register_message_callback(0xFECAU, &test_node, tp_rx_callback);

	zassert_true(ok, "First callback registration failed");
}

ZTEST(j1939_tp, test_tp_register_callback_duplicate_updates)
{
	j1939_tp_register_message_callback(0xFECAU, &test_node, tp_rx_callback);
	bool ok = j1939_tp_register_message_callback(0xFECAU, &test_node, tp_rx_callback);

	zassert_true(ok, "Duplicate registration should update callback and return true");
}

ZTEST(j1939_tp, test_tp_register_callback_full)
{
	for (int i = 0; i < CONFIG_J1939TP_NUM_ALLOWED_RECEIVE_PGN; i++) {
		bool ok = j1939_tp_register_message_callback(0xFE00U + i, &test_node,
							     tp_rx_callback);
		zassert_true(ok, "Registration %d unexpectedly failed", i);
	}

	bool overflow = j1939_tp_register_message_callback(0xFF00U, &test_node, tp_rx_callback);

	zassert_false(overflow, "Registration beyond capacity should return false");
}

/* ── Transmit acceptance/rejection ── */

ZTEST(j1939_tp, test_tp_transmit_too_small)
{
	uint8_t data[J1939TP_MIN_BYTES - 1] = {0};
	J1939Tp_Message_T result;

	result = j1939_tp_transmit_multi_packet(0xFECAU, J1939_GLOBAL_ADDRESS,
						J1939TP_MIN_BYTES - 1, data, &test_node);

	zassert_equal(result, j1939_tp_message_not_accepted,
		      "Expected rejection for %u bytes (below minimum %u)",
		      J1939TP_MIN_BYTES - 1, J1939TP_MIN_BYTES);
}

ZTEST(j1939_tp, test_tp_transmit_too_large)
{
	uint8_t data[1] = {0};
	J1939Tp_Message_T result;

	result = j1939_tp_transmit_multi_packet(0xFECAU, J1939_GLOBAL_ADDRESS,
						J1939TP_MAX_BYTES + 1, data, &test_node);

	zassert_equal(result, j1939_tp_message_not_accepted,
		      "Expected rejection for %u bytes (above maximum %u)",
		      J1939TP_MAX_BYTES + 1, J1939TP_MAX_BYTES);
}

ZTEST(j1939_tp, test_tp_transmit_bam_accepted)
{
	uint8_t data[J1939TP_MIN_BYTES];
	J1939Tp_Message_T result;

	memset(data, 0xAA, sizeof(data));
	result = j1939_tp_transmit_multi_packet(0xFECAU, J1939_GLOBAL_ADDRESS,
						J1939TP_MIN_BYTES, data, &test_node);

	zassert_equal(result, j1939_tp_message_accepted, "BAM transmission should be accepted");
}

ZTEST(j1939_tp, test_tp_transmit_bam_rejected_when_active)
{
	uint8_t data[J1939TP_MIN_BYTES];
	J1939Tp_Message_T result;

	memset(data, 0x55, sizeof(data));
	j1939_tp_transmit_multi_packet(0xFECAU, J1939_GLOBAL_ADDRESS,
				       J1939TP_MIN_BYTES, data, &test_node);

	/* Second BAM while the first is still active must be rejected */
	memset(data, 0x55, sizeof(data));
	result = j1939_tp_transmit_multi_packet(0xFECAU, J1939_GLOBAL_ADDRESS,
						J1939TP_MIN_BYTES, data, &test_node);

	zassert_equal(result, j1939_tp_message_not_accepted,
		      "Second concurrent BAM should be rejected");
}

ZTEST(j1939_tp, test_tp_transmit_rts_accepted)
{
	uint8_t data[J1939TP_MIN_BYTES];
	J1939Tp_Message_T result;

	memset(data, 0xBB, sizeof(data));
	result = j1939_tp_transmit_multi_packet(0xFECAU, 0x34U,
						J1939TP_MIN_BYTES, data, &test_node);

	zassert_equal(result, j1939_tp_message_accepted,
		      "RTS-CTS transmission should be accepted");
}

ZTEST(j1939_tp, test_tp_transmit_rts_max_bytes_accepted)
{
	uint8_t data[J1939TP_MAX_BYTES];
	J1939Tp_Message_T result;

	memset(data, 0xCC, sizeof(data));
	result = j1939_tp_transmit_multi_packet(0xFECAU, 0x34U,
						J1939TP_MAX_BYTES, data, &test_node);

	zassert_equal(result, j1939_tp_message_accepted,
		      "RTS-CTS at maximum size should be accepted");
}

ZTEST(j1939_tp, test_tp_session_not_exists_after_init)
{
	bool exists = j1939_tp_transmit_session_exists(0x34U, &test_node);

	zassert_false(exists, "No transmit sessions should exist immediately after init");
}

ZTEST(j1939_tp, test_tp_session_exists_after_rts)
{
	uint8_t data[J1939TP_MIN_BYTES];

	memset(data, 0xDD, sizeof(data));
	j1939_tp_transmit_multi_packet(0xFECAU, 0x34U, J1939TP_MIN_BYTES, data, &test_node);

	bool exists = j1939_tp_transmit_session_exists(0x34U, &test_node);

	zassert_true(exists, "Transmit session should exist after RTS");
}
