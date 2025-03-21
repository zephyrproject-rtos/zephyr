/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

#include "common/hci_common_internal.h"
#include "common/bt_str.h"

#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#include "common_defs.h"

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

DEFINE_FLAG_STATIC(is_connected);
DEFINE_FLAG_STATIC(flag_data_length_updated);
DEFINE_FLAG_STATIC(flag_handle);
DEFINE_FLAG_STATIC(flag_write_ack);
DEFINE_FLAG_STATIC(flag_indication_ack);

static uint16_t server_write_handle;

static K_FIFO_DEFINE(rx_queue);

#define CMD_BUF_SIZE MAX(BT_BUF_EVT_RX_SIZE, BT_BUF_CMD_TX_SIZE)
NET_BUF_POOL_FIXED_DEFINE(hci_cmd_pool, BT_BUF_CMD_TX_COUNT,
			  CMD_BUF_SIZE, 8, NULL);

static K_SEM_DEFINE(cmd_sem, 1, 1);
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

	TEST_ASSERT(status == 0x00, "cmd status: %x", status);

	TEST_ASSERT(active_opcode == opcode, "unexpected opcode %x != %x", active_opcode, opcode);

	if (active_opcode) {
		active_opcode = 0xFFFF;
		cmd_rsp = net_buf_ref(buf);
		net_buf_simple_restore(&buf->b, &state);
	}

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
	case BT_HCI_EVT_LE_DATA_LEN_CHANGE:
		SET_FLAG(flag_data_length_updated);
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

static void handle_att_notification(struct net_buf *buf)
{
	uint16_t handle = net_buf_pull_le16(buf);

	LOG_INF("Got notification for 0x%04x len %d", handle, buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "payload");

	server_write_handle = net_buf_pull_le16(buf);
	LOG_INF("Retrieved handle to write to: 0x%x", server_write_handle);
	SET_FLAG(flag_handle);
}

struct net_buf *alloc_l2cap_pdu(void);
static void send_l2cap_packet(struct net_buf *buf, uint16_t cid);

static void send_write_rsp(void)
{
	struct net_buf *buf = alloc_l2cap_pdu();

	net_buf_add_u8(buf, BT_ATT_OP_WRITE_RSP);
	send_l2cap_packet(buf, BT_L2CAP_CID_ATT);
}

static void handle_att_write(struct net_buf *buf)
{
	uint16_t handle = net_buf_pull_le16(buf);

	LOG_INF("Got write for 0x%04x len %d", handle, buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "payload");

	send_write_rsp();
}

static void handle_att(struct net_buf *buf)
{
	uint8_t op = net_buf_pull_u8(buf);

	switch (op) {
	case BT_ATT_OP_NOTIFY:
		handle_att_notification(buf);
		return;
	case BT_ATT_OP_WRITE_REQ:
		handle_att_write(buf);
		return;
	case BT_ATT_OP_WRITE_RSP:
		LOG_INF("got ATT write RSP");
		SET_FLAG(flag_write_ack);
		return;
	case BT_ATT_OP_CONFIRM:
		LOG_INF("got ATT indication confirm");
		SET_FLAG(flag_indication_ack);
		return;
	case BT_ATT_OP_MTU_RSP:
		LOG_INF("got ATT MTU RSP");
		return;
	default:
		LOG_HEXDUMP_ERR(buf->data, buf->len, "payload");
		TEST_FAIL("unhandled opcode %x", op);
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
	TEST_ASSERT_NO_MSG(active_opcode == 0xFFFF);

	active_opcode = opcode;

	LOG_HEXDUMP_DBG(cmd->data, cmd->len, "HCI TX");
	bt_send(cmd);

	/* Wait until the command completes */
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

static void read_max_data_len(uint16_t *tx_octets, uint16_t *tx_time)
{
	struct bt_hci_rp_le_read_max_data_len *rp;
	struct net_buf *rsp;

	send_cmd(BT_HCI_OP_LE_READ_MAX_DATA_LEN, NULL, &rsp);

	rp = (void *)rsp->data;
	*tx_octets = sys_le16_to_cpu(rp->max_tx_octets);
	*tx_time = sys_le16_to_cpu(rp->max_tx_time);
	net_buf_unref(rsp);
}

static void write_default_data_len(uint16_t tx_octets, uint16_t tx_time)
{
	struct bt_hci_cp_le_write_default_data_len *cp;
	struct net_buf *buf = bt_hci_cmd_create(BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN, sizeof(*cp));

	TEST_ASSERT_NO_MSG(buf);

	cp = net_buf_add(buf, sizeof(*cp));
	cp->max_tx_octets = sys_cpu_to_le16(tx_octets);
	cp->max_tx_time = sys_cpu_to_le16(tx_time);

	send_cmd(BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN, buf, NULL);
}

static void set_data_len(void)
{
	uint16_t tx_octets, tx_time;

	read_max_data_len(&tx_octets, &tx_time);
	write_default_data_len(tx_octets, tx_time);
}

static void set_event_mask(uint16_t opcode)
{
	struct bt_hci_cp_set_event_mask *cp_mask;
	struct net_buf *buf;
	uint64_t mask = 0U;

	/* The two commands have the same length/params */
	buf = bt_hci_cmd_create(opcode, sizeof(*cp_mask));
	TEST_ASSERT_NO_MSG(buf);

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
	TEST_ASSERT_NO_MSG(buf);

	net_buf_add_mem(buf, &addr.a, sizeof(addr.a));
	send_cmd(BT_HCI_OP_LE_SET_RANDOM_ADDRESS, buf, NULL);
}

void start_adv(void)
{
	struct bt_hci_cp_le_set_adv_param set_param;
	struct net_buf *buf;
	uint16_t interval = 60; /* Interval doesn't matter */

	(void)memset(&set_param, 0, sizeof(set_param));

	set_param.min_interval = sys_cpu_to_le16(interval);
	set_param.max_interval = sys_cpu_to_le16(interval);
	set_param.channel_map = 0x07;
	set_param.filter_policy = BT_LE_ADV_FP_NO_FILTER;
	set_param.type = BT_HCI_ADV_IND;
	set_param.own_addr_type = BT_HCI_OWN_ADDR_RANDOM;

	/* configure */
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_PARAM, sizeof(set_param));
	net_buf_add_mem(buf, &set_param, sizeof(set_param));
	send_cmd(BT_HCI_OP_LE_SET_ADV_PARAM, buf, NULL);

	/* start */
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
	net_buf_add_u8(buf, BT_HCI_LE_ADV_ENABLE);
	send_cmd(BT_HCI_OP_LE_SET_ADV_ENABLE, buf, NULL);
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

static int send_acl(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	uint8_t flags = BT_ACL_START_NO_FLUSH;

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_acl_handle_pack(conn_handle, flags));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));

	bt_buf_set_type(buf, BT_BUF_ACL_OUT);

	k_sem_take(&acl_pkts, K_FOREVER);

	return bt_send(buf);
}

static void send_l2cap_packet(struct net_buf *buf, uint16_t cid)
{
	struct bt_l2cap_hdr *hdr;

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));
	hdr->cid = sys_cpu_to_le16(cid);

	/* Always entire packets, no HCI fragmentation */
	TEST_ASSERT(buf->len <= CONFIG_BT_BUF_ACL_TX_SIZE,
	       "Fragmentation not supported");

	send_acl(buf);
}

static void gatt_write(uint16_t op)
{
	static uint8_t data[] = "write";
	uint16_t handle = server_write_handle;
	struct net_buf *buf = alloc_l2cap_pdu();

	net_buf_add_u8(buf, op);
	net_buf_add_le16(buf, handle);
	net_buf_add_mem(buf, data, sizeof(data));

	LOG_INF("send ATT write %s",
		op == BT_ATT_OP_WRITE_REQ ? "REQ" : "CMD");

	send_l2cap_packet(buf, BT_L2CAP_CID_ATT);
}

static void gatt_notify(void)
{
	static uint8_t data[] = NOTIFICATION_PAYLOAD;
	uint16_t handle = HVX_HANDLE;
	struct net_buf *buf = alloc_l2cap_pdu();

	net_buf_add_u8(buf, BT_ATT_OP_NOTIFY);
	net_buf_add_le16(buf, handle);
	net_buf_add_mem(buf, data, sizeof(data));

	LOG_INF("send ATT notification");
	send_l2cap_packet(buf, BT_L2CAP_CID_ATT);
}

static void gatt_indicate(void)
{
	static uint8_t data[] = INDICATION_PAYLOAD;
	uint16_t handle = HVX_HANDLE;
	struct net_buf *buf = alloc_l2cap_pdu();

	net_buf_add_u8(buf, BT_ATT_OP_INDICATE);
	net_buf_add_le16(buf, handle);
	net_buf_add_mem(buf, data, sizeof(data));

	LOG_INF("send ATT indication");
	send_l2cap_packet(buf, BT_L2CAP_CID_ATT);
}

static void prepare_controller(void)
{
	/* Initialize controller */
	struct net_buf *rsp;

	send_cmd(BT_HCI_OP_RESET, NULL, NULL);
	send_cmd(BT_HCI_OP_LE_READ_BUFFER_SIZE, NULL, &rsp);
	le_read_buffer_size_complete(rsp);

	set_data_len();
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

void test_procedure_0(void)
{
	init_tinyhost();

	/* Start advertising & wait for a connection */
	start_adv();
	WAIT_FOR_FLAG(is_connected);
	LOG_INF("connected");

	/* We need this to be able to send whole L2CAP PDUs on-air. */
	WAIT_FOR_FLAG(flag_data_length_updated);

	/* Get handle we will write to */
	WAIT_FOR_FLAG(flag_handle);

	LOG_INF("##################### START TEST #####################");

	gatt_write(BT_ATT_OP_WRITE_REQ);	/* will prompt a response PDU */
	gatt_indicate();			/* will prompt a confirmation PDU */

	gatt_notify();
	gatt_write(BT_ATT_OP_WRITE_CMD);

	gatt_notify();
	gatt_write(BT_ATT_OP_WRITE_CMD);

	WAIT_FOR_FLAG(flag_write_ack);
	WAIT_FOR_FLAG(flag_indication_ack);

	TEST_PASS("Tester done");
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "tester",
		.test_main_f = test_procedure_0,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};


int main(void)
{
	bst_main();

	return 0;
}
