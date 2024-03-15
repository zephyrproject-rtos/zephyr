/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BAP_STREAM_EXPECTS_H_
#define MOCKS_BAP_STREAM_EXPECTS_H_

#include <zephyr/bluetooth/audio/bap.h>

#include "bap_stream.h"
#include "expects_util.h"

#define expect_bt_bap_stream_ops_configured_called_once(_stream, _pref)                            \
do {                                                                                               \
	const char *func_name = "bt_bap_stream_ops.configured";                                    \
												   \
	zexpect_call_count(func_name, 1, mock_bap_stream_configured_cb_fake.call_count);           \
												   \
	if (mock_bap_stream_configured_cb_fake.call_count > 0) {                                   \
		IF_NOT_EMPTY(_stream, (                                                            \
			zexpect_equal_ptr(_stream, mock_bap_stream_configured_cb_fake.arg0_val,    \
					  "'%s()' was called with incorrect '%s' value",           \
					  func_name, "stream");))                                  \
												   \
		IF_NOT_EMPTY(_pref, (                                                              \
			/* TODO */                                                                 \
			zassert_unreachable("Not implemented");))                                  \
	}                                                                                          \
} while (0)

static inline void expect_bt_bap_stream_ops_configured_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.configured";

	zexpect_call_count(func_name, 0, mock_bap_stream_configured_cb_fake.call_count);
}

static inline void expect_bt_bap_stream_ops_qos_set_called_once(struct bt_bap_stream *stream)
{
	const char *func_name = "bt_bap_stream_ops.qos_set";

	zexpect_call_count(func_name, 1, mock_bap_stream_qos_set_cb_fake.call_count);

	if (mock_bap_stream_qos_set_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_qos_set_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}
}

static inline void expect_bt_bap_stream_ops_qos_set_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.qos_set";

	zexpect_call_count(func_name, 0, mock_bap_stream_qos_set_cb_fake.call_count);
}

static inline void expect_bt_bap_stream_ops_enabled_called_once(struct bt_bap_stream *stream)
{
	const char *func_name = "bt_bap_stream_ops.enabled";

	zexpect_call_count(func_name, 1, mock_bap_stream_enabled_cb_fake.call_count);

	if (mock_bap_stream_enabled_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_enabled_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}
}

static inline void expect_bt_bap_stream_ops_enabled_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.enabled";

	zexpect_call_count(func_name, 0, mock_bap_stream_enabled_cb_fake.call_count);
}

static inline void expect_bt_bap_stream_ops_metadata_updated_called_once(
								struct bt_bap_stream *stream)
{
	const char *func_name = "bt_bap_stream_ops.metadata_updated";

	zexpect_call_count(func_name, 1, mock_bap_stream_metadata_updated_cb_fake.call_count);

	if (mock_bap_stream_metadata_updated_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_metadata_updated_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}
}

static inline void expect_bt_bap_stream_ops_metadata_updated_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.metadata_updated";

	zexpect_call_count(func_name, 0, mock_bap_stream_metadata_updated_cb_fake.call_count);
}

static inline void expect_bt_bap_stream_ops_disabled_called_once(struct bt_bap_stream *stream)
{
	const char *func_name = "bt_bap_stream_ops.disabled";

	zexpect_call_count(func_name, 1, mock_bap_stream_disabled_cb_fake.call_count);

	if (mock_bap_stream_disabled_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_disabled_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}
}

static inline void expect_bt_bap_stream_ops_disabled_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.disabled";

	zexpect_call_count(func_name, 0, mock_bap_stream_disabled_cb_fake.call_count);
}

static inline void expect_bt_bap_stream_ops_released_called(const struct bt_bap_stream *streams[],
							    unsigned int count)
{
	const char *func_name = "bt_bap_stream_ops.released";

	zexpect_call_count(func_name, count, mock_bap_stream_released_cb_fake.call_count);

	for (unsigned int i = 0; i < count; i++) {
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

static inline void expect_bt_bap_stream_ops_released_called_once(const struct bt_bap_stream *stream)
{
	expect_bt_bap_stream_ops_released_called(&stream, 1);
}

static inline void expect_bt_bap_stream_ops_released_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.released";

	zexpect_equal(0, mock_bap_stream_released_cb_fake.call_count,
		      "'%s()' was called unexpectedly", func_name);
}

static inline void expect_bt_bap_stream_ops_started_called_once(struct bt_bap_stream *stream)
{
	const char *func_name = "bt_bap_stream_ops.started";

	zexpect_call_count(func_name, 1, mock_bap_stream_started_cb_fake.call_count);

	if (mock_bap_stream_started_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_started_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}
}

static inline void expect_bt_bap_stream_ops_started_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.started";

	zexpect_call_count(func_name, 0, mock_bap_stream_started_cb_fake.call_count);
}

#define expect_bt_bap_stream_ops_stopped_called_once(_stream, _reason)                             \
do {                                                                                               \
	const char *func_name = "bt_bap_stream_ops.stopped";                                       \
												   \
	zexpect_call_count(func_name, 1, mock_bap_stream_stopped_cb_fake.call_count);              \
												   \
	if (mock_bap_stream_stopped_cb_fake.call_count > 0) {                                      \
		IF_NOT_EMPTY(_stream, (                                                            \
			zexpect_equal_ptr(_stream, mock_bap_stream_stopped_cb_fake.arg0_val,       \
					  "'%s()' was called with incorrect '%s' value",           \
					  func_name, "stream");))                                  \
												   \
		IF_NOT_EMPTY(_reason, (                                                            \
			zexpect_equal(_reason, mock_bap_stream_stopped_cb_fake.arg1_val,           \
				      "'%s()' was called with incorrect '%s' value",               \
				      func_name, "reason");))                                      \
	}                                                                                          \
} while (0)

static inline void expect_bt_bap_stream_ops_stopped_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.stopped";

	zexpect_call_count(func_name, 0, mock_bap_stream_stopped_cb_fake.call_count);
}

static inline void
expect_bt_bap_stream_ops_connected_called_once(const struct bt_bap_stream *stream)
{
	const char *func_name = "bt_bap_stream_ops.connected";

	zexpect_call_count(func_name, 1, mock_bap_stream_connected_cb_fake.call_count);

	if (mock_bap_stream_connected_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_connected_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}
}

static inline void
expect_bt_bap_stream_ops_connected_called_twice(const struct bt_bap_stream *stream)
{
	const char *func_name = "bt_bap_stream_ops.connected";

	zexpect_call_count(func_name, 2, mock_bap_stream_connected_cb_fake.call_count);

	if (mock_bap_stream_connected_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_connected_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}
}

static inline void
expect_bt_bap_stream_ops_disconnected_called_once(const struct bt_bap_stream *stream)
{
	const char *func_name = "bt_bap_stream_ops.disconnected";

	zexpect_call_count(func_name, 1, mock_bap_stream_disconnected_cb_fake.call_count);

	if (mock_bap_stream_disconnected_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_disconnected_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}
}

static inline void expect_bt_bap_stream_ops_recv_called_once(struct bt_bap_stream *stream,
							     const struct bt_iso_recv_info *info,
							     struct net_buf *buf)
{
	const char *func_name = "bt_bap_stream_ops.recv";

	zexpect_call_count(func_name, 1, mock_bap_stream_recv_cb_fake.call_count);

	if (mock_bap_stream_recv_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_recv_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}

	/* TODO: validate info && buf */
}

static inline void expect_bt_bap_stream_ops_recv_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.recv";

	zexpect_call_count(func_name, 0, mock_bap_stream_recv_cb_fake.call_count);
}

static inline void expect_bt_bap_stream_ops_sent_called_once(struct bt_bap_stream *stream)
{
	const char *func_name = "bt_bap_stream_ops.sent";

	zexpect_call_count(func_name, 1, mock_bap_stream_sent_cb_fake.call_count);

	if (mock_bap_stream_sent_cb_fake.call_count > 0) {
		zexpect_equal_ptr(stream, mock_bap_stream_sent_cb_fake.arg0_val,
				  "'%s()' was called with incorrect '%s'", func_name, "stream");
	}
}

static inline void expect_bt_bap_stream_ops_sent_not_called(void)
{
	const char *func_name = "bt_bap_stream_ops.sent";

	zexpect_call_count(func_name, 0, mock_bap_stream_sent_cb_fake.call_count);
}

#endif /* MOCKS_BAP_STREAM_EXPECTS_H_ */
