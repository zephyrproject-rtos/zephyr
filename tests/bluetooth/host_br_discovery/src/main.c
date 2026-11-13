/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#include "host/hci_core.h"

NET_BUF_POOL_FIXED_DEFINE(command_pool, 1, 16, 0, NULL);
NET_BUF_POOL_FIXED_DEFINE(event_pool, 1, sizeof(struct bt_hci_evt_remote_name_req_complete), 0,
			  NULL);

static const bt_addr_t peers[] = {
	{.val = {1, 2, 3, 4, 5, 6}},
	{.val = {6, 5, 4, 3, 2, 1}},
};
static const struct bt_br_discovery_param param = {
	.length = 1,
};
static struct bt_br_discovery_result results_a[2];
static struct bt_br_discovery_result results_b[2];
static size_t command_count;
static size_t inquiry_count;
static size_t name_request_count;
static bt_addr_t name_requests[ARRAY_SIZE(peers)];
static size_t recv_count;
static bool restart_in_timeout;

struct completion {
	const struct bt_br_discovery_result *results;
	size_t count;
};

static struct completion completions[2][2];
static size_t completion_count[2];

struct net_buf *__wrap_bt_hci_cmd_alloc(k_timeout_t timeout)
{
	struct net_buf *buf;

	ARG_UNUSED(timeout);

	buf = net_buf_alloc(&command_pool, K_NO_WAIT);
	zassert_not_null(buf);

	return buf;
}

int __wrap_bt_hci_cmd_send_sync(uint16_t opcode, struct net_buf *buf, struct net_buf **rsp)
{
	zassert_is_null(rsp);
	command_count++;

	switch (opcode) {
	case BT_HCI_OP_INQUIRY:
		zassert_not_null(buf);
		zassert_equal(buf->len, sizeof(struct bt_hci_op_inquiry));
		inquiry_count++;
		break;
	case BT_HCI_OP_REMOTE_NAME_REQUEST: {
		const struct bt_hci_cp_remote_name_request *cp;

		zassert_not_null(buf);
		cp = (const void *)buf->data;
		zassert_equal(buf->len, sizeof(*cp));
		zassert_true(name_request_count < ARRAY_SIZE(name_requests));
		bt_addr_copy(&name_requests[name_request_count++], &cp->bdaddr);
		break;
	}
	case BT_HCI_OP_INQUIRY_CANCEL:
		zassert_is_null(buf);
		break;
	case BT_HCI_OP_REMOTE_NAME_CANCEL:
		zassert_not_null(buf);
		zassert_equal(buf->len, sizeof(struct bt_hci_cp_remote_name_cancel));
		break;
	default:
		zassert_unreachable("Unexpected HCI command 0x%04x", opcode);
		break;
	}

	if (buf != NULL) {
		net_buf_unref(buf);
	}
	return 0;
}

static void discovery_recv(const struct bt_br_discovery_result *result)
{
	ARG_UNUSED(result);

	recv_count++;
}

static void record_completion(size_t listener, const struct bt_br_discovery_result *results,
			      size_t count)
{
	size_t index = completion_count[listener]++;

	zassert_true(index < ARRAY_SIZE(completions[listener]));
	completions[listener][index].results = results;
	completions[listener][index].count = count;
}

static void discovery_timeout(const struct bt_br_discovery_result *results, size_t count)
{
	zassert_false(atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY));
	record_completion(0, results, count);

	if (restart_in_timeout && completion_count[0] == 1) {
		zassert_ok(bt_br_discovery_start(&param, results_b, ARRAY_SIZE(results_b)));
	}
}

static void second_timeout(const struct bt_br_discovery_result *results, size_t count)
{
	record_completion(1, results, count);
}

static struct bt_br_discovery_cb listeners[] = {
	{
		.recv = discovery_recv,
		.timeout = discovery_timeout,
	},
	{
		.timeout = second_timeout,
	},
};

static struct net_buf *event_buf(const void *event, size_t size)
{
	struct net_buf *buf = net_buf_alloc(&event_pool, K_NO_WAIT);

	zassert_not_null(buf);
	net_buf_add_mem(buf, event, size);
	return buf;
}

static void inquiry_result(const bt_addr_t *addr, bool with_name)
{
	struct bt_hci_evt_extended_inquiry_result event = {
		.num_reports = 1,
		.pscan_rep_mode = 1,
		.rssi = -30,
	};
	struct net_buf *buf;

	bt_addr_copy(&event.addr, addr);
	if (with_name) {
		static const uint8_t name[] = {4, BT_DATA_NAME_COMPLETE, 'o', 'l', 'd'};

		memcpy(event.eir, name, sizeof(name));
	}

	buf = event_buf(&event, sizeof(event));
	bt_hci_extended_inquiry_result(buf);
	net_buf_unref(buf);
}

static void inquiry_complete(void)
{
	struct bt_hci_evt_inquiry_complete event = {0};
	struct net_buf *buf = event_buf(&event, sizeof(event));

	bt_hci_inquiry_complete(buf);
	net_buf_unref(buf);
}

static void name_complete(const bt_addr_t *addr, uint8_t status)
{
	struct bt_hci_evt_remote_name_req_complete event = {
		.status = status,
		.name = "remote",
	};
	struct net_buf *buf;

	bt_addr_copy(&event.bdaddr, addr);
	buf = event_buf(&event, sizeof(event));
	bt_hci_remote_name_request_complete(buf);
	net_buf_unref(buf);
}

static void check_completion(size_t index, const struct bt_br_discovery_result *results,
			     size_t count)
{
	for (size_t i = 0; i < ARRAY_SIZE(listeners); i++) {
		zassert_equal(completion_count[i], index + 1);
		zassert_equal_ptr(completions[i][index].results, results);
		zassert_equal(completions[i][index].count, count);
	}
}

static void check_late_names_ignored(void)
{
	struct bt_br_discovery_result snapshot[ARRAY_SIZE(results_a)];
	size_t saved_recv_count = recv_count;
	size_t saved_command_count = command_count;
	static const uint8_t statuses[] = {BT_HCI_ERR_SUCCESS, BT_HCI_ERR_PAGE_TIMEOUT};

	memcpy(snapshot, results_a, sizeof(snapshot));
	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(statuses); j++) {
			name_complete(&peers[i], statuses[j]);
			zassert_mem_equal(results_a, snapshot, sizeof(snapshot));
			zassert_equal(recv_count, saved_recv_count);
			zassert_equal(command_count, saved_command_count);
			for (size_t k = 0; k < ARRAY_SIZE(listeners); k++) {
				zassert_equal(completion_count[k], 1);
			}
		}
	}
	zassert_false(atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY));
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	command_count = 0;
	inquiry_count = 0;
	name_request_count = 0;
	recv_count = 0;
	restart_in_timeout = false;
	memset(completions, 0, sizeof(completions));
	memset(completion_count, 0, sizeof(completion_count));

	for (size_t i = 0; i < ARRAY_SIZE(listeners); i++) {
		bt_br_discovery_cb_register(&listeners[i]);
	}
	zassert_ok(bt_br_discovery_start(&param, results_a, ARRAY_SIZE(results_a)));
}

static void after(void *fixture)
{
	int err;

	ARG_UNUSED(fixture);

	err = bt_br_discovery_stop();
	for (size_t i = 0; i < ARRAY_SIZE(listeners); i++) {
		bt_br_discovery_cb_unregister(&listeners[i]);
	}
	atomic_clear_bit(bt_dev.flags, BT_DEV_READY);
	zassert_true(err == 0 || err == -EALREADY);
}

ZTEST_SUITE(br_discovery, NULL, NULL, before, after, NULL);

static void complete_name_resolution(uint8_t status)
{
	inquiry_result(&peers[0], false);
	inquiry_complete();
	zassert_equal(name_request_count, 1);
	zassert_true(bt_addr_eq(&name_requests[0], &peers[0]));
	zassert_true(atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY));
	zassert_equal(completion_count[0], 0);
	zassert_equal(completion_count[1], 0);

	name_complete(&peers[0], status);
	check_completion(0, results_a, 1);
	zassert_equal(recv_count, status == BT_HCI_ERR_SUCCESS ? 2 : 1);
	if (status == BT_HCI_ERR_SUCCESS) {
		zassert_mem_equal(&results_a[0].eir[2], "remote", strlen("remote"));
	}
	check_late_names_ignored();
}

ZTEST(br_discovery, test_name_success_resets_discovery)
{
	complete_name_resolution(BT_HCI_ERR_SUCCESS);
}

ZTEST(br_discovery, test_name_failure_resets_discovery)
{
	complete_name_resolution(BT_HCI_ERR_PAGE_TIMEOUT);
}

ZTEST(br_discovery, test_empty_inquiry_completes)
{
	inquiry_complete();
	check_completion(0, results_a, 0);
	zassert_equal(recv_count, 0);
	zassert_equal(name_request_count, 0);
	check_late_names_ignored();
}

ZTEST(br_discovery, test_eir_name_skips_resolution)
{
	inquiry_result(&peers[0], true);
	inquiry_complete();
	check_completion(0, results_a, 1);
	zassert_equal(recv_count, 1);
	zassert_equal(name_request_count, 0);
	check_late_names_ignored();
}

ZTEST(br_discovery, test_all_names_resolved_before_completion)
{
	inquiry_result(&peers[0], false);
	inquiry_result(&peers[1], false);
	inquiry_complete();
	zassert_equal(name_request_count, 1);
	zassert_true(bt_addr_eq(&name_requests[0], &peers[0]));

	name_complete(&peers[0], BT_HCI_ERR_SUCCESS);
	zassert_true(atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY));
	zassert_equal(completion_count[0], 0);
	zassert_equal(completion_count[1], 0);
	zassert_equal(name_request_count, 2);
	zassert_true(bt_addr_eq(&name_requests[1], &peers[1]));

	name_complete(&peers[1], BT_HCI_ERR_PAGE_TIMEOUT);
	check_completion(0, results_a, ARRAY_SIZE(peers));
	zassert_equal(recv_count, 3);
	check_late_names_ignored();
}

static void restart_from_timeout(bool resolve_name)
{
	restart_in_timeout = true;
	inquiry_result(&peers[0], !resolve_name);
	inquiry_complete();
	if (resolve_name) {
		name_complete(&peers[0], BT_HCI_ERR_SUCCESS);
	}

	/* Both listeners must see A even though the first has already started B. */
	check_completion(0, results_a, 1);
	zassert_equal(inquiry_count, 2);
	zassert_equal(name_request_count, resolve_name ? 1 : 0);
	zassert_true(atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY));

	inquiry_result(&peers[1], true);
	inquiry_complete();
	check_completion(1, results_b, 1);
	zassert_true(bt_addr_eq(&results_b[0].addr, &peers[1]));
	zassert_false(atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY));
}

ZTEST(br_discovery, test_restart_from_inquiry_timeout)
{
	restart_from_timeout(false);
}

ZTEST(br_discovery, test_restart_from_name_timeout)
{
	restart_from_timeout(true);
}
