/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "common/hci_common_internal.h"
#include "host/conn_internal.h"
#include "host/classic/sco_internal.h"

BUILD_ASSERT(BT_BUF_CMD_TX_COUNT == 1, "The test requires exactly one command buffer");

static const bt_addr_t peer = {{1, 2, 3, 4, 5, 6}};
static struct bt_conn acl;
static struct bt_conn sco;
static struct bt_sco_chan chan;
static uint16_t expected_opcode;
static uint8_t expected_params[sizeof(struct bt_hci_cp_accept_sync_conn_req)];
static size_t expected_len;
static unsigned int hook_count;
static unsigned int send_count;
static unsigned int cleanup_count;
static int send_result;

struct bt_conn *__wrap_bt_conn_add_sco(const bt_addr_t *addr, int link_type)
{
	zassert_mem_equal(addr, &peer, sizeof(peer));
	return &sco;
}

struct bt_conn *__wrap_bt_conn_lookup_addr_sco(const bt_addr_t *addr)
{
	zassert_mem_equal(addr, &peer, sizeof(peer));
	return NULL;
}

void __wrap_bt_conn_set_state(struct bt_conn *conn, bt_conn_state_t state)
{
	zassert_equal_ptr(conn, &sco);
	conn->state = state;
}

void __wrap_bt_conn_unref(struct bt_conn *conn)
{
	zassert_equal_ptr(conn, &sco);
}

void __wrap_bt_sco_cleanup(struct bt_conn *conn)
{
	zassert_equal_ptr(conn, &sco);
	cleanup_count++;
}

int __wrap_bt_hci_cmd_send_sync(uint16_t opcode, struct net_buf *buf, struct net_buf **rsp)
{
	zassert_equal(opcode, expected_opcode);
	zassert_equal(hook_count, 2);
	zassert_equal(buf->len, expected_len);
	zassert_mem_equal(buf->data, expected_params, expected_len);
	zassert_is_null(rsp);
	send_count++;
	net_buf_unref(buf);
	return send_result;
}

static void check_command_buffer_available(void)
{
	struct net_buf *buf;

	/* Fail immediately rather than blocking if the caller holds the only buffer. */
	buf = bt_hci_cmd_alloc(K_NO_WAIT);
	zassert_not_null(buf, "SCO hook must not run while its caller holds a command buffer");
	net_buf_unref(buf);
	hook_count++;
}

static void setup_hook(struct bt_conn *conn, struct bt_hci_cp_setup_sync_conn *cp)
{
	struct bt_hci_cp_setup_sync_conn expected = {
		.handle = sys_cpu_to_le16(acl.handle),
		.tx_bandwidth = sys_cpu_to_le32(0x00001f40),
		.rx_bandwidth = sys_cpu_to_le32(0x00001f40),
		.max_latency = sys_cpu_to_le16(BT_HCI_SCO_MAX_LATENCY_DEFAULT),
		.content_format = sys_cpu_to_le16(chan.voice_setting),
		.retrans_effort = BT_HCI_SCO_RETRANS_EFFORT_DEFAULT,
		.pkt_type = sys_cpu_to_le16(sco.sco.pkt_type),
	};

	zassert_equal_ptr(conn, &acl);
	zassert_equal(hook_count, 0);
	zassert_mem_equal(cp, &expected, sizeof(expected));
	check_command_buffer_available();
	cp->max_latency = sys_cpu_to_le16(0x1234);
}

static void accept_hook(struct bt_hci_cp_accept_sync_conn_req *cp)
{
	struct bt_hci_cp_accept_sync_conn_req expected = {
		.bdaddr = peer,
		.tx_bandwidth = sys_cpu_to_le32(0x00001f40),
		.rx_bandwidth = sys_cpu_to_le32(0x00001f40),
		.max_latency = sys_cpu_to_le16(BT_HCI_SCO_MAX_LATENCY_DEFAULT),
		.content_format = sys_cpu_to_le16(chan.voice_setting),
		.retrans_effort = BT_HCI_SCO_RETRANS_EFFORT_DEFAULT,
		.pkt_type = sys_cpu_to_le16(sco.sco.pkt_type),
	};

	zassert_equal(hook_count, 0);
	zassert_mem_equal(cp, &expected, sizeof(expected));
	check_command_buffer_available();
	cp->max_latency = sys_cpu_to_le16(0x1234);
}

static void static_setup_hook(struct bt_conn *conn, struct bt_hci_cp_setup_sync_conn *cp)
{
	zassert_equal_ptr(conn, &acl);
	zassert_equal(hook_count, 1);
	zassert_equal(sys_le16_to_cpu(cp->max_latency), 0x1234);
	check_command_buffer_available();
	cp->pkt_type = sys_cpu_to_le16(0x0020);
	expected_len = sizeof(*cp);
	memcpy(expected_params, cp, expected_len);
}

static void static_accept_hook(struct bt_hci_cp_accept_sync_conn_req *cp)
{
	zassert_equal(hook_count, 1);
	zassert_equal(sys_le16_to_cpu(cp->max_latency), 0x1234);
	check_command_buffer_available();
	cp->pkt_type = sys_cpu_to_le16(0x0020);
	expected_len = sizeof(*cp);
	memcpy(expected_params, cp, expected_len);
}

BT_SCO_HCI_CB_DEFINE(sco_hooks) = {
	.setup = static_setup_hook,
	.accept = static_accept_hook,
};

static struct bt_sco_hci_cb hooks = {
	.setup = setup_hook,
	.accept = accept_hook,
};

static int server_accept(const struct bt_sco_accept_info *info, struct bt_sco_chan **channel)
{
	zassert_equal_ptr(info->acl, &acl);
	*channel = &chan;
	return 0;
}

static struct bt_sco_server server = {
	.sec_level = BT_SECURITY_L1,
	.accept = server_accept,
};

static void before(void *fixture)
{
	static const struct bt_sco_chan_ops ops;

	memset(&acl, 0, sizeof(acl));
	memset(&sco, 0, sizeof(sco));
	memset(&chan, 0, sizeof(chan));
	acl.handle = 0x0123;
	acl.sec_level = BT_SECURITY_L1;
	sco.type = BT_CONN_TYPE_SCO;
	sco.sco.acl = &acl;
	sco.sco.pkt_type = 0x003f;
	chan.ops = &ops;
	chan.voice_setting = BT_HCI_VOICE_SETTINGS(
		BT_HCI_VOICE_SETTING_AIR_CODING_FMT_CVSD,
		BT_HCI_VOICE_SETTING_PCM_BIT_POS_DEFAULT,
		BT_HCI_VOICE_SETTING_SAMPLE_SIZE_16_BITS,
		BT_HCI_VOICE_SETTING_DATA_FMT_2_COMPLEMENT,
		BT_HCI_VOICE_SETTING_CODING_FMT_LINEAR);
	hook_count = 0;
	send_count = 0;
	cleanup_count = 0;
	send_result = 0;
	expected_len = 0;
	zassert_ok(bt_sco_hci_cb_register(&hooks));
	zassert_ok(bt_sco_server_register(&server));
}

static void after(void *fixture)
{
	zassert_ok(bt_sco_hci_cb_unregister(&hooks));
	zassert_ok(bt_sco_server_unregister(&server));
	zassert_equal(send_count, 1);
	zassert_equal(hook_count, 2);
	check_command_buffer_available();
}

static uint8_t accept_connection(void)
{
	struct bt_hci_evt_conn_request evt = {
		.bdaddr = peer,
		.link_type = BT_HCI_ESCO,
	};

	expected_opcode = BT_HCI_OP_ACCEPT_SYNC_CONN_REQ;
	return bt_esco_conn_req(&evt);
}

ZTEST(sco_hooks, test_setup)
{
	expected_opcode = BT_HCI_OP_SETUP_SYNC_CONN;
	zassert_equal_ptr(bt_conn_create_sco(&peer, &chan), &sco);
	zassert_equal(chan.state, BT_SCO_STATE_CONNECTING);
	zassert_equal(cleanup_count, 0);
}

ZTEST(sco_hooks, test_accept)
{
	zassert_equal(accept_connection(), BT_HCI_ERR_SUCCESS);
	zassert_equal(chan.state, BT_SCO_STATE_CONNECTING);
	zassert_equal(cleanup_count, 0);
}

ZTEST(sco_hooks, test_setup_send_error)
{
	expected_opcode = BT_HCI_OP_SETUP_SYNC_CONN;
	send_result = -EIO;
	zassert_is_null(bt_conn_create_sco(&peer, &chan));
	zassert_equal(chan.state, BT_SCO_STATE_DISCONNECTED);
	zassert_equal(cleanup_count, 1);
}

ZTEST(sco_hooks, test_accept_send_error)
{
	send_result = -EIO;
	zassert_equal(accept_connection(), BT_HCI_ERR_UNSPECIFIED);
	zassert_equal(chan.state, BT_SCO_STATE_DISCONNECTED);
	zassert_is_null(chan.sco);
	zassert_equal(cleanup_count, 1);
}

ZTEST_SUITE(sco_hooks, NULL, NULL, before, after, NULL);
