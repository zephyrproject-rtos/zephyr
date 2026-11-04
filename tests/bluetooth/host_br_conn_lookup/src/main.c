/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* The application may keep a reference to the object of a BR/EDR or SCO connection after the
 * disconnection. That object is no longer the connection to the peer: the next connection with
 * the peer gets an object of its own, in either direction.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "common/hci_common_internal.h"
#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/classic/sco_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of the commands without a handler, which are zeroed. */
#define GENERIC_RP_SIZE 64U

#define ACL_HANDLE      0x0001U
#define NEXT_ACL_HANDLE 0x0002U
#define SCO_HANDLE      0x0003U
#define NEXT_SCO_HANDLE 0x0004U

#define ACL_MAX_LEN 255U
#define ACL_MAX_NUM 4U
#define LE_MAX_LEN  27U
#define LE_MAX_NUM  3U

#define REASON BT_HCI_ERR_REMOTE_USER_TERM_CONN

#define TIMEOUT K_SECONDS(1)

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	/* Sets evt to NULL when it sends its events itself */
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

static const bt_addr_t peer = {
	.val = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U},
};

static K_SEM_DEFINE(accept_sem, 0, 1);

/* The status of the next command with this opcode. */
static uint16_t refused_opcode;
static uint8_t refused_status;

/* The ACL connection to take down when the Host sets up an SCO connection on it, before the
 * command is answered.
 */
static bool acl_lost_on_sco_setup;

/* What a lookup finds while the Host accepts a connection */
static bool lookup_on_accept;
static struct bt_conn *conn_on_accept;
static enum bt_conn_state state_on_accept;

static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static void cmd_status(uint16_t opcode)
{
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = (opcode == refused_opcode) ? refused_status : BT_HCI_ERR_SUCCESS;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
	bt_hci_recv(DEVICE_DT_INST_GET(0), net_buf_take(&buf));
}

static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(buf);

	rp = cmd_complete(evt, len, opcode);
	(void)memset(rp, 0, len);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE and BR/EDR supported */
	rp->features[4] = BIT(6);
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
}

static void read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			     uint16_t opcode)
{
	struct bt_hci_rp_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->acl_max_len = sys_cpu_to_le16(ACL_MAX_LEN);
	rp->acl_max_num = sys_cpu_to_le16(ACL_MAX_NUM);
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(LE_MAX_LEN);
	rp->le_max_num = LE_MAX_NUM;
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

static void success_status(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(evt);

	cmd_status(opcode);
}

static void accept_req(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(evt);

	if (lookup_on_accept) {
		struct bt_conn_info info;

		lookup_on_accept = false;
		conn_on_accept = bt_conn_lookup_addr_br(&peer);
		if (conn_on_accept != NULL && bt_conn_get_info(conn_on_accept, &info) == 0) {
			state_on_accept = info.state;
		}
	}

	cmd_status(opcode);
	k_sem_give(&accept_sem);
}

static void disconn_complete(uint16_t handle)
{
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(handle);
	evt->reason = REASON;
	bt_hci_recv(DEVICE_DT_INST_GET(0), net_buf_take(&buf));
}

static void work_submit(struct k_work *work)
{
	int err = bt_work_submit(work);

	zassert_true(err >= 0, "Submitting work failed (err %d)", err);
}

static void sco_setup_status(struct k_work *work)
{
	ARG_UNUSED(work);

	cmd_status(BT_HCI_OP_SETUP_SYNC_CONN);
}

static K_WORK_DEFINE(sco_setup_status_work, sco_setup_status);

/* Handling the event queues work for the disconnection, behind the one that runs this */
static void sco_setup_status_queue(struct k_work *work)
{
	ARG_UNUSED(work);

	work_submit(&sco_setup_status_work);
}

static K_WORK_DEFINE(sco_setup_status_queue_work, sco_setup_status_queue);

static void setup_sync_conn(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	const struct bt_hci_cp_setup_sync_conn *cp = (const void *)buf->data;

	ARG_UNUSED(len);
	ARG_UNUSED(evt);

	if (acl_lost_on_sco_setup) {
		acl_lost_on_sco_setup = false;
		disconn_complete(sys_le16_to_cpu(cp->handle));
		/* Answer the command only after the Host has handled the event */
		work_submit(&sco_setup_status_queue_work);
		return;
	}

	cmd_status(opcode);
}

/* The controller takes the connection down at once */
static void disconnect(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	const struct bt_hci_cp_disconnect *cp = (const void *)buf->data;

	ARG_UNUSED(len);
	ARG_UNUSED(evt);

	cmd_status(opcode);
	disconn_complete(sys_le16_to_cpu(cp->handle));
}

static const struct cmd_handler cmds[] = {
	{
		BT_HCI_OP_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_read_local_features),
		read_local_features,
	},
	{
		BT_HCI_OP_READ_SUPPORTED_COMMANDS,
		sizeof(struct bt_hci_rp_read_supported_commands),
		read_supported_commands,
	},
	{
		BT_HCI_OP_READ_BD_ADDR,
		sizeof(struct bt_hci_rp_read_bd_addr),
		read_bd_addr,
	},
	{
		BT_HCI_OP_READ_BUFFER_SIZE,
		sizeof(struct bt_hci_rp_read_buffer_size),
		read_buffer_size,
	},
	{
		BT_HCI_OP_LE_READ_BUFFER_SIZE,
		sizeof(struct bt_hci_rp_le_read_buffer_size),
		le_read_buffer_size,
	},
	{
		BT_HCI_OP_LE_READ_SUPP_STATES,
		sizeof(struct bt_hci_rp_le_read_supp_states),
		le_read_supp_states,
	},
	{BT_HCI_OP_ACCEPT_CONN_REQ, 0U, accept_req},
	{BT_HCI_OP_ACCEPT_SYNC_CONN_REQ, 0U, accept_req},
	{BT_HCI_OP_CONNECT, 0U, success_status},
	{BT_HCI_OP_SETUP_SYNC_CONN, 0U, setup_sync_conn},
	{BT_HCI_OP_READ_REMOTE_FEATURES, 0U, success_status},
	{BT_HCI_OP_DISCONNECT, 0U, disconnect},
};

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct bt_hci_cmd_hdr *chdr;
	struct net_buf *evt = NULL;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	ARRAY_FOR_EACH_PTR(cmds, handler) {
		if (handler->opcode == opcode) {
			handler->handler(cmd, &evt, handler->len, opcode);
			if (evt != NULL) {
				bt_hci_recv(dev, net_buf_take(&evt));
			}

			return;
		}
	}

	generic_success(cmd, &evt, GENERIC_RP_SIZE, opcode);
	bt_hci_recv(dev, net_buf_take(&evt));
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		cmd_handle(dev, buf);
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void conn_request(uint8_t link_type)
{
	struct bt_hci_evt_conn_request *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CONN_REQUEST, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CONN_REQUEST, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = link_type;
	bt_hci_recv(DEVICE_DT_INST_GET(0), net_buf_take(&buf));
}

static void conn_complete(uint16_t handle, uint8_t status)
{
	struct bt_hci_evt_conn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = status;
	evt->handle = sys_cpu_to_le16(handle);
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = BT_HCI_ACL;
	bt_hci_recv(DEVICE_DT_INST_GET(0), net_buf_take(&buf));
}

static void sync_conn_complete(uint16_t handle, uint8_t link_type, uint8_t status)
{
	struct bt_hci_evt_sync_conn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_SYNC_CONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_SYNC_CONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = status;
	evt->handle = sys_cpu_to_le16(handle);
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = link_type;
	bt_hci_recv(DEVICE_DT_INST_GET(0), net_buf_take(&buf));
}

/* What the callback was last called with. It holds a reference. */
static struct bt_conn *connected_conn;
static uint8_t connected_err;

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);

/* The callbacks are also called for SCO objects */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (!bt_conn_is_br(conn)) {
		return;
	}

	bt_conn_drop(&connected_conn);
	connected_conn = bt_conn_ref(conn);
	connected_err = err;
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (!bt_conn_is_br(conn)) {
		return;
	}

	k_sem_give(&disconnected_sem);
}

static uint8_t sco_disconnected_reason;
static uint32_t sco_disconnected_count;

static K_SEM_DEFINE(sco_connected_sem, 0, 1);
static K_SEM_DEFINE(sco_disconnected_sem, 0, 1);

static void sco_connected(struct bt_sco_chan *chan)
{
	ARG_UNUSED(chan);

	k_sem_give(&sco_connected_sem);
}

static void sco_disconnected(struct bt_sco_chan *chan, uint8_t reason)
{
	ARG_UNUSED(chan);

	sco_disconnected_reason = reason;
	sco_disconnected_count++;
	k_sem_give(&sco_disconnected_sem);
}

static const struct bt_sco_chan_ops sco_ops = {
	.connected = sco_connected,
	.disconnected = sco_disconnected,
};

static struct bt_sco_chan sco_chan = {
	.ops = &sco_ops,
};

static int sco_accept(const struct bt_sco_accept_info *info, struct bt_sco_chan **chan)
{
	ARG_UNUSED(info);

	*chan = &sco_chan;

	return 0;
}

static void expect_state(struct bt_conn *conn, enum bt_conn_state state)
{
	struct bt_conn_info info;

	zassert_ok(bt_conn_get_info(conn, &info), "Getting the connection info failed");
	zassert_equal(info.state, state, "Connection in state %u", info.state);
}

/* The Host sends its reply to the request */
static void peer_requests(uint8_t link_type)
{
	conn_request(link_type);
	zassert_ok(k_sem_take(&accept_sem, TIMEOUT), "The connection was not accepted");
}

/* The connected callback came. Returns the reference to the object it came for. */
static struct bt_conn *wait_connected(void)
{
	zassert_ok(k_sem_take(&connected_sem, TIMEOUT), "No connected callback");

	return bt_conn_take(&connected_conn);
}

/* The connected callback came for this object, without an error */
static void expect_connected(const struct bt_conn *expected)
{
	struct bt_conn *conn = wait_connected();

	zassert_equal(conn, expected, "Connected callback for another object");
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	bt_conn_unref(conn);
}

/* The peer connects. Returns a reference to the object of the connection. */
static struct bt_conn *peer_connects(uint16_t handle)
{
	struct bt_conn *conn;

	peer_requests(BT_HCI_ACL);
	conn_complete(handle, BT_HCI_ERR_SUCCESS);
	conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);

	return conn;
}

static void peer_disconnects(uint16_t handle)
{
	disconn_complete(handle);
	zassert_ok(k_sem_take(&disconnected_sem, TIMEOUT), "No disconnected callback");
}

/* A connection came and went, and the application keeps the reference to its object */
static struct bt_conn *old_connection(void)
{
	struct bt_conn *conn;

	conn = peer_connects(ACL_HANDLE);
	peer_disconnects(ACL_HANDLE);
	expect_state(conn, BT_CONN_STATE_DISCONNECTED);

	return conn;
}

/* No object is left behind: all of them can be allocated */
static void expect_all_objects_free(void)
{
	struct bt_conn *conns[CONFIG_BT_MAX_CONN];

	ARRAY_FOR_EACH(conns, i) {
		conns[i] = bt_conn_add_br(&peer);
		zassert_not_null(conns[i], "Object %zu is not free", i);
	}

	ARRAY_FOR_EACH(conns, i) {
		bt_conn_drop(&conns[i]);
	}
}

/* No SCO object is left behind. Needs an ACL connection to the peer. */
static void expect_all_sco_objects_free(void)
{
	struct bt_conn *conns[CONFIG_BT_MAX_SCO_CONN];

	ARRAY_FOR_EACH(conns, i) {
		conns[i] = bt_conn_add_sco(&peer, BT_HCI_ESCO);
		zassert_not_null(conns[i], "SCO object %zu is not free", i);
	}

	ARRAY_FOR_EACH(conns, i) {
		bt_sco_cleanup(bt_conn_take(&conns[i]));
	}
}

/* Starts an SCO connection or a BR/EDR connection in a thread of its own, for what happens while
 * that thread is blocked.
 */
#define CREATOR_STACK_SIZE 1024
/* Above the cooperative test thread, so that it runs up to where it blocks when that yields */
#define CREATOR_PRIO       (CONFIG_ZTEST_THREAD_PRIORITY - 1)

static K_THREAD_STACK_DEFINE(creator_stack, CREATOR_STACK_SIZE);
static struct k_thread creator_thread;
static bool creator_started;
static struct bt_conn *created_conn;

static void create_sco(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	created_conn = bt_conn_create_sco(&peer, &sco_chan);
}

static void create_br(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	created_conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
}

/* Starts the thread and returns when it is blocked */
static void creator_start(k_thread_entry_t entry)
{
	creator_started = true;
	(void)k_thread_create(&creator_thread, creator_stack, K_THREAD_STACK_SIZEOF(creator_stack),
			      entry, NULL, NULL, NULL, CREATOR_PRIO, 0, K_NO_WAIT);
	k_yield();
	zassert_equal(k_thread_join(&creator_thread, K_NO_WAIT), -EBUSY,
		      "The creator did not block");
}

static void creator_join(void)
{
	zassert_ok(k_thread_join(&creator_thread, TIMEOUT), "The creator did not finish");
	creator_started = false;
}

static struct net_buf *cmd_bufs[BT_BUF_CMD_TX_COUNT];

/* The next command waits for a buffer until cmd_bufs_release() */
static void cmd_bufs_take(void)
{
	/* With a timeout: the buffer of the last command may still be on its way back */
	ARRAY_FOR_EACH(cmd_bufs, i) {
		cmd_bufs[i] = bt_hci_cmd_alloc(TIMEOUT);
		zassert_not_null(cmd_bufs[i], "No command buffer %zu", i);
	}
}

static void cmd_bufs_release(void)
{
	ARRAY_FOR_EACH(cmd_bufs, i) {
		net_buf_drop(&cmd_bufs[i]);
	}
}

static void work_nothing(struct k_work *work)
{
	ARG_UNUSED(work);
}

/* Handling an event can queue work behind what is already queued */
#define HOST_SETTLE_ROUNDS 2U

static K_SEM_DEFINE(work_q_held_sem, 0, 1);
static K_SEM_DEFINE(work_q_release_sem, 0, 1);

/* Holds the Bluetooth work queue, and the work queued behind this, until work_q_release_sem */
static void work_q_hold(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&work_q_held_sem);
	zassert_ok(k_sem_take(&work_q_release_sem, TIMEOUT), "The work queue was not released");
}

/* Returns when the Host has handled the last event and the work it started */
static void host_settle(void)
{
	for (uint8_t i = 0U; i < HOST_SETTLE_ROUNDS; i++) {
		struct k_work_sync sync;
		struct k_work work;

		k_work_init(&work, work_nothing);
		work_submit(&work);
		(void)k_work_flush(&work, &sync);
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Also what a failed test left */
	cmd_bufs_release();
	if (creator_started) {
		creator_join();
	}

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	bt_conn_drop(&created_conn);
	bt_conn_drop(&connected_conn);
	bt_conn_drop(&conn_on_accept);

	refused_opcode = BT_OP_NOP;
	acl_lost_on_sco_setup = false;
	lookup_on_accept = false;
	sco_chan.sco = NULL;
	sco_disconnected_count = 0U;

	k_sem_reset(&accept_sem);
	k_sem_reset(&connected_sem);
	k_sem_reset(&disconnected_sem);
	k_sem_reset(&sco_connected_sem);
	k_sem_reset(&sco_disconnected_sem);
	k_sem_reset(&work_q_held_sem);
	k_sem_reset(&work_q_release_sem);
}

static void *setup(void)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
		.disconnected = disconnected,
	};
	static struct bt_sco_server sco_server = {
		.sec_level = BT_SECURITY_L1,
		.accept = sco_accept,
	};

	zassert_ok(bt_conn_cb_register(&conn_cb), "Registering the callbacks failed");
	zassert_ok(bt_sco_server_register(&sco_server), "Registering the SCO server failed");

	return NULL;
}

ZTEST_SUITE(host_br_conn_lookup, NULL, setup, before, after, NULL);

static ZTEST(host_br_conn_lookup, test_peer_connects_again)
{
	struct bt_conn *old_conn;
	struct bt_conn *conn;

	old_conn = old_connection();

	conn = peer_connects(NEXT_ACL_HANDLE);
	zassert_not_equal(conn, old_conn, "The connection came up on the old object");
	expect_state(conn, BT_CONN_STATE_CONNECTED);
	expect_state(old_conn, BT_CONN_STATE_DISCONNECTED);
	zassert_equal(bt_conn_disconnect(old_conn, REASON), -ENOTCONN,
		      "The old object is connected");

	peer_disconnects(NEXT_ACL_HANDLE);
	bt_conn_drop(&conn);
	bt_conn_drop(&old_conn);
	expect_all_objects_free();
}

/* The failure goes to the object of the attempt */
static ZTEST(host_br_conn_lookup, test_peer_fails_to_connect_again)
{
	struct bt_conn *old_conn;
	struct bt_conn *conn;

	old_conn = old_connection();

	peer_requests(BT_HCI_ACL);
	conn_complete(NEXT_ACL_HANDLE, BT_HCI_ERR_CONN_TIMEOUT);
	conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_CONN_TIMEOUT, "Connected with 0x%02x",
		      connected_err);
	zassert_not_equal(conn, old_conn, "The failure went to the old object");
	bt_conn_drop(&conn);
	bt_conn_drop(&old_conn);
	expect_all_objects_free();
}

static ZTEST(host_br_conn_lookup, test_connect_to_peer_again)
{
	struct bt_conn *old_conn;
	struct bt_conn *conn;

	old_conn = old_connection();

	conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(conn, "Creating the connection failed");
	zassert_not_equal(conn, old_conn, "The old object was used");

	conn_complete(NEXT_ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	expect_state(conn, BT_CONN_STATE_CONNECTED);
	expect_state(old_conn, BT_CONN_STATE_DISCONNECTED);

	zassert_ok(bt_conn_disconnect(conn, REASON), "Disconnecting failed");
	zassert_ok(k_sem_take(&disconnected_sem, TIMEOUT), "No disconnected callback");
	bt_conn_drop(&conn);
	bt_conn_drop(&old_conn);
	expect_all_objects_free();
}

static ZTEST(host_br_conn_lookup, test_lookup)
{
	struct bt_conn *setup_conn;
	struct bt_conn *old_conn;
	struct bt_conn *conn;

	conn = peer_connects(ACL_HANDLE);
	old_conn = bt_conn_lookup_addr_br(&peer);
	zassert_equal(old_conn, conn, "The connection was not found");
	bt_conn_drop(&conn);

	peer_disconnects(ACL_HANDLE);
	zassert_is_null(bt_conn_lookup_addr_br(&peer), "A disconnected object was found");

	/* Link Key Request and others come before the connection is up */
	lookup_on_accept = true;
	peer_requests(BT_HCI_ACL);
	zassert_not_null(conn_on_accept, "The connection being accepted was not found");
	zassert_not_equal(conn_on_accept, old_conn, "A disconnected object was found");
	zassert_equal(state_on_accept, BT_CONN_STATE_CONNECTING, "Connection in state %u",
		      state_on_accept);
	bt_conn_drop(&conn_on_accept);
	setup_conn = bt_conn_lookup_addr_br(&peer);
	zassert_not_null(setup_conn, "The connection being set up was not found");
	zassert_not_equal(setup_conn, old_conn, "A disconnected object was found");
	expect_state(setup_conn, BT_CONN_STATE_CONNECTING);

	conn_complete(NEXT_ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(setup_conn);

	conn = bt_conn_lookup_addr_br(&peer);
	zassert_equal(conn, setup_conn, "The connection was not found");
	bt_conn_drop(&conn);

	peer_disconnects(NEXT_ACL_HANDLE);
	bt_conn_unref(setup_conn);
	bt_conn_unref(old_conn);
}

static ZTEST(host_br_conn_lookup, test_peer_connects_sco_again)
{
	struct bt_conn *old_sco;
	struct bt_conn *acl;

	acl = peer_connects(ACL_HANDLE);

	peer_requests(BT_HCI_ESCO);
	sync_conn_complete(SCO_HANDLE, BT_HCI_ESCO, BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&sco_connected_sem, TIMEOUT), "No SCO connected callback");
	old_sco = bt_conn_ref(sco_chan.sco);
	disconn_complete(SCO_HANDLE);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");

	peer_requests(BT_HCI_ESCO);
	sync_conn_complete(NEXT_SCO_HANDLE, BT_HCI_ESCO, BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&sco_connected_sem, TIMEOUT), "No SCO connected callback");
	zassert_not_equal(sco_chan.sco, old_sco, "The connection came up on the old object");
	expect_state(sco_chan.sco, BT_CONN_STATE_CONNECTED);

	disconn_complete(NEXT_SCO_HANDLE);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");
	bt_conn_drop(&old_sco);
	expect_all_sco_objects_free();

	peer_disconnects(ACL_HANDLE);
	bt_conn_unref(acl);
}

/* A kept SCO object reports the address of its peer, also with the ACL connection gone */
static ZTEST(host_br_conn_lookup, test_sco_address_after_disconnection)
{
	char expected[BT_ADDR_STR_LEN];
	struct bt_conn_tmp_str str;
	struct bt_conn *acl;
	struct bt_conn *sco;

	(void)bt_addr_to_str(&peer, expected, sizeof(expected));

	acl = peer_connects(ACL_HANDLE);

	peer_requests(BT_HCI_ESCO);
	sync_conn_complete(SCO_HANDLE, BT_HCI_ESCO, BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&sco_connected_sem, TIMEOUT), "No SCO connected callback");
	sco = bt_conn_ref(sco_chan.sco);
	str = bt_conn_dst_tmp_str(sco);
	zassert_str_equal(str.str, expected, "Address %s", str.str);

	disconn_complete(SCO_HANDLE);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");
	str = bt_conn_dst_tmp_str(sco);
	zassert_str_equal(str.str, expected, "Address %s", str.str);

	bt_conn_drop(&sco);
	expect_all_sco_objects_free();

	peer_disconnects(ACL_HANDLE);
	bt_conn_unref(acl);
}

static ZTEST(host_br_conn_lookup, test_connect_sco_to_peer_again)
{
	struct bt_conn *old_sco;
	struct bt_conn *sco;
	struct bt_conn *acl;

	acl = peer_connects(ACL_HANDLE);

	old_sco = bt_conn_create_sco(&peer, &sco_chan);
	zassert_not_null(old_sco, "Creating the SCO connection failed");
	sync_conn_complete(SCO_HANDLE, BT_HCI_SCO, BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&sco_connected_sem, TIMEOUT), "No SCO connected callback");
	disconn_complete(SCO_HANDLE);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");

	sco = bt_conn_create_sco(&peer, &sco_chan);
	zassert_not_null(sco, "Creating the SCO connection failed");
	zassert_not_equal(sco, old_sco, "The old object was used");
	sync_conn_complete(NEXT_SCO_HANDLE, BT_HCI_SCO, BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&sco_connected_sem, TIMEOUT), "No SCO connected callback");
	expect_state(sco, BT_CONN_STATE_CONNECTED);

	disconn_complete(NEXT_SCO_HANDLE);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");
	bt_conn_drop(&sco);
	bt_conn_drop(&old_sco);
	expect_all_sco_objects_free();

	peer_disconnects(ACL_HANDLE);
	bt_conn_unref(acl);
}

/* The ACL connection goes down while an SCO connection is being set up, which has no handle yet.
 * A Synchronous Connection Complete event may or may not follow.
 */
static void acl_gone_during_sco_setup(uint8_t status)
{
	struct bt_conn *acl;

	acl = peer_connects(ACL_HANDLE);
	peer_requests(BT_HCI_ESCO);
	zassert_not_null(sco_chan.sco, "No SCO connection");

	peer_disconnects(ACL_HANDLE);
	bt_conn_drop(&acl);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");
	zassert_equal(sco_disconnected_reason, REASON, "SCO disconnected with 0x%02x",
		      sco_disconnected_reason);
	zassert_is_null(sco_chan.sco, "The channel keeps its connection");

	sync_conn_complete(SCO_HANDLE, BT_HCI_ESCO, status);
	host_settle();
	zassert_not_ok(k_sem_take(&sco_connected_sem, K_NO_WAIT), "SCO connected callback");
	zassert_equal(sco_disconnected_count, 1U, "%u SCO disconnected callbacks",
		      sco_disconnected_count);

	acl = peer_connects(NEXT_ACL_HANDLE);
	expect_all_sco_objects_free();

	peer_disconnects(NEXT_ACL_HANDLE);
	bt_conn_unref(acl);
}

static ZTEST(host_br_conn_lookup, test_acl_gone_during_sco_setup)
{
	acl_gone_during_sco_setup(BT_HCI_ERR_CONN_TIMEOUT);
}

static ZTEST(host_br_conn_lookup, test_acl_gone_during_sco_setup_late_success)
{
	acl_gone_during_sco_setup(BT_HCI_ERR_SUCCESS);
}

/* The ACL connection goes down before the command that sets up the SCO connection is answered */
static void acl_gone_during_sco_command(uint8_t status)
{
	struct bt_conn *acl;
	struct bt_conn *sco;

	acl = peer_connects(ACL_HANDLE);

	refused_opcode = BT_HCI_OP_SETUP_SYNC_CONN;
	refused_status = status;
	acl_lost_on_sco_setup = true;

	sco = bt_conn_create_sco(&peer, &sco_chan);
	zassert_is_null(sco, "Got an SCO connection that is gone");
	zassert_ok(k_sem_take(&disconnected_sem, TIMEOUT), "No disconnected callback");
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");
	zassert_equal(sco_disconnected_count, 1U, "%u SCO disconnected callbacks",
		      sco_disconnected_count);
	zassert_is_null(sco_chan.sco, "The channel keeps its connection");
	zassert_equal(sco_chan.state, BT_SCO_STATE_DISCONNECTED, "Channel in state %u",
		      sco_chan.state);
	bt_conn_drop(&acl);

	acl = peer_connects(NEXT_ACL_HANDLE);
	expect_all_sco_objects_free();

	peer_disconnects(NEXT_ACL_HANDLE);
	bt_conn_unref(acl);
}

static ZTEST(host_br_conn_lookup, test_acl_gone_during_sco_command_refused)
{
	acl_gone_during_sco_command(BT_HCI_ERR_UNKNOWN_CONN_ID);
}

static ZTEST(host_br_conn_lookup, test_acl_gone_during_sco_command_success)
{
	acl_gone_during_sco_command(BT_HCI_ERR_SUCCESS);
}

/* The controller refuses the command, with the ACL connection up */
static ZTEST(host_br_conn_lookup, test_sco_command_refused)
{
	struct bt_conn *acl;

	acl = peer_connects(ACL_HANDLE);

	refused_opcode = BT_HCI_OP_SETUP_SYNC_CONN;
	refused_status = BT_HCI_ERR_CONN_LIMIT_EXCEEDED;
	zassert_is_null(bt_conn_create_sco(&peer, &sco_chan), "Got an SCO connection");
	refused_opcode = BT_OP_NOP;

	zassert_equal(sco_disconnected_count, 1U, "%u SCO disconnected callbacks",
		      sco_disconnected_count);
	zassert_equal(sco_disconnected_reason, BT_HCI_ERR_UNSPECIFIED,
		      "SCO disconnected with 0x%02x", sco_disconnected_reason);
	zassert_is_null(sco_chan.sco, "The channel keeps its connection");
	expect_state(acl, BT_CONN_STATE_CONNECTED);
	expect_all_sco_objects_free();

	peer_disconnects(ACL_HANDLE);
	bt_conn_unref(acl);
}

/* The ACL connection goes down while the SCO connection waits for a command buffer */
static ZTEST(host_br_conn_lookup, test_acl_gone_waiting_for_sco_command_buf)
{
	struct bt_conn *acl;

	acl = peer_connects(ACL_HANDLE);

	cmd_bufs_take();
	creator_start(create_sco);

	peer_disconnects(ACL_HANDLE);
	bt_conn_drop(&acl);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");

	cmd_bufs_release();
	creator_join();
	zassert_is_null(created_conn, "Got an SCO connection that is gone");
	zassert_is_null(sco_chan.sco, "The channel keeps its connection");
	zassert_equal(sco_chan.state, BT_SCO_STATE_DISCONNECTED, "Channel in state %u",
		      sco_chan.state);
	zassert_equal(sco_disconnected_count, 1U, "%u SCO disconnected callbacks",
		      sco_disconnected_count);

	acl = peer_connects(NEXT_ACL_HANDLE);
	expect_all_sco_objects_free();

	peer_disconnects(NEXT_ACL_HANDLE);
	bt_conn_unref(acl);
}

/* The ACL connection goes down while the SCO connection waits for a command buffer, and the
 * buffer comes before the work that takes the SCO connection down with the ACL connection
 */
static ZTEST(host_br_conn_lookup, test_acl_gone_before_sco_teardown)
{
	static K_WORK_DEFINE(work_q_hold_work, work_q_hold);
	struct bt_conn *acl;

	acl = peer_connects(ACL_HANDLE);

	cmd_bufs_take();
	creator_start(create_sco);

	disconn_complete(ACL_HANDLE);
	/* Behind the event, ahead of the work that handling it queues */
	work_submit(&work_q_hold_work);
	zassert_ok(k_sem_take(&work_q_held_sem, TIMEOUT), "The work queue is not held");

	cmd_bufs_release();
	creator_join();
	k_sem_give(&work_q_release_sem);
	zassert_ok(k_sem_take(&disconnected_sem, TIMEOUT), "No disconnected callback");
	bt_conn_drop(&acl);

	zassert_is_null(created_conn, "Got an SCO connection that is gone");
	zassert_is_null(sco_chan.sco, "The channel keeps its connection");
	zassert_equal(sco_chan.state, BT_SCO_STATE_DISCONNECTED, "Channel in state %u",
		      sco_chan.state);
	zassert_equal(sco_disconnected_count, 1U, "%u SCO disconnected callbacks",
		      sco_disconnected_count);
	zassert_equal(sco_disconnected_reason, BT_HCI_ERR_UNSPECIFIED,
		      "SCO disconnected with 0x%02x", sco_disconnected_reason);

	acl = peer_connects(NEXT_ACL_HANDLE);
	expect_all_sco_objects_free();

	peer_disconnects(NEXT_ACL_HANDLE);
	bt_conn_unref(acl);
}

/* A connection being created is found while it waits for a command buffer */
static ZTEST(host_br_conn_lookup, test_lookup_waiting_for_command_buf)
{
	struct bt_conn *conn;

	cmd_bufs_take();
	creator_start(create_br);

	conn = bt_conn_lookup_addr_br(&peer);
	cmd_bufs_release();
	creator_join();
	zassert_not_null(created_conn, "Creating the connection failed");
	zassert_equal(conn, created_conn, "The connection being created was not found");
	bt_conn_drop(&conn);

	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(created_conn);
	peer_disconnects(ACL_HANDLE);
	bt_conn_drop(&created_conn);
	expect_all_objects_free();
}

/* The controller refuses to create a connection */
static ZTEST(host_br_conn_lookup, test_connect_refused)
{
	struct bt_conn *conn;

	refused_opcode = BT_HCI_OP_CONNECT;
	refused_status = BT_HCI_ERR_CONN_LIMIT_EXCEEDED;
	zassert_is_null(bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT), "Got a connection");
	refused_opcode = BT_OP_NOP;

	zassert_is_null(bt_conn_lookup_addr_br(&peer), "An object was left behind");
	expect_all_objects_free();

	conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(conn, "Creating the connection failed");
	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	peer_disconnects(ACL_HANDLE);
	bt_conn_unref(conn);
}
