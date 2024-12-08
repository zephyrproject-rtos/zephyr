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

#include "common/bt_str.h"

#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#include "utils.h"
#include "common.h"
#include "bstests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_tinyhost, 3);

DEFINE_FLAG(is_connected);
DEFINE_FLAG(flag_l2cap_connected);
DEFINE_FLAG(flag_data_length_updated);

static K_FIFO_DEFINE(rx_queue);

#define CMD_BUF_SIZE MAX(BT_BUF_EVT_RX_SIZE, BT_BUF_CMD_TX_SIZE)
NET_BUF_POOL_FIXED_DEFINE(hci_cmd_pool, CONFIG_BT_BUF_CMD_TX_COUNT,
			  CMD_BUF_SIZE, 8, NULL);

static K_SEM_DEFINE(cmd_sem, 1, 1);
static struct k_sem acl_pkts;
static struct k_sem tx_credits;
static uint16_t peer_mps;
static uint16_t conn_handle;

static uint16_t active_opcode = 0xFFFF;
static struct net_buf *cmd_rsp;

struct net_buf *bt_hci_cmd_create(uint16_t opcode, uint8_t param_len)
{
	struct bt_hci_cmd_hdr *hdr;
	struct net_buf *buf;

	LOG_DBG("opcode 0x%04x param_len %u", opcode, param_len);

	buf = net_buf_alloc(&hci_cmd_pool, K_FOREVER);
	__ASSERT_NO_MSG(buf);

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
		__ASSERT_NO_MSG(0);
	}

	LOG_DBG("opcode 0x%04x status %x", opcode, status);

	__ASSERT(status == 0x00, "cmd status: %x", status);

	__ASSERT(active_opcode == opcode, "unexpected opcode %x != %x", active_opcode, opcode);

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
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));

	struct bt_hci_evt_num_completed_packets *evt = (void *)buf->data;

	uint16_t handle, count;

	handle = sys_le16_to_cpu(evt->h[0].handle);
	count = sys_le16_to_cpu(evt->h[0].count);

	LOG_DBG("sent %d packets", count);

	while (count--) {
		k_sem_give(&acl_pkts);
	}
}

static void handle_l2cap_credits(struct net_buf *buf)
{
	struct bt_l2cap_le_credits *ev = (void *)buf->data;
	uint16_t credits = sys_le16_to_cpu(ev->credits);

	LOG_DBG("got credits: %d", credits);
	while (credits--) {
		k_sem_give(&tx_credits);
	}
}

static void handle_l2cap_connected(struct net_buf *buf)
{
	struct bt_l2cap_le_conn_rsp *rsp = (void *)buf->data;

	uint16_t credits = sys_le16_to_cpu(rsp->credits);
	uint16_t mtu = sys_le16_to_cpu(rsp->mtu);
	uint16_t mps = sys_le16_to_cpu(rsp->mps);

	peer_mps = mps;

	LOG_DBG("l2cap connected: mtu %d mps %d credits: %d", mtu, mps, credits);

	k_sem_init(&tx_credits, credits, credits);
	SET_FLAG(flag_l2cap_connected);
}

static void handle_sig(struct net_buf *buf)
{
	struct bt_l2cap_sig_hdr *hdr;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));

	switch (hdr->code) {
	case BT_L2CAP_LE_CONN_RSP:
		handle_l2cap_connected(buf);
		return;
	case BT_L2CAP_LE_CREDITS:
		handle_l2cap_credits(buf);
		return;
	case BT_L2CAP_DISCONN_REQ:
		FAIL("channel disconnected\n");
		return;
	default:
		FAIL("unhandled opcode %x\n", hdr->code);
		return;
	}
}

static void handle_l2cap(struct net_buf *buf)
{
	struct bt_l2cap_hdr *hdr;
	uint16_t cid;

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	cid = sys_le16_to_cpu(hdr->cid);

	__ASSERT_NO_MSG(buf->len == hdr->len);
	LOG_DBG("Packet for CID %u len %u", cid, buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "l2cap");

	/* signaling PDU */
	if (cid == 0x0005) {
		handle_sig(buf);
		return;
	}

	/* CoC PDU */
	if (cid == 0x0040) {
		FAIL("unexpected data rx");
	}
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

	/* fragmentation not supported */
	__ASSERT_NO_MSG(flags == BT_ACL_START);

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
	__ASSERT_NO_MSG(active_opcode == 0xFFFF);

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

	while (1) {
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

	__ASSERT_NO_MSG(buf);

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
	__ASSERT_NO_MSG(buf);

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
	__ASSERT_NO_MSG(buf);

	net_buf_add_mem(buf, &addr.a, sizeof(addr.a));
	send_cmd(BT_HCI_OP_LE_SET_RANDOM_ADDRESS, buf, NULL);
}

void start_adv(uint16_t interval)
{
	struct bt_hci_cp_le_set_adv_param set_param;
	struct net_buf *buf;

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

NET_BUF_POOL_DEFINE(acl_tx_pool, 5, BT_L2CAP_BUF_SIZE(200), 8, NULL);

struct net_buf *alloc_l2cap_pdu(void)
{
	struct net_buf *buf;
	uint16_t reserve;

	buf = net_buf_alloc(&acl_tx_pool, K_FOREVER);
	__ASSERT_NO_MSG(buf);

	reserve = sizeof(struct bt_l2cap_hdr);
	reserve += sizeof(struct bt_hci_acl_hdr) + BT_BUF_RESERVE;

	net_buf_reserve(buf, reserve);

	return buf;
}

static struct net_buf *l2cap_create_le_sig_pdu(uint8_t code, uint8_t ident, uint16_t len)
{
	struct bt_l2cap_sig_hdr *hdr;
	struct net_buf *buf;

	buf = alloc_l2cap_pdu();

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = code;
	hdr->ident = ident;
	hdr->len = sys_cpu_to_le16(len);

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
	__ASSERT_NO_MSG(buf->len <= CONFIG_BT_BUF_ACL_TX_SIZE);

	send_acl(buf);
}

static void open_l2cap(void)
{
	struct net_buf *buf;
	struct bt_l2cap_le_conn_req *req;

	buf = l2cap_create_le_sig_pdu(BT_L2CAP_LE_CONN_REQ, 1, sizeof(*req));

	req = net_buf_add(buf, sizeof(*req));
	req->psm = sys_cpu_to_le16(L2CAP_PSM);
	req->scid = sys_cpu_to_le16(L2CAP_CID);

	/* we don't intend on receiving anything. use the smallest allowed
	 * values and no initial credits.
	 */
	req->mtu = sys_cpu_to_le16(23);
	req->mps = sys_cpu_to_le16(23);
	req->credits = sys_cpu_to_le16(0);

	send_l2cap_packet(buf, BT_L2CAP_CID_LE_SIG);

	WAIT_FOR_FLAG(flag_l2cap_connected);
}

static void send_l2cap_sdu(uint8_t *data, uint16_t sdu_len, uint16_t mps)
{
	uint16_t pdu_len;

	/* have some fun with the packet size if not specified */
	bool shenanigans = !mps;
	int increment = -1;

	if (!mps) {
		/* need at least two bytes to fit the SDU length */
		mps = 2;
	}

	for (int i = 0; sdu_len; i++) {
		struct net_buf *buf = net_buf_alloc(&acl_tx_pool, K_FOREVER);

		__ASSERT_NO_MSG(buf);
		net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);

		pdu_len = MIN(sdu_len, mps);

		if (i == 0) {
			/* add SDU len to first packet */
			net_buf_push_le16(buf, sdu_len);
			pdu_len -= BT_L2CAP_SDU_HDR_SIZE;
		}

		net_buf_add_mem(buf, data, pdu_len); /* add data */

		data = &data[pdu_len];
		sdu_len -= pdu_len;

		if (shenanigans) {
			if (mps == 1) {
				increment = 1;
			} else if (mps == 10) {
				increment = -1;
			}
			mps += increment;
		}

		LOG_INF("send PDU %d (%d bytes, remaining %d)", i, buf->len, sdu_len);
		LOG_HEXDUMP_DBG(buf->data, buf->len, "PDU");

		k_sem_take(&tx_credits, K_FOREVER);
		send_l2cap_packet(buf, 0x0040);
	}

	LOG_INF("SDU sent ok");
}

void test_procedure_0(void)
{
	bt_enable_raw(&rx_queue);

	/* Start the RX thread */
	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack), rx_thread,
			NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_thread_name_set(&rx_thread_data, "HCI RX");

	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(0));

	/* Initialize controller */
	struct net_buf *rsp;

	send_cmd(BT_HCI_OP_RESET, NULL, NULL);
	send_cmd(BT_HCI_OP_LE_READ_BUFFER_SIZE, NULL, &rsp);
	le_read_buffer_size_complete(rsp);

	set_data_len();
	set_event_mask(BT_HCI_OP_SET_EVENT_MASK);
	set_event_mask(BT_HCI_OP_LE_SET_EVENT_MASK);
	set_random_address();

	/* Start advertising & wait for a connection */
	start_adv(60); /* Interval doesn't matter */
	WAIT_FOR_FLAG(is_connected);
	LOG_DBG("connected");

	/* We need this to be able to send whole L2CAP PDUs on-air. */
	WAIT_FOR_FLAG(flag_data_length_updated);

	/* Connect to the central's dynamic L2CAP server */
	open_l2cap();

	/* Prepare the data for sending */
	uint8_t data[L2CAP_SDU_LEN];

	for (int i = 0; i < ARRAY_SIZE(data); i++) {
		data[i] = (uint8_t)i;
	}

	/* Send the first SDU, lowering the PDU size for each subsequent PDU */
	send_l2cap_sdu(data, sizeof(data), 0);

	/* Send the second SDU respecting peer's MPS */
	send_l2cap_sdu(data, sizeof(data), peer_mps);

	WAIT_FOR_FLAG_UNSET(is_connected);
	LOG_DBG("disconnected");

	PASS("Tester done\n");
}

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "test_0",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
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
