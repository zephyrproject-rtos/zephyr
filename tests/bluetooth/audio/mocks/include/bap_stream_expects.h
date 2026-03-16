/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BAP_STREAM_EXPECTS_H_
#define MOCKS_BAP_STREAM_EXPECTS_H_

#include <stdbool.h>

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest_assert.h>

#include "bap_stream.h"
#include "expects_util.h"

static inline void expect_bt_bap_stream_ops_configured_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[],
	void *pref[])
{
	const char *func_name = "bt_bap_stream_ops.configured";

	zexpect_call_count(func_name,
		expected_count, mock_bap_stream_configured_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_configured_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_configured_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect 'stream[%i]' value",
			func_name, i);
	}

	if (pref) {
		/* TODO */
		zassert_unreachable("Not implemented");
	}
}

static inline void expect_bt_bap_stream_ops_qos_set_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_stream_ops.qos_set";

	zexpect_call_count(func_name,
		expected_count, mock_bap_stream_qos_set_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_qos_set_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_qos_set_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]'", func_name, "stream", i);
	}
}

static inline void expect_bt_bap_stream_ops_enabled_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_stream_ops.enabled";

	zexpect_call_count(func_name, expected_count, mock_bap_stream_enabled_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_enabled_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_enabled_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]'", func_name, "stream", i);
	}
}

static inline void expect_bt_bap_stream_ops_metadata_updated_called(
	int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_stream_ops.metadata_updated";

	zexpect_call_count(func_name,
		expected_count, mock_bap_stream_metadata_updated_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_metadata_updated_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_metadata_updated_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]'", func_name, "stream", i);
	}
}

static inline void expect_bt_bap_stream_ops_disabled_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_stream_ops.disabled";

	zexpect_call_count(func_name, expected_count, mock_bap_stream_disabled_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_disabled_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_disabled_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]'", func_name, "stream", i);
	}
}

static inline void expect_bt_bap_stream_ops_released_called(
	unsigned int expected_count,
	const struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_stream_ops.released";

	zexpect_call_count(func_name, expected_count, mock_bap_stream_released_cb_fake.call_count);

	for (unsigned int i = 0; i < expected_count; i++) {
		bool found = false;

		for (unsigned int j = 0; j < mock_bap_stream_released_cb_fake.call_count; j++) {
			found = streams[i] == mock_bap_stream_released_cb_fake.arg0_history[j];
			if (found) {
				break;
			}
		}

		zexpect_true(found, "'%s()' not called with %p stream", func_name, streams[i]);
	}
}

static inline void expect_bt_bap_stream_ops_started_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_stream_ops.started";

	zexpect_call_count(func_name, expected_count, mock_bap_stream_started_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_started_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_started_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]'", func_name, "stream", i);
	}
}

static inline void expect_bt_bap_stream_ops_stopped_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[],
	const uint8_t reasons[])
{
	const char *func_name = "bt_bap_stream_ops.stopped";

	zexpect_call_count(func_name, expected_count, mock_bap_stream_stopped_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_stopped_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_stopped_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]' value", func_name, "stream", i);
		zexpect_equal(reasons[i],
			mock_bap_stream_stopped_cb_fake.arg1_history[i],
			"'%s()' was called with incorrect '%s[%i]' value", func_name, "reason", i);
	}
}

static inline void
expect_bt_bap_stream_ops_connected_called(
	unsigned int expected_count,
	const struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_stream_ops.connected";

	zexpect_call_count(func_name,
		expected_count, mock_bap_stream_connected_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_connected_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_connected_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]'", func_name, "stream", i);
	}
}

static inline void
expect_bt_bap_stream_ops_disconnected_called(
	unsigned int expected_count,
	const struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_stream_ops.disconnected";

	zexpect_call_count(func_name,
		expected_count, mock_bap_stream_disconnected_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_disconnected_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_disconnected_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]'", func_name, "stream", i);
	}
}

static inline void
expect_bt_bap_stream_ops_recv_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[],
	const struct bt_iso_recv_info *info,
	struct net_buf *buf)
{
	const char *func_name = "bt_bap_stream_ops.recv";

	zexpect_call_count(func_name, expected_count, mock_bap_stream_recv_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_recv_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_recv_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]'", func_name, "stream", i);
	}

	/* TODO: validate info && buf */
}

static inline void expect_bt_bap_stream_ops_sent_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_stream_ops.sent";

	zexpect_call_count(func_name, expected_count, mock_bap_stream_sent_cb_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_stream_sent_cb_fake.call_count; i++) {
		zexpect_equal_ptr(streams[i],
			mock_bap_stream_sent_cb_fake.arg0_history[i],
			"'%s()' was called with incorrect '%s[%i]'", func_name, "stream", i);
	}
}

#endif /* MOCKS_BAP_STREAM_EXPECTS_H_ */
