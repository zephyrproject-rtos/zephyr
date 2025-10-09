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

#define expect_bt_bap_unicast_server_cb_config_called(_expected_count, _conns, _eps, _dirs, _codec)    \
do {                                                                                                   \
	const char *func_name = "bt_bap_unicast_server_cb.config";                                         \
                                                                                                       \
	zexpect_call_count(func_name, _expected_count, mock_bap_unicast_server_cb_config_fake.call_count); \
                                                                                                       \
	for (int i = 0; i < mock_bap_unicast_server_cb_config_fake.call_count; i++) {                      \
		IF_NOT_EMPTY(_conns[i], (                                                                      \
			zassert_equal_ptr(_conns[i],                                                               \
				mock_bap_unicast_server_cb_config_fake.arg0_history[i],                                \
				"'%s()' was called with incorrect 'conn[%i]' value",                                   \
				func_name, i);))                                                                       \
                                                                                                       \
		IF_NOT_EMPTY(_eps[i], (                                                                        \
			zassert_equal_ptr(_eps[i],                                                                 \
				mock_bap_unicast_server_cb_config_fake.arg1_history[i],                                \
				"'%s()' was called with incorrect 'ep[%i]' value",                                     \
				func_name, i);))                                                                       \
                                                                                                       \
		IF_NOT_EMPTY(_dirs[i], (                                                                       \
			zassert_equal(_dirs[i],                                                                    \
				mock_bap_unicast_server_cb_config_fake.arg2_history[i],                                \
				"'%s()' was called with incorrect 'dir[%i]' value",                                    \
				func_name, i);))                                                                       \
	}                                                                                                  \
                                                                                                       \
	IF_NOT_EMPTY(_codec, (                                                                             \
		/* TODO */                                                                                     \
		zassert_unreachable("Not implemented");))                                                      \
} while (0)

#define expect_bt_bap_unicast_server_cb_reconfig_called(_expected_count, _streams, _dirs, _codec)        \
do {                                                                                                     \
	const char *func_name = "bt_bap_unicast_server_cb.reconfig";                                         \
                                                                                                         \
	zexpect_call_count(func_name, _expected_count, mock_bap_unicast_server_cb_reconfig_fake.call_count); \
                                                                                                         \
	for (int i = 0; i < mock_bap_unicast_server_cb_reconfig_fake.call_count; i++) {                      \
		IF_NOT_EMPTY(_streams[i], (                                                                      \
			zassert_equal_ptr(_streams[i],                                                               \
				mock_bap_unicast_server_cb_reconfig_fake.arg0_history[i],                                \
				"'%s()' was called with incorrect 'stream[%i]' value",                                   \
				func_name, i);))                                                                         \
                                                                                                         \
		IF_NOT_EMPTY(_dirs[i], (                                                                         \
			zassert_equal(_dirs[i],                                                                      \
				mock_bap_unicast_server_cb_reconfig_fake.arg1_history[i],                                \
				"'%s()' was called with incorrect 'dir[%i]' value",                                      \
				func_name, i);))                                                                         \
	}                                                                                                    \
                                                                                                         \
	IF_NOT_EMPTY(_codec, (                                                                               \
		/* TODO */                                                                                       \
		zassert_unreachable("Not implemented");))                                                        \
} while (0)

#define expect_bt_bap_unicast_server_cb_qos_called(_expected_count, _streams, _qos)                  \
do {                                                                                                 \
	const char *func_name = "bt_bap_unicast_server_cb.qos";                                          \
                                                                                                     \
	zexpect_call_count(func_name, _expected_count, mock_bap_unicast_server_cb_qos_fake.call_count);  \
                                                                                                     \
	for (int i = 0; i < mock_bap_unicast_server_cb_qos_fake.call_count; i++) {                       \
		IF_NOT_EMPTY(_streams[i], (                                                                  \
			zassert_equal_ptr(_streams[i],                                                           \
				mock_bap_unicast_server_cb_qos_fake.arg0_history[i],                                 \
				"'%s()' was called with incorrect 'stream[%i]' value",                               \
				func_name, i);))                                                                     \
	}                                                                                                \
                                                                                                     \
	IF_NOT_EMPTY(_qos, (                                                                             \
		/* TODO */                                                                                   \
		zassert_unreachable("Not implemented");))                                                    \
} while (0)

#define expect_bt_bap_unicast_server_cb_enable_called(_expected_count, _streams, _meta, _meta_len)       \
do {                                                                                                     \
	const char *func_name = "bt_bap_unicast_server_cb.enable";                                           \
                                                                                                         \
	zexpect_call_count(func_name, _expected_count, mock_bap_unicast_server_cb_enable_fake.call_count);   \
                                                                                                         \
	for (int i = 0; i < mock_bap_unicast_server_cb_enable_fake.call_count; i++) {                        \
		IF_NOT_EMPTY(_streams[i], (                                                                      \
			zassert_equal_ptr(_streams[i],                                                               \
				mock_bap_unicast_server_cb_enable_fake.arg0_history[i],                                  \
				"'%s()' was called with incorrect 'stream[%i]' value",                                   \
				func_name, i);))                                                                         \
                                                                                                         \
	IF_NOT_EMPTY(_meta, (                                                                                \
		/* TODO */                                                                                       \
		zassert_unreachable("Not implemented");))                                                        \
                                                                                                         \
	IF_NOT_EMPTY(_meta_len, (                                                                            \
		/* TODO */                                                                                       \
		zassert_unreachable("Not implemented");))                                                        \
	}                                                                                                    \
} while (0)

#define expect_bt_bap_unicast_server_cb_metadata_called(_expected_count, _streams, _meta, _meta_len)     \
do {                                                                                                     \
	const char *func_name = "bt_bap_unicast_server_cb.enable";                                           \
                                                                                                         \
	zexpect_call_count(func_name, _expected_count, mock_bap_unicast_server_cb_metadata_fake.call_count); \
                                                                                                         \
	for (int i = 0; i < mock_bap_unicast_server_cb_metadata_fake.call_count; i++) {                      \
		IF_NOT_EMPTY(_streams[i], (                                                                      \
			zassert_equal_ptr(_streams[i],                                                               \
				mock_bap_unicast_server_cb_metadata_fake.arg0_history[i],                                \
				"'%s()' was called with incorrect 'stream[%i]' value",                                   \
				func_name, i);))                                                                         \
	}                                                                                                    \
                                                                                                         \
	IF_NOT_EMPTY(_meta, (                                                                                \
		/* TODO */                                                                                       \
		zassert_unreachable("Not implemented");))                                                        \
                                                                                                         \
	IF_NOT_EMPTY(_meta_len, (                                                                            \
		/* TODO */                                                                                       \
		zassert_unreachable("Not implemented");))                                                        \
} while (0)

#define expect_bt_bap_unicast_server_cb_disable_called(_expected_count, _streams)                       \
do {                                                                                                    \
	const char *func_name = "bt_bap_unicast_server_cb.disable";                                         \
                                                                                                        \
	zexpect_call_count(func_name, _expected_count, mock_bap_unicast_server_cb_disable_fake.call_count); \
                                                                                                        \
	for (int i = 0; i < mock_bap_unicast_server_cb_disable_fake.call_count; i++) {                      \
		IF_NOT_EMPTY(_streams[i], (                                                                     \
			zassert_equal_ptr(_streams[i],                                                              \
				mock_bap_unicast_server_cb_disable_fake.arg0_history[i],                                \
				"'%s()' was called with incorrect 'stream[%i]' value",                                  \
				func_name, i);))                                                                        \
	}                                                                                                   \
} while (0)

#define expect_bt_bap_unicast_server_cb_release_called(_expected_count, _streams)                       \
do {                                                                                                    \
	const char *func_name = "bt_bap_unicast_server_cb.release";                                         \
                                                                                                        \
	zexpect_call_count(func_name, _expected_count, mock_bap_unicast_server_cb_release_fake.call_count); \
                                                                                                        \
	for (int i = 0; i < mock_bap_unicast_server_cb_release_fake.call_count; i++) {                      \
		IF_NOT_EMPTY(_streams[i], (                                                                     \
			zassert_equal_ptr(_streams[i],                                                              \
				mock_bap_unicast_server_cb_release_fake.arg0_history[i],                                \
				"'%s()' was called with incorrect 'stream[%i]' value",                                  \
				func_name, i);))                                                                        \
	}                                                                                                   \
} while (0)

#define expect_bt_bap_unicast_server_cb_start_called(_expected_count, _streams)                         \
do {                                                                                                    \
	const char *func_name = "bt_bap_unicast_server_cb.start";                                           \
                                                                                                        \
	zexpect_call_count(func_name, _expected_count, mock_bap_unicast_server_cb_start_fake.call_count);   \
                                                                                                        \
	for (int i = 0; i < mock_bap_unicast_server_cb_start_fake.call_count; i++) {                        \
		IF_NOT_EMPTY(_streams[i], (                                                                     \
			zassert_equal_ptr(_streams[i],                                                              \
				mock_bap_unicast_server_cb_start_fake.arg0_history[i],                                  \
				"'%s()' was called with incorrect 'stream[%i]' value",                                  \
				func_name, i);))                                                                        \
	}                                                                                                   \
} while (0)

#define expect_bt_bap_unicast_server_cb_stop_called(_expected_count, _streams)                          \
do {                                                                                                    \
	const char *func_name = "bt_bap_unicast_server_cb.stop";                                            \
                                                                                                        \
	zexpect_call_count(func_name, _expected_count, mock_bap_unicast_server_cb_stop_fake.call_count);    \
                                                                                                        \
	for (int i = 0; i < mock_bap_unicast_server_cb_stop_fake.call_count; i++) {                         \
		IF_NOT_EMPTY(_streams[i], (                                                                     \
			zassert_equal_ptr(_streams[i],                                                              \
				mock_bap_unicast_server_cb_stop_fake.arg0_history[i],                                   \
				"'%s()' was called with incorrect 'stream[%i]' value",                                  \
				func_name, i);))                                                                        \
	}                                                                                                   \
} while (0)

static inline void expect_bt_bap_unicast_server_cb_config_not_called(void)
{
	const char *func_name = "bt_bap_unicast_server_cb.config";

	zexpect_call_count(func_name, 0, mock_bap_unicast_server_cb_config_fake.call_count);
}

#endif /* MOCKS_BAP_UNICAST_SERVER_EXPECTS_H_ */
