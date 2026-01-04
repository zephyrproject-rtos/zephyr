/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BAP_UNICAST_SERVER_EXPECTS_H_
#define MOCKS_BAP_UNICAST_SERVER_EXPECTS_H_

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/ztest_assert.h>

#include "bap_unicast_server.h"
#include "expects_util.h"

static inline void expect_bt_bap_unicast_server_cb_config_called(
	unsigned int expected_count,
	struct bt_conn *conns[],
	struct bt_bap_ep *eps[],
	enum bt_audio_dir dirs[],
	void *codec[])
{
	const char *func_name = "bt_bap_unicast_server_cb.config";

	zexpect_call_count(func_name,
		expected_count, mock_bap_unicast_server_cb_config_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_unicast_server_cb_config_fake.call_count; i++) {
		zassert_equal_ptr(conns[i],
			mock_bap_unicast_server_cb_config_fake.arg0_history[i],
			"'%s()' was called with incorrect 'conn[%i]' value", func_name, i);
		if (eps) {
			zassert_equal_ptr(eps[i],
				mock_bap_unicast_server_cb_config_fake.arg1_history[i],
				"'%s()' was called with incorrect 'ep[%i]' value", func_name, i);
		}
		zassert_equal(dirs[i],
			mock_bap_unicast_server_cb_config_fake.arg2_history[i],
			"'%s()' was called with incorrect 'dir[%i]' value", func_name, i);
	}

	if (codec) {
		/* TODO */
		zassert_unreachable("Not implemented");
	}
}

static inline void expect_bt_bap_unicast_server_cb_reconfig_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[],
	const enum bt_audio_dir dirs[],
	void *codec[])
{
	const char *func_name = "bt_bap_unicast_server_cb.reconfig";

	zexpect_call_count(func_name,
		expected_count, mock_bap_unicast_server_cb_reconfig_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_unicast_server_cb_reconfig_fake.call_count; i++) {
		zassert_equal_ptr(streams[i],
			mock_bap_unicast_server_cb_reconfig_fake.arg0_history[i],
			"'%s()' was called with incorrect 'stream[%i]' value", func_name, i);
		zassert_equal(dirs[i],
			mock_bap_unicast_server_cb_reconfig_fake.arg1_history[i],
			"'%s()' was called with incorrect 'dir[%i]' value", func_name, i);
	}

	if (codec) {
		/* TODO */
		zassert_unreachable("Not implemented");
	}
}

static inline void expect_bt_bap_unicast_server_cb_qos_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[],
	void *qos[])
{
	const char *func_name = "bt_bap_unicast_server_cb.qos";

	zexpect_call_count(func_name,
		expected_count, mock_bap_unicast_server_cb_qos_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_unicast_server_cb_qos_fake.call_count; i++) {
		zassert_equal_ptr(streams[i],
			mock_bap_unicast_server_cb_qos_fake.arg0_history[i],
			"'%s()' was called with incorrect 'stream[%i]' value", func_name, i);
	}

	if (qos) {
		/* TODO */
		zassert_unreachable("Not implemented");
	}
}

static inline void expect_bt_bap_unicast_server_cb_enable_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[],
	void *meta[],
	void *meta_len[])
{
	const char *func_name = "bt_bap_unicast_server_cb.enable";

	zexpect_call_count(func_name,
		expected_count, mock_bap_unicast_server_cb_enable_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_unicast_server_cb_enable_fake.call_count; i++) {
		zassert_equal_ptr(streams[i],
			mock_bap_unicast_server_cb_enable_fake.arg0_history[i],
			"'%s()' was called with incorrect 'stream[%i]' value", func_name, i);
	}

	if (meta) {
		/* TODO */
		zassert_unreachable("Not implemented");
	}

	if (meta_len) {
		/* TODO */
		zassert_unreachable("Not implemented");
	}
}

static inline void expect_bt_bap_unicast_server_cb_metadata_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[],
	void *meta[],
	void *meta_len[])
{
	const char *func_name = "bt_bap_unicast_server_cb.enable";

	zexpect_call_count(func_name,
		expected_count, mock_bap_unicast_server_cb_metadata_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_unicast_server_cb_metadata_fake.call_count; i++) {
		zassert_equal_ptr(streams[i],
			mock_bap_unicast_server_cb_metadata_fake.arg0_history[i],
			"'%s()' was called with incorrect 'stream[%i]' value", func_name, i);
	}

	if (meta) {
		/* TODO */
		zassert_unreachable("Not implemented");
	}

	if (meta_len) {
		/* TODO */
		zassert_unreachable("Not implemented");
	}
}

static inline void expect_bt_bap_unicast_server_cb_disable_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_unicast_server_cb.disable";

	zexpect_call_count(func_name,
		expected_count, mock_bap_unicast_server_cb_disable_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_unicast_server_cb_disable_fake.call_count; i++) {
		zassert_equal_ptr(streams[i],
			mock_bap_unicast_server_cb_disable_fake.arg0_history[i],
			"'%s()' was called with incorrect 'stream[%i]' value", func_name, i);
	}
}

static inline void expect_bt_bap_unicast_server_cb_release_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_unicast_server_cb.release";

	zexpect_call_count(func_name,
		expected_count, mock_bap_unicast_server_cb_release_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_unicast_server_cb_release_fake.call_count; i++) {
		zassert_equal_ptr(streams[i],
			mock_bap_unicast_server_cb_release_fake.arg0_history[i],
			"'%s()' was called with incorrect 'stream[%i]' value", func_name, i);
	}
}

static inline void expect_bt_bap_unicast_server_cb_start_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_unicast_server_cb.start";

	zexpect_call_count(func_name,
		expected_count, mock_bap_unicast_server_cb_start_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_unicast_server_cb_start_fake.call_count; i++) {
		zassert_equal_ptr(streams[i],
			mock_bap_unicast_server_cb_start_fake.arg0_history[i],
			"'%s()' was called with incorrect 'stream[%i]' value", func_name, i);
	}
}

static inline void expect_bt_bap_unicast_server_cb_stop_called(
	unsigned int expected_count,
	struct bt_bap_stream *streams[])
{
	const char *func_name = "bt_bap_unicast_server_cb.stop";

	zexpect_call_count(func_name,
		expected_count, mock_bap_unicast_server_cb_stop_fake.call_count);

	for (unsigned int i = 0; i < mock_bap_unicast_server_cb_stop_fake.call_count; i++) {
		zassert_equal_ptr(streams[i],
			mock_bap_unicast_server_cb_stop_fake.arg0_history[i],
			"'%s()' was called with incorrect 'stream[%i]' value", func_name, i);
	}
}

#endif /* MOCKS_BAP_UNICAST_SERVER_EXPECTS_H_ */
