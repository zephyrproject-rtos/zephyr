/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* The application may keep a reference to the object of a BR/EDR or SCO connection after the
 * disconnection. That object is no longer the connection to the peer: the next connection with
 * the peer gets an object of its own, in either direction.
 *
 * Only one ACL connection can exist with a peer, so a request from the peer does not get a second
 * object while a connection with it is being set up or is up.
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
static K_SEM_DEFINE(reject_sem, 0, 1);

/* The status of the next command with this opcode. */
static uint16_t refused_opcode;
static uint8_t refused_status;

/* The ACL connection to take down when the Host sets up an SCO connection on it, before the
 * command is answered.
 */
static bool acl_lost_on_sco_setup;

/* The Command Status of Create Connection waits for the test */
static bool hold_connect_status;

/* The Disconnection Complete event waits for the test */
static bool hold_disconn_complete;

/* The reason of the last Reject Connection Request */
static uint8_t reject_reason;

/* The last Disconnect command */
static uint16_t disconnect_handle;
static uint8_t disconnect_reason;
static K_SEM_DEFINE(disconnect_sem, 0, 1);

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

static void cmd_status_with(uint16_t opcode, uint8_t status)
{
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = status;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
	bt_hci_recv(DEVICE_DT_INST_GET(0), net_buf_take(&buf));
}

static void cmd_status(uint16_t opcode)
{
	cmd_status_with(opcode, (opcode == refused_opcode) ? refused_status : BT_HCI_ERR_SUCCESS);
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

static void reject_req(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	const struct bt_hci_cp_reject_conn_req *cp = (const void *)buf->data;

	ARG_UNUSED(len);
	ARG_UNUSED(evt);

	reject_reason = cp->reason;
	if (cp->reason < BT_HCI_ERR_INSUFFICIENT_RESOURCES ||
	    cp->reason > BT_HCI_ERR_BD_ADDR_UNACCEPTABLE) {
		cmd_status_with(opcode, BT_HCI_ERR_INVALID_PARAM);
	} else {
		cmd_status(opcode);
	}

	k_sem_give(&reject_sem);
}

static void create_conn(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(evt);

	if (!hold_connect_status) {
		cmd_status(opcode);
	}
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

	disconnect_handle = sys_le16_to_cpu(cp->handle);
	disconnect_reason = cp->reason;
	k_sem_give(&disconnect_sem);

	cmd_status(opcode);
	if (!hold_disconn_complete) {
		disconn_complete(sys_le16_to_cpu(cp->handle));
	}
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
	{BT_HCI_OP_REJECT_CONN_REQ, 0U, reject_req},
	{BT_HCI_OP_CONNECT, 0U, create_conn},
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

static void role_change(uint8_t role)
{
	struct bt_hci_evt_role_change *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_ROLE_CHANGE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_ROLE_CHANGE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->role = role;
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

/* For a request of the peer while the other channel has its connection */
static struct bt_sco_chan sco_chan_other = {
	.ops = &sco_ops,
};

static int sco_accept(const struct bt_sco_accept_info *info, struct bt_sco_chan **chan)
{
	ARG_UNUSED(info);

	*chan = (sco_chan.sco != NULL) ? &sco_chan_other : &sco_chan;

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

/* What the application replies to a connection request */
static enum bt_br_conn_req_rsp conn_req_rsp;

static K_SEM_DEFINE(conn_req_sem, 0, 1);

static enum bt_br_conn_req_rsp conn_req(const bt_addr_t *addr, uint32_t cod)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(cod);

	k_sem_give(&conn_req_sem);

	return conn_req_rsp;
}

static bool conn_req_used;

/* The application replies to a connection request with this */
static void conn_req_reply_set(enum bt_br_conn_req_rsp rsp)
{
	conn_req_rsp = rsp;
	conn_req_used = true;
	zassert_ok(bt_br_set_connectable(true, conn_req), "Setting connectable failed");
}

static void work_nothing(struct k_work *work)
{
	ARG_UNUSED(work);
}

/* Handling an event can queue work behind what is already queued */
#define HOST_SETTLE_ROUNDS 2U

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

	if (conn_req_used) {
		conn_req_used = false;
		/* Setting connectable is what removes the callback, and the driver has no reset */
		zassert_ok(bt_br_set_connectable(false, NULL), "Clearing connectable failed");
		zassert_ok(bt_br_set_connectable(true, NULL), "Setting connectable failed");
		zassert_ok(bt_br_set_connectable(false, NULL), "Clearing connectable failed");
	}

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	bt_conn_drop(&created_conn);
	bt_conn_drop(&connected_conn);
	bt_conn_drop(&conn_on_accept);

	refused_opcode = BT_OP_NOP;
	acl_lost_on_sco_setup = false;
	hold_connect_status = false;
	hold_disconn_complete = false;
	lookup_on_accept = false;
	sco_chan.sco = NULL;
	sco_chan_other.sco = NULL;
	sco_disconnected_count = 0U;

	k_sem_reset(&accept_sem);
	k_sem_reset(&disconnect_sem);
	k_sem_reset(&reject_sem);
	k_sem_reset(&conn_req_sem);
	k_sem_reset(&connected_sem);
	k_sem_reset(&disconnected_sem);
	k_sem_reset(&sco_connected_sem);
	k_sem_reset(&sco_disconnected_sem);
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

/* The controller refuses that the connection from the peer is accepted */
static ZTEST(host_br_conn_lookup, test_accept_refused)
{
	struct bt_conn *conn;

	refused_opcode = BT_HCI_OP_ACCEPT_CONN_REQ;
	refused_status = BT_HCI_ERR_INSUFFICIENT_RESOURCES;
	peer_requests(BT_HCI_ACL);

	/* bt_conn_create_br() can have handed out the object while the command was on its way */
	conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_INSUFFICIENT_RESOURCES, "Connected with 0x%02x",
		      connected_err);
	refused_opcode = BT_OP_NOP;
	bt_conn_drop(&conn);

	zassert_is_null(bt_conn_lookup_addr_br(&peer), "An object was left behind");
	expect_all_objects_free();
}

/* Sets up a connection to the peer, and the request of the peer crosses it. Returns a reference to
 * the object of the connection.
 */
static struct bt_conn *crossing_request(void)
{
	struct bt_conn *conn;

	conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(conn, "Creating the connection failed");
	peer_requests(BT_HCI_ACL);

	return conn;
}

static void expect_no_connected_callback(void)
{
	host_settle();
	zassert_equal(k_sem_take(&connected_sem, K_NO_WAIT), -EBUSY,
		      "Unexpected connected callback");
}

static void expect_role(struct bt_conn *conn, uint8_t role)
{
	struct bt_conn_info info;

	zassert_ok(bt_conn_get_info(conn, &info), "Getting the connection info failed");
	zassert_equal(info.role, role, "Connection with role %u", info.role);
}

/* The connection is up on this object with this handle, and no other object has the peer */
static void expect_up(struct bt_conn *conn, uint16_t handle)
{
	struct bt_conn *found;
	uint16_t conn_handle;

	host_settle();
	expect_state(conn, BT_CONN_STATE_CONNECTED);
	zassert_equal(k_sem_take(&disconnected_sem, K_NO_WAIT), -EBUSY, "Disconnected callback");
	zassert_ok(bt_hci_get_conn_handle(conn, &conn_handle), "Getting the handle failed");
	zassert_equal(conn_handle, handle, "Connection with handle %u", conn_handle);

	found = bt_conn_lookup_addr_br(&peer);
	zassert_equal(found, conn, "The lookup found another object");
	bt_conn_unref(found);
}

/* The connected callback came with this error for this object, and the object is gone */
static void expect_failed(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn *failed;

	failed = wait_connected();
	zassert_equal(failed, conn, "The failure went to another object");
	zassert_equal(connected_err, err, "Connected with 0x%02x", connected_err);
	bt_conn_drop(&failed);
	expect_state(conn, BT_CONN_STATE_DISCONNECTED);
	zassert_is_null(bt_conn_lookup_addr_br(&peer), "An object was left behind");
}

/* The connection goes down, and with the reference the last object is gone */
static void expect_gone_after_disconnection(struct bt_conn **conn, uint16_t handle)
{
	peer_disconnects(handle);
	bt_conn_drop(conn);
	expect_all_objects_free();
}

/* The controller reports one event for the two commands */
static ZTEST(host_br_conn_lookup, test_crossing_one_success)
{
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	expect_up(conn, ACL_HANDLE);
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* The connection is the one that the peer asked for */
static ZTEST(host_br_conn_lookup, test_crossing_role)
{
	struct bt_conn *conn;

	conn = crossing_request();
	expect_role(conn, BT_CONN_ROLE_PERIPHERAL);
	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	expect_role(conn, BT_CONN_ROLE_PERIPHERAL);
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* The attempt that lost is reported after the connection is up, which stays up */
static ZTEST(host_br_conn_lookup, test_crossing_success_then_failure)
{
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	conn_complete(0U, BT_HCI_ERR_CONN_ALREADY_EXISTS);
	expect_up(conn, ACL_HANDLE);
	expect_no_connected_callback();

	zassert_ok(bt_conn_disconnect(conn, REASON), "Disconnecting failed");
	zassert_ok(k_sem_take(&disconnected_sem, TIMEOUT), "No disconnected callback");
	bt_conn_drop(&conn);
	expect_all_objects_free();
}

/* The attempt that lost is reported first, and the object waits for the other one */
static ZTEST(host_br_conn_lookup, test_crossing_lost_then_success)
{
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(0U, BT_HCI_ERR_CONN_ALREADY_EXISTS);
	expect_no_connected_callback();
	expect_state(conn, BT_CONN_STATE_CONNECTING);

	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	expect_up(conn, ACL_HANDLE);
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* The attempt that lost is reported first, and the other one fails as well */
static ZTEST(host_br_conn_lookup, test_crossing_lost_then_failure)
{
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(0U, BT_HCI_ERR_CONN_ALREADY_EXISTS);
	expect_no_connected_callback();

	conn_complete(0U, BT_HCI_ERR_CONN_TIMEOUT);
	expect_failed(conn, BT_HCI_ERR_CONN_TIMEOUT);
	bt_conn_drop(&conn);
	expect_all_objects_free();
}

/* The controller reports one failure for the two commands: nothing waits for a second event */
static ZTEST(host_br_conn_lookup, test_crossing_one_failure)
{
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(0U, BT_HCI_ERR_CONN_TIMEOUT);
	expect_failed(conn, BT_HCI_ERR_CONN_TIMEOUT);
	expect_no_connected_callback();
	bt_conn_drop(&conn);
	expect_all_objects_free();

	/* The next attempt pages again */
	conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(conn, "Creating the connection failed");
	conn_complete(NEXT_ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	expect_gone_after_disconnection(&conn, NEXT_ACL_HANDLE);
}

/* A failure that can be about either command ends the object. The link that comes up after it gets
 * a new one.
 */
static ZTEST(host_br_conn_lookup, test_crossing_failure_then_success)
{
	struct bt_conn *new_conn;
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(0U, BT_HCI_ERR_CONN_TIMEOUT);
	expect_failed(conn, BT_HCI_ERR_CONN_TIMEOUT);

	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	new_conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	zassert_not_equal(new_conn, conn, "The link came up on the object of the failure");
	expect_up(new_conn, ACL_HANDLE);
	expect_role(new_conn, BT_CONN_ROLE_PERIPHERAL);
	expect_state(conn, BT_CONN_STATE_DISCONNECTED);

	bt_conn_drop(&conn);
	expect_gone_after_disconnection(&new_conn, ACL_HANDLE);
}

/* The Create Connection fails before the request arrives: two attempts, one after the other. An
 * Intel AX200 reports a crossing like this, with a Page Timeout long before the page timeout.
 */
static ZTEST(host_br_conn_lookup, test_failure_before_request)
{
	struct bt_conn *new_conn;
	struct bt_conn *conn;

	conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(conn, "Creating the connection failed");
	conn_complete(0U, BT_HCI_ERR_PAGE_TIMEOUT);
	expect_failed(conn, BT_HCI_ERR_PAGE_TIMEOUT);

	new_conn = peer_connects(ACL_HANDLE);
	zassert_not_equal(new_conn, conn, "The link came up on the object of the failure");
	expect_up(new_conn, ACL_HANDLE);

	bt_conn_drop(&conn);
	expect_gone_after_disconnection(&new_conn, ACL_HANDLE);
}

/* The application cancels its connection, while the request of the peer is being accepted */
static ZTEST(host_br_conn_lookup, test_crossing_cancel)
{
	struct bt_conn *new_conn;
	struct bt_conn *conn;

	conn = crossing_request();
	zassert_ok(bt_conn_disconnect(conn, REASON), "Cancelling failed");
	conn_complete(0U, BT_HCI_ERR_UNKNOWN_CONN_ID);
	expect_failed(conn, BT_HCI_ERR_UNKNOWN_CONN_ID);

	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	new_conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	expect_up(new_conn, ACL_HANDLE);

	bt_conn_drop(&conn);
	expect_gone_after_disconnection(&new_conn, ACL_HANDLE);
}

/* The application hears of the failure once */
static ZTEST(host_br_conn_lookup, test_crossing_both_fail)
{
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(0U, BT_HCI_ERR_PAGE_TIMEOUT);
	expect_failed(conn, BT_HCI_ERR_PAGE_TIMEOUT);

	conn_complete(0U, BT_HCI_ERR_CONN_TIMEOUT);
	expect_no_connected_callback();
	bt_conn_drop(&conn);
	expect_all_objects_free();
}

/* A second success can not take over the connection that is up */
static ZTEST(host_br_conn_lookup, test_crossing_second_success)
{
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	conn_complete(NEXT_ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_up(conn, ACL_HANDLE);
	expect_no_connected_callback();
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* The connection goes down before the attempt that lost is reported */
static ZTEST(host_br_conn_lookup, test_crossing_late_failure)
{
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	peer_disconnects(ACL_HANDLE);

	conn_complete(0U, BT_HCI_ERR_CONN_ALREADY_EXISTS);
	expect_no_connected_callback();
	bt_conn_drop(&conn);
	expect_all_objects_free();
}

/* The other command brings up a link after the connection of the first one went down */
static ZTEST(host_br_conn_lookup, test_crossing_success_after_disconnection)
{
	struct bt_conn *new_conn;
	struct bt_conn *conn;

	conn = crossing_request();
	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	peer_disconnects(ACL_HANDLE);

	conn_complete(NEXT_ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	new_conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	zassert_not_equal(new_conn, conn, "The link came up on the old object");
	expect_up(new_conn, NEXT_ACL_HANDLE);

	bt_conn_drop(&conn);
	expect_gone_after_disconnection(&new_conn, NEXT_ACL_HANDLE);
}

/* No object is left for a link that comes up after a failure: the Host takes the link down */
static ZTEST(host_br_conn_lookup, test_success_without_free_object)
{
	struct bt_conn *conns[CONFIG_BT_MAX_CONN];

	ARRAY_FOR_EACH(conns, i) {
		conns[i] = bt_conn_add_br(&peer);
		zassert_not_null(conns[i], "Object %zu is not free", i);
	}

	hold_disconn_complete = true;
	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&disconnect_sem, TIMEOUT), "The link was not disconnected");
	zassert_equal(disconnect_handle, ACL_HANDLE, "Disconnected handle %u", disconnect_handle);
	zassert_equal(disconnect_reason, BT_HCI_ERR_REMOTE_LOW_RESOURCES,
		      "Disconnected with 0x%02x", disconnect_reason);
	expect_no_connected_callback();

	ARRAY_FOR_EACH(conns, i) {
		bt_conn_drop(&conns[i]);
	}

	expect_all_objects_free();
}

/* The application rejects a request: no object, and it does not hear of its own decision */
static ZTEST(host_br_conn_lookup, test_request_rejected)
{
	conn_req_reply_set(BT_BR_CONN_REQ_REJECT_NO_RESOURCES);

	conn_request(BT_HCI_ACL);
	zassert_ok(k_sem_take(&reject_sem, TIMEOUT), "The connection was not rejected");
	expect_no_connected_callback();
	zassert_is_null(bt_conn_lookup_addr_br(&peer), "An object was left behind");
	expect_all_objects_free();
}

/* The application rejects the request of the peer, while the attempt of its own goes on */
static ZTEST(host_br_conn_lookup, test_crossing_request_rejected)
{
	struct bt_conn *conn;

	conn_req_reply_set(BT_BR_CONN_REQ_REJECT_NO_RESOURCES);

	conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(conn, "Creating the connection failed");
	conn_request(BT_HCI_ACL);
	zassert_ok(k_sem_take(&reject_sem, TIMEOUT), "The connection was not rejected");
	zassert_equal(reject_reason, BT_HCI_ERR_INSUFFICIENT_RESOURCES, "Rejected with 0x%02x",
		      reject_reason);

	conn_complete(0U, BT_HCI_ERR_INSUFFICIENT_RESOURCES);
	expect_no_connected_callback();
	expect_state(conn, BT_CONN_STATE_CONNECTING);

	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	expect_up(conn, ACL_HANDLE);
	expect_role(conn, BT_CONN_ROLE_CENTRAL);
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* The controller refuses the rejection, so only the attempt of its own has an event to come */
static ZTEST(host_br_conn_lookup, test_crossing_reject_refused)
{
	struct bt_conn *failed;
	struct bt_conn *conn;

	conn_req_reply_set(BT_BR_CONN_REQ_REJECT_NO_RESOURCES);

	refused_opcode = BT_HCI_OP_REJECT_CONN_REQ;
	refused_status = BT_HCI_ERR_CMD_DISALLOWED;
	conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(conn, "Creating the connection failed");
	conn_request(BT_HCI_ACL);
	zassert_ok(k_sem_take(&reject_sem, TIMEOUT), "The connection was not rejected");
	host_settle();
	refused_opcode = BT_OP_NOP;

	conn_complete(0U, BT_HCI_ERR_PAGE_TIMEOUT);
	failed = wait_connected();
	zassert_equal(failed, conn, "The failure went to another object");
	zassert_equal(connected_err, BT_HCI_ERR_PAGE_TIMEOUT, "Connected with 0x%02x",
		      connected_err);
	bt_conn_drop(&failed);
	bt_conn_drop(&conn);
	expect_all_objects_free();
}

/* The controller refuses to accept the request of the peer */
static ZTEST(host_br_conn_lookup, test_crossing_accept_refused)
{
	struct bt_conn *conn;

	refused_opcode = BT_HCI_OP_ACCEPT_CONN_REQ;
	refused_status = BT_HCI_ERR_CMD_DISALLOWED;
	conn = crossing_request();
	zassert_ok(k_sem_take(&reject_sem, TIMEOUT), "The connection was not rejected");
	host_settle();
	refused_opcode = BT_OP_NOP;
	zassert_equal(reject_reason, BT_HCI_ERR_INSUFFICIENT_RESOURCES, "Rejected with 0x%02x",
		      reject_reason);

	conn_complete(0U, BT_HCI_ERR_INSUFFICIENT_RESOURCES);
	expect_no_connected_callback();
	expect_state(conn, BT_CONN_STATE_CONNECTING);

	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	expect_connected(conn);
	expect_up(conn, ACL_HANDLE);
	expect_role(conn, BT_CONN_ROLE_CENTRAL);
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* The request reaches the Host while Create Connection waits for its status, which is a refusal,
 * as seen from a CSR8510. The Accept follows, and the thread of Create Connection can run.
 */
static void crossing_connect_refused(void)
{
	conn_req_reply_set(BT_BR_CONN_REQ_ACCEPT_PERIPHERAL);

	hold_connect_status = true;
	creator_start(create_br);

	conn_request(BT_HCI_ACL);
	zassert_ok(k_sem_take(&conn_req_sem, TIMEOUT), "The request did not reach the Host");

	refused_opcode = BT_HCI_OP_CONNECT;
	refused_status = BT_HCI_ERR_CMD_DISALLOWED;
	cmd_status(BT_HCI_OP_CONNECT);
}

/* The request arrives before Create Connection is refused: the connection comes up anyway */
static ZTEST(host_br_conn_lookup, test_crossing_connect_refused)
{
	struct bt_conn *conn;

	crossing_connect_refused();
	creator_join();
	zassert_is_null(created_conn, "Got a connection");
	zassert_ok(k_sem_take(&accept_sem, TIMEOUT), "The connection was not accepted");

	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	expect_up(conn, ACL_HANDLE);
	expect_role(conn, BT_CONN_ROLE_PERIPHERAL);
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* Both commands are refused, so nothing is left to come: the failure is reported */
static ZTEST(host_br_conn_lookup, test_crossing_connect_and_accept_refused)
{
	struct bt_conn *conn;

	crossing_connect_refused();
	refused_opcode = BT_HCI_OP_ACCEPT_CONN_REQ;

	conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_INSUFFICIENT_RESOURCES, "Connected with 0x%02x",
		      connected_err);
	creator_join();
	zassert_is_null(created_conn, "Got a connection");
	expect_no_connected_callback();
	bt_conn_drop(&conn);
	expect_all_objects_free();
}

/* The accepted request fails after Create Connection has returned */
static ZTEST(host_br_conn_lookup, test_crossing_connect_refused_then_failure)
{
	struct bt_conn *conn;

	crossing_connect_refused();
	creator_join();
	zassert_ok(k_sem_take(&accept_sem, TIMEOUT), "The connection was not accepted");

	conn_complete(0U, BT_HCI_ERR_CONN_TIMEOUT);
	conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_CONN_TIMEOUT, "Connected with 0x%02x",
		      connected_err);
	expect_no_connected_callback();
	bt_conn_drop(&conn);
	expect_all_objects_free();
}

/* The accepted request fails before the thread of Create Connection has run */
static ZTEST(host_br_conn_lookup, test_crossing_failure_then_connect_refused)
{
	struct bt_conn *conn;

	crossing_connect_refused();
	k_thread_priority_set(&creator_thread, K_LOWEST_APPLICATION_THREAD_PRIO);
	zassert_ok(k_sem_take(&accept_sem, TIMEOUT), "The connection was not accepted");

	conn_complete(0U, BT_HCI_ERR_CONN_TIMEOUT);
	conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_CONN_TIMEOUT, "Connected with 0x%02x",
		      connected_err);

	creator_join();
	zassert_is_null(created_conn, "Got a connection");
	expect_no_connected_callback();
	bt_conn_drop(&conn);
	expect_all_objects_free();
}

/* A request from a peer with a connection up is rejected, which does not touch that connection */
static ZTEST(host_br_conn_lookup, test_request_while_connected)
{
	struct bt_conn *conn;

	conn = peer_connects(ACL_HANDLE);

	conn_request(BT_HCI_ACL);
	zassert_ok(k_sem_take(&reject_sem, TIMEOUT), "The connection was not rejected");
	zassert_equal(reject_reason, BT_HCI_ERR_BD_ADDR_UNACCEPTABLE, "Rejected with 0x%02x",
		      reject_reason);

	conn_complete(0U, BT_HCI_ERR_BD_ADDR_UNACCEPTABLE);
	expect_up(conn, ACL_HANDLE);
	expect_no_connected_callback();
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* The controller refuses the rejection, which does not touch the connection either */
static ZTEST(host_br_conn_lookup, test_request_while_connected_reject_refused)
{
	struct bt_conn *conn;

	conn = peer_connects(ACL_HANDLE);

	refused_opcode = BT_HCI_OP_REJECT_CONN_REQ;
	refused_status = BT_HCI_ERR_CMD_DISALLOWED;
	conn_request(BT_HCI_ACL);
	zassert_ok(k_sem_take(&reject_sem, TIMEOUT), "The connection was not rejected");
	expect_up(conn, ACL_HANDLE);
	refused_opcode = BT_OP_NOP;

	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* The same while the connection is going down */
static ZTEST(host_br_conn_lookup, test_request_while_disconnecting)
{
	struct bt_conn *conn;

	conn = peer_connects(ACL_HANDLE);

	hold_disconn_complete = true;
	zassert_ok(bt_conn_disconnect(conn, REASON), "Disconnecting failed");

	conn_request(BT_HCI_ACL);
	zassert_ok(k_sem_take(&reject_sem, TIMEOUT), "The connection was not rejected");
	conn_complete(0U, BT_HCI_ERR_BD_ADDR_UNACCEPTABLE);
	expect_no_connected_callback();
	expect_state(conn, BT_CONN_STATE_DISCONNECTING);
	zassert_equal(k_sem_take(&disconnected_sem, K_NO_WAIT), -EBUSY, "Disconnected callback");

	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* A second request while the first one is being accepted only gets a rejection */
static ZTEST(host_br_conn_lookup, test_second_request_while_accepting)
{
	struct bt_conn *conn;

	peer_requests(BT_HCI_ACL);

	conn_request(BT_HCI_ACL);
	zassert_ok(k_sem_take(&reject_sem, TIMEOUT), "The connection was not rejected");
	zassert_equal(reject_reason, BT_HCI_ERR_BD_ADDR_UNACCEPTABLE, "Rejected with 0x%02x",
		      reject_reason);

	conn_complete(0U, BT_HCI_ERR_BD_ADDR_UNACCEPTABLE);
	expect_no_connected_callback();

	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	expect_up(conn, ACL_HANDLE);
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

/* A role switch during the setup does not make the object look like one of this side */
static ZTEST(host_br_conn_lookup, test_second_request_after_role_change)
{
	struct bt_conn *conn;

	conn_req_reply_set(BT_BR_CONN_REQ_ACCEPT_CENTRAL);

	peer_requests(BT_HCI_ACL);
	role_change(BT_HCI_ROLE_CENTRAL);
	host_settle();

	conn_request(BT_HCI_ACL);
	zassert_ok(k_sem_take(&reject_sem, TIMEOUT), "The connection was not rejected");
	conn_complete(0U, BT_HCI_ERR_BD_ADDR_UNACCEPTABLE);
	expect_no_connected_callback();

	conn_complete(ACL_HANDLE, BT_HCI_ERR_SUCCESS);
	conn = wait_connected();
	zassert_equal(connected_err, BT_HCI_ERR_SUCCESS, "Connected with 0x%02x", connected_err);
	expect_up(conn, ACL_HANDLE);
	expect_role(conn, BT_CONN_ROLE_CENTRAL);
	expect_gone_after_disconnection(&conn, ACL_HANDLE);
}

static void expect_sco_handle(const struct bt_conn *sco, uint16_t handle)
{
	uint16_t conn_handle;

	zassert_ok(bt_hci_get_conn_handle(sco, &conn_handle), "Getting the handle failed");
	zassert_equal(conn_handle, handle, "SCO connection with handle %u", conn_handle);
}

/* An SCO connection of this side and a request from the peer: each gets its own connection */
static ZTEST(host_br_conn_lookup, test_sco_crossing)
{
	struct bt_conn *sco;
	struct bt_conn *acl;

	acl = peer_connects(ACL_HANDLE);

	sco = bt_conn_create_sco(&peer, &sco_chan);
	zassert_not_null(sco, "Creating the SCO connection failed");
	peer_requests(BT_HCI_ESCO);

	sync_conn_complete(SCO_HANDLE, BT_HCI_ESCO, BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&sco_connected_sem, TIMEOUT), "No SCO connected callback");
	sync_conn_complete(NEXT_SCO_HANDLE, BT_HCI_ESCO, BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&sco_connected_sem, TIMEOUT), "No SCO connected callback");

	expect_state(sco_chan.sco, BT_CONN_STATE_CONNECTED);
	expect_state(sco_chan_other.sco, BT_CONN_STATE_CONNECTED);
	expect_sco_handle(sco_chan.sco, SCO_HANDLE);
	expect_sco_handle(sco_chan_other.sco, NEXT_SCO_HANDLE);

	disconn_complete(SCO_HANDLE);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");
	disconn_complete(NEXT_SCO_HANDLE);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");
	bt_conn_drop(&sco);
	expect_all_sco_objects_free();

	peer_disconnects(ACL_HANDLE);
	bt_conn_unref(acl);
}

/* The failure of another attempt does not end the SCO connection that is up */
static ZTEST(host_br_conn_lookup, test_sco_late_failure)
{
	struct bt_conn *acl;

	acl = peer_connects(ACL_HANDLE);

	peer_requests(BT_HCI_ESCO);
	sync_conn_complete(SCO_HANDLE, BT_HCI_ESCO, BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&sco_connected_sem, TIMEOUT), "No SCO connected callback");

	sync_conn_complete(0U, BT_HCI_ESCO, BT_HCI_ERR_CONN_ALREADY_EXISTS);
	host_settle();
	expect_state(sco_chan.sco, BT_CONN_STATE_CONNECTED);
	expect_sco_handle(sco_chan.sco, SCO_HANDLE);
	zassert_equal(k_sem_take(&sco_disconnected_sem, K_NO_WAIT), -EBUSY,
		      "SCO disconnected callback");

	disconn_complete(SCO_HANDLE);
	zassert_ok(k_sem_take(&sco_disconnected_sem, TIMEOUT), "No SCO disconnected callback");
	expect_all_sco_objects_free();

	peer_disconnects(ACL_HANDLE);
	bt_conn_unref(acl);
}
