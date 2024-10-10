/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/buf.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <zephyr/bluetooth/hci_types.h>

#include "common/bt_str.h"

#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

#include "data.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_tinyhost, LOG_LEVEL_INF);

#define BT_ATT_OP_MTU_REQ   0x02
#define BT_ATT_OP_MTU_RSP   0x03
#define BT_ATT_OP_WRITE_REQ 0x12
#define BT_ATT_OP_WRITE_RSP 0x13
#define BT_ATT_OP_NOTIFY    0x1b
#define BT_ATT_OP_INDICATE  0x1d
#define BT_ATT_OP_CONFIRM   0x1e
#define BT_ATT_OP_WRITE_CMD 0x52
#define BT_L2CAP_CID_ATT    0x0004
#define LAST_SUPPORTED_ATT_OPCODE 0x20

DEFINE_FLAG(is_connected);

static K_FIFO_DEFINE(rx_queue);

#define CMD_BUF_SIZE MAX(BT_BUF_EVT_RX_SIZE, BT_BUF_CMD_TX_SIZE)
NET_BUF_POOL_FIXED_DEFINE(hci_cmd_pool, CONFIG_BT_BUF_CMD_TX_COUNT,
			  CMD_BUF_SIZE, 8, NULL);

#define MAX_CMD_COUNT 1
static K_SEM_DEFINE(cmd_sem, MAX_CMD_COUNT, MAX_CMD_COUNT);
static struct k_sem acl_pkts;
static uint16_t conn_handle;

static volatile uint16_t active_opcode = 0xFFFF;
static struct net_buf *cmd_rsp;

struct net_buf *bt_hci_cmd_create(uint16_t opcode, uint8_t param_len)
{
	struct bt_hci_cmd_hdr *hdr;
	struct net_buf *buf;

	LOG_DBG("opcode 0x%04x param_len %u", opcode, param_len);

	buf = net_buf_alloc(&hci_cmd_pool, K_FOREVER);
	TEST_ASSERT(buf, "failed allocation");

	LOG_DBG("buf %p", buf);

	net_buf_reserve(buf, BT_BUF_RESERVE);

	bt_buf_set_type(buf, BT_BUF_CMD);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->opcode = sys_cpu_to_le16(opcode);
	hdr->param_len = param_len;

	return buf;
}

static void handle_cmd_complete(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr;
	uint8_t status, ncmd;
	uint16_t opcode;
	struct net_buf_simple_state state;

	net_buf_simple_save(&buf->b, &state);

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));

	if (hdr->evt == BT_HCI_EVT_CMD_COMPLETE) {
		struct bt_hci_evt_cmd_complete *evt;

		evt = net_buf_pull_mem(buf, sizeof(*evt));
		status = 0;
		ncmd = evt->ncmd;
		opcode = sys_le16_to_cpu(evt->opcode);

	} else if (hdr->evt == BT_HCI_EVT_CMD_STATUS) {
		struct bt_hci_evt_cmd_status *evt;

		evt = net_buf_pull_mem(buf, sizeof(*evt));
		status = buf->data[0];
		ncmd = evt->ncmd;
		opcode = sys_le16_to_cpu(evt->opcode);

	} else {
		TEST_FAIL("unhandled event 0x%x", hdr->evt);
	}

	LOG_DBG("opcode 0x%04x status %x", opcode, status);

	TEST_ASSERT(status == 0x00, "cmd 0x%x status: 0x%x", opcode, status);

	TEST_ASSERT(active_opcode == opcode, "unexpected opcode %x != %x", active_opcode, opcode);

	active_opcode = 0xFFFF;
	cmd_rsp = net_buf_ref(buf);
	net_buf_simple_restore(&buf->b, &state);

	if (ncmd) {
		k_sem_give(&cmd_sem);
	}
}

static void handle_meta_event(struct net_buf *buf)
{
	uint8_t code = buf->data[2];

	switch (code) {
	case BT_HCI_EVT_LE_ENH_CONN_COMPLETE:
	case BT_HCI_EVT_LE_ENH_CONN_COMPLETE_V2:
		conn_handle = sys_get_le16(&buf->data[4]);
		LOG_DBG("connected: handle: %d", conn_handle);
		SET_FLAG(is_connected);
		break;
	case BT_HCI_EVT_LE_CHAN_SEL_ALGO:
		/* do nothing */
		break;
	default:
		LOG_ERR("unhandled meta event %x", code);
		LOG_HEXDUMP_ERR(buf->data, buf->len, "HCI META EVT");
	}
}

static void handle_ncp(struct net_buf *buf)
{
	struct bt_hci_evt_num_completed_packets *evt;
	struct bt_hci_evt_hdr *hdr;
	uint16_t handle, count;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));

	evt = (void *)buf->data;
	handle = sys_le16_to_cpu(evt->h[0].handle);
	count = sys_le16_to_cpu(evt->h[0].count);

	LOG_DBG("sent %d packets", count);

	while (count--) {
		k_sem_give(&acl_pkts);
	}
}

struct net_buf *alloc_l2cap_pdu(void);
static void send_l2cap_packet(struct net_buf *buf, uint16_t cid);

static void handle_att(struct net_buf *buf)
{
	uint8_t op = net_buf_pull_u8(buf);

	switch (op) {
	case BT_ATT_OP_NOTIFY:
		LOG_INF("got ATT notification");
		return;
	case BT_ATT_OP_WRITE_RSP:
		LOG_INF("got ATT write RSP");
		return;
	case BT_ATT_OP_MTU_RSP:
		LOG_INF("got ATT MTU RSP");
		return;
	default:
		LOG_HEXDUMP_ERR(buf->data, buf->len, "payload");
		TEST_FAIL("unhandled opcode %x\n", op);
		return;
	}
}

static void handle_l2cap(struct net_buf *buf)
{
	struct bt_l2cap_hdr *hdr;
	uint16_t cid;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	cid = sys_le16_to_cpu(hdr->cid);

	LOG_DBG("Packet for CID %u len %u", cid, buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "l2cap");

	/* Make sure we don't have to recombine packets */
	TEST_ASSERT(buf->len == hdr->len, "buflen = %d != hdrlen %d",
	       buf->len, hdr->len);

	TEST_ASSERT(cid == BT_L2CAP_CID_ATT, "We only support (U)ATT");

	/* (U)ATT PDU */
	handle_att(buf);
}

static void handle_acl(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	uint16_t len, handle;
	uint8_t flags;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = sys_le16_to_cpu(hdr->len);
	handle = sys_le16_to_cpu(hdr->handle);

	flags = bt_acl_flags(handle);
	handle = bt_acl_handle(handle);

	TEST_ASSERT(flags == BT_ACL_START,
	       "Fragmentation not supported");

	LOG_DBG("ACL: conn %d len %d flags %d", handle, len, flags);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "HCI ACL");

	handle_l2cap(buf);
}

static void recv(struct net_buf *buf)
{
	LOG_HEXDUMP_DBG(buf->data, buf->len, "HCI RX");

	uint8_t code = buf->data[0];

	if (bt_buf_get_type(buf) == BT_BUF_EVT) {
		switch (code) {
		case BT_HCI_EVT_CMD_COMPLETE:
		case BT_HCI_EVT_CMD_STATUS:
			handle_cmd_complete(buf);
			break;
		case BT_HCI_EVT_LE_META_EVENT:
			handle_meta_event(buf);
			break;
		case BT_HCI_EVT_DISCONN_COMPLETE:
			UNSET_FLAG(is_connected);
			break;
		case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
			handle_ncp(buf);
			break;
		default:
			LOG_ERR("unhandled msg %x", code);
			LOG_HEXDUMP_ERR(buf->data, buf->len, "HCI EVT");
		}

		/* handlers should take a ref if they want to access the buffer
		 * later
		 */
		net_buf_unref(buf);
		return;
	}

	if (bt_buf_get_type(buf) == BT_BUF_ACL_IN) {
		handle_acl(buf);
		net_buf_unref(buf);
		return;
	}

	LOG_ERR("HCI RX (not data or event)");
	net_buf_unref(buf);
}

static void send_cmd(uint16_t opcode, struct net_buf *cmd, struct net_buf **rsp)
{
	LOG_DBG("opcode %x", opcode);

	if (!cmd) {
		cmd = bt_hci_cmd_create(opcode, 0);
	}

	k_sem_take(&cmd_sem, K_FOREVER);
	TEST_ASSERT(active_opcode == 0xFFFF, "");

	__ASSERT_NO_MSG(opcode);
	active_opcode = opcode;

	LOG_HEXDUMP_DBG(cmd->data, cmd->len, "HCI TX");
	bt_send(cmd);

	/* Wait until the command completes:
	 *
	 * Use `cmd_sem` as a signal that we are able to send another command,
	 * which means that the current command (for which we took cmd_sem
	 * above) likely has gotten a response.
	 *
	 * We don't actually want to send anything more, so when we got that
	 * signal (ie the thread is un-suspended), then we release the sem
	 * immediately.
	 */
	BUILD_ASSERT(MAX_CMD_COUNT == 1, "Logic depends on only 1 cmd at a time");
	k_sem_take(&cmd_sem, K_FOREVER);
	k_sem_give(&cmd_sem);

	net_buf_unref(cmd);

	/* return response. it's okay if cmd_rsp gets overwritten, since the app
	 * gets the ref to the underlying buffer when this fn returns.
	 */
	if (rsp) {
		*rsp = cmd_rsp;
	} else {
		net_buf_unref(cmd_rsp);
		cmd_rsp = NULL;
	}
}

static K_THREAD_STACK_DEFINE(rx_thread_stack, 1024);
static struct k_thread rx_thread_data;

static void rx_thread(void *p1, void *p2, void *p3)
{
	LOG_DBG("start HCI rx");

	while (true) {
		struct net_buf *buf;

		/* Wait until a buffer is available */
		buf = k_fifo_get(&rx_queue, K_FOREVER);
		recv(buf);
	}
}

static void le_read_buffer_size_complete(struct net_buf *rsp)
{
	struct bt_hci_rp_le_read_buffer_size *rp = (void *)rsp->data;

	LOG_DBG("status 0x%02x", rp->status);
	LOG_DBG("max len %d max num %d", rp->le_max_len, rp->le_max_num);

	k_sem_init(&acl_pkts, rp->le_max_num, rp->le_max_num);
	net_buf_unref(rsp);
}

static void set_event_mask(uint16_t opcode)
{
	struct bt_hci_cp_set_event_mask *cp_mask;
	struct net_buf *buf;
	uint64_t mask = 0U;

	/* The two commands have the same length/params */
	buf = bt_hci_cmd_create(opcode, sizeof(*cp_mask));
	TEST_ASSERT(buf, "");

	/* Forward all events */
	cp_mask = net_buf_add(buf, sizeof(*cp_mask));
	mask = UINT64_MAX;
	sys_put_le64(mask, cp_mask->events);

	send_cmd(opcode, buf, NULL);
}

static void set_random_address(void)
{
	struct net_buf *buf;
	bt_addr_le_t addr = {BT_ADDR_LE_RANDOM, {{0x0A, 0x89, 0x67, 0x45, 0x23, 0xC1}}};

	LOG_DBG("%s", bt_addr_str(&addr.a));

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_RANDOM_ADDRESS, sizeof(addr.a));
	TEST_ASSERT(buf, "");

	net_buf_add_mem(buf, &addr.a, sizeof(addr.a));
	send_cmd(BT_HCI_OP_LE_SET_RANDOM_ADDRESS, buf, NULL);
}

static void start_adv(uint16_t interval, const char *name, size_t name_len)
{
	struct bt_hci_cp_le_set_adv_data data;
	struct bt_hci_cp_le_set_adv_param set_param;
	struct net_buf *buf;

	/* name_len should also not include the \0 */
	__ASSERT(name_len < (31 - 2), "name_len should be < 30");

	(void)memset(&data, 0, sizeof(data));
	data.len = name_len + 2;
	data.data[0] = name_len + 1;
	data.data[1] = BT_DATA_NAME_COMPLETE;
	memcpy(&data.data[2], name, name_len);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_DATA, sizeof(data));
	__ASSERT_NO_MSG(buf);
	net_buf_add_mem(buf, &data, sizeof(data));
	send_cmd(BT_HCI_OP_LE_SET_ADV_DATA, buf, NULL);

	(void)memset(&set_param, 0, sizeof(set_param));
	set_param.min_interval = sys_cpu_to_le16(interval);
	set_param.max_interval = sys_cpu_to_le16(interval);
	set_param.channel_map = 0x07;
	set_param.filter_policy = BT_LE_ADV_FP_NO_FILTER;
	set_param.type = BT_HCI_ADV_IND;
	set_param.own_addr_type = BT_HCI_OWN_ADDR_RANDOM;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_PARAM, sizeof(set_param));
	__ASSERT_NO_MSG(buf);
	net_buf_add_mem(buf, &set_param, sizeof(set_param));

	send_cmd(BT_HCI_OP_LE_SET_ADV_PARAM, buf, NULL);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
	__ASSERT_NO_MSG(buf);

	net_buf_add_u8(buf, BT_HCI_LE_ADV_ENABLE);
	send_cmd(BT_HCI_OP_LE_SET_ADV_ENABLE, buf, NULL);
}

static void disconnect(void)
{
	struct net_buf *buf;
	struct bt_hci_cp_disconnect *disconn;
	uint8_t reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	uint16_t handle = conn_handle;

	LOG_INF("Disconnecting");

	buf = bt_hci_cmd_create(BT_HCI_OP_DISCONNECT, sizeof(*disconn));
	TEST_ASSERT(buf);

	disconn = net_buf_add(buf, sizeof(*disconn));
	disconn->handle = sys_cpu_to_le16(handle);
	disconn->reason = reason;

	send_cmd(BT_HCI_OP_DISCONNECT, buf, NULL);

	WAIT_FOR_FLAG_UNSET(is_connected);
	LOG_INF("Disconnected");
}

NET_BUF_POOL_DEFINE(acl_tx_pool, 5, BT_L2CAP_BUF_SIZE(200), 8, NULL);

struct net_buf *alloc_l2cap_pdu(void)
{
	struct net_buf *buf;
	uint16_t reserve;

	buf = net_buf_alloc(&acl_tx_pool, K_FOREVER);
	TEST_ASSERT(buf, "failed ACL allocation");

	reserve = sizeof(struct bt_l2cap_hdr);
	reserve += sizeof(struct bt_hci_acl_hdr) + BT_BUF_RESERVE;

	net_buf_reserve(buf, reserve);

	return buf;
}

static int send_acl(struct net_buf *buf, uint8_t flags)
{
	struct bt_hci_acl_hdr *hdr;

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_acl_handle_pack(conn_handle, flags));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));

	bt_buf_set_type(buf, BT_BUF_ACL_OUT);

	k_sem_take(&acl_pkts, K_FOREVER);

	return bt_send(buf);
}

static void push_l2cap_pdu_header(struct net_buf *dst, uint16_t len, uint16_t cid)
{
	struct bt_l2cap_hdr *hdr;

	hdr = net_buf_push(dst, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(len);
	hdr->cid = sys_cpu_to_le16(cid);
}

static void send_l2cap_packet(struct net_buf *buf, uint16_t cid)
{
	push_l2cap_pdu_header(buf, buf->len, cid);
	send_acl(buf, BT_ACL_START_NO_FLUSH);
}

static void prepare_controller(void)
{
	/* Initialize controller */
	struct net_buf *rsp;

	send_cmd(BT_HCI_OP_RESET, NULL, NULL);
	send_cmd(BT_HCI_OP_LE_READ_BUFFER_SIZE, NULL, &rsp);
	le_read_buffer_size_complete(rsp);

	set_event_mask(BT_HCI_OP_SET_EVENT_MASK);
	set_event_mask(BT_HCI_OP_LE_SET_EVENT_MASK);
	set_random_address();
}

static void init_tinyhost(void)
{
	bt_enable_raw(&rx_queue);

	/* Start the RX thread */
	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack), rx_thread,
			NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_thread_name_set(&rx_thread_data, "HCI RX");

	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(0));

	prepare_controller();
}

static void gatt_notify(void)
{
	static uint8_t data[] = NOTIFICATION_PAYLOAD;
	uint16_t handle = GATT_HANDLE;
	struct net_buf *buf = alloc_l2cap_pdu();

	net_buf_add_u8(buf, BT_ATT_OP_NOTIFY);
	net_buf_add_le16(buf, handle);
	net_buf_add_mem(buf, data, sizeof(data));

	LOG_INF("Sending complete notification");
	send_l2cap_packet(buf, BT_L2CAP_CID_ATT);
}

/* Send all but the last fragment of a notification */
static void gatt_notify_without_last_fragment(void)
{
	static uint8_t data[] = NOTIFICATION_PAYLOAD;
	uint16_t handle = GATT_HANDLE;
	struct net_buf *att_packet = alloc_l2cap_pdu();

	/* Prepare (G)ATT notification packet */
	net_buf_add_u8(att_packet, BT_ATT_OP_NOTIFY);
	net_buf_add_le16(att_packet, handle);
	net_buf_add_mem(att_packet, data, sizeof(data));

	size_t on_air_size = 5;
	uint8_t flags = BT_ACL_START_NO_FLUSH;

	LOG_INF("Sending partial notification");

	for (size_t i = 0; att_packet->len > on_air_size; i++) {
		struct net_buf *buf = alloc_l2cap_pdu();

		__ASSERT_NO_MSG(buf);

		/* This is the size of the ACL payload. I.e. not including the HCI header. */
		size_t frag_len = MIN(att_packet->len, on_air_size);

		if (i == 0) {
			/* Only first fragment should have L2CAP PDU header */
			push_l2cap_pdu_header(buf, att_packet->len, BT_L2CAP_CID_ATT);
			frag_len -= BT_L2CAP_HDR_SIZE;
		}

		/* copy data into ACL frag */
		net_buf_add_mem(buf,
				net_buf_pull_mem(att_packet, frag_len),
				frag_len);

		LOG_DBG("send ACL frag %d (%d bytes, remaining %d)", i, buf->len, att_packet->len);
		LOG_HEXDUMP_DBG(buf->data, buf->len, "ACL Fragment");

		send_acl(buf, flags);
		flags = BT_ACL_CONT;
	}

	net_buf_unref(att_packet);

	/* Hey! You didn't send the last frag, no fair!
	 *   - The DUT (probably)
	 */
	LOG_INF("Partial notification sent");
}

static void run_test_iteration(void)
{
	LOG_INF("advertise");

	/* Start advertising & wait for a connection */
	start_adv(40, PEER_NAME, sizeof(PEER_NAME) - 1);
	WAIT_FOR_FLAG(is_connected);
	LOG_INF("connected");

	/* Generous time allotment for dut to fake-subscribe */
	k_sleep(K_MSEC(100));

	gatt_notify();
	gatt_notify_without_last_fragment();
	disconnect();
}

void entrypoint_peer(void)
{
	init_tinyhost();

	LOG_INF("##################### START TEST #####################");

	for (size_t i = 0; i < TEST_ITERATIONS; i++) {
		LOG_INF("## Iteration %d", i);
		run_test_iteration();
	}

	TEST_PASS_AND_EXIT("Peer (tester) done\n");
}
