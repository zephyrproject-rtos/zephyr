/* hci_core.c - HCI core Bluetooth handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/byteorder.h>

#include <bluetooth/log.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/driver.h>

#include "stack.h"

#include "keys.h"
#include "hci_core.h"

#if defined(CONFIG_BLUETOOTH_CONN)
#include "conn_internal.h"
#include "l2cap_internal.h"
#endif /* CONFIG_BLUETOOTH_CONN */

#if defined(CONFIG_BLUETOOTH_SMP)
#include "smp.h"
#endif /* CONFIG_BLUETOOTH_SMP */

#if !defined(CONFIG_BLUETOOTH_DEBUG_HCI_CORE)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

/* Stacks for the fibers */
static BT_STACK_NOINIT(rx_fiber_stack, 1024);
static BT_STACK_NOINIT(rx_prio_fiber_stack, 256);
static BT_STACK_NOINIT(cmd_tx_fiber_stack, 256);

struct bt_dev bt_dev;

static bt_le_scan_cb_t *scan_dev_found_cb;

struct cmd_data {
	/** The command OpCode that the buffer contains */
	uint16_t opcode;

	/** Used by bt_hci_cmd_send_sync. Initially contains the waiting
	 *  semaphore, as the semaphore is given back contains the bt_buf
	 *  for the return parameters.
	 */
	void *sync;

};

struct acl_data {
	/** ACL connection handle */
	uint16_t handle;
};

#define cmd(buf) ((struct cmd_data *)net_buf_user_data(buf))
#define acl(buf) ((struct acl_data *)net_buf_user_data(buf))

/* HCI command buffers */
#define CMD_BUF_SIZE (CONFIG_BLUETOOTH_HCI_SEND_RESERVE + \
		      sizeof(struct bt_hci_cmd_hdr) + \
		      CONFIG_BLUETOOTH_MAX_CMD_LEN)
static struct nano_fifo avail_hci_cmd;
static NET_BUF_POOL(hci_cmd_pool, CONFIG_BLUETOOTH_HCI_CMD_COUNT, CMD_BUF_SIZE,
		    &avail_hci_cmd, NULL, sizeof(struct cmd_data));

/* HCI event buffers */
#define EVT_BUF_SIZE (CONFIG_BLUETOOTH_HCI_RECV_RESERVE + \
		      sizeof(struct bt_hci_evt_hdr) + \
		      CONFIG_BLUETOOTH_MAX_EVT_LEN)
static struct nano_fifo avail_hci_evt;
static NET_BUF_POOL(hci_evt_pool, CONFIG_BLUETOOTH_HCI_EVT_COUNT, EVT_BUF_SIZE,
		    &avail_hci_evt, NULL, 0);

#if defined(CONFIG_BLUETOOTH_CONN)
static void report_completed_packet(struct net_buf *buf)
{

	struct bt_hci_cp_host_num_completed_packets *cp;
	uint16_t handle = acl(buf)->handle;
	struct bt_hci_handle_count *hc;

	BT_DBG("Reporting completed packet for handle %u", handle);

	nano_fifo_put(buf->free, buf);

	buf = bt_hci_cmd_create(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS,
				sizeof(*cp) + sizeof(*hc));
	if (!buf) {
		BT_ERR("Unable to allocate new HCI command");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->num_handles = sys_cpu_to_le16(1);

	hc = net_buf_add(buf, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(handle);
	hc->count  = sys_cpu_to_le16(1);

	bt_hci_cmd_send(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS, buf);
}

static struct nano_fifo avail_acl_in;
static NET_BUF_POOL(acl_in_pool, CONFIG_BLUETOOTH_ACL_IN_COUNT,
		    BT_L2CAP_BUF_SIZE(CONFIG_BLUETOOTH_L2CAP_IN_MTU),
		    &avail_acl_in, report_completed_packet,
		    sizeof(struct acl_data));
#endif /* CONFIG_BLUETOOTH_CONN */

/* Incoming buffer type lookup helper */
static enum bt_buf_type bt_type(struct net_buf *buf)
{
	if (buf->free == &avail_hci_evt) {
		return BT_EVT;
	} else {
		return BT_ACL_IN;
	}
}

#if defined(CONFIG_BLUETOOTH_DEBUG)
const char *bt_addr_str(const bt_addr_t *addr)
{
	static char bufs[2][18];
	static uint8_t cur;
	char *str;

	str = bufs[cur++];
	cur %= ARRAY_SIZE(bufs);
	bt_addr_to_str(addr, str, sizeof(bufs[cur]));

	return str;
}

const char *bt_addr_le_str(const bt_addr_le_t *addr)
{
	static char bufs[2][27];
	static uint8_t cur;
	char *str;

	str = bufs[cur++];
	cur %= ARRAY_SIZE(bufs);
	bt_addr_le_to_str(addr, str, sizeof(bufs[cur]));

	return str;
}
#endif /* CONFIG_BLUETOOTH_DEBUG */

struct net_buf *bt_hci_cmd_create(uint16_t opcode, uint8_t param_len)
{
	struct bt_hci_cmd_hdr *hdr;
	struct net_buf *buf;

	BT_DBG("opcode %x param_len %u", opcode, param_len);

	buf = net_buf_get(&avail_hci_cmd, CONFIG_BLUETOOTH_HCI_SEND_RESERVE);
	if (!buf) {
		BT_ERR("Cannot get free buffer");
		return NULL;
	}

	BT_DBG("buf %p", buf);

	cmd(buf)->opcode = opcode;
	cmd(buf)->sync = NULL;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->opcode = sys_cpu_to_le16(opcode);
	hdr->param_len = param_len;

	return buf;
}

int bt_hci_cmd_send(uint16_t opcode, struct net_buf *buf)
{
	if (!buf) {
		buf = bt_hci_cmd_create(opcode, 0);
		if (!buf) {
			return -ENOBUFS;
		}
	}

	BT_DBG("opcode %x len %u", opcode, buf->len);

	/* Host Number of Completed Packets can ignore the ncmd value
	 * and does not generate any cmd complete/status events.
	 */
	if (opcode == BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS) {
		int err;

		err = bt_dev.drv->send(BT_CMD, buf);
		if (err) {
			BT_ERR("Unable to send to driver (err %d)", err);
			net_buf_unref(buf);
		}

		return err;
	}

	nano_fifo_put(&bt_dev.cmd_tx_queue, buf);

	return 0;
}

int bt_hci_cmd_send_sync(uint16_t opcode, struct net_buf *buf,
			 struct net_buf **rsp)
{
	struct nano_sem sync_sem;
	int err;

	if (!buf) {
		buf = bt_hci_cmd_create(opcode, 0);
		if (!buf) {
			return -ENOBUFS;
		}
	}

	BT_DBG("opcode %x len %u", opcode, buf->len);

	nano_sem_init(&sync_sem);
	cmd(buf)->sync = &sync_sem;

	nano_fifo_put(&bt_dev.cmd_tx_queue, buf);

	nano_sem_take_wait(&sync_sem);

	/* Indicate failure if we failed to get the return parameters */
	if (!cmd(buf)->sync) {
		err = -EIO;
	} else {
		err = 0;
	}

	if (rsp) {
		*rsp = cmd(buf)->sync;
	} else if (cmd(buf)->sync) {
		net_buf_unref(cmd(buf)->sync);
	}

	net_buf_unref(buf);

	return err;
}

static int bt_hci_stop_scanning(void)
{
	struct net_buf *buf, *rsp;
	struct bt_hci_cp_le_set_scan_enable *scan_enable;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		return -EALREADY;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_ENABLE,
				sizeof(*scan_enable));
	if (!buf) {
		return -ENOBUFS;
	}

	scan_enable = net_buf_add(buf, sizeof(*scan_enable));
	memset(scan_enable, 0, sizeof(*scan_enable));
	scan_enable->filter_dup = 0x00;
	scan_enable->enable = 0x00;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_SCAN_ENABLE, buf, &rsp);
	if (err) {
		return err;
	}

	/* Update scan state in case of success (0) status */
	err = rsp->data[0];
	if (!err) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_SCANNING);
	}

	net_buf_unref(rsp);

	return err;
}

static const bt_addr_le_t *find_id_addr(const bt_addr_le_t *addr)
{
#if defined(CONFIG_BLUETOOTH_SMP)
	struct bt_keys *keys;

	keys = bt_keys_find_irk(addr);
	if (keys) {
		BT_DBG("Identity %s matched RPA %s",
		       bt_addr_le_str(&keys->addr), bt_addr_le_str(addr));
		return &keys->addr;
	}
#endif
	return addr;
}

#if defined(CONFIG_BLUETOOTH_CONN)
static void hci_acl(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr = (void *)buf->data;
	uint16_t handle, len = sys_le16_to_cpu(hdr->len);
	struct bt_conn *conn;
	uint8_t flags;

	BT_DBG("buf %p", buf);

	handle = sys_le16_to_cpu(hdr->handle);
	flags = bt_acl_flags(handle);

	acl(buf)->handle = bt_acl_handle(handle);

	net_buf_pull(buf, sizeof(*hdr));

	BT_DBG("handle %u len %u flags %u", acl(buf)->handle, len, flags);

	if (buf->len != len) {
		BT_ERR("ACL data length mismatch (%u != %u)", buf->len, len);
		net_buf_unref(buf);
		return;
	}

	conn = bt_conn_lookup_handle(acl(buf)->handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", acl(buf)->handle);
		net_buf_unref(buf);
		return;
	}

	bt_conn_recv(conn, buf, flags);
	bt_conn_unref(conn);
}

static void hci_num_completed_packets(struct net_buf *buf)
{
	struct bt_hci_evt_num_completed_packets *evt = (void *)buf->data;
	uint16_t i, num_handles = sys_le16_to_cpu(evt->num_handles);

	BT_DBG("num_handles %u", num_handles);

	for (i = 0; i < num_handles; i++) {
		uint16_t handle, count;
		struct bt_conn *conn;

		handle = sys_le16_to_cpu(evt->h[i].handle);
		count = sys_le16_to_cpu(evt->h[i].count);

		BT_DBG("handle %u count %u", handle, count);

		conn = bt_conn_lookup_handle(handle);
		if (!conn) {
			BT_ERR("No connection for handle %u", handle);
			continue;
		}

		if (conn->pending_pkts >= count) {
			conn->pending_pkts -= count;
		} else {
			BT_ERR("completed packets mismatch: %u > %u",
			       count, conn->pending_pkts);
			conn->pending_pkts = 0;
		}

		while (count--) {
			nano_fiber_sem_give(bt_conn_get_pkts(conn));
		}

		bt_conn_unref(conn);
	}
}

static int hci_le_create_conn(const bt_addr_le_t *addr)
{
	struct net_buf *buf;
	struct bt_hci_cp_le_create_conn *cp;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_CONN, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memset(cp, 0, sizeof(*cp));

	/* Interval == window for continuous scanning */
	cp->scan_interval = sys_cpu_to_le16(0x0060);
	cp->scan_window = cp->scan_interval;

	bt_addr_le_copy(&cp->peer_addr, addr);
	cp->conn_interval_max = sys_cpu_to_le16(0x0028);
	cp->conn_interval_min = sys_cpu_to_le16(0x0018);
	cp->supervision_timeout = sys_cpu_to_le16(0x07D0);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CONN, buf, NULL);
}

static void hci_disconn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_disconn_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u reason %u", evt->status, handle,
	       evt->reason);

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	/* Check stacks usage (no-ops if not enabled) */
	stack_analyze("rx stack", rx_fiber_stack, sizeof(rx_fiber_stack));
	stack_analyze("cmd rx stack", rx_prio_fiber_stack,
		      sizeof(rx_prio_fiber_stack));
	stack_analyze("cmd tx stack", cmd_tx_fiber_stack,
		      sizeof(cmd_tx_fiber_stack));
	stack_analyze("conn tx stack", conn->stack, sizeof(conn->stack));

	bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
	conn->handle = 0;

	if (atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT)) {
		bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);
		bt_le_scan_update();
	}

	bt_conn_unref(conn);

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		struct net_buf *buf;
		uint8_t adv_enable = 0x01;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
		if (buf) {
			memcpy(net_buf_add(buf, 1), &adv_enable, 1);
			bt_hci_cmd_send(BT_HCI_OP_LE_SET_ADV_ENABLE, buf);
		}
	}
}

static int hci_le_read_remote_features(struct bt_conn *conn)
{
	struct bt_hci_cp_le_read_remote_features *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_READ_REMOTE_FEATURES,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	bt_hci_cmd_send(BT_HCI_OP_LE_READ_REMOTE_FEATURES, buf);

	return 0;
}

static int update_conn_params(struct bt_conn *conn)
{
	BT_DBG("conn %p features 0x%x", conn, conn->le.features[0]);

	/* Check if there's a need to update conn params */
	if (conn->le.conn_interval >= LE_CONN_MIN_INTERVAL &&
	    conn->le.conn_interval <= LE_CONN_MAX_INTERVAL) {
		return -EALREADY;
	}

	if ((conn->role == BT_HCI_ROLE_SLAVE) &&
	    !(bt_dev.le.features[0] & BT_HCI_LE_CONN_PARAM_REQ_PROC)) {
		return bt_l2cap_update_conn_param(conn);
	}

	if ((conn->le.features[0] & BT_HCI_LE_CONN_PARAM_REQ_PROC) &&
	    (bt_dev.le.features[0] & BT_HCI_LE_CONN_PARAM_REQ_PROC)) {
		return bt_conn_le_conn_update(conn, LE_CONN_MIN_INTERVAL,
					     LE_CONN_MAX_INTERVAL,
					     LE_CONN_LATENCY, LE_CONN_TIMEOUT);
	}

	return -EBUSY;
}

static void le_conn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	const bt_addr_le_t *id_addr;
	struct bt_conn *conn;
	bt_addr_le_t src;
	int err;

	BT_DBG("status %u handle %u role %u %s", evt->status, handle,
	       evt->role, bt_addr_le_str(&evt->peer_addr));

	id_addr = find_id_addr(&evt->peer_addr);

	/* Make lookup to check if there's a connection object in CONNECT state
	 * associated with passed peer LE address.
	 */
	conn = bt_conn_lookup_state(id_addr, BT_CONN_CONNECT);

	if (evt->status) {
		if (!conn) {
			return;
		}

		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);

		/* Drop the reference got by lookup call in CONNECT state.
		 * We are now in DISCONNECTED state since no successful LE
		 * link been made.
		 */
		bt_conn_unref(conn);

		return;
	}

	if (!conn) {
		conn = bt_conn_add_le(id_addr);
	}

	if (!conn) {
		BT_ERR("Unable to add new conn for handle %u", handle);
		return;
	}

	conn->handle   = handle;
	bt_addr_le_copy(&conn->le.dst, id_addr);
	conn->le.conn_interval = sys_le16_to_cpu(evt->interval);
	conn->role = evt->role;

	src.type = BT_ADDR_LE_PUBLIC;
	memcpy(src.val, bt_dev.bdaddr.val, sizeof(bt_dev.bdaddr.val));

	/* use connection address (instead of identity address) as initiator
	 * or responder address
	 */
	if (conn->role == BT_HCI_ROLE_MASTER) {
		bt_addr_le_copy(&conn->le.init_addr, &src);
		bt_addr_le_copy(&conn->le.resp_addr, &evt->peer_addr);
	} else {
		bt_addr_le_copy(&conn->le.init_addr, &evt->peer_addr);
		bt_addr_le_copy(&conn->le.resp_addr, &src);
	}

	bt_conn_set_state(conn, BT_CONN_CONNECTED);

	if ((evt->role == BT_HCI_ROLE_MASTER) ||
	    (bt_dev.le.features[0] & BT_HCI_LE_SLAVE_FEATURES)) {
		err = hci_le_read_remote_features(conn);
		if (!err) {
			goto done;
		}
	}

	update_conn_params(conn);

done:
	bt_conn_unref(conn);
	bt_le_scan_update();
}

static void le_remote_feat_complete(struct net_buf *buf)
{
	struct bt_hci_ev_le_remote_feat_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	if (!evt->status) {
		memcpy(conn->le.features, evt->features,
		       sizeof(conn->le.features));
	}

	update_conn_params(conn);

	bt_conn_unref(conn);
}

static int le_conn_param_neg_reply(uint16_t handle, uint8_t reason)
{
	struct bt_hci_cp_le_conn_param_req_neg_reply *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->reason = sys_cpu_to_le16(reason);

	return bt_hci_cmd_send(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY, buf);
}

static int le_conn_param_req_reply(uint16_t handle, uint16_t min, uint16_t max,
				   uint16_t latency, uint16_t timeout)
{
	struct bt_hci_cp_le_conn_param_req_reply *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memset(cp, 0, sizeof(*cp));

	cp->handle = sys_cpu_to_le16(handle);
	cp->interval_min = sys_cpu_to_le16(min);
	cp->interval_max = sys_cpu_to_le16(max);
	cp->latency = sys_cpu_to_le16(latency);
	cp->timeout = sys_cpu_to_le16(timeout);

	return bt_hci_cmd_send(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY, buf);
}

static int le_conn_param_req(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_param_req *evt = (void *)buf->data;
	struct bt_conn *conn;
	uint16_t handle, min, max, latency, timeout;

	handle = sys_le16_to_cpu(evt->handle);
	min = sys_le16_to_cpu(evt->interval_min);
	max = sys_le16_to_cpu(evt->interval_max);
	latency = sys_le16_to_cpu(evt->latency);
	timeout = sys_le16_to_cpu(evt->timeout);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return le_conn_param_neg_reply(handle,
					       BT_HCI_ERR_UNKNOWN_CONN_ID);
	}

	bt_conn_unref(conn);

	if (!bt_le_conn_params_valid(min, max, latency, timeout)) {
		return le_conn_param_neg_reply(handle,
					       BT_HCI_ERR_INVALID_LL_PARAMS);
	}

	return le_conn_param_req_reply(handle, min, max, latency, timeout);
}

static void le_conn_update_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_update_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	uint16_t handle, interval;

	handle = sys_le16_to_cpu(evt->handle);
	interval = sys_le16_to_cpu(evt->interval);

	BT_DBG("status %u, handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	if (!evt->status) {
		conn->le.conn_interval = interval;
	}

	/* TODO Notify about connection */

	bt_conn_unref(conn);
}

static void check_pending_conn(const bt_addr_le_t *id_addr,
			       const bt_addr_le_t *addr, uint8_t evtype)
{
	struct bt_conn *conn;

	/* No connections are allowed during explicit scanning */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return;
	}

	/* Return if event is not connectable */
	if (evtype != BT_LE_ADV_IND && evtype != BT_LE_ADV_DIRECT_IND) {
		return;
	}

	conn = bt_conn_lookup_state(id_addr, BT_CONN_CONNECT_SCAN);
	if (!conn) {
		return;
	}

	if (bt_hci_stop_scanning()) {
		goto done;
	}

	if (hci_le_create_conn(addr)) {
		goto done;
	}

	bt_conn_set_state(conn, BT_CONN_CONNECT);

done:
	bt_conn_unref(conn);
}

static int set_flow_control(void)
{
	struct bt_hci_cp_host_buffer_size *hbs;
	struct net_buf *buf;
	uint8_t *enable;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_HOST_BUFFER_SIZE,
				sizeof(*hbs));
	if (!buf) {
		return -ENOBUFS;
	}

	hbs = net_buf_add(buf, sizeof(*hbs));
	memset(hbs, 0, sizeof(*hbs));
	hbs->acl_mtu = sys_cpu_to_le16(CONFIG_BLUETOOTH_L2CAP_IN_MTU +
				       sizeof(struct bt_l2cap_hdr));
	hbs->acl_pkts = sys_cpu_to_le16(CONFIG_BLUETOOTH_ACL_IN_COUNT);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_HOST_BUFFER_SIZE, buf, NULL);
	if (err) {
		return err;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_CTL_TO_HOST_FLOW, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	enable = net_buf_add(buf, sizeof(*enable));
	*enable = 0x01;
	return bt_hci_cmd_send_sync(BT_HCI_OP_SET_CTL_TO_HOST_FLOW, buf, NULL);
}
#endif /* CONFIG_BLUETOOTH_CONN */

#if defined(CONFIG_BLUETOOTH_SMP)
static void update_sec_level(struct bt_conn *conn)
{
	if (!conn->encrypt) {
		conn->sec_level = BT_SECURITY_LOW;
		return;
	}

	if (conn->keys && conn->keys->type == BT_KEYS_AUTHENTICATED) {
		if (conn->keys->keys & BT_KEYS_LTK_P256) {
			conn->sec_level = BT_SECURITY_FIPS;
		} else {
			conn->sec_level = BT_SECURITY_HIGH;
		}
	} else {
		conn->sec_level = BT_SECURITY_MEDIUM;
	}

	if (conn->required_sec_level > conn->sec_level) {
		BT_ERR("Failed to set required security level");
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
	}
}

static void hci_encrypt_change(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_change *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u encrypt 0x%02x", evt->status, handle,
	       evt->encrypt);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	if (evt->status) {
		/* TODO report error */
		/* reset required security level in case of error */
		conn->required_sec_level = conn->sec_level;
		bt_conn_unref(conn);
		return;
	}

	conn->encrypt = evt->encrypt;

	/*
	 * we update keys properties only on successful encryption to avoid
	 * losing valid keys if encryption was not successful
	 *
	 * Update keys with last pairing info for proper sec level update.
	 */
	if (conn->encrypt) {
		bt_smp_update_keys(conn);
	}

	update_sec_level(conn);
	bt_l2cap_encrypt_change(conn);
	bt_conn_security_changed(conn);

	bt_conn_unref(conn);
}

static void hci_encrypt_key_refresh_complete(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	uint16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status %u handle %u", evt->status, handle);

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	update_sec_level(conn);
	bt_l2cap_encrypt_change(conn);
	bt_conn_security_changed(conn);
	bt_conn_unref(conn);
}

static void le_ltk_request(struct net_buf *buf)
{
	struct bt_hci_evt_le_ltk_request *evt = (void *)buf->data;
	struct bt_conn *conn;
	uint16_t handle;
	uint8_t tk[16];

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("handle %u", handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	/*
	 * if TK is present use it, that means pairing is in progress and
	 * we should use new TK for encryption
	 *
	 * Both legacy STK and LE SC LTK have rand and ediv equal to zero.
	 */
	if (evt->rand == 0 && evt->ediv == 0 && bt_smp_get_tk(conn, tk)) {
		struct bt_hci_cp_le_ltk_req_reply *cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_REPLY,
					sizeof(*cp));
		if (!buf) {
			BT_ERR("Out of command buffers");
			goto done;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->handle = evt->handle;
		memcpy(cp->ltk, tk, sizeof(cp->ltk));

		bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_REPLY, buf);
		goto done;
	}

	if (!conn->keys) {
		conn->keys = bt_keys_find(BT_KEYS_LTK_P256, &conn->le.dst);
		if (!conn->keys) {
			conn->keys = bt_keys_find(BT_KEYS_SLAVE_LTK,
						  &conn->le.dst);
		}
	}

	if (conn->keys && (conn->keys->keys & BT_KEYS_LTK_P256) &&
	    evt->rand == 0 && evt->ediv == 0) {
		struct bt_hci_cp_le_ltk_req_reply *cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_REPLY,
					sizeof(*cp));
		if (!buf) {
			BT_ERR("Out of command buffers");
			goto done;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->handle = evt->handle;

		/* use only enc_size bytes of key for encryption */
		memcpy(cp->ltk, conn->keys->ltk.val, conn->keys->enc_size);
		if (conn->keys->enc_size < sizeof(cp->ltk)) {
			memset(cp->ltk + conn->keys->enc_size, 0,
			       sizeof(cp->ltk) - conn->keys->enc_size);
		}

		bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_REPLY, buf);
	} else if (conn->keys && (conn->keys->keys & BT_KEYS_SLAVE_LTK) &&
		   conn->keys->slave_ltk.rand == evt->rand &&
		   conn->keys->slave_ltk.ediv == evt->ediv) {
		struct bt_hci_cp_le_ltk_req_reply *cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_REPLY,
					sizeof(*cp));
		if (!buf) {
			BT_ERR("Out of command buffers");
			goto done;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->handle = evt->handle;

		/* use only enc_size bytes of key for encryption */
		memcpy(cp->ltk, conn->keys->slave_ltk.val,
		       conn->keys->enc_size);
		if (conn->keys->enc_size < sizeof(cp->ltk)) {
			memset(cp->ltk + conn->keys->enc_size, 0,
			       sizeof(cp->ltk) - conn->keys->enc_size);
		}

		bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_REPLY, buf);
	} else {
		struct bt_hci_cp_le_ltk_req_neg_reply *cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY,
					sizeof(*cp));
		if (!buf) {
			BT_ERR("Out of command buffers");
			goto done;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->handle = evt->handle;

		bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY, buf);
	}

done:
	bt_conn_unref(conn);
}

/* TODO check tinycrypt define when ECC is added */
#if !defined(CONFIG_TINYCRYPT_ECC)
static void le_pkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_p256_public_key_complete *evt = (void *)buf->data;

	BT_DBG("status: 0x%x", evt->status);

	if (evt->status) {
		return;
	}

	/* TODO
	 * init should be blocked until this is received or we need to notify
	 * SMP code about Public Key being available.
	 */

	memcpy(bt_dev.pkey, evt->key, sizeof(bt_dev.pkey));
}

static void le_dhkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_generate_dhkey_complete *evt = (void *)buf->data;

	BT_DBG("status: 0x%x", evt->status);

	if (evt->status) {
		bt_smp_dhkey_ready(NULL);
		return;
	}

	bt_smp_dhkey_ready(evt->dhkey);
}
#endif
#endif /* CONFIG_BLUETOOTH_SMP */

static void hci_reset_complete(struct net_buf *buf)
{
	uint8_t status = buf->data[0];

	BT_DBG("status %u", status);

	if (status) {
		return;
	}

	scan_dev_found_cb = NULL;
	atomic_set(bt_dev.flags, 0);
}

static void hci_cmd_done(uint16_t opcode, uint8_t status, struct net_buf *buf)
{
	struct net_buf *sent = bt_dev.sent_cmd;

	if (!sent) {
		return;
	}

	if (cmd(sent)->opcode != opcode) {
		BT_ERR("Unexpected completion of opcode 0x%04x", opcode);
		return;
	}

	bt_dev.sent_cmd = NULL;

	/* If the command was synchronous wake up bt_hci_cmd_send_sync() */
	if (cmd(sent)->sync) {
		struct nano_sem *sem = cmd(sent)->sync;

		if (status) {
			cmd(sent)->sync = NULL;
		} else {
			cmd(sent)->sync = net_buf_ref(buf);
		}

		nano_fiber_sem_give(sem);
	} else {
		net_buf_unref(sent);
	}
}

static void hci_cmd_complete(struct net_buf *buf)
{
	struct hci_evt_cmd_complete *evt = (void *)buf->data;
	uint16_t opcode = sys_le16_to_cpu(evt->opcode);
	uint8_t *status;

	BT_DBG("opcode %x", opcode);

	net_buf_pull(buf, sizeof(*evt));

	/* All command return parameters have a 1-byte status in the
	 * beginning, so we can safely make this generalization.
	 */
	status = buf->data;

	switch (opcode) {
	case BT_HCI_OP_RESET:
		hci_reset_complete(buf);
		break;
	default:
		BT_DBG("Unhandled opcode %x", opcode);
		break;
	}

	hci_cmd_done(opcode, *status, buf);

	if (evt->ncmd && !bt_dev.ncmd) {
		/* Allow next command to be sent */
		bt_dev.ncmd = 1;
		nano_fiber_sem_give(&bt_dev.ncmd_sem);
	}
}

static void hci_cmd_status(struct net_buf *buf)
{
	struct bt_hci_evt_cmd_status *evt = (void *)buf->data;
	uint16_t opcode = sys_le16_to_cpu(evt->opcode);

	BT_DBG("opcode %x", opcode);

	net_buf_pull(buf, sizeof(*evt));

	switch (opcode) {
	default:
		BT_DBG("Unhandled opcode %x", opcode);
		break;
	}

	hci_cmd_done(opcode, evt->status, buf);

	if (evt->ncmd && !bt_dev.ncmd) {
		/* Allow next command to be sent */
		bt_dev.ncmd = 1;
		nano_fiber_sem_give(&bt_dev.ncmd_sem);
	}
}

static int start_le_scan(uint8_t scan_type, uint16_t interval, uint16_t window,
			 uint8_t filter_dup)
{
	struct net_buf *buf, *rsp;
	struct bt_hci_cp_le_set_scan_params *set_param;
	struct bt_hci_cp_le_set_scan_enable *scan_enable;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_PARAMS,
				sizeof(*set_param));
	if (!buf) {
		return -ENOBUFS;
	}

	set_param = net_buf_add(buf, sizeof(*set_param));
	memset(set_param, 0, sizeof(*set_param));
	set_param->scan_type = scan_type;

	/* for the rest parameters apply default values according to
	 *  spec 4.2, vol2, part E, 7.8.10
	 */
	set_param->interval = sys_cpu_to_le16(interval);
	set_param->window = sys_cpu_to_le16(window);
	set_param->filter_policy = 0x00;
	set_param->addr_type = 0x00;

	bt_hci_cmd_send(BT_HCI_OP_LE_SET_SCAN_PARAMS, buf);
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_ENABLE,
				sizeof(*scan_enable));
	if (!buf) {
		return -ENOBUFS;
	}

	scan_enable = net_buf_add(buf, sizeof(*scan_enable));
	memset(scan_enable, 0, sizeof(*scan_enable));
	scan_enable->filter_dup = filter_dup;
	scan_enable->enable = 0x01;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_SCAN_ENABLE, buf, &rsp);
	if (err) {
		return err;
	}

	/* Update scan state in case of success (0) status */
	err = rsp->data[0];
	if (!err) {
		atomic_set_bit(bt_dev.flags, BT_DEV_SCANNING);
	}

	net_buf_unref(rsp);

	return err;
}

int bt_le_scan_update(void)
{
	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		int err;

		if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
			return 0;
		}

		err = bt_hci_stop_scanning();
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BLUETOOTH_CONN)
	if (!atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		struct bt_conn *conn;

		conn = bt_conn_lookup_state(BT_ADDR_LE_ANY,
					    BT_CONN_CONNECT_SCAN);
		if (!conn) {
			return 0;
		}

		bt_conn_unref(conn);

		return start_le_scan(BT_HCI_LE_SCAN_PASSIVE, 0x0060, 0x0030,
				     0x01);
	}
#endif /* CONFIG_BLUETOOTH_CONN */

	return 0;
}

static void le_adv_report(struct net_buf *buf)
{
	uint8_t num_reports = buf->data[0];
	struct bt_hci_ev_le_advertising_info *info;

	BT_DBG("Adv number of reports %u",  num_reports);

	info = net_buf_pull(buf, sizeof(num_reports));

	while (num_reports--) {
		int8_t rssi = info->data[info->length];
		const bt_addr_le_t *addr;

		BT_DBG("%s event %u, len %u, rssi %d dBm",
		       bt_addr_le_str(&info->addr),
		       info->evt_type, info->length, rssi);

		addr = find_id_addr(&info->addr);

		if (scan_dev_found_cb) {
			scan_dev_found_cb(addr, rssi, info->evt_type,
					  info->data, info->length);
		}

#if defined(CONFIG_BLUETOOTH_CONN)
		check_pending_conn(addr, &info->addr, info->evt_type);
#endif /* CONFIG_BLUETOOTH_CONN */
		/* Get next report iteration by moving pointer to right offset
		 * in buf according to spec 4.2, Vol 2, Part E, 7.7.65.2.
		 */
		info = net_buf_pull(buf, sizeof(*info) + info->length +
				    sizeof(rssi));
	}
}

static void hci_le_meta_event(struct net_buf *buf)
{
	struct bt_hci_evt_le_meta_event *evt = (void *)buf->data;

	net_buf_pull(buf, sizeof(*evt));

	switch (evt->subevent) {
#if defined(CONFIG_BLUETOOTH_CONN)
	case BT_HCI_EVT_LE_CONN_COMPLETE:
		le_conn_complete(buf);
		break;
	case BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE:
		le_conn_update_complete(buf);
		break;
	case BT_HCI_EV_LE_REMOTE_FEAT_COMPLETE:
		le_remote_feat_complete(buf);
		break;
	case BT_HCI_EVT_LE_CONN_PARAM_REQ:
		le_conn_param_req(buf);
		break;
#endif /* CONFIG_BLUETOOTH_CONN */
#if defined(CONFIG_BLUETOOTH_SMP)
	case BT_HCI_EVT_LE_LTK_REQUEST:
		le_ltk_request(buf);
		break;
/* TODO check tinycrypt define when ECC is added */
#if !defined(CONFIG_TINYCRYPT_ECC)
	case BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE:
		le_pkey_complete(buf);
		break;
	case BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE:
		le_dhkey_complete(buf);
		break;
#endif
#endif /* CONFIG_BLUETOOTH_SMP */
	case BT_HCI_EVT_LE_ADVERTISING_REPORT:
		le_adv_report(buf);
		break;
	default:
		BT_DBG("Unhandled LE event %x", evt->subevent);
		break;
	}
}

static void hci_event(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr = (void *)buf->data;

	BT_DBG("event %u", hdr->evt);

	net_buf_pull(buf, sizeof(*hdr));

	switch (hdr->evt) {
#if defined(CONFIG_BLUETOOTH_CONN)
	case BT_HCI_EVT_DISCONN_COMPLETE:
		hci_disconn_complete(buf);
		break;
#endif /* CONFIG_BLUETOOTH_CONN */
#if defined(CONFIG_BLUETOOTH_SMP)
	case BT_HCI_EVT_ENCRYPT_CHANGE:
		hci_encrypt_change(buf);
		break;
	case BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE:
		hci_encrypt_key_refresh_complete(buf);
		break;
#endif /* CONFIG_BLUETOOTH_SMP */
	case BT_HCI_EVT_LE_META_EVENT:
		hci_le_meta_event(buf);
		break;
	default:
		BT_WARN("Unhandled event 0x%02x", hdr->evt);
		break;

	}

	net_buf_unref(buf);
}

static void hci_cmd_tx_fiber(void)
{
	struct bt_driver *drv = bt_dev.drv;

	BT_DBG("started");

	while (1) {
		struct net_buf *buf;
		int err;

		/* Wait until ncmd > 0 */
		BT_DBG("calling sem_take_wait");
		nano_fiber_sem_take_wait(&bt_dev.ncmd_sem);

		/* Get next command - wait if necessary */
		BT_DBG("calling fifo_get_wait");
		buf = nano_fifo_get_wait(&bt_dev.cmd_tx_queue);
		bt_dev.ncmd = 0;

		/* Clear out any existing sent command */
		if (bt_dev.sent_cmd) {
			BT_ERR("Uncleared pending sent_cmd");
			net_buf_unref(bt_dev.sent_cmd);
			bt_dev.sent_cmd = NULL;
		}

		bt_dev.sent_cmd = net_buf_ref(buf);

		BT_DBG("Sending command %x (buf %p) to driver",
		       cmd(buf)->opcode, buf);

		err = drv->send(BT_CMD, buf);
		if (err) {
			BT_ERR("Unable to send to driver (err %d)", err);
			nano_fiber_sem_give(&bt_dev.ncmd_sem);
			net_buf_unref(bt_dev.sent_cmd);
			bt_dev.sent_cmd = NULL;
			net_buf_unref(buf);
		}
	}
}

static void rx_prio_fiber(void)
{
	struct net_buf *buf;

	BT_DBG("started");

	while (1) {
		struct bt_hci_evt_hdr *hdr;

		BT_DBG("calling fifo_get_wait");
		buf = nano_fifo_get_wait(&bt_dev.rx_prio_queue);

		BT_DBG("buf %p type %u len %u", buf, bt_type(buf), buf->len);

		if (bt_type(buf) != BT_EVT) {
			BT_ERR("Unknown buf type %u", bt_type(buf));
			net_buf_unref(buf);
			continue;
		}

		hdr = (void *)buf->data;
		net_buf_pull(buf, sizeof(*hdr));

		switch (hdr->evt) {
		case BT_HCI_EVT_CMD_COMPLETE:
			hci_cmd_complete(buf);
			break;
		case BT_HCI_EVT_CMD_STATUS:
			hci_cmd_status(buf);
			break;
#if defined(CONFIG_BLUETOOTH_CONN)
		case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
			hci_num_completed_packets(buf);
			break;
#endif /* CONFIG_BLUETOOTH_CONN */
		default:
			BT_ERR("Unknown event 0x%02x", hdr->evt);
			break;
		}

		net_buf_unref(buf);
	}
}

static void read_local_features_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_local_features *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	memcpy(bt_dev.features, rp->features, sizeof(bt_dev.features));
}

static void read_local_ver_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_local_version_info *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	bt_dev.hci_version = rp->hci_version;
	bt_dev.hci_revision = sys_le16_to_cpu(rp->hci_revision);
	bt_dev.manufacturer = sys_le16_to_cpu(rp->manufacturer);
}

static void read_bdaddr_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_bd_addr *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	bt_addr_copy(&bt_dev.bdaddr, &rp->bdaddr);
}

static void read_le_features_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_local_features *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	memcpy(bt_dev.le.features, rp->features, sizeof(bt_dev.le.features));
}

static void init_sem(struct nano_sem *sem, size_t count)
{
	/* Initialize & prime the semaphore for counting controller-side
	 * available ACL packet buffers.
	 */
	nano_sem_init(sem);
	while (count--) {
		nano_sem_give(sem);
	};
}

#if defined(CONFIG_BLUETOOTH_BREDR)
static void read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;
	uint16_t pkts;

	BT_DBG("status %u", rp->status);

	bt_dev.br.mtu = sys_le16_to_cpu(rp->acl_max_len);
	pkts = sys_le16_to_cpu(rp->acl_max_num);

	BT_DBG("ACL BR/EDR buffers: pkts %u mtu %u", pkts, bt_dev.br.mtu);

	init_sem(&bt_dev.br.pkts, pkts);
}
#else
static void read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;
	uint16_t pkts;

	BT_DBG("status %u", rp->status);

	/* If LE-side has buffers we can ignore the BR/EDR values */
	if (bt_dev.le.mtu) {
		return;
	}

	bt_dev.le.mtu = sys_le16_to_cpu(rp->acl_max_len);
	pkts = sys_le16_to_cpu(rp->acl_max_num);

	BT_DBG("ACL BR/EDR buffers: pkts %u mtu %u", pkts, bt_dev.le.mtu);

	init_sem(&bt_dev.le.pkts, pkts);
}
#endif

static void le_read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_buffer_size *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	bt_dev.le.mtu = sys_le16_to_cpu(rp->le_max_len);

	if (bt_dev.le.mtu) {
		init_sem(&bt_dev.le.pkts, rp->le_max_num);
		BT_DBG("ACL LE buffers: pkts %u mtu %u", rp->le_max_num,
		       bt_dev.le.mtu);
	}
}

static void read_supported_commands_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_supported_commands *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	memcpy(bt_dev.supported_commands, rp->commands,
	       sizeof(bt_dev.supported_commands));
}

static int common_init(void)
{
	struct net_buf *rsp;
	int err;

	/* Send HCI_RESET */
	err = bt_hci_cmd_send(BT_HCI_OP_RESET, NULL);
	if (err) {
		return err;
	}

	/* Read Local Supported Features */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_FEATURES, NULL, &rsp);
	if (err) {
		return err;
	}
	read_local_features_complete(rsp);
	net_buf_unref(rsp);

	/* Read Local Version Information */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL,
				   &rsp);
	if (err) {
		return err;
	}
	read_local_ver_complete(rsp);
	net_buf_unref(rsp);

	/* Read Bluetooth Address */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR, NULL, &rsp);
	if (err) {
		return err;
	}
	read_bdaddr_complete(rsp);
	net_buf_unref(rsp);

#if defined(CONFIG_BLUETOOTH_CONN)
	err = set_flow_control();
	if (err) {
		return err;
	}
#endif /* CONFIG_BLUETOOTH_CONN */

	return 0;
}

static int le_init(void)
{
	struct bt_hci_cp_write_le_host_supp *cp_le;
	struct bt_hci_cp_le_set_event_mask *cp_mask;
	struct net_buf *buf;
	struct net_buf *rsp;
	int err;

	/* For now we only support LE capable controllers */
	if (!lmp_le_capable(bt_dev)) {
		BT_ERR("Non-LE capable controller detected!");
		return -ENODEV;
	}

	/* Read Low Energy Supported Features */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_LOCAL_FEATURES, NULL,
				   &rsp);
	if (err) {
		return err;
	}
	read_le_features_complete(rsp);
	net_buf_unref(rsp);

	/* Read Local Supported Commands */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_SUPPORTED_COMMANDS, NULL,
				   &rsp);
	if (err) {
		return err;
	}
	read_supported_commands_complete(rsp);
	net_buf_unref(rsp);

	/* Read LE Buffer Size */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_BUFFER_SIZE, NULL, &rsp);
	if (err) {
		return err;
	}
	le_read_buffer_size_complete(rsp);
	net_buf_unref(rsp);

	if (lmp_bredr_capable(bt_dev)) {
		buf = bt_hci_cmd_create(BT_HCI_OP_LE_WRITE_LE_HOST_SUPP,
					sizeof(*cp_le));
		if (!buf) {
			return -ENOBUFS;
		}

		cp_le = net_buf_add(buf, sizeof(*cp_le));

		/* Excplicitly enable LE for dual-mode controllers */
		cp_le->le = 0x01;
		cp_le->simul = 0x00;
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_WRITE_LE_HOST_SUPP, buf,
					   NULL);
		if (err) {
			return err;
		}
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_EVENT_MASK, sizeof(*cp_mask));
	if (!buf) {
		return -ENOBUFS;
	}

	cp_mask = net_buf_add(buf, sizeof(*cp_mask));
	memset(cp_mask, 0, sizeof(*cp_mask));

	cp_mask->events[0] |= 0x02; /* LE Advertising Report Event */

#if defined(CONFIG_BLUETOOTH_CONN)
	cp_mask->events[0] |= 0x01; /* LE Connection Complete Event */
	cp_mask->events[0] |= 0x04; /* LE Connection Update Complete Event */
	cp_mask->events[0] |= 0x08; /* LE Read Remote Used Features Compl Evt */
#endif /* CONFIG_BLUETOOTH_CONN */

#if defined(CONFIG_BLUETOOTH_SMP)
	cp_mask->events[0] |= 0x10; /* LE Long Term Key Request Event */

/* TODO check tinycrypt define when ECC is added */
#if !defined(CONFIG_TINYCRYPT_ECC)
	/*
	 * If controller based ECC is to be used and
	 * "LE Read Local P-256 Public Key" and "LE Generate DH Key" are
	 * supported we need to enable events generated by those commands.
	 */
	if ((bt_dev.supported_commands[34] & 0x02) &&
	    (bt_dev.supported_commands[34] & 0x04)) {
		cp_mask->events[0] |= 0x80; /* LE Read Local P-256 PKey Compl */
		cp_mask->events[1] |= 0x01; /* LE Generate DHKey Compl Event */
	}
#endif /* !CONFIG_TINYCRYPT_ECC */
#endif /* CONFIG_BLUETOOTH_SMP */

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EVENT_MASK, buf, NULL);
	if (err) {
		return err;
	}

/* TODO check tinycrypt define when ECC is added */
#if defined(CONFIG_BLUETOOTH_SMP) && !defined(CONFIG_TINYCRYPT_ECC)
	/*
	 * We check for both "LE Read Local P-256 Public Key" and
	 * "LE Generate DH Key" support here since both commands are needed for
	 * LE SC support. If "LE Generate DH Key" is not supported then there
	 * is no point in reading local public key.
	 */
	if ((bt_dev.supported_commands[34] & 0x02) &&
	    (bt_dev.supported_commands[34] & 0x04)) {
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_P256_PUBLIC_KEY, NULL,
					   NULL);
		if (err) {
			return err;
		}
	}
#endif
	return 0;
}

#if defined(CONFIG_BLUETOOTH_BREDR)
static int br_init(void)
{
	struct net_buf *rsp;
	int err;

	/* Get BR/EDR buffer size */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BUFFER_SIZE, NULL, &rsp);
	if (err) {
		return err;
	}

	read_buffer_size_complete(rsp);
	net_buf_unref(rsp);

	return 0;
}
#else
static int br_init(void)
{
	struct net_buf *rsp;
	int err;

	if (bt_dev.le.mtu) {
		return 0;
	}

	/* Use BR/EDR buffer size if LE reports zero buffers */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BUFFER_SIZE, NULL, &rsp);
	if (err) {
		return err;
	}

	read_buffer_size_complete(rsp);
	net_buf_unref(rsp);

	return 0;
}
#endif

static int set_event_mask(void)
{
	struct bt_hci_cp_set_event_mask *ev;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_EVENT_MASK, sizeof(*ev));
	if (!buf) {
		return -ENOBUFS;
	}

	ev = net_buf_add(buf, sizeof(*ev));
	memset(ev, 0, sizeof(*ev));

	ev->events[1] |= 0x20; /* Command Complete */
	ev->events[1] |= 0x40; /* Command Status */
	ev->events[1] |= 0x80; /* Hardware Error */
	ev->events[3] |= 0x02; /* Data Buffer Overflow */
	ev->events[7] |= 0x20; /* LE Meta-Event */

#if defined(CONFIG_BLUETOOTH_CONN)
	ev->events[0] |= 0x10; /* Disconnection Complete */
	ev->events[1] |= 0x08; /* Read Remote Version Information Complete */
	ev->events[2] |= 0x04; /* Number of Completed Packets */
#endif /* CONFIG_BLUETOOTH_CONN */

#if defined(CONFIG_BLUETOOTH_SMP)
	if (bt_dev.le.features[0] & BT_HCI_LE_ENCRYPTION) {
		ev->events[0] |= 0x80; /* Encryption Change */
		ev->events[5] |= 0x80; /* Encryption Key Refresh Complete */
	}
#endif /* CONFIG_BLUETOOTH_SMP */

	return bt_hci_cmd_send_sync(BT_HCI_OP_SET_EVENT_MASK, buf, NULL);
}

static int hci_init(void)
{
	int err;

	err = common_init();
	if (err) {
		return err;
	}

	err = le_init();
	if (err) {
		return err;
	}

	if (lmp_bredr_capable(bt_dev)) {
		err = br_init();
		if (err) {
			return err;
		}
	} else {
		BT_DBG("Non-BR/EDR controller detected! Skipping BR init.");
	}

	err = set_event_mask();
	if (err) {
		return err;
	}

	BT_DBG("HCI ver %u rev %u, manufacturer %u", bt_dev.hci_version,
	       bt_dev.hci_revision, bt_dev.manufacturer);

	return 0;
}

/* Interface to HCI driver layer */

void bt_recv(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr;

	BT_DBG("buf %p len %u", buf, buf->len);

	if (bt_type(buf) == BT_ACL_IN) {
		nano_fifo_put(&bt_dev.rx_queue, buf);
		return;
	}

	if (bt_type(buf) != BT_EVT) {
		BT_ERR("Invalid buf type %u", bt_type(buf));
		net_buf_unref(buf);
		return;
	}

	/* Command Complete/Status events have their own cmd_rx queue,
	 * all other events go through rx queue.
	 */
	hdr = (void *)buf->data;
	if (hdr->evt == BT_HCI_EVT_CMD_COMPLETE ||
	    hdr->evt == BT_HCI_EVT_CMD_STATUS ||
	    hdr->evt == BT_HCI_EVT_NUM_COMPLETED_PACKETS) {
		nano_fifo_put(&bt_dev.rx_prio_queue, buf);
		return;
	}

	nano_fifo_put(&bt_dev.rx_queue, buf);
}

int bt_driver_register(struct bt_driver *drv)
{
	if (bt_dev.drv) {
		return -EALREADY;
	}

	if (!drv->open || !drv->send) {
		return -EINVAL;
	}

	bt_dev.drv = drv;

	return 0;
}

void bt_driver_unregister(struct bt_driver *drv)
{
	bt_dev.drv = NULL;
}

static int bt_init(void)
{
	struct bt_driver *drv = bt_dev.drv;
	int err;

	err = drv->open();
	if (err) {
		BT_ERR("HCI driver open failed (%d)", err);
		return err;
	}

	err = hci_init();

#if defined(CONFIG_BLUETOOTH_CONN)
	if (!err) {
		err = bt_conn_init();
	}
#endif /* CONFIG_BLUETOOTH_CONN */

	return err;
}

static void hci_rx_fiber(bt_ready_cb_t ready_cb)
{
	struct net_buf *buf;

	BT_DBG("started");

	if (ready_cb) {
		ready_cb(bt_init());
	}

	while (1) {
		BT_DBG("calling fifo_get_wait");
		buf = nano_fifo_get_wait(&bt_dev.rx_queue);

		BT_DBG("buf %p type %u len %u", buf, bt_type(buf), buf->len);

		switch (bt_type(buf)) {
#if defined(CONFIG_BLUETOOTH_CONN)
		case BT_ACL_IN:
			hci_acl(buf);
			break;
#endif /* CONFIG_BLUETOOTH_CONN */
		case BT_EVT:
			hci_event(buf);
			break;
		default:
			BT_ERR("Unknown buf type %u", bt_type(buf));
			net_buf_unref(buf);
			break;
		}

	}
}

int bt_enable(bt_ready_cb_t cb)
{
	if (!bt_dev.drv) {
		BT_ERR("No HCI driver registered");
		return -ENODEV;
	}

	/* Initialize the buffer pools */
	net_buf_pool_init(hci_cmd_pool);
	net_buf_pool_init(hci_evt_pool);
#if defined(CONFIG_BLUETOOTH_CONN)
	net_buf_pool_init(acl_in_pool);
#endif /* CONFIG_BLUETOOTH_CONN */

	/* Give cmd_sem allowing to send first HCI_Reset cmd */
	bt_dev.ncmd = 1;
	nano_sem_init(&bt_dev.ncmd_sem);
	nano_task_sem_give(&bt_dev.ncmd_sem);

	/* TX fiber */
	nano_fifo_init(&bt_dev.cmd_tx_queue);
	fiber_start(cmd_tx_fiber_stack, sizeof(cmd_tx_fiber_stack),
		    (nano_fiber_entry_t)hci_cmd_tx_fiber, 0, 0, 7, 0);

	/* RX prio fiber */
	nano_fifo_init(&bt_dev.rx_prio_queue);
	fiber_start(rx_prio_fiber_stack, sizeof(rx_prio_fiber_stack),
		    (nano_fiber_entry_t)rx_prio_fiber, 0, 0, 7, 0);

	/* RX fiber */
	nano_fifo_init(&bt_dev.rx_queue);
	fiber_start(rx_fiber_stack, sizeof(rx_fiber_stack),
		    (nano_fiber_entry_t)hci_rx_fiber, (int)cb, 0, 7, 0);

	if (!cb) {
		return bt_init();
	}

	return 0;
}

int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_eir *ad, const struct bt_eir *sd)
{
	struct net_buf *buf;
	struct bt_hci_cp_le_set_adv_data *set_data;
	struct bt_hci_cp_le_set_adv_data *scan_rsp;
	struct bt_hci_cp_le_set_adv_parameters *set_param;
	uint8_t adv_enable;
	int i, err;

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return -EALREADY;
	}

	if (!ad) {
		goto send_scan_rsp;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_DATA, sizeof(*set_data));
	if (!buf) {
		return -ENOBUFS;
	}

	set_data = net_buf_add(buf, sizeof(*set_data));

	memset(set_data, 0, sizeof(*set_data));

	for (i = 0; ad[i].len; i++) {
		/* Check if ad fit in the remaining buffer */
		if (set_data->len + ad[i].len + 1 > 29) {
			break;
		}

		memcpy(&set_data->data[set_data->len], &ad[i], ad[i].len + 1);
		set_data->len += ad[i].len + 1;
	}

	bt_hci_cmd_send(BT_HCI_OP_LE_SET_ADV_DATA, buf);

send_scan_rsp:
	if (!sd) {
		goto send_set_param;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_RSP_DATA,
				sizeof(*scan_rsp));
	if (!buf) {
		return -ENOBUFS;
	}

	scan_rsp = net_buf_add(buf, sizeof(*scan_rsp));

	memset(scan_rsp, 0, sizeof(*scan_rsp));

	for (i = 0; sd[i].len; i++) {
		/* Check if ad fit in the remaining buffer */
		if (scan_rsp->len + sd[i].len + 1 > 29) {
			break;
		}

		memcpy(&scan_rsp->data[scan_rsp->len], &sd[i], sd[i].len + 1);
		scan_rsp->len += sd[i].len + 1;
	}

	bt_hci_cmd_send(BT_HCI_OP_LE_SET_SCAN_RSP_DATA, buf);

send_set_param:
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_PARAMETERS,
				sizeof(*set_param));
	if (!buf) {
		return -ENOBUFS;
	}

	set_param = net_buf_add(buf, sizeof(*set_param));

	memset(set_param, 0, sizeof(*set_param));
	set_param->min_interval		= sys_cpu_to_le16(0x0800);
	set_param->max_interval		= sys_cpu_to_le16(0x0800);
	set_param->type			= param->type;
	set_param->channel_map		= 0x07;

	bt_hci_cmd_send(BT_HCI_OP_LE_SET_ADV_PARAMETERS, buf);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	adv_enable = 0x01;
	memcpy(net_buf_add(buf, 1), &adv_enable, 1);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	atomic_set_bit(bt_dev.flags, BT_DEV_ADVERTISING);

	return 0;
}

int bt_le_adv_stop(void)
{
	struct net_buf *buf;
	uint8_t adv_enable;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return -EALREADY;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	adv_enable = 0x00;
	memcpy(net_buf_add(buf, 1), &adv_enable, 1);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_ADVERTISING);

	return 0;
}

static bool valid_le_scan_param(const struct bt_le_scan_param *param)
{
	if (param->type != BT_HCI_LE_SCAN_PASSIVE &&
	    param->type != BT_HCI_LE_SCAN_ACTIVE) {
		return false;
	}

	if (param->interval < 0x0004 || param->interval > 0x4000) {
		return false;
	}

	if (param->window < 0x0004 || param->window > 0x4000) {
		return false;
	}

	if (param->window > param->interval) {
		return false;
	}

	return true;
}

int bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb)
{
	int err;

	/* Check that the parameters have valid values */
	if (!valid_le_scan_param(param)) {
		return -EINVAL;
	}

	/* Return if active scan is already enabled */
	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return -EALREADY;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		err = bt_hci_stop_scanning();
		if (err) {
			atomic_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN);
			return err;
		}
	}

	err = start_le_scan(param->type, param->interval, param->window,
			    param->filter_dup);
	if (err) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN);
		return err;
	}

	scan_dev_found_cb = cb;

	return 0;
}

int bt_le_scan_stop(void)
{
	/* Return if active scanning is already disabled */
	if (!atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return -EALREADY;
	}

	scan_dev_found_cb = NULL;

	return bt_le_scan_update();
}

struct net_buf *bt_buf_get_evt(void)
{
	return net_buf_get(&avail_hci_evt, CONFIG_BLUETOOTH_HCI_RECV_RESERVE);
}

struct net_buf *bt_buf_get_acl(void)
{
#if defined(CONFIG_BLUETOOTH_CONN)
	return net_buf_get(&avail_acl_in, CONFIG_BLUETOOTH_HCI_RECV_RESERVE);
#else
	return NULL;
#endif /* CONFIG_BLUETOOTH_CONN */
}

#if defined(CONFIG_BLUETOOTH_BREDR)
static int write_scan_enable(uint8_t scan)
{
	struct net_buf *buf;
	int err;

	BT_DBG("type %u", scan);

	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_SCAN_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	memcpy(net_buf_add(buf, 1), &scan, 1);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_SCAN_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	if (scan & BT_BREDR_SCAN_INQUIRY) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ISCAN);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_ISCAN);
	}

	if (scan & BT_BREDR_SCAN_PAGE) {
		atomic_set_bit(bt_dev.flags, BT_DEV_PSCAN);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_PSCAN);
	}

	return 0;
}

int bt_br_set_connectable(bool enable)
{
	if (enable) {
		if (atomic_test_bit(bt_dev.flags, BT_DEV_PSCAN)) {
			return -EALREADY;
		} else {
			return write_scan_enable(BT_BREDR_SCAN_PAGE);
		}
	} else {
		if (!atomic_test_bit(bt_dev.flags, BT_DEV_PSCAN)) {
			return -EALREADY;
		} else {
			return write_scan_enable(BT_BREDR_SCAN_DISABLED);
		}
	}
}

int bt_br_set_discoverable(bool enable)
{
	if (enable) {
		if (atomic_test_bit(bt_dev.flags, BT_DEV_ISCAN)) {
			return -EALREADY;
		}

		if (!atomic_test_bit(bt_dev.flags, BT_DEV_PSCAN)) {
			return -EPERM;
		}

		return write_scan_enable(BT_BREDR_SCAN_INQUIRY |
					 BT_BREDR_SCAN_PAGE);
	} else {
		if (!atomic_test_bit(bt_dev.flags, BT_DEV_ISCAN)) {
			return -EALREADY;
		}

		return write_scan_enable(BT_BREDR_SCAN_PAGE);
	}
}
#endif
