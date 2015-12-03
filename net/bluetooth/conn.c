/* conn.c - Bluetooth connection handling */

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
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/driver.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "keys.h"
#include "smp.h"
#include "att.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_CONN)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

/* Pool for outgoing ACL fragments */
static struct nano_fifo frag_buf;
static NET_BUF_POOL(frag_pool, 1, BT_L2CAP_BUF_SIZE(23), &frag_buf, NULL, 0);

/* Pool for dummy buffers to wake up the tx fibers */
static struct nano_fifo dummy;
static NET_BUF_POOL(dummy_pool, CONFIG_BLUETOOTH_MAX_CONN, 0, &dummy, NULL, 0);

/* How long until we cancel HCI_LE_Create_Connection */
#define CONN_TIMEOUT	(3 * sys_clock_ticks_per_sec)

static struct bt_conn conns[CONFIG_BLUETOOTH_MAX_CONN];
static struct bt_conn_cb *callback_list;

#if defined(CONFIG_BLUETOOTH_DEBUG_CONN)
static const char *state2str(bt_conn_state_t state)
{
	switch (state) {
	case BT_CONN_DISCONNECTED:
		return "disconnected";
	case BT_CONN_CONNECT_SCAN:
		return "connect-scan";
	case BT_CONN_CONNECT:
		return "connect";
	case BT_CONN_CONNECTED:
		return "connected";
	case BT_CONN_DISCONNECT:
		return "disconnect";
	default:
		return "(unknown)";
	}
}
#endif

static void notify_connected(struct bt_conn *conn)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->connected) {
			cb->connected(conn);
		}
	}
}

static void notify_disconnected(struct bt_conn *conn)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->disconnected) {
			cb->disconnected(conn);
		}
	}
}

#if defined(CONFIG_BLUETOOTH_SMP)
void bt_conn_identity_resolved(struct bt_conn *conn)
{
	const bt_addr_le_t *rpa;
	struct bt_conn_cb *cb;

	if (conn->role == BT_HCI_ROLE_MASTER) {
		rpa = &conn->le.resp_addr;
	} else {
		rpa = &conn->le.init_addr;
	}

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->identity_resolved) {
			cb->identity_resolved(conn, rpa, &conn->le.dst);
		}
	}
}

void bt_conn_security_changed(struct bt_conn *conn)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->security_changed) {
			cb->security_changed(conn, conn->sec_level);
		}
	}
}

int bt_conn_le_start_encryption(struct bt_conn *conn, uint64_t rand,
				uint16_t ediv, const uint8_t *ltk, size_t len)
{
	struct bt_hci_cp_le_start_encryption *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_START_ENCRYPTION, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->rand = rand;
	cp->ediv = ediv;

	memcpy(cp->ltk, ltk, len);
	if (len < sizeof(cp->ltk)) {
		memset(cp->ltk + len, 0, sizeof(cp->ltk) - len);
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_START_ENCRYPTION, buf, NULL);
}

static int start_security(struct bt_conn *conn)
{
	switch (conn->role) {
#if defined(CONFIG_BLUETOOTH_CENTRAL)
	case BT_HCI_ROLE_MASTER:
	{
		if (!conn->keys) {
			conn->keys = bt_keys_find(BT_KEYS_LTK_P256,
						  &conn->le.dst);
			if (!conn->keys) {
				conn->keys = bt_keys_find(BT_KEYS_LTK,
							  &conn->le.dst);
			}
		}

		if (!conn->keys ||
		    !(conn->keys->keys & (BT_KEYS_LTK | BT_KEYS_LTK_P256))) {
			return bt_smp_send_pairing_req(conn);
		}

		if (conn->required_sec_level > BT_SECURITY_MEDIUM &&
		    conn->keys->type != BT_KEYS_AUTHENTICATED) {
			return bt_smp_send_pairing_req(conn);
		}

		if (conn->required_sec_level > BT_SECURITY_HIGH &&
		    conn->keys->type != BT_KEYS_AUTHENTICATED &&
		    !(conn->keys->keys & BT_KEYS_LTK_P256)) {
			return bt_smp_send_pairing_req(conn);
		}

		/* LE SC LTK and legacy master LTK are stored in same place */
		return bt_conn_le_start_encryption(conn, conn->keys->ltk.rand,
						   conn->keys->ltk.ediv,
						   conn->keys->ltk.val,
						   conn->keys->enc_size);
	}
#endif /* CONFIG_BLUETOOTH_CENTRAL */
#if defined(CONFIG_BLUETOOTH_PERIPHERAL)
	case BT_HCI_ROLE_SLAVE:
		return bt_smp_send_security_req(conn);
#endif /* CONFIG_BLUETOOTH_PERIPHERAL */
	default:
		return -EINVAL;
	}
}

int bt_conn_security(struct bt_conn *conn, bt_security_t sec)
{
	int err;

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	/* nothing to do */
	if (conn->sec_level >= sec || conn->required_sec_level >= sec) {
		return 0;
	}

	conn->required_sec_level = sec;

	err = start_security(conn);

	/* reset required security level in case of error */
	if (err) {
		conn->required_sec_level = conn->sec_level;
	}

	return err;
}
#endif /* CONFIG_BLUETOOTH_SMP */

void bt_conn_cb_register(struct bt_conn_cb *cb)
{
	cb->_next = callback_list;
	callback_list = cb;
}

static void bt_conn_reset_rx_state(struct bt_conn *conn)
{
	if (!conn->rx_len) {
		return;
	}

	net_buf_unref(conn->rx);
	conn->rx = NULL;
	conn->rx_len = 0;
}

void bt_conn_recv(struct bt_conn *conn, struct net_buf *buf, uint8_t flags)
{
	struct bt_l2cap_hdr *hdr;
	uint16_t len;

	BT_DBG("handle %u len %u flags %02x", conn->handle, buf->len, flags);

	/* Check packet boundary flags */
	switch (flags) {
	case BT_ACL_START:
		hdr = (void *)buf->data;
		len = sys_le16_to_cpu(hdr->len);

		BT_DBG("First, len %u final %u", buf->len, len);

		if (conn->rx_len) {
			BT_ERR("Unexpected first L2CAP frame");
			bt_conn_reset_rx_state(conn);
		}

		conn->rx_len = (sizeof(*hdr) + len) - buf->len;
		BT_DBG("rx_len %u", conn->rx_len);
		if (conn->rx_len) {
			conn->rx = buf;
			return;
		}

		break;
	case BT_ACL_CONT:
		if (!conn->rx_len) {
			BT_ERR("Unexpected L2CAP continuation");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		if (buf->len > conn->rx_len) {
			BT_ERR("L2CAP data overflow");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		BT_DBG("Cont, len %u rx_len %u", buf->len, conn->rx_len);

		if (buf->len > net_buf_tailroom(conn->rx)) {
			BT_ERR("Not enough buffer space for L2CAP data");
			bt_conn_reset_rx_state(conn);
			net_buf_unref(buf);
			return;
		}

		memcpy(net_buf_add(conn->rx, buf->len), buf->data, buf->len);
		conn->rx_len -= buf->len;
		net_buf_unref(buf);

		if (conn->rx_len) {
			return;
		}

		buf = conn->rx;
		conn->rx = NULL;
		conn->rx_len = 0;

		break;
	default:
		BT_ERR("Unexpected ACL flags (0x%02x)", flags);
		bt_conn_reset_rx_state(conn);
		net_buf_unref(buf);
		return;
	}

	hdr = (void *)buf->data;
	len = sys_le16_to_cpu(hdr->len);

	if (sizeof(*hdr) + len != buf->len) {
		BT_ERR("ACL len mismatch (%u != %u)", len, buf->len);
		net_buf_unref(buf);
		return;
	}

	BT_DBG("Successfully parsed %u byte L2CAP packet", buf->len);

	bt_l2cap_recv(conn, buf);
}

void bt_conn_send(struct bt_conn *conn, struct net_buf *buf)
{
	BT_DBG("conn handle %u buf len %u", conn->handle, buf->len);

	if (conn->state != BT_CONN_CONNECTED) {
		BT_ERR("not connected!");
		net_buf_unref(buf);
		return;
	}

	nano_fifo_put(&conn->tx_queue, buf);
}

static bool send_frag(struct bt_conn *conn, struct net_buf *buf, uint8_t flags,
		      bool always_consume)
{
	struct bt_hci_acl_hdr *hdr;
	int err;

	BT_DBG("conn %p buf %p len %u flags 0x%02x", conn, buf, buf->len,
	       flags);

	/* Wait until the controller can accept ACL packets */
	nano_fiber_sem_take_wait(bt_conn_get_pkts(conn));

	/* Check for disconnection while waiting for pkts_sem */
	if (conn->state != BT_CONN_CONNECTED) {
		goto fail;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_acl_handle_pack(conn->handle, flags));
	hdr->len = sys_cpu_to_le16(buf->len - sizeof(*hdr));

	BT_DBG("passing buf %p len %u to driver", buf, buf->len);
	err = bt_dev.drv->send(BT_ACL_OUT, buf);
	if (err) {
		BT_ERR("Unable to send to driver (err %d)", err);
		goto fail;
	}

	conn->pending_pkts++;
	return true;

fail:
	nano_fiber_sem_give(bt_conn_get_pkts(conn));
	if (always_consume) {
		net_buf_unref(buf);
	}
	return false;
}

static inline uint16_t conn_mtu(struct bt_conn *conn)
{
#if defined(CONFIG_BLUETOOTH_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		return bt_dev.br.mtu;
	}
#endif /* CONFIG_BLUETOOTH_BREDR */

	return bt_dev.le.mtu;
}

static struct net_buf *create_frag(struct bt_conn *conn, struct net_buf *buf)
{
	struct net_buf *frag;

	frag = bt_conn_create_pdu(&frag_buf, 0);
	if (conn->state != BT_CONN_CONNECTED) {
		if (frag) {
			net_buf_unref(frag);
		}

		return NULL;
	}

	memcpy(net_buf_add(frag, conn_mtu(conn)), buf->data, conn_mtu(conn));
	net_buf_pull(buf, conn_mtu(conn));

	return frag;
}

static bool send_buf(struct bt_conn *conn, struct net_buf *buf)
{
	struct net_buf *frag;

	BT_DBG("conn %p buf %p len %u", conn, buf, buf->len);

	/* Send directly if the packet fits the ACL MTU */
	if (buf->len <= conn_mtu(conn)) {
		return send_frag(conn, buf, BT_ACL_START_NO_FLUSH, false);
	}

	/* Create & enqueue first fragment */
	frag = create_frag(conn, buf);
	if (!frag) {
		return false;
	}

	if (!send_frag(conn, frag, BT_ACL_START_NO_FLUSH, true)) {
		return false;
	}

	/*
	 * Send the fragments. For the last one simply use the original
	 * buffer (which works since we've used net_buf_pull on it.
	 */
	while (buf->len > conn_mtu(conn)) {
		frag = create_frag(conn, buf);
		if (!frag) {
			return false;
		}

		if (!send_frag(conn, frag, BT_ACL_CONT, true)) {
			return false;
		}
	}

	return send_frag(conn, buf, BT_ACL_CONT, false);
}

static void conn_tx_fiber(int arg1, int arg2)
{
	struct bt_conn *conn = (struct bt_conn *)arg1;
	struct net_buf *buf;

	BT_DBG("Started for handle %u", conn->handle);

	while (conn->state == BT_CONN_CONNECTED) {
		/* Get next ACL packet for connection */
		buf = nano_fifo_get_wait(&conn->tx_queue);
		if (conn->state != BT_CONN_CONNECTED) {
			net_buf_unref(buf);
			break;
		}

		if (!send_buf(conn, buf)) {
			net_buf_unref(buf);
		}
	}

	BT_DBG("handle %u disconnected - cleaning up", conn->handle);

	/* Give back any allocated buffers */
	while ((buf = nano_fifo_get(&conn->tx_queue))) {
		net_buf_unref(buf);
	}

	/* Return any unacknowledged packets */
	if (conn->pending_pkts) {
		while (conn->pending_pkts--) {
			nano_fiber_sem_give(bt_conn_get_pkts(conn));
		}
	}

	bt_conn_reset_rx_state(conn);

	BT_DBG("handle %u exiting", conn->handle);
	bt_conn_unref(conn);
}

static struct bt_conn *conn_new(void)
{
	struct bt_conn *conn = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(conns); i++) {
		if (!atomic_get(&conns[i].ref)) {
			conn = &conns[i];
			break;
		}
	}

	if (!conn) {
		return NULL;
	}

	memset(conn, 0, sizeof(*conn));

	atomic_set(&conn->ref, 1);

	return conn;
}

struct bt_conn *bt_conn_add_le(const bt_addr_le_t *peer)
{
	struct bt_conn *conn = conn_new();

	if (!conn) {
		return NULL;
	}

	bt_addr_le_copy(&conn->le.dst, peer);
#if defined(CONFIG_BLUETOOTH_SMP)
	conn->sec_level = BT_SECURITY_LOW;
	conn->required_sec_level = BT_SECURITY_LOW;
#endif /* CONFIG_BLUETOOTH_SMP */
	conn->type = BT_CONN_TYPE_LE;
	conn->le.interval_min = BT_GAP_INIT_CONN_INT_MIN;
	conn->le.interval_max = BT_GAP_INIT_CONN_INT_MAX;

	return conn;
}

#if defined(CONFIG_BLUETOOTH_BREDR)
struct bt_conn *bt_conn_lookup_addr_br(const bt_addr_t *peer)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(conns); i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		if (conns[i].type != BT_CONN_TYPE_BR) {
			continue;
		}

		if (!bt_addr_cmp(peer, &conns[i].br.dst)) {
			return bt_conn_ref(&conns[i]);
		}
	}

	return NULL;
}

struct bt_conn *bt_conn_add_br(const bt_addr_t *peer)
{
	struct bt_conn *conn = conn_new();

	if (!conn) {
		return NULL;
	}

	bt_addr_copy(&conn->br.dst, peer);
	conn->type = BT_CONN_TYPE_BR;

	return conn;
}
#endif

static void timeout_fiber(int arg1, int arg2)
{
	struct bt_conn *conn = (struct bt_conn *)arg1;
	ARG_UNUSED(arg2);

	conn->timeout = NULL;

	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	bt_conn_unref(conn);
}

void bt_conn_set_state(struct bt_conn *conn, bt_conn_state_t state)
{
	bt_conn_state_t old_state;

	BT_DBG("%s -> %s", state2str(conn->state), state2str(state));

	if (conn->state == state) {
		BT_WARN("no transition");
		return;
	}

	old_state = conn->state;
	conn->state = state;

	/* Actions needed for exiting the old state */
	switch (old_state) {
	case BT_CONN_DISCONNECTED:
		/* Take a reference for the first state transition after
		 * bt_conn_add_le() and keep it until reaching DISCONNECTED
		 * again.
		 */
		bt_conn_ref(conn);
		break;
	case BT_CONN_CONNECT:
		if (conn->timeout) {
			fiber_fiber_delayed_start_cancel(conn->timeout);
			conn->timeout = NULL;

			/* Drop the reference taken by timeout fiber */
			bt_conn_unref(conn);
		}
		break;
	default:
		break;
	}

	/* Actions needed for entering the new state */
	switch (conn->state){
	case BT_CONN_CONNECTED:
		nano_fifo_init(&conn->tx_queue);
		fiber_start(conn->stack, sizeof(conn->stack), conn_tx_fiber,
			    (int)bt_conn_ref(conn), 0, 7, 0);

		bt_l2cap_connected(conn);
		notify_connected(conn);
		break;
	case BT_CONN_DISCONNECTED:
		/* Notify disconnection and queue a dummy buffer to wake
		 * up and stop the tx fiber for states where it was
		 * running.
		 */
		if (old_state == BT_CONN_CONNECTED ||
		    old_state == BT_CONN_DISCONNECT) {
			bt_l2cap_disconnected(conn);
			notify_disconnected(conn);

			nano_fifo_put(&conn->tx_queue, net_buf_get(&dummy, 0));
		}

		/* Release the reference we took for the very first
		 * state transition.
		 */
		bt_conn_unref(conn);

		break;
	case BT_CONN_CONNECT_SCAN:
		break;
	case BT_CONN_CONNECT:
		/* Add LE Create Connection timeout */
		conn->timeout = fiber_delayed_start(conn->stack,
						    sizeof(conn->stack),
						    timeout_fiber,
						    (int)bt_conn_ref(conn),
						    0, 7, 0, CONN_TIMEOUT);
		break;
	case BT_CONN_DISCONNECT:
		break;
	default:
		BT_WARN("no valid (%u) state was set", state);

		break;
	}
}

struct bt_conn *bt_conn_lookup_handle(uint16_t handle)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(conns); i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		/* We only care about connections with a valid handle */
		if (conns[i].state != BT_CONN_CONNECTED &&
		    conns[i].state != BT_CONN_DISCONNECT) {
			continue;
		}

		if (conns[i].handle == handle) {
			return bt_conn_ref(&conns[i]);
		}
	}

	return NULL;
}

struct bt_conn *bt_conn_lookup_addr_le(const bt_addr_le_t *peer)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(conns); i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		if (conns[i].type != BT_CONN_TYPE_LE) {
			continue;
		}

		if (!bt_addr_le_cmp(peer, &conns[i].le.dst)) {
			return bt_conn_ref(&conns[i]);
		}
	}

	return NULL;
}

struct bt_conn *bt_conn_lookup_state_le(const bt_addr_le_t *peer,
					const bt_conn_state_t state)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(conns); i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		if (conns[i].type != BT_CONN_TYPE_LE) {
			continue;
		}

		if (bt_addr_le_cmp(peer, BT_ADDR_LE_ANY) &&
		    bt_addr_le_cmp(peer, &conns[i].le.dst)) {
			continue;
		}

		if (conns[i].state == state) {
			return bt_conn_ref(&conns[i]);
		}
	}

	return NULL;
}

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	atomic_inc(&conn->ref);

	BT_DBG("handle %u ref %u", conn->handle, atomic_get(&conn->ref));

	return conn;
}

void bt_conn_unref(struct bt_conn *conn)
{
	atomic_dec(&conn->ref);

	BT_DBG("handle %u ref %u", conn->handle, atomic_get(&conn->ref));
}

const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn)
{
	return &conn->le.dst;
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	info->type = conn->type;

	switch (conn->type) {
	case BT_CONN_TYPE_LE:
		if (conn->role == BT_HCI_ROLE_MASTER) {
			info->le.src = &conn->le.init_addr;
			info->le.dst = &conn->le.resp_addr;
		} else {
			info->le.src = &conn->le.resp_addr;
			info->le.dst = &conn->le.init_addr;
		}
		return 0;
	}

	return -EINVAL;
}

static int bt_hci_disconnect(struct bt_conn *conn, uint8_t reason)
{
	struct net_buf *buf;
	struct bt_hci_cp_disconnect *disconn;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_DISCONNECT, sizeof(*disconn));
	if (!buf) {
		return -ENOBUFS;
	}

	disconn = net_buf_add(buf, sizeof(*disconn));
	disconn->handle = sys_cpu_to_le16(conn->handle);
	disconn->reason = reason;

	err = bt_hci_cmd_send(BT_HCI_OP_DISCONNECT, buf);
	if (err) {
		return err;
	}

	bt_conn_set_state(conn, BT_CONN_DISCONNECT);

	return 0;
}

static int bt_hci_connect_le_cancel(struct bt_conn *conn)
{
	int err;

	if (conn->timeout) {
		fiber_fiber_delayed_start_cancel(conn->timeout);
		conn->timeout = NULL;

		/* Drop the reference took by timeout fiber */
		bt_conn_unref(conn);
	}

	err = bt_hci_cmd_send(BT_HCI_OP_LE_CREATE_CONN_CANCEL, NULL);
	if (err) {
		return err;
	}

	return 0;
}

int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
#if defined(CONFIG_BLUETOOTH_CENTRAL)
	/* Disconnection is initiated by us, so auto connection shall
	 * be disabled. Otherwise the passive scan would be enabled
	 * and we could send LE Create Connection as soon as the remote
	 * starts advertising.
	 */
	if (conn->type == BT_CONN_TYPE_LE) {
		bt_le_set_auto_conn(&conn->le.dst, false);
	}
#endif

	switch (conn->state) {
	case BT_CONN_CONNECT_SCAN:
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		bt_le_scan_update(false);
		return 0;
	case BT_CONN_CONNECT:
		return bt_hci_connect_le_cancel(conn);
	case BT_CONN_CONNECTED:
		return bt_hci_disconnect(conn, reason);
	case BT_CONN_DISCONNECT:
		return 0;
	case BT_CONN_DISCONNECTED:
	default:
		return -ENOTCONN;
	}
}

struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer,
				  const struct bt_le_conn_param *param)
{
	struct bt_conn *conn;

	if (!bt_le_conn_params_valid(param->interval_min, param->interval_max,
				     param->latency, param->timeout)) {
		return NULL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return NULL;
	}

	conn = bt_conn_lookup_addr_le(peer);
	if (conn) {
		switch (conn->state) {
		case BT_CONN_CONNECT_SCAN:
			bt_conn_set_param_le(conn, param);
			return conn;
		case BT_CONN_CONNECT:
		case BT_CONN_CONNECTED:
			return conn;
		default:
			bt_conn_unref(conn);
			return NULL;
		}
	}

	conn = bt_conn_add_le(peer);
	if (!conn) {
		return NULL;
	}

	bt_conn_set_param_le(conn, param);

	bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);

	bt_le_scan_update(true);

	return conn;
}

int bt_conn_le_conn_update(struct bt_conn *conn, uint16_t min, uint16_t max,
			   uint16_t latency, uint16_t timeout)
{
	struct hci_cp_le_conn_update *conn_update;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_UPDATE,
				sizeof(*conn_update));
	if (!buf) {
		return -ENOBUFS;
	}

	conn_update = net_buf_add(buf, sizeof(*conn_update));
	memset(conn_update, 0, sizeof(*conn_update));
	conn_update->handle = sys_cpu_to_le16(conn->handle);
	conn_update->conn_interval_min = sys_cpu_to_le16(min);
	conn_update->conn_interval_max = sys_cpu_to_le16(max);
	conn_update->conn_latency = sys_cpu_to_le16(latency);
	conn_update->supervision_timeout = sys_cpu_to_le16(timeout);

	return bt_hci_cmd_send(BT_HCI_OP_LE_CONN_UPDATE, buf);
}

uint8_t bt_conn_enc_key_size(struct bt_conn *conn)
{
	return conn->keys ? conn->keys->enc_size : 0;
}

struct net_buf *bt_conn_create_pdu(struct nano_fifo *fifo, size_t reserve)
{
	size_t head_reserve = reserve + sizeof(struct bt_hci_acl_hdr) +
					CONFIG_BLUETOOTH_HCI_SEND_RESERVE;

	return net_buf_get(fifo, head_reserve);
}

static void background_scan_init(void)
{
#if defined(CONFIG_BLUETOOTH_CENTRAL)
	int i;

	for (i = 0; i < ARRAY_SIZE(conns); i++) {
		struct bt_conn *conn = &conns[i];

		if (!atomic_get(&conn->ref)) {
			continue;
		}

		if (atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT)) {
			bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);
		}
	}
#endif /* CONFIG_BLUETOOTH_CENTRAL */
}

int bt_conn_init(void)
{
	int err;

	net_buf_pool_init(frag_pool);
	net_buf_pool_init(dummy_pool);

	bt_att_init();

	err = bt_smp_init();
	if (err) {
		return err;
	}

	bt_l2cap_init();

	background_scan_init();

	return 0;
}
