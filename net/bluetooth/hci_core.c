/* hci_core.c - HCI core Bluetooth handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <misc/util.h>
#include <misc/byteorder.h>

#include <bluetooth/log.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "hci_core.h"
#include "keys.h"
#include "conn_internal.h"
#include "l2cap.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_HCI_CORE)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

/* How many buffers to use for incoming ACL data */
#define ACL_IN_MAX	7
#define ACL_OUT_MAX	7

/* Stacks for the fibers */
static BT_STACK_NOINIT(rx_fiber_stack, 1024);
static BT_STACK_NOINIT(rx_prio_fiber_stack, 256);
static BT_STACK_NOINIT(cmd_tx_fiber_stack, 256);

#if defined(CONFIG_BLUETOOTH_DEBUG)
static nano_thread_id_t rx_prio_fiber_id;
#endif

struct bt_dev bt_dev;

static bt_le_scan_cb_t *scan_dev_found_cb;

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

struct bt_buf *bt_hci_cmd_create(uint16_t opcode, uint8_t param_len)
{
	struct bt_hci_cmd_hdr *hdr;
	struct bt_buf *buf;

	BT_DBG("opcode %x param_len %u\n", opcode, param_len);

	buf = bt_buf_get(BT_CMD, bt_dev.drv->head_reserve);
	if (!buf) {
		BT_ERR("Cannot get free buffer\n");
		return NULL;
	}

	BT_DBG("buf %p\n", buf);

	buf->hci.opcode = opcode;
	buf->hci.sync = NULL;

	hdr = bt_buf_add(buf, sizeof(*hdr));
	hdr->opcode = sys_cpu_to_le16(opcode);
	hdr->param_len = param_len;

	return buf;
}

int bt_hci_cmd_send(uint16_t opcode, struct bt_buf *buf)
{
	if (!buf) {
		buf = bt_hci_cmd_create(opcode, 0);
		if (!buf) {
			return -ENOBUFS;
		}
	}

	BT_DBG("opcode %x len %u\n", opcode, buf->len);

	/* Host Number of Completed Packets can ignore the ncmd value
	 * and does not generate any cmd complete/status events.
	 */
	if (opcode == BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS) {
		bt_dev.drv->send(buf);
		bt_buf_put(buf);
		return 0;
	}

	nano_fifo_put(&bt_dev.cmd_tx_queue, buf);

	return 0;
}

int bt_hci_cmd_send_sync(uint16_t opcode, struct bt_buf *buf,
			 struct bt_buf **rsp)
{
	struct nano_sem sync_sem;
	int err;

	/* This function cannot be called from the rx fiber since it
	 * relies on the very same fiber in processing the cmd_complete
	 * event and giving back the blocking semaphore.
	 */
#if defined(CONFIG_BLUETOOTH_DEBUG)
	if (sys_thread_self_get() == rx_prio_fiber_id) {
		BT_ERR("called from invalid context!\n");
		return -EDEADLK;
	}
#endif

	if (!buf) {
		buf = bt_hci_cmd_create(opcode, 0);
		if (!buf) {
			return -ENOBUFS;
		}
	}

	BT_DBG("opcode %x len %u\n", opcode, buf->len);

	nano_sem_init(&sync_sem);
	buf->hci.sync = &sync_sem;

	nano_fifo_put(&bt_dev.cmd_tx_queue, buf);

	nano_sem_take_wait(&sync_sem);

	/* Indicate failure if we failed to get the return parameters */
	if (!buf->hci.sync) {
		err = -EIO;
	} else {
		err = 0;
	}

	if (rsp) {
		*rsp = buf->hci.sync;
	} else if (buf->hci.sync) {
		bt_buf_put(buf->hci.sync);
	}

	bt_buf_put(buf);

	return err;
}

static void hci_acl(struct bt_buf *buf)
{
	struct bt_hci_acl_hdr *hdr = (void *)buf->data;
	uint16_t handle, len = sys_le16_to_cpu(hdr->len);
	struct bt_conn *conn;
	uint8_t flags;

	BT_DBG("buf %p\n", buf);

	handle = sys_le16_to_cpu(hdr->handle);
	flags = (handle >> 12);
	buf->acl.handle = bt_acl_handle(handle);

	bt_buf_pull(buf, sizeof(*hdr));

	BT_DBG("handle %u len %u flags %u\n", buf->acl.handle, len, flags);

	if (buf->len != len) {
		BT_ERR("ACL data length mismatch (%u != %u)\n", buf->len, len);
		bt_buf_put(buf);
		return;
	}

	conn = bt_conn_lookup_handle(buf->acl.handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u\n", buf->acl.handle);
		bt_buf_put(buf);
		return;
	}

	bt_conn_recv(conn, buf, flags);
	bt_conn_put(conn);
}

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_PRINTK)
#include <offsets.h>
#include <misc/printk.h>

enum {
	STACK_DIRECTION_UP,
	STACK_DIRECTION_DOWN,
};

static void analyze_stack(const char *name, const char *stack, unsigned size,
			  int stack_growth)
{
	unsigned i, stack_offset, pcnt, unused = 0;

	/* The TCS is always placed on a 4-byte aligned boundary - if
	 * the stack beginning doesn't match that there will be some
	 * unused bytes in the beginning.
	 */
	stack_offset = __tTCS_SIZEOF + ((4 - ((unsigned)stack % 4)) % 4);

	if (stack_growth == STACK_DIRECTION_DOWN) {
		for (i = stack_offset; i < size; i++) {
			if ((unsigned char)stack[i] == 0xaa) {
				unused++;
			} else {
				break;
			}
		}
	} else {
		for (i = size - 1; i >= stack_offset; i--) {
			if ((unsigned char)stack[i] == 0xaa) {
				unused++;
			} else {
				break;
			}
		}
	}

	/* Calculate the real size reserved for the stack */
	size -= stack_offset;
	pcnt = ((size - unused) * 100) / size;

	printk("%s (real size %u):\tunused %u\tusage %u / %u (%u %%)\n", name,
	       size + stack_offset, unused, size - unused, size, pcnt);
}

static void analyze_stacks(struct bt_conn *conn, struct bt_conn **ref)
{
	int stack_growth;

	printk("sizeof(tTCS) = %u\n", __tTCS_SIZEOF);

	if (conn > *ref) {
		printk("stack grows up\n");
		stack_growth = STACK_DIRECTION_UP;
	} else {
		printk("stack grows down\n");
		stack_growth = STACK_DIRECTION_DOWN;
	}

	analyze_stack("rx stack", rx_fiber_stack, sizeof(rx_fiber_stack),
		      stack_growth);
	analyze_stack("cmd rx stack", rx_prio_fiber_stack,
		      sizeof(rx_prio_fiber_stack), stack_growth);
	analyze_stack("cmd tx stack", cmd_tx_fiber_stack,
		      sizeof(cmd_tx_fiber_stack), stack_growth);
	analyze_stack("conn tx stack", conn->stack, sizeof(conn->stack),
		      stack_growth);
}
#else
#define analyze_stacks(...)
#endif

/* HCI event processing */

static void update_sec_level(struct bt_conn *conn)
{
	struct bt_keys *keys;

	if (!conn->encrypt) {
		conn->sec_level = BT_SECURITY_LOW;
		return;
	}

	keys = bt_keys_find_addr(&conn->dst);
	if (keys && keys->type == BT_KEYS_AUTHENTICATED) {
		conn->sec_level = BT_SECURITY_HIGH;
	} else {
		conn->sec_level = BT_SECURITY_MEDIUM;
	}

	if (conn->required_sec_level > conn->sec_level) {
		BT_ERR("Failed to set required security level\n");
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
	}
}

static void hci_encrypt_change(struct bt_buf *buf)
{
	struct bt_hci_evt_encrypt_change *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u encrypt 0x%02x\n", evt->status, handle,
	       evt->encrypt);

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u\n", handle);
		return;
	}

	conn->encrypt = evt->encrypt;

	update_sec_level(conn);
	bt_l2cap_encrypt_change(conn);
	bt_conn_security_changed(conn);

	bt_conn_put(conn);
}

static void hci_reset_complete(struct bt_buf *buf)
{
	uint8_t status = buf->data[0];

	BT_DBG("status %u\n", status);

	if (status) {
		return;
	}

	scan_dev_found_cb = NULL;
	atomic_set(bt_dev.flags, 0);
	atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP);
}

static void hci_cmd_done(uint16_t opcode, uint8_t status, struct bt_buf *buf)
{
	struct bt_buf *sent = bt_dev.sent_cmd;

	if (!sent) {
		return;
	}

	if (bt_dev.sent_cmd->hci.opcode != opcode) {
		BT_ERR("Unexpected completion of opcode 0x%04x\n", opcode);
		return;
	}

	bt_dev.sent_cmd = NULL;

	/* If the command was synchronous wake up bt_hci_cmd_send_sync() */
	if (sent->hci.sync) {
		struct nano_sem *sem = sent->hci.sync;

		if (status) {
			sent->hci.sync = NULL;
		} else {
			sent->hci.sync = bt_buf_hold(buf);
		}

		nano_fiber_sem_give(sem);
	} else {
		bt_buf_put(sent);
	}
}

static void hci_cmd_complete(struct bt_buf *buf)
{
	struct hci_evt_cmd_complete *evt = (void *)buf->data;
	uint16_t opcode = sys_le16_to_cpu(evt->opcode);
	uint8_t *status;

	BT_DBG("opcode %x\n", opcode);

	bt_buf_pull(buf, sizeof(*evt));

	/* All command return parameters have a 1-byte status in the
	 * beginning, so we can safely make this generalization.
	 */
	status = buf->data;

	switch (opcode) {
	case BT_HCI_OP_RESET:
		hci_reset_complete(buf);
		break;
	default:
		BT_DBG("Unhandled opcode %x\n", opcode);
		break;
	}

	hci_cmd_done(opcode, *status, buf);

	if (evt->ncmd && !bt_dev.ncmd) {
		/* Allow next command to be sent */
		bt_dev.ncmd = 1;
		nano_fiber_sem_give(&bt_dev.ncmd_sem);
	}
}

static void hci_cmd_status(struct bt_buf *buf)
{
	struct bt_hci_evt_cmd_status *evt = (void *)buf->data;
	uint16_t opcode = sys_le16_to_cpu(evt->opcode);

	BT_DBG("opcode %x\n", opcode);

	bt_buf_pull(buf, sizeof(*evt));

	switch (opcode) {
	default:
		BT_DBG("Unhandled opcode %x\n", opcode);
		break;
	}

	hci_cmd_done(opcode, evt->status, buf);

	if (evt->ncmd && !bt_dev.ncmd) {
		/* Allow next command to be sent */
		bt_dev.ncmd = 1;
		nano_fiber_sem_give(&bt_dev.ncmd_sem);
	}
}

static void hci_num_completed_packets(struct bt_buf *buf)
{
	struct bt_hci_evt_num_completed_packets *evt = (void *)buf->data;
	uint16_t i, num_handles = sys_le16_to_cpu(evt->num_handles);

	BT_DBG("num_handles %u\n", num_handles);

	for (i = 0; i < num_handles; i++) {
		uint16_t handle, count;

		handle = sys_le16_to_cpu(evt->h[i].handle);
		count = sys_le16_to_cpu(evt->h[i].count);

		BT_DBG("handle %u count %u\n", handle, count);

		while (count--)
			nano_fiber_sem_give(&bt_dev.le_pkts_sem);
	}
}

static void hci_encrypt_key_refresh_complete(struct bt_buf *buf)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	uint16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status %u handle %u\n", evt->status, handle);

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u\n", handle);
		return;
	}

	bt_l2cap_encrypt_change(conn);
	bt_conn_put(conn);
}

static void copy_id_addr(struct bt_conn *conn, const bt_addr_le_t *addr)
{
	struct bt_keys *keys;

	/* If we have a keys struct we already know the identity */
	if (conn->keys) {
		return;
	}

	keys = bt_keys_find_irk(addr);
	if (keys) {
		bt_addr_le_copy(&conn->dst, &keys->addr);
		conn->keys = keys;
	} else {
		bt_addr_le_copy(&conn->dst, addr);
	}
}

static int bt_hci_start_scanning(uint8_t scan_type)
{
	struct bt_buf *buf, *rsp;
	struct bt_hci_cp_le_set_scan_params *set_param;
	struct bt_hci_cp_le_set_scan_enable *scan_enable;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_PARAMS,
				sizeof(*set_param));
	if (!buf) {
		return -ENOBUFS;
	}

	set_param = bt_buf_add(buf, sizeof(*set_param));
	memset(set_param, 0, sizeof(*set_param));
	set_param->scan_type = scan_type;

	/* for the rest parameters apply default values according to
	 *  spec 4.2, vol2, part E, 7.8.10
	 */
	set_param->interval = sys_cpu_to_le16(0x0010);
	set_param->window = sys_cpu_to_le16(0x0010);
	set_param->filter_policy = 0x00;
	set_param->addr_type = 0x00;

	bt_hci_cmd_send(BT_HCI_OP_LE_SET_SCAN_PARAMS, buf);
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_ENABLE,
				sizeof(*scan_enable));
	if (!buf) {
		return -ENOBUFS;
	}

	scan_enable = bt_buf_add(buf, sizeof(*scan_enable));
	memset(scan_enable, 0, sizeof(*scan_enable));
	scan_enable->filter_dup = atomic_test_bit(bt_dev.flags,
						  BT_DEV_SCAN_FILTER_DUP);
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

	bt_buf_put(rsp);

	return err;
}

static int bt_hci_stop_scanning(void)
{
	struct bt_buf *buf, *rsp;
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

	scan_enable = bt_buf_add(buf, sizeof(*scan_enable));
	memset(scan_enable, 0x0, sizeof(*scan_enable));
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

	bt_buf_put(rsp);

	return err;
}

static int hci_le_create_conn(const bt_addr_le_t *addr)
{
	struct bt_buf *buf;
	struct bt_hci_cp_le_create_conn *cp;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_CONN, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	memset(cp, 0x0, sizeof(*cp));
	bt_addr_le_copy(&cp->peer_addr, addr);
	cp->conn_interval_max = sys_cpu_to_le16(0x0028);
	cp->conn_interval_min = sys_cpu_to_le16(0x0018);
	cp->scan_interval = sys_cpu_to_le16(0x0060);
	cp->scan_window = sys_cpu_to_le16(0x0030);
	cp->supervision_timeout = sys_cpu_to_le16(0x07D0);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CONN, buf, NULL);
}

/* Used to determine whether to start scan and which scan type should be used */
int bt_le_scan_update(void)
{
	struct bt_conn *conn;

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		int err;

		if (scan_dev_found_cb) {
			return 0;
		}

		err = bt_hci_stop_scanning();
		if (err) {
			return err;
		}
	}

	if (scan_dev_found_cb) {
		return bt_hci_start_scanning(BT_LE_SCAN_ACTIVE);
	}

	conn = bt_conn_lookup_state(BT_ADDR_LE_ANY, BT_CONN_CONNECT_SCAN);
	if (!conn) {
		return 0;
	}

	bt_conn_put(conn);

	return bt_hci_start_scanning(BT_LE_SCAN_PASSIVE);
}

static void hci_disconn_complete(struct bt_buf *buf)
{
	struct bt_hci_evt_disconn_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u reason %u\n", evt->status, handle,
	       evt->reason);

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u\n", handle);
		return;
	}

	/* Check stack usage (no-op if not enabled) */
	analyze_stacks(conn, &conn);

	bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
	conn->handle = 0;

	if (atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT)) {
		bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);
		bt_le_scan_update();
	}

	bt_conn_put(conn);

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		struct bt_buf *buf;
		uint8_t adv_enable = 0x01;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
		if (buf) {
			memcpy(bt_buf_add(buf, 1), &adv_enable, 1);
			bt_hci_cmd_send(BT_HCI_OP_LE_SET_ADV_ENABLE, buf);
		}
	}
}

static int hci_le_read_remote_features(struct bt_conn *conn)
{
	struct bt_hci_cp_le_read_remote_features *cp;
	struct bt_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_READ_REMOTE_FEATURES,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	bt_hci_cmd_send(BT_HCI_OP_LE_READ_REMOTE_FEATURES, buf);

	return 0;
}

static int update_conn_params(struct bt_conn *conn)
{
	BT_DBG("conn %p features 0x%x\n", conn, conn->le_features[0]);

	/* Check if there's a need to update conn params */
	if (conn->le_conn_interval >= LE_CONN_MIN_INTERVAL &&
	    conn->le_conn_interval <= LE_CONN_MAX_INTERVAL) {
		return -EALREADY;
	}

	if ((conn->role == BT_HCI_ROLE_SLAVE) &&
	    !(bt_dev.le_features[0] & BT_HCI_LE_CONN_PARAM_REQ_PROC)) {
		return bt_l2cap_update_conn_param(conn);
	}

	if ((conn->le_features[0] & BT_HCI_LE_CONN_PARAM_REQ_PROC) &&
	    (bt_dev.le_features[0] & BT_HCI_LE_CONN_PARAM_REQ_PROC)) {
		return bt_conn_le_conn_update(conn, LE_CONN_MIN_INTERVAL,
					     LE_CONN_MAX_INTERVAL,
					     LE_CONN_LATENCY, LE_CONN_TIMEOUT);
	}

	return -EBUSY;
}

static void le_conn_complete(struct bt_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;
	struct bt_keys *keys;
	bt_addr_le_t src;
	int err;

	BT_DBG("status %u handle %u role %u %s\n", evt->status, handle,
	       evt->role, bt_addr_le_str(&evt->peer_addr));

	/* Make lookup to check if there's a connection object in CONNECT state
	 * associated with passed peer LE address.
	 */
	keys = bt_keys_find_irk(&evt->peer_addr);
	if (keys) {
		conn = bt_conn_lookup_state(&keys->addr, BT_CONN_CONNECT);
	} else {
		conn = bt_conn_lookup_state(&evt->peer_addr, BT_CONN_CONNECT);
	}

	if (evt->status) {
		if (!conn) {
			return;
		}

		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);

		/* Drop the reference got by lookup call in CONNECT state.
		 * We are now in DISCONNECTED state since no successful LE
		 * link been made.
		 */
		bt_conn_put(conn);

		return;
	}

	if (!conn) {
		conn = bt_conn_add(&evt->peer_addr);
	}

	if (!conn) {
		BT_ERR("Unable to add new conn for handle %u\n", handle);
		return;
	}

	conn->handle   = handle;
	copy_id_addr(conn, &evt->peer_addr);
	conn->le_conn_interval = sys_le16_to_cpu(evt->interval);
	conn->role = evt->role;

	src.type = BT_ADDR_LE_PUBLIC;
	memcpy(src.val, bt_dev.bdaddr.val, sizeof(bt_dev.bdaddr.val));

	if (conn->role == BT_HCI_ROLE_MASTER) {
		bt_addr_le_copy(&conn->init_addr, &src);
		bt_addr_le_copy(&conn->resp_addr, &evt->peer_addr);
	} else {
		bt_addr_le_copy(&conn->init_addr, &evt->peer_addr);
		bt_addr_le_copy(&conn->resp_addr, &src);
	}

	bt_conn_set_state(conn, BT_CONN_CONNECTED);

	if ((evt->role == BT_HCI_ROLE_MASTER) ||
	    (bt_dev.le_features[0] & BT_HCI_LE_SLAVE_FEATURES)) {
		err = hci_le_read_remote_features(conn);
		if (!err) {
			goto done;
		}
	}

	if (update_conn_params(conn)) {
		bt_conn_connected(conn);
	}

done:
	bt_conn_put(conn);
	bt_le_scan_update();
}

static void le_remote_feat_complete(struct bt_buf *buf)
{
	struct bt_hci_ev_le_remote_feat_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u\n", handle);
		return;
	}

	if (!evt->status) {
		memcpy(conn->le_features, evt->features,
		       sizeof(conn->le_features));
	}

	if (update_conn_params(conn)) {
		bt_conn_connected(conn);
	}

	bt_conn_put(conn);
}

static int le_conn_param_neg_reply(uint16_t handle, uint8_t reason)
{
	struct bt_hci_cp_le_conn_param_req_neg_reply *cp;
	struct bt_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->reason = sys_cpu_to_le16(reason);

	return bt_hci_cmd_send(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY, buf);
}

static int le_conn_param_req_reply(uint16_t handle, uint16_t min, uint16_t max,
				   uint16_t latency, uint16_t timeout)
{
	struct bt_hci_cp_le_conn_param_req_reply *cp;
	struct bt_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = bt_buf_add(buf, sizeof(*cp));
	memset(cp, 0x0, sizeof(*cp));

	cp->handle = sys_cpu_to_le16(handle);
	cp->interval_min = sys_cpu_to_le16(min);
	cp->interval_max = sys_cpu_to_le16(max);
	cp->latency = sys_cpu_to_le16(latency);
	cp->timeout = sys_cpu_to_le16(timeout);

	return bt_hci_cmd_send(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY, buf);
}

static int le_conn_param_req(struct bt_buf *buf)
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
		BT_ERR("Unable to lookup conn for handle %u\n", handle);
		return le_conn_param_neg_reply(handle,
					       BT_HCI_ERR_UNKNOWN_CONN_ID);
	}

	bt_conn_put(conn);

	if (!bt_le_conn_params_valid(min, max, latency, timeout)) {
		return le_conn_param_neg_reply(handle,
					       BT_HCI_ERR_INVALID_LL_PARAMS);
	}

	return le_conn_param_req_reply(handle, min, max, latency, timeout);
}

static void le_conn_update_complete(struct bt_buf *buf)
{
	struct bt_hci_evt_le_conn_update_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	uint16_t handle, interval;

	handle = sys_le16_to_cpu(evt->handle);
	interval = sys_le16_to_cpu(evt->interval);

	BT_DBG("status %u, handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u\n", handle);
		return;
	}

	if (!evt->status) {
		conn->le_conn_interval = interval;
	}

	/* TODO Notify about connection */

	bt_conn_put(conn);
}

static void check_pending_conn(const bt_addr_le_t *addr, uint8_t evtype)
{
	struct bt_conn *conn;

	/* Return if event is not connectable */
	if (evtype != BT_LE_ADV_IND && evtype != BT_LE_ADV_DIRECT_IND) {
		return;
	}

	conn = bt_conn_lookup_state(addr, BT_CONN_CONNECT_SCAN);
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
	bt_conn_put(conn);
}

static void le_adv_report(struct bt_buf *buf)
{
	uint8_t num_reports = buf->data[0];
	struct bt_hci_ev_le_advertising_info *info;

	BT_DBG("Adv number of reports %u\n",  num_reports);

	info = bt_buf_pull(buf, sizeof(num_reports));

	while (num_reports--) {
		int8_t rssi = info->data[info->length];
		const bt_addr_le_t *addr;
		struct bt_keys *keys;

		BT_DBG("%s event %u, len %u, rssi %d dBm\n",
			bt_addr_le_str(&info->addr),
			info->evt_type, info->length, rssi);

		keys = bt_keys_find_irk(&info->addr);
		if (keys) {
			addr = &keys->addr;
			BT_DBG("Identity %s matched RPA %s\n",
			       bt_addr_le_str(&keys->addr),
			       bt_addr_le_str(&info->addr));
		} else {
			addr = &info->addr;
		}

		if (scan_dev_found_cb) {
			scan_dev_found_cb(addr, rssi, info->evt_type,
					  info->data, info->length);
		}

		check_pending_conn(addr, info->evt_type);

		/* Get next report iteration by moving pointer to right offset
		 * in buf according to spec 4.2, Vol 2, Part E, 7.7.65.2.
		 */
		info = bt_buf_pull(buf, sizeof(*info) + info->length +
					sizeof(rssi));
	}
}

static void le_ltk_request(struct bt_buf *buf)
{
	struct bt_hci_evt_le_ltk_request *evt = (void *)buf->data;
	struct bt_conn *conn;
	uint16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("handle %u\n", handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u\n", handle);
		return;
	}

	if (!conn->keys) {
		conn->keys = bt_keys_find(BT_KEYS_SLAVE_LTK, &conn->dst);
	}

	if (conn->keys && (conn->keys->keys & BT_KEYS_SLAVE_LTK) &&
	    conn->keys->slave_ltk.rand == evt->rand &&
	    conn->keys->slave_ltk.ediv == evt->ediv) {
		struct bt_hci_cp_le_ltk_req_reply *cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_REPLY,
					sizeof(*cp));
		if (!buf) {
			BT_ERR("Out of command buffers\n");
			goto done;
		}

		cp = bt_buf_add(buf, sizeof(*cp));
		cp->handle = evt->handle;
		memcpy(cp->ltk, conn->keys->slave_ltk.val, 16);

		bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_REPLY, buf);
	} else {
		struct bt_hci_cp_le_ltk_req_neg_reply *cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY,
					sizeof(*cp));
		if (!buf) {
			BT_ERR("Out of command buffers\n");
			goto done;
		}

		cp = bt_buf_add(buf, sizeof(*cp));
		cp->handle = evt->handle;

		bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY, buf);
	}

done:
	bt_conn_put(conn);
}

static void hci_le_meta_event(struct bt_buf *buf)
{
	struct bt_hci_evt_le_meta_event *evt = (void *)buf->data;

	bt_buf_pull(buf, sizeof(*evt));

	switch (evt->subevent) {
	case BT_HCI_EVT_LE_CONN_COMPLETE:
		le_conn_complete(buf);
		break;
	case BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE:
		le_conn_update_complete(buf);
		break;
	case BT_HCI_EVT_LE_ADVERTISING_REPORT:
		le_adv_report(buf);
		break;
	case BT_HCI_EVT_LE_LTK_REQUEST:
		le_ltk_request(buf);
		break;
	case BT_HCI_EV_LE_REMOTE_FEAT_COMPLETE:
		le_remote_feat_complete(buf);
		break;
	case BT_HCI_EVT_LE_CONN_PARAM_REQ:
		le_conn_param_req(buf);
		break;
	default:
		BT_DBG("Unhandled LE event %x\n", evt->subevent);
		break;
	}
}

static void hci_event(struct bt_buf *buf)
{
	struct bt_hci_evt_hdr *hdr = (void *)buf->data;

	BT_DBG("event %u\n", hdr->evt);

	bt_buf_pull(buf, sizeof(*hdr));

	switch (hdr->evt) {
	case BT_HCI_EVT_DISCONN_COMPLETE:
		hci_disconn_complete(buf);
		break;
	case BT_HCI_EVT_ENCRYPT_CHANGE:
		hci_encrypt_change(buf);
		break;
	case BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE:
		hci_encrypt_key_refresh_complete(buf);
		break;
	case BT_HCI_EVT_LE_META_EVENT:
		hci_le_meta_event(buf);
		break;
	default:
		BT_WARN("Unhandled event 0x%02x\n", hdr->evt);
		break;

	}

	bt_buf_put(buf);
}

static void hci_cmd_tx_fiber(void)
{
	struct bt_driver *drv = bt_dev.drv;

	BT_DBG("started\n");

	while (1) {
		struct bt_buf *buf;

		/* Wait until ncmd > 0 */
		BT_DBG("calling sem_take_wait\n");
		nano_fiber_sem_take_wait(&bt_dev.ncmd_sem);

		/* Get next command - wait if necessary */
		BT_DBG("calling fifo_get_wait\n");
		buf = nano_fifo_get_wait(&bt_dev.cmd_tx_queue);
		bt_dev.ncmd = 0;

		BT_DBG("Sending command %x (buf %p) to driver\n",
		       buf->hci.opcode, buf);

		drv->send(buf);

		/* Clear out any existing sent command */
		if (bt_dev.sent_cmd) {
			BT_ERR("Uncleared pending sent_cmd\n");
			bt_buf_put(bt_dev.sent_cmd);
			bt_dev.sent_cmd = NULL;
		}

		bt_dev.sent_cmd = buf;
	}
}

static void rx_prio_fiber(void)
{
	struct bt_buf *buf;

	BT_DBG("started\n");

	/* So we can avoid bt_hci_cmd_send_sync deadlocks */
#if defined(CONFIG_BLUETOOTH_DEBUG)
	rx_prio_fiber_id = sys_thread_self_get();
#endif

	while (1) {
		struct bt_hci_evt_hdr *hdr;

		BT_DBG("calling fifo_get_wait\n");
		buf = nano_fifo_get_wait(&bt_dev.rx_prio_queue);

		BT_DBG("buf %p type %u len %u\n", buf, buf->type, buf->len);

		if (buf->type != BT_EVT) {
			BT_ERR("Unknown buf type %u\n", buf->type);
			bt_buf_put(buf);
			continue;
		}

		hdr = (void *)buf->data;
		bt_buf_pull(buf, sizeof(*hdr));

		switch (hdr->evt) {
		case BT_HCI_EVT_CMD_COMPLETE:
			hci_cmd_complete(buf);
			break;
		case BT_HCI_EVT_CMD_STATUS:
			hci_cmd_status(buf);
			break;
		case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
			hci_num_completed_packets(buf);
			break;
		default:
			BT_ERR("Unknown event 0x%02x\n", hdr->evt);
			break;
		}

		bt_buf_put(buf);
	}
}

static void read_local_features_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_read_local_features *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	memcpy(bt_dev.features, rp->features, sizeof(bt_dev.features));
}

static void read_local_ver_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_read_local_version_info *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	bt_dev.hci_version = rp->hci_version;
	bt_dev.hci_revision = sys_le16_to_cpu(rp->hci_revision);
	bt_dev.manufacturer = sys_le16_to_cpu(rp->manufacturer);
}

static void read_bdaddr_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_read_bd_addr *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	bt_addr_copy(&bt_dev.bdaddr, &rp->bdaddr);
}

static void read_le_features_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_le_read_local_features *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	memcpy(bt_dev.le_features, rp->features, sizeof(bt_dev.le_features));
}

static void read_buffer_size_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	/* If LE-side has buffers we can ignore the BR/EDR values */
	if (bt_dev.le_mtu) {
		return;
	}

	bt_dev.le_mtu = sys_le16_to_cpu(rp->acl_max_len);
	bt_dev.le_pkts = sys_le16_to_cpu(rp->acl_max_num);
}

static void le_read_buffer_size_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_le_read_buffer_size *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	bt_dev.le_mtu = sys_le16_to_cpu(rp->le_max_len);
	bt_dev.le_pkts = rp->le_max_num;
}

static int hci_init(void)
{
	struct bt_hci_cp_host_buffer_size *hbs;
	struct bt_hci_cp_set_event_mask *ev;
	struct bt_buf *buf, *rsp;
	uint8_t *enable;
	int i, err;

	/* Send HCI_RESET */
	bt_hci_cmd_send(BT_HCI_OP_RESET, NULL);

	/* Read Local Supported Features */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_FEATURES, NULL, &rsp);
	if (err) {
		return err;
	}
	read_local_features_complete(rsp);
	bt_buf_put(rsp);

	/* Read Local Version Information */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL,
				   &rsp);
	if (err) {
		return err;
	}
	read_local_ver_complete(rsp);
	bt_buf_put(rsp);

	/* Read Bluetooth Address */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR, NULL, &rsp);
	if (err) {
		return err;
	}
	read_bdaddr_complete(rsp);
	bt_buf_put(rsp);

	/* For now we only support LE capable controllers */
	if (!lmp_le_capable(bt_dev)) {
		BT_ERR("Non-LE capable controller detected!\n");
		return -ENODEV;
	}

	/* Read Low Energy Supported Features */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_LOCAL_FEATURES, NULL,
				   &rsp);
	if (err) {
		return err;
	}
	read_le_features_complete(rsp);
	bt_buf_put(rsp);

	/* Read LE Buffer Size */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_BUFFER_SIZE, NULL, &rsp);
	if (err) {
		return err;
	}
	le_read_buffer_size_complete(rsp);
	bt_buf_put(rsp);

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_EVENT_MASK, sizeof(*ev));
	if (!buf) {
		return -ENOBUFS;
	}

	ev = bt_buf_add(buf, sizeof(*ev));
	memset(ev, 0, sizeof(*ev));
	ev->events[0] |= 0x10; /* Disconnection Complete */
	ev->events[1] |= 0x08; /* Read Remote Version Information Complete */
	ev->events[1] |= 0x20; /* Command Complete */
	ev->events[1] |= 0x40; /* Command Status */
	ev->events[1] |= 0x80; /* Hardware Error */
	ev->events[2] |= 0x04; /* Number of Completed Packets */
	ev->events[3] |= 0x02; /* Data Buffer Overflow */
	ev->events[7] |= 0x20; /* LE Meta-Event */

	if (bt_dev.le_features[0] & BT_HCI_LE_ENCRYPTION) {
		ev->events[0] |= 0x80; /* Encryption Change */
		ev->events[5] |= 0x80; /* Encryption Key Refresh Complete */
	}

	bt_hci_cmd_send_sync(BT_HCI_OP_SET_EVENT_MASK, buf, NULL);

	buf = bt_hci_cmd_create(BT_HCI_OP_HOST_BUFFER_SIZE, sizeof(*hbs));
	if (!buf) {
		return -ENOBUFS;
	}

	hbs = bt_buf_add(buf, sizeof(*hbs));
	memset(hbs, 0, sizeof(*hbs));
	hbs->acl_mtu = sys_cpu_to_le16(BT_BUF_MAX_DATA -
				       sizeof(struct bt_hci_acl_hdr) -
				       bt_dev.drv->head_reserve);
	hbs->acl_pkts = sys_cpu_to_le16(ACL_IN_MAX);

	err = bt_hci_cmd_send(BT_HCI_OP_HOST_BUFFER_SIZE, buf);
	if (err) {
		return err;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_CTL_TO_HOST_FLOW, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	enable = bt_buf_add(buf, sizeof(*enable));
	*enable = 0x01;
	err = bt_hci_cmd_send_sync(BT_HCI_OP_SET_CTL_TO_HOST_FLOW, buf, NULL);
	if (err) {
		return err;
	}

	if (lmp_bredr_capable(bt_dev)) {
		struct bt_hci_cp_write_le_host_supp *cp;

		/* Use BR/EDR buffer size if LE reports zero buffers */
		if (!bt_dev.le_mtu) {
			err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BUFFER_SIZE,
						   NULL, &rsp);
			if (err) {
				return err;
			}
			read_buffer_size_complete(rsp);
			bt_buf_put(rsp);
		}

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_WRITE_LE_HOST_SUPP,
					sizeof(*cp));
		if (!buf) {
			return -ENOBUFS;
		}

		cp = bt_buf_add(buf, sizeof*cp);

		/* Excplicitly enable LE for dual-mode controllers */
		cp->le = 0x01;
		cp->simul = 0x00;
		bt_hci_cmd_send_sync(BT_HCI_OP_LE_WRITE_LE_HOST_SUPP, buf,
				     NULL);
	}

	BT_DBG("HCI ver %u rev %u, manufacturer %u\n", bt_dev.hci_version,
	       bt_dev.hci_revision, bt_dev.manufacturer);
	BT_DBG("ACL buffers: pkts %u mtu %u\n", bt_dev.le_pkts, bt_dev.le_mtu);

	/* Initialize & prime the semaphore for counting controller-side
	 * available ACL packet buffers.
	 */
	nano_sem_init(&bt_dev.le_pkts_sem);
	for (i = 0; i < bt_dev.le_pkts; i++) {
		nano_sem_give(&bt_dev.le_pkts_sem);
	}

	return 0;
}

/* Interface to HCI driver layer */

void bt_recv(struct bt_buf *buf)
{
	struct bt_hci_evt_hdr *hdr;

	BT_DBG("buf %p len %u\n", buf, buf->len);

	if (buf->type == BT_ACL_IN) {
		nano_fifo_put(&bt_dev.rx_queue, buf);
		return;
	}

	if (buf->type != BT_EVT) {
		BT_ERR("Invalid buf type %u\n", buf->type);
		bt_buf_put(buf);
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
		BT_ERR("HCI driver open failed (%d)\n", err);
		return err;
	}

	err = hci_init();
	if (err) {
		return err;
	}

	return bt_l2cap_init();
}

static void hci_rx_fiber(bt_ready_cb_t ready_cb)
{
	struct bt_buf *buf;

	BT_DBG("started\n");

	if (ready_cb) {
		ready_cb(bt_init());
	}

	while (1) {
		BT_DBG("calling fifo_get_wait\n");
		buf = nano_fifo_get_wait(&bt_dev.rx_queue);

		BT_DBG("buf %p type %u len %u\n", buf, buf->type, buf->len);

		switch (buf->type) {
		case BT_ACL_IN:
			hci_acl(buf);
			break;
		case BT_EVT:
			hci_event(buf);
			break;
		default:
			BT_ERR("Unknown buf type %u\n", buf->type);
			bt_buf_put(buf);
			break;
		}

	}
}

int bt_enable(bt_ready_cb_t cb)
{
	if (!bt_dev.drv) {
		BT_ERR("No HCI driver registered\n");
		return -ENODEV;
	}

	bt_buf_init(ACL_IN_MAX, ACL_OUT_MAX);

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

int bt_start_advertising(uint8_t type, const struct bt_eir *ad,
			 const struct bt_eir *sd)
{
	struct bt_buf *buf;
	struct bt_hci_cp_le_set_adv_data *set_data;
	struct bt_hci_cp_le_set_adv_data *scan_rsp;
	struct bt_hci_cp_le_set_adv_parameters *set_param;
	uint8_t adv_enable;
	int i, err;

	if (!ad) {
		goto send_scan_rsp;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_DATA, sizeof(*set_data));
	if (!buf) {
		return -ENOBUFS;
	}

	set_data = bt_buf_add(buf, sizeof(*set_data));

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

	scan_rsp = bt_buf_add(buf, sizeof(*scan_rsp));

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

	set_param = bt_buf_add(buf, sizeof(*set_param));

	memset(set_param, 0, sizeof(*set_param));
	set_param->min_interval		= sys_cpu_to_le16(0x0800);
	set_param->max_interval		= sys_cpu_to_le16(0x0800);
	set_param->type			= type;
	set_param->channel_map		= 0x07;

	bt_hci_cmd_send(BT_HCI_OP_LE_SET_ADV_PARAMETERS, buf);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	adv_enable = 0x01;
	memcpy(bt_buf_add(buf, 1), &adv_enable, 1);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	atomic_set_bit(bt_dev.flags, BT_DEV_ADVERTISING);

	return 0;
}

int bt_stop_advertising(void)
{
	struct bt_buf *buf;
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
	memcpy(bt_buf_add(buf, 1), &adv_enable, 1);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_ADVERTISING);

	return 0;
}

int bt_start_scanning(bt_scan_filter_dup_t filter, bt_le_scan_cb_t cb)
{
	/* Return if active scan is already enabled */
	if (scan_dev_found_cb) {
		return -EALREADY;
	}

	scan_dev_found_cb = cb;
	if (filter == BT_SCAN_FILTER_DUP_DISABLE) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP);
	}

	return bt_le_scan_update();
}

int bt_stop_scanning(void)
{
	/* Return if active scanning is already disabled */
	if (!scan_dev_found_cb) {
		return -EALREADY;
	}

	scan_dev_found_cb = NULL;
	atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP);

	return bt_le_scan_update();
}
