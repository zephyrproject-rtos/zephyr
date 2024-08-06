/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BAP_UNICAST_SERVER_EXPECTS_H_
#define MOCKS_BAP_UNICAST_SERVER_EXPECTS_H_

#include <zephyr/bluetooth/audio/bap.h>

#include "bap_unicast_server.h"
#include "expects_util.h"

#define expect_bt_bap_unicast_server_cb_config_called_once(_conn, _ep, _dir, _codec)               \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.config";                                 \
												   \
	zexpect_call_count(func_name, 1, mock_bap_unicast_server_cb_config_fake.call_count);       \
												   \
	IF_NOT_EMPTY(_conn, (                                                                      \
		     zassert_equal_ptr(_conn, mock_bap_unicast_server_cb_config_fake.arg0_val,     \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "conn");))                                       \
												   \
	IF_NOT_EMPTY(_ep, (                                                                        \
		     zassert_equal_ptr(_ep, mock_bap_unicast_server_cb_config_fake.arg1_val,       \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "ep");))                                         \
												   \
	IF_NOT_EMPTY(_dir, (                                                                       \
		     zassert_equal(_dir, mock_bap_unicast_server_cb_config_fake.arg2_val,          \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "_dir");))                                       \
												   \
	IF_NOT_EMPTY(_codec, (                                                                     \
		     /* TODO */                                                                    \
		     zassert_unreachable("Not implemented");))                                     \
} while (0)

#define expect_bt_bap_unicast_server_cb_reconfig_called_once(_stream, _dir, _codec)                \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.reconfig";                               \
												   \
	zexpect_call_count(func_name, 1, mock_bap_unicast_server_cb_reconfig_fake.call_count);     \
												   \
	IF_NOT_EMPTY(_stream, (                                                                    \
		     zassert_equal_ptr(_stream, mock_bap_unicast_server_cb_reconfig_fake.arg0_val, \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream");))                                     \
												   \
	IF_NOT_EMPTY(_dir, (                                                                       \
		     zassert_equal(_dir, mock_bap_unicast_server_cb_reconfig_fake.arg1_val,        \
				   "'%s()' was called with incorrect '%s' value",                  \
				   func_name, "_dir");))                                           \
												   \
	IF_NOT_EMPTY(_codec, (                                                                     \
		     /* TODO */                                                                    \
		     zassert_unreachable("Not implemented");))                                     \
} while (0)

#define expect_bt_bap_unicast_server_cb_qos_called_once(_stream, _qos)                             \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.qos";                                    \
												   \
	zexpect_call_count(func_name, 1, mock_bap_unicast_server_cb_qos_fake.call_count);          \
												   \
	IF_NOT_EMPTY(_stream, (                                                                    \
		     zassert_equal_ptr(_stream, mock_bap_unicast_server_cb_qos_fake.arg0_val,      \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream");))                                     \
												   \
	IF_NOT_EMPTY(_qos, (                                                                       \
		     /* TODO */                                                                    \
		     zassert_unreachable("Not implemented");))                                     \
} while (0)

#define expect_bt_bap_unicast_server_cb_enable_called_once(_stream, _meta, _meta_len)            \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.enable";                                 \
												   \
	zexpect_call_count(func_name, 1, mock_bap_unicast_server_cb_enable_fake.call_count);       \
												   \
	IF_NOT_EMPTY(_stream, (                                                                    \
		     zassert_equal_ptr(_stream, mock_bap_unicast_server_cb_enable_fake.arg0_val,   \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream");))                                     \
												   \
	IF_NOT_EMPTY(_meta, (                                                                      \
		     /* TODO */                                                                    \
		     zassert_unreachable("Not implemented");))                                     \
												   \
	IF_NOT_EMPTY(_meta_len, (                                                                \
		     /* TODO */                                                                    \
		     zassert_unreachable("Not implemented");))                                     \
} while (0)

#define expect_bt_bap_unicast_server_cb_metadata_called_once(_stream, _meta, _meta_len)          \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.enable";                                 \
												   \
	zexpect_call_count(func_name, 1, mock_bap_unicast_server_cb_metadata_fake.call_count);     \
												   \
	IF_NOT_EMPTY(_stream, (                                                                    \
		     zassert_equal_ptr(_stream, mock_bap_unicast_server_cb_metadata_fake.arg0_val, \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream");))                                     \
												   \
	IF_NOT_EMPTY(_meta, (                                                                      \
		     /* TODO */                                                                    \
		     zassert_unreachable("Not implemented");))                                     \
												   \
	IF_NOT_EMPTY(_meta_len, (                                                                \
		     /* TODO */                                                                    \
		     zassert_unreachable("Not implemented");))                                     \
} while (0)

#define expect_bt_bap_unicast_server_cb_disable_called_once(_stream)                               \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.disable";                                \
												   \
	zexpect_call_count(func_name, 1, mock_bap_unicast_server_cb_disable_fake.call_count);      \
												   \
	IF_NOT_EMPTY(_stream, (                                                                    \
		     zassert_equal_ptr(_stream, mock_bap_unicast_server_cb_disable_fake.arg0_val,  \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream");))                                     \
} while (0)

#define expect_bt_bap_unicast_server_cb_release_called_once(_stream)                               \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.release";                                \
												   \
	zexpect_call_count(func_name, 1, mock_bap_unicast_server_cb_release_fake.call_count);      \
												   \
	IF_NOT_EMPTY(_stream, (                                                                    \
		     zassert_equal_ptr(_stream, mock_bap_unicast_server_cb_release_fake.arg0_val,  \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream");))                                     \
} while (0)

#define expect_bt_bap_unicast_server_cb_release_called_twice(_streams)                             \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.release";                                \
												   \
	zexpect_call_count(func_name, 2, mock_bap_unicast_server_cb_release_fake.call_count);      \
												   \
	IF_NOT_EMPTY(_stream[0], (                                                                 \
		     zassert_equal_ptr(_streams[0],                                                \
				       mock_bap_unicast_server_cb_release_fake.arg0_history[0],    \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream[0]");))                                  \
	IF_NOT_EMPTY(_stream[1], (                                                                 \
		     zassert_equal_ptr(_streams[1],                                                \
				       mock_bap_unicast_server_cb_release_fake.arg0_history[1],    \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream[1]");))                                  \
} while (0)

#define expect_bt_bap_unicast_server_cb_start_called_once(_stream)                                 \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.start";                                  \
												   \
	zexpect_call_count(func_name, 1, mock_bap_unicast_server_cb_start_fake.call_count);        \
												   \
	IF_NOT_EMPTY(_stream, (                                                                    \
		     zassert_equal_ptr(_stream, mock_bap_unicast_server_cb_start_fake.arg0_val,    \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream");))                                     \
} while (0)

#define expect_bt_bap_unicast_server_cb_stop_called_once(_stream)                                  \
do {                                                                                               \
	const char *func_name = "bt_bap_unicast_server_cb.stop";                                   \
												   \
	zexpect_call_count(func_name, 1, mock_bap_unicast_server_cb_stop_fake.call_count);         \
												   \
	IF_NOT_EMPTY(_stream, (                                                                    \
		     zassert_equal_ptr(_stream, mock_bap_unicast_server_cb_stop_fake.arg0_val,     \
				       "'%s()' was called with incorrect '%s' value",              \
				       func_name, "stream");))                                     \
} while (0)

static inline void expect_bt_bap_unicast_server_cb_config_not_called(void)
{
	const char *func_name = "bt_bap_unicast_server_cb.config";

	zexpect_call_count(func_name, 0, mock_bap_unicast_server_cb_config_fake.call_count);
}

#endif /* MOCKS_BAP_UNICAST_SERVER_EXPECTS_H_ */
