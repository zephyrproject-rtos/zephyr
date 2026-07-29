/** @file
 *  @brief Bluetooth Call Control Profile (CCP) Call Controller role.
 *
 *  Copyright (c) 2020-2024 Nordic Semiconductor ASA
 *  Copyright (c) 2022 Codecoup
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#define URI_SEPARATOR ":"
#define CALLER_ID "friend"

static uint8_t new_call_index;
static char remote_uri[CONFIG_BT_TBS_MAX_URI_LENGTH + 1];

static K_SEM_DEFINE(sem_discovery_done, 0U, 1U);

static struct bt_conn *default_conn;

static void discover_cb(struct bt_conn *conn, int err, uint8_t tbs_count, bool gtbs_found)
{
	ARG_UNUSED(tbs_count);

	if (!gtbs_found) {
		printk("CCP: Failed to discover GTBS\n");
		return;
	}

	if (err != 0) {
		printk("Discovery failed: %d\n", err);
		return;
	}

	printk("CCP: Discovered GTBS\n");

	/* Read Bearer URI Schemes Supported List Characteristic */
	err = bt_tbs_client_read_uri_list(conn, BT_TBS_GTBS_INDEX);
	if (err != 0) {
		printk("Failed to initialize read URI list: %d\n", err);
	}
}

static void originate_call_cb(struct bt_conn *conn, int err, uint8_t inst_index, uint8_t call_index)
{
	ARG_UNUSED(conn);

	if (inst_index != BT_TBS_GTBS_INDEX) {
		printk("Unexpected %s for instance %u\n", __func__, inst_index);
		return;
	}

	if (err != 0) {
		printk("Originate call failed: %d\n", err);
		return;
	}

	printk("CCP: Call originate successful\n");
	new_call_index = call_index;
}

static void terminate_call_cb(struct bt_conn *conn, int err,
			      uint8_t inst_index, uint8_t call_index)
{
	ARG_UNUSED(conn);

	if (inst_index != BT_TBS_GTBS_INDEX) {
		printk("Unexpected %s for instance %u\n", __func__, inst_index);
		return;
	}

	if (err != 0) {
		printk("Terminate call failed: %d\n", err);
		return;
	}

	printk("CCP: Call with id %d terminated\n", call_index);
}

static void read_uri_schemes_string_cb(struct bt_conn *conn, int err,
				       uint8_t inst_index, const char *value)
{
	const char *uri_scheme = value;
	const char *uri_schemes_end = value + strlen(value);
	const size_t suffix_len = strlen(URI_SEPARATOR) + strlen(CALLER_ID);
	size_t uri_scheme_len;

	ARG_UNUSED(conn);

	if (inst_index != BT_TBS_GTBS_INDEX) {
		printk("Unexpected %s for instance %u\n", __func__, inst_index);
		return;
	}

	if (err != 0) {
		printk("Read URI schemes failed: %d\n", err);
		return;
	}

	/* Save first remote URI
	 *
	 * First search for a URI scheme that can fit with the separator and caller ID.
	 * Then compose the URI for later use.
	 */
	while (uri_scheme < uri_schemes_end) {
		uri_scheme_len = 0U;
		while (uri_scheme[uri_scheme_len] != '\0' &&
		       uri_scheme[uri_scheme_len] != ',') {
			uri_scheme_len++;
		}

		if (uri_scheme_len == 0U) {
			uri_scheme++;
			continue;
		}

		if (uri_scheme_len + suffix_len <= CONFIG_BT_TBS_MAX_URI_LENGTH) {
			break;
		}

		uri_scheme += uri_scheme_len + 1U;
	}

	if (uri_scheme >= uri_schemes_end) {
		printk("No URI scheme fits originate URI: %s\n", value);
		return;
	}

	(void)memcpy(remote_uri, uri_scheme, uri_scheme_len);
	remote_uri[uri_scheme_len] = '\0';
	(void)strcat(remote_uri, URI_SEPARATOR);
	(void)strcat(remote_uri, CALLER_ID);

	printk("CCP: Discovered remote URI: %s\n", remote_uri);
	k_sem_give(&sem_discovery_done);
}

struct bt_tbs_client_cb tbs_client_cb = {
	.discover = discover_cb,
	.uri_list = read_uri_schemes_string_cb,
	.originate_call = originate_call_cb,
	.terminate_call = terminate_call_cb,
};

int ccp_call_ctrl_init(struct bt_conn *conn)
{
	int err;

	default_conn = bt_conn_ref(conn);
	err = bt_tbs_client_register_cb(&tbs_client_cb);
	if (err != 0) {
		return err;
	}

	err = bt_tbs_client_discover(conn);
	if (err != 0) {
		return err;
	}
	err = k_sem_take(&sem_discovery_done, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);

	return err;
}

int ccp_originate_call(void)
{
	int err;

	err = bt_tbs_client_originate_call(default_conn, BT_TBS_GTBS_INDEX, remote_uri);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		printk("TBS originate call failed: %d\n", err);
	}

	return err;
}

int ccp_terminate_call(void)
{
	int err;

	err = bt_tbs_client_terminate_call(default_conn, BT_TBS_GTBS_INDEX, new_call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		printk("TBS terminate call failed: %d\n", err);
	}

	return err;
}
