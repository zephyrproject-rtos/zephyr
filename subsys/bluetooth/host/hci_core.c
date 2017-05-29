/* hci_core.c - HCI core Bluetooth handling */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <misc/byteorder.h>
#include <misc/stack.h>
#include <misc/__assert.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_driver.h>
#include <bluetooth/storage.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_DEBUG_HCI_CORE)
#include "common/log.h"

#include "common/rpa.h"
#include "keys.h"
#include "monitor.h"
#include "hci_core.h"
#include "hci_ecc.h"
#include "ecc.h"

#include "conn_internal.h"
#include "l2cap_internal.h"
#include "smp.h"

#if defined(CONFIG_BLUETOOTH_HOST_CRYPTO)
#include "crypto.h"
#endif

/* Peripheral timeout to initialize Connection Parameter Update procedure */
#define CONN_UPDATE_TIMEOUT  K_SECONDS(5)
#define RPA_TIMEOUT          K_SECONDS(CONFIG_BLUETOOTH_RPA_TIMEOUT)

#define HCI_CMD_TIMEOUT      K_SECONDS(10)

/* Stacks for the threads */
#if !defined(CONFIG_BLUETOOTH_RECV_IS_RX_THREAD)
static struct k_thread rx_thread_data;
static BT_STACK_NOINIT(rx_thread_stack, CONFIG_BLUETOOTH_RX_STACK_SIZE);
#endif
static struct k_thread tx_thread_data;
static BT_STACK_NOINIT(tx_thread_stack, CONFIG_BLUETOOTH_HCI_TX_STACK_SIZE);

static void init_work(struct k_work *work);

struct bt_dev bt_dev = {
	.init          = K_WORK_INITIALIZER(init_work),
	/* Give cmd_sem allowing to send first HCI_Reset cmd, the only
	 * exception is if the controller requests to wait for an
	 * initial Command Complete for NOP.
	 */
#if !defined(CONFIG_BLUETOOTH_WAIT_NOP)
	.ncmd_sem      = K_SEM_INITIALIZER(bt_dev.ncmd_sem, 1, 1),
#else
	.ncmd_sem      = K_SEM_INITIALIZER(bt_dev.ncmd_sem, 0, 1),
#endif
	.cmd_tx_queue  = K_FIFO_INITIALIZER(bt_dev.cmd_tx_queue),
#if !defined(CONFIG_BLUETOOTH_RECV_IS_RX_THREAD)
	.rx_queue      = K_FIFO_INITIALIZER(bt_dev.rx_queue),
#endif
};

static bt_ready_cb_t ready_cb;

const struct bt_storage *bt_storage;

static bt_le_scan_cb_t *scan_dev_found_cb;

static u8_t pub_key[64];
static struct bt_pub_key_cb *pub_key_cb;
static bt_dh_key_cb_t dh_key_cb;

#if defined(CONFIG_BLUETOOTH_BREDR)
static bt_br_discovery_cb_t *discovery_cb;
struct bt_br_discovery_result *discovery_results;
static size_t discovery_results_size;
static size_t discovery_results_count;
#endif /* CONFIG_BLUETOOTH_BREDR */

struct cmd_data {
	/** BT_BUF_CMD */
	u8_t  type;

	/** HCI status of the command completion */
	u8_t  status;

	/** The command OpCode that the buffer contains */
	u16_t opcode;

	/** Used by bt_hci_cmd_send_sync. */
	struct k_sem *sync;
};

struct acl_data {
	/** BT_BUF_ACL_IN */
	u8_t  type;

	/** ACL connection handle */
	u16_t handle;
};

#define cmd(buf) ((struct cmd_data *)net_buf_user_data(buf))
#define acl(buf) ((struct acl_data *)net_buf_user_data(buf))

/* HCI command buffers. Derive the needed size from BT_BUF_RX_SIZE since
 * the same buffer is also used for the response.
 */
#define CMD_BUF_SIZE BT_BUF_RX_SIZE
NET_BUF_POOL_DEFINE(hci_cmd_pool, CONFIG_BLUETOOTH_HCI_CMD_COUNT,
		    CMD_BUF_SIZE, sizeof(struct cmd_data), NULL);

NET_BUF_POOL_DEFINE(hci_rx_pool, CONFIG_BLUETOOTH_RX_BUF_COUNT,
		    BT_BUF_RX_SIZE, BT_BUF_USER_DATA_MIN, NULL);

#if defined(CONFIG_BLUETOOTH_HCI_ACL_FLOW_CONTROL)
static void report_completed_packet(struct net_buf *buf)
{

	struct bt_hci_cp_host_num_completed_packets *cp;
	u16_t handle = acl(buf)->handle;
	struct bt_hci_handle_count *hc;

	net_buf_destroy(buf);

	/* Do nothing if controller to host flow control is not supported */
	if (!(bt_dev.supported_commands[10] & 0x20)) {
		return;
	}

	BT_DBG("Reporting completed packet for handle %u", handle);

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

#define ACL_IN_SIZE BT_L2CAP_BUF_SIZE(CONFIG_BLUETOOTH_L2CAP_RX_MTU)
NET_BUF_POOL_DEFINE(acl_in_pool, CONFIG_BLUETOOTH_ACL_RX_COUNT, ACL_IN_SIZE,
		    BT_BUF_USER_DATA_MIN, report_completed_packet);
#endif /* CONFIG_BLUETOOTH_HCI_ACL_FLOW_CONTROL */

struct net_buf *bt_hci_cmd_create(u16_t opcode, u8_t param_len)
{
	struct bt_hci_cmd_hdr *hdr;
	struct net_buf *buf;

	BT_DBG("opcode 0x%04x param_len %u", opcode, param_len);

	buf = net_buf_alloc(&hci_cmd_pool, K_FOREVER);
	__ASSERT_NO_MSG(buf);

	BT_DBG("buf %p", buf);

	net_buf_reserve(buf, CONFIG_BLUETOOTH_HCI_RESERVE);

	cmd(buf)->type = BT_BUF_CMD;
	cmd(buf)->opcode = opcode;
	cmd(buf)->sync = NULL;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->opcode = sys_cpu_to_le16(opcode);
	hdr->param_len = param_len;

	return buf;
}

int bt_hci_cmd_send(u16_t opcode, struct net_buf *buf)
{
	if (!buf) {
		buf = bt_hci_cmd_create(opcode, 0);
		if (!buf) {
			return -ENOBUFS;
		}
	}

	BT_DBG("opcode 0x%04x len %u", opcode, buf->len);

	/* Host Number of Completed Packets can ignore the ncmd value
	 * and does not generate any cmd complete/status events.
	 */
	if (opcode == BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS) {
		int err;

		err = bt_send(buf);
		if (err) {
			BT_ERR("Unable to send to driver (err %d)", err);
			net_buf_unref(buf);
		}

		return err;
	}

	net_buf_put(&bt_dev.cmd_tx_queue, buf);

	return 0;
}

int bt_hci_cmd_send_sync(u16_t opcode, struct net_buf *buf,
			 struct net_buf **rsp)
{
	struct k_sem sync_sem;
	int err;

	if (!buf) {
		buf = bt_hci_cmd_create(opcode, 0);
		if (!buf) {
			return -ENOBUFS;
		}
	}

	BT_DBG("buf %p opcode 0x%04x len %u", buf, opcode, buf->len);

	k_sem_init(&sync_sem, 0, 1);
	cmd(buf)->sync = &sync_sem;

	/* Make sure the buffer stays around until the command completes */
	net_buf_ref(buf);

	net_buf_put(&bt_dev.cmd_tx_queue, buf);

	err = k_sem_take(&sync_sem, HCI_CMD_TIMEOUT);
	__ASSERT(err == 0, "k_sem_take failed with err %d", err);

	BT_DBG("opcode 0x%04x status 0x%02x", opcode, cmd(buf)->status);

	if (cmd(buf)->status) {
		err = -EIO;
		net_buf_unref(buf);
	} else {
		err = 0;
		if (rsp) {
			*rsp = buf;
		} else {
			net_buf_unref(buf);
		}
	}

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
	scan_enable->filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE;
	scan_enable->enable = BT_HCI_LE_SCAN_DISABLE;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_SCAN_ENABLE, buf, &rsp);
	if (err) {
		return err;
	}

	/* Update scan state in case of success (0) status */
	err = rsp->data[0];
	if (!err) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_SCANNING);
		atomic_clear_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN);
	}

	net_buf_unref(rsp);

	return err;
}

static const bt_addr_le_t *find_id_addr(const bt_addr_le_t *addr)
{
	if (IS_ENABLED(CONFIG_BLUETOOTH_SMP)) {
		struct bt_keys *keys;

		keys = bt_keys_find_irk(addr);
		if (keys) {
			BT_DBG("Identity %s matched RPA %s",
			       bt_addr_le_str(&keys->addr),
			       bt_addr_le_str(addr));
			return &keys->addr;
		}
	}

	return addr;
}

static int set_advertise_enable(bool enable)
{
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	if (enable) {
		net_buf_add_u8(buf, BT_HCI_LE_ADV_ENABLE);
	} else {
		net_buf_add_u8(buf, BT_HCI_LE_ADV_DISABLE);
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	if (enable) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ADVERTISING);
	} else {
		atomic_clear_bit(bt_dev.flags, BT_DEV_ADVERTISING);
	}

	return 0;
}

static int set_random_address(const bt_addr_t *addr)
{
	struct net_buf *buf;
	int err;

	BT_DBG("%s", bt_addr_str(addr));

	/* Do nothing if we already have the right address */
	if (!bt_addr_cmp(addr, &bt_dev.random_addr.a)) {
		return 0;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_RANDOM_ADDRESS, sizeof(*addr));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_mem(buf, addr, sizeof(*addr));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_RANDOM_ADDRESS, buf, NULL);
	if (err) {
		return err;
	}

	bt_addr_copy(&bt_dev.random_addr.a, addr);
	bt_dev.random_addr.type = BT_ADDR_LE_RANDOM;
	return 0;
}

#if defined(CONFIG_BLUETOOTH_PRIVACY)
/* this function sets new RPA only if current one is no longer valid */
static int le_set_private_addr(void)
{
	bt_addr_t rpa;
	int err;

	/* check if RPA is valid */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_RPA_VALID)) {
		return 0;
	}

	err = bt_rpa_create(bt_dev.irk, &rpa);
	if (!err) {
		err = set_random_address(&rpa);
		if (!err) {
			atomic_set_bit(bt_dev.flags, BT_DEV_RPA_VALID);
		}
	}

	/* restart timer even if failed to set new RPA */
	k_delayed_work_submit(&bt_dev.rpa_update, RPA_TIMEOUT);

	return err;
}

static void rpa_timeout(struct k_work *work)
{
	BT_DBG("");

	/* Invalidate RPA */
	atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);

	/*
	 * we need to update rpa only if advertising is ongoing, with
	 * BT_DEV_KEEP_ADVERTISING flag is handled in disconnected event
	 */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		/* make sure new address is used */
		set_advertise_enable(false);
		le_set_private_addr();
		set_advertise_enable(true);
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN)) {
		/* TODO do we need to toggle scan? */
		le_set_private_addr();
	}
}
#else
static int le_set_private_addr(void)
{
	bt_addr_t nrpa;
	int err;

	err = bt_rand(nrpa.val, sizeof(nrpa.val));
	if (err) {
		return err;
	}

	nrpa.val[5] &= 0x3f;

	return set_random_address(&nrpa);
}
#endif

#if defined(CONFIG_BLUETOOTH_CONN)
static void hci_acl(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr = (void *)buf->data;
	u16_t handle, len = sys_le16_to_cpu(hdr->len);
	struct bt_conn *conn;
	u8_t flags;

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
	u16_t i, num_handles = sys_le16_to_cpu(evt->num_handles);

	BT_DBG("num_handles %u", num_handles);

	for (i = 0; i < num_handles; i++) {
		u16_t handle, count;
		struct bt_conn *conn;
		unsigned int key;

		handle = sys_le16_to_cpu(evt->h[i].handle);
		count = sys_le16_to_cpu(evt->h[i].count);

		BT_DBG("handle %u count %u", handle, count);

		key = irq_lock();

		conn = bt_conn_lookup_handle(handle);
		if (!conn) {
			BT_ERR("No connection for handle %u", handle);
			irq_unlock(key);
			continue;
		}

		irq_unlock(key);

		while (count--) {
			sys_snode_t *node;

			key = irq_lock();
			node = sys_slist_get(&conn->tx_pending);
			irq_unlock(key);

			if (!node) {
				BT_ERR("packets count mismatch");
				break;
			}

			k_fifo_put(&conn->tx_notify, node);
			k_sem_give(bt_conn_get_pkts(conn));
		}

		bt_conn_unref(conn);
	}
}

static int hci_le_create_conn(const struct bt_conn *conn)
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
	cp->scan_interval = sys_cpu_to_le16(BT_GAP_SCAN_FAST_INTERVAL);
	cp->scan_window = cp->scan_interval;

	bt_addr_le_copy(&cp->peer_addr, &conn->le.resp_addr);
	cp->own_addr_type = conn->le.init_addr.type;
	cp->conn_interval_min = sys_cpu_to_le16(conn->le.interval_min);
	cp->conn_interval_max = sys_cpu_to_le16(conn->le.interval_max);
	cp->conn_latency = sys_cpu_to_le16(conn->le.latency);
	cp->supervision_timeout = sys_cpu_to_le16(conn->le.timeout);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CONN, buf, NULL);
}

static void hci_disconn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_disconn_complete *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u reason %u", evt->status, handle,
	       evt->reason);

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		goto advertise;
	}

	conn->err = evt->reason;

	/* Check stacks usage (no-ops if not enabled) */
	k_call_stacks_analyze();
#if !defined(CONFIG_BLUETOOTH_RECV_IS_RX_THREAD)
	stack_analyze("rx stack", rx_thread_stack, sizeof(rx_thread_stack));
#endif
	stack_analyze("tx stack", tx_thread_stack,
		      sizeof(tx_thread_stack));

	bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
	conn->handle = 0;

	if (conn->type != BT_CONN_TYPE_LE) {
#if defined(CONFIG_BLUETOOTH_BREDR)
		if (conn->type == BT_CONN_TYPE_SCO) {
			bt_sco_cleanup(conn);
			return;
		}
		/*
		 * If only for one connection session bond was set, clear keys
		 * database row for this connection.
		 */
		if (conn->type == BT_CONN_TYPE_BR &&
		    atomic_test_and_clear_bit(conn->flags, BT_CONN_BR_NOBOND)) {
			bt_keys_link_key_clear(conn->br.link_key);
		}
#endif
		bt_conn_unref(conn);
		return;
	}

	if (atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT)) {
		bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);
		bt_le_scan_update(false);
	}

	bt_conn_unref(conn);

advertise:
	if (atomic_test_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING) &&
	    !atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		if (IS_ENABLED(CONFIG_BLUETOOTH_PRIVACY)) {
			le_set_private_addr();
		}

		set_advertise_enable(true);
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

static int hci_le_set_data_len(struct bt_conn *conn)
{
	struct bt_hci_rp_le_read_max_data_len *rp;
	struct bt_hci_cp_le_set_data_len *cp;
	struct net_buf *buf, *rsp;
	u16_t tx_octets, tx_time;
	int err;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_MAX_DATA_LEN, NULL, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	tx_octets = sys_le16_to_cpu(rp->max_tx_octets);
	tx_time = sys_le16_to_cpu(rp->max_tx_time);
	net_buf_unref(rsp);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_DATA_LEN, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->tx_octets = sys_cpu_to_le16(tx_octets);
	cp->tx_time = sys_cpu_to_le16(tx_time);
	err = bt_hci_cmd_send(BT_HCI_OP_LE_SET_DATA_LEN, buf);
	if (err) {
		return err;
	}

	return 0;
}

static int hci_le_set_phy(struct bt_conn *conn)
{
	struct bt_hci_cp_le_set_phy *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PHY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->all_phys = 0;
	cp->tx_phys = BT_HCI_LE_PHY_PREFER_2M;
	cp->rx_phys = BT_HCI_LE_PHY_PREFER_2M;
	cp->phy_opts = BT_HCI_LE_PHY_CODED_ANY;
	bt_hci_cmd_send(BT_HCI_OP_LE_SET_PHY, buf);

	return 0;
}

static void update_conn_param(struct bt_conn *conn)
{
	/*
	 * Core 4.2 Vol 3, Part C, 9.3.12.2
	 * The Peripheral device should not perform a Connection Parameter
	 * Update procedure within 5 s after establishing a connection.
	 */
	k_delayed_work_submit(&conn->le.update_work,
				 conn->role == BT_HCI_ROLE_MASTER ? K_NO_WAIT :
				 CONN_UPDATE_TIMEOUT);
}

static void le_conn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	const bt_addr_le_t *id_addr;
	struct bt_conn *conn;
	int err;

	BT_DBG("status %u handle %u role %u %s", evt->status, handle,
	       evt->role, bt_addr_le_str(&evt->peer_addr));

	if (evt->status) {
		/*
		 * if there was an error we are only interested in pending
		 * connection so there is no need to check ID address as
		 * only one connection can be in that state
		 *
		 * Depending on error code address might not be valid anyway.
		 */
		conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT);
		if (!conn) {
			return;
		}

		conn->err = evt->status;

		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);

		/* Drop the reference got by lookup call in CONNECT state.
		 * We are now in DISCONNECTED state since no successful LE
		 * link been made.
		 */
		bt_conn_unref(conn);

		return;
	}

	id_addr = find_id_addr(&evt->peer_addr);

	/*
	 * Make lookup to check if there's a connection object in
	 * CONNECT state associated with passed peer LE address.
	 */
	conn = bt_conn_lookup_state_le(id_addr, BT_CONN_CONNECT);

	if (evt->role == BT_CONN_ROLE_SLAVE) {
		/*
		 * clear advertising even if we are not able to add connection
		 * object to keep host in sync with controller state
		 */
		atomic_clear_bit(bt_dev.flags, BT_DEV_ADVERTISING);

		/* only for slave we may need to add new connection */
		if (!conn) {
			conn = bt_conn_add_le(id_addr);
		}
	}

	if (!conn) {
		BT_ERR("Unable to add new conn for handle %u", handle);
		return;
	}

	conn->handle   = handle;
	bt_addr_le_copy(&conn->le.dst, id_addr);
	conn->le.interval = sys_le16_to_cpu(evt->interval);
	conn->le.latency = sys_le16_to_cpu(evt->latency);
	conn->le.timeout = sys_le16_to_cpu(evt->supv_timeout);
	conn->role = evt->role;

	/*
	 * Use connection address (instead of identity address) as initiator
	 * or responder address. Only slave needs to be updated. For master all
	 * was set during outgoing connection creation.
	 */
	if (conn->role == BT_HCI_ROLE_SLAVE) {
		bt_addr_le_copy(&conn->le.init_addr, &evt->peer_addr);

		/* TODO Handle the probability that random address could have
		 * been updated by rpa_timeout or numerous other places it is
		 * called in this file before le_conn_complete is processed
		 * here.
		 */
		if (IS_ENABLED(CONFIG_BLUETOOTH_PRIVACY)) {
			bt_addr_le_copy(&conn->le.resp_addr,
					&bt_dev.random_addr);
		} else {
			bt_addr_le_copy(&conn->le.resp_addr, &bt_dev.id_addr);
		}

		/* if the controller supports, lets advertise for another
		 * slave connection.
		 * check for connectable advertising state is sufficient as
		 * this is how this le connection complete for slave occurred.
		 */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING) &&
		    BT_LE_STATES_SLAVE_CONN_ADV(bt_dev.le.states)) {
			if (IS_ENABLED(CONFIG_BLUETOOTH_PRIVACY)) {
				le_set_private_addr();
			}

			set_advertise_enable(true);
		}

	}

	bt_conn_set_state(conn, BT_CONN_CONNECTED);

	/*
	 * it is possible that connection was disconnected directly from
	 * connected callback so we must check state before doing connection
	 * parameters update
	 */
	if (conn->state != BT_CONN_CONNECTED) {
		goto done;
	}

	if ((evt->role == BT_HCI_ROLE_MASTER) ||
	    BT_FEAT_LE_SLAVE_FEATURE_XCHG(bt_dev.le.features)) {
		err = hci_le_read_remote_features(conn);
		if (!err) {
			goto done;
		}
	}

	if (BT_FEAT_LE_PHY_2M(bt_dev.le.features)) {
		err = hci_le_set_phy(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_PHY_UPDATE);
			goto done;
		}
	}

	if (BT_FEAT_LE_DLE(bt_dev.le.features)) {
		err = hci_le_set_data_len(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_DATA_LEN);
			goto done;
		}
	}

	update_conn_param(conn);

done:
	bt_conn_unref(conn);
	bt_le_scan_update(false);
}

static void le_remote_feat_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_remote_feat_complete *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
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

	if (BT_FEAT_LE_PHY_2M(bt_dev.le.features) &&
	    BT_FEAT_LE_PHY_2M(conn->le.features)) {
		int err;

		err = hci_le_set_phy(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_PHY_UPDATE);
			goto done;
		}
	}

	if (BT_FEAT_LE_DLE(bt_dev.le.features) &&
	    BT_FEAT_LE_DLE(conn->le.features)) {
		int err;

		err = hci_le_set_data_len(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_DATA_LEN);
			goto done;
		}
	}

	update_conn_param(conn);

done:
	bt_conn_unref(conn);
}

static void le_data_len_change(struct net_buf *buf)
{
	struct bt_hci_evt_le_data_len_change *evt = (void *)buf->data;
	u16_t max_tx_octets = sys_le16_to_cpu(evt->max_tx_octets);
	u16_t max_rx_octets = sys_le16_to_cpu(evt->max_rx_octets);
	u16_t max_tx_time = sys_le16_to_cpu(evt->max_tx_time);
	u16_t max_rx_time = sys_le16_to_cpu(evt->max_rx_time);
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	BT_DBG("max. tx: %u (%uus), max. rx: %u (%uus)", max_tx_octets,
	       max_tx_time, max_rx_octets, max_rx_time);

	if (!atomic_test_and_clear_bit(conn->flags, BT_CONN_AUTO_DATA_LEN)) {
		goto done;
	}

	update_conn_param(conn);

done:
	bt_conn_unref(conn);
}

static void le_phy_update_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_phy_update_complete *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	BT_DBG("PHY updated: status: 0x%x, tx: %u, rx: %u",
	       evt->status, evt->tx_phy, evt->rx_phy);

	if (!atomic_test_and_clear_bit(conn->flags, BT_CONN_AUTO_PHY_UPDATE)) {
		goto done;
	}

	if (BT_FEAT_LE_DLE(bt_dev.le.features) &&
	    BT_FEAT_LE_DLE(conn->le.features)) {
		int err;

		err = hci_le_set_data_len(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_DATA_LEN);
			goto done;
		}
	}

	update_conn_param(conn);

done:
	bt_conn_unref(conn);
}

bool bt_le_conn_params_valid(const struct bt_le_conn_param *param)
{
	/* All limits according to BT Core spec 5.0 [Vol 2, Part E, 7.8.12] */

	if (param->interval_min > param->interval_max ||
	    param->interval_min < 6 || param->interval_max > 3200) {
		return false;
	}

	if (param->latency > 499) {
		return false;
	}

	if (param->timeout < 10 || param->timeout > 3200 ||
	    ((4 * param->timeout) <=
	     ((1 + param->latency) * param->interval_max))) {
		return false;
	}

	return true;
}

static int le_conn_param_neg_reply(u16_t handle, u8_t reason)
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

static int le_conn_param_req_reply(u16_t handle,
				   const struct bt_le_conn_param *param)
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
	cp->interval_min = sys_cpu_to_le16(param->interval_min);
	cp->interval_max = sys_cpu_to_le16(param->interval_max);
	cp->latency = sys_cpu_to_le16(param->latency);
	cp->timeout = sys_cpu_to_le16(param->timeout);

	return bt_hci_cmd_send(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY, buf);
}

static int le_conn_param_req(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_param_req *evt = (void *)buf->data;
	struct bt_le_conn_param param;
	struct bt_conn *conn;
	u16_t handle;
	int err;

	handle = sys_le16_to_cpu(evt->handle);
	param.interval_min = sys_le16_to_cpu(evt->interval_min);
	param.interval_max = sys_le16_to_cpu(evt->interval_max);
	param.latency = sys_le16_to_cpu(evt->latency);
	param.timeout = sys_le16_to_cpu(evt->timeout);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return le_conn_param_neg_reply(handle,
					       BT_HCI_ERR_UNKNOWN_CONN_ID);
	}

	if (!le_param_req(conn, &param)) {
		err = le_conn_param_neg_reply(handle,
					      BT_HCI_ERR_INVALID_LL_PARAM);
	} else {
		err = le_conn_param_req_reply(handle, &param);
	}

	bt_conn_unref(conn);
	return err;
}

static void le_conn_update_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_update_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status %u, handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	if (!evt->status) {
		conn->le.interval = sys_le16_to_cpu(evt->interval);
		conn->le.latency = sys_le16_to_cpu(evt->latency);
		conn->le.timeout = sys_le16_to_cpu(evt->supv_timeout);
		notify_le_param_updated(conn);
	}

	bt_conn_unref(conn);
}

static void check_pending_conn(const bt_addr_le_t *id_addr,
			       const bt_addr_le_t *addr, u8_t evtype)
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

	conn = bt_conn_lookup_state_le(id_addr, BT_CONN_CONNECT_SCAN);
	if (!conn) {
		return;
	}

	if (bt_hci_stop_scanning()) {
		goto failed;
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_PRIVACY)) {
		if (le_set_private_addr()) {
			goto failed;
		}

		bt_addr_le_copy(&conn->le.init_addr, &bt_dev.random_addr);
	} else {
		/* If Static Random address is used as Identity address we
		 * need to restore it before creating connection. Otherwise
		 * NRPA used for active scan could be used for connection.
		 */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_ID_STATIC_RANDOM)) {
			set_random_address(&bt_dev.id_addr.a);
		}

		bt_addr_le_copy(&conn->le.init_addr, &bt_dev.id_addr);
	}

	bt_addr_le_copy(&conn->le.resp_addr, addr);

	if (hci_le_create_conn(conn)) {
		goto failed;
	}

	bt_conn_set_state(conn, BT_CONN_CONNECT);
	bt_conn_unref(conn);
	return;

failed:
	conn->err = BT_HCI_ERR_UNSPECIFIED;
	bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
	bt_conn_unref(conn);
	bt_le_scan_update(false);
}

#if defined(CONFIG_BLUETOOTH_HCI_ACL_FLOW_CONTROL)
static int set_flow_control(void)
{
	struct bt_hci_cp_host_buffer_size *hbs;
	struct net_buf *buf;
	int err;

	/* Check if host flow control is actually supported */
	if (!(bt_dev.supported_commands[10] & 0x20)) {
		BT_WARN("Controller to host flow control not supported");
		return 0;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_HOST_BUFFER_SIZE,
				sizeof(*hbs));
	if (!buf) {
		return -ENOBUFS;
	}

	hbs = net_buf_add(buf, sizeof(*hbs));
	memset(hbs, 0, sizeof(*hbs));
	hbs->acl_mtu = sys_cpu_to_le16(CONFIG_BLUETOOTH_L2CAP_RX_MTU +
				       sizeof(struct bt_l2cap_hdr));
	hbs->acl_pkts = sys_cpu_to_le16(CONFIG_BLUETOOTH_ACL_RX_COUNT);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_HOST_BUFFER_SIZE, buf, NULL);
	if (err) {
		return err;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_CTL_TO_HOST_FLOW, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_u8(buf, BT_HCI_CTL_TO_HOST_FLOW_ENABLE);
	return bt_hci_cmd_send_sync(BT_HCI_OP_SET_CTL_TO_HOST_FLOW, buf, NULL);
}
#endif /* CONFIG_BLUETOOTH_HCI_ACL_FLOW_CONTROL */
#endif /* CONFIG_BLUETOOTH_CONN */

#if defined(CONFIG_BLUETOOTH_BREDR)
static void reset_pairing(struct bt_conn *conn)
{
	atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING);
	atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING_INITIATOR);
	atomic_clear_bit(conn->flags, BT_CONN_BR_LEGACY_SECURE);

	/* Reset required security level to current operational */
	conn->required_sec_level = conn->sec_level;
}

static int reject_conn(const bt_addr_t *bdaddr, u8_t reason)
{
	struct bt_hci_cp_reject_conn_req *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_REJECT_CONN_REQ, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	cp->reason = reason;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_REJECT_CONN_REQ, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int accept_sco_conn(const bt_addr_t *bdaddr, struct bt_conn *sco_conn)
{
	struct bt_hci_cp_accept_sync_conn_req *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_ACCEPT_SYNC_CONN_REQ, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	cp->pkt_type = sco_conn->sco.pkt_type;
	cp->tx_bandwidth = 0x00001f40;
	cp->rx_bandwidth = 0x00001f40;
	cp->max_latency = 0x0007;
	cp->retrans_effort = 0x01;
	cp->content_format = BT_VOICE_CVSD_16BIT;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_ACCEPT_SYNC_CONN_REQ, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int accept_conn(const bt_addr_t *bdaddr)
{
	struct bt_hci_cp_accept_conn_req *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_ACCEPT_CONN_REQ, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	cp->role = BT_HCI_ROLE_SLAVE;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_ACCEPT_CONN_REQ, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static void bt_esco_conn_req(struct bt_hci_evt_conn_request *evt)
{
	struct bt_conn *sco_conn;

	sco_conn = bt_conn_add_sco(&evt->bdaddr, evt->link_type);
	if (!sco_conn) {
		reject_conn(&evt->bdaddr, BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	if (accept_sco_conn(&evt->bdaddr, sco_conn)) {
		BT_ERR("Error accepting connection from %s",
		       bt_addr_str(&evt->bdaddr));
		reject_conn(&evt->bdaddr, BT_HCI_ERR_UNSPECIFIED);
		bt_sco_cleanup(sco_conn);
		return;
	}

	sco_conn->role = BT_HCI_ROLE_SLAVE;
	bt_conn_set_state(sco_conn, BT_CONN_CONNECT);
	bt_conn_unref(sco_conn);
}

static void conn_req(struct net_buf *buf)
{
	struct bt_hci_evt_conn_request *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("conn req from %s, type 0x%02x", bt_addr_str(&evt->bdaddr),
	       evt->link_type);

	if (evt->link_type != BT_HCI_ACL) {
		bt_esco_conn_req(evt);
		return;
	}

	conn = bt_conn_add_br(&evt->bdaddr);
	if (!conn) {
		reject_conn(&evt->bdaddr, BT_HCI_ERR_INSUFFICIENT_RESOURCES);
		return;
	}

	accept_conn(&evt->bdaddr);
	conn->role = BT_HCI_ROLE_SLAVE;
	bt_conn_set_state(conn, BT_CONN_CONNECT);
	bt_conn_unref(conn);
}

static void update_sec_level_br(struct bt_conn *conn)
{
	if (!conn->encrypt) {
		conn->sec_level = BT_SECURITY_LOW;
		return;
	}

	if (conn->br.link_key) {
		if (atomic_test_bit(conn->br.link_key->flags,
				    BT_LINK_KEY_AUTHENTICATED)) {
			if (conn->encrypt == 0x02) {
				conn->sec_level = BT_SECURITY_FIPS;
			} else {
				conn->sec_level = BT_SECURITY_HIGH;
			}
		} else {
			conn->sec_level = BT_SECURITY_MEDIUM;
		}
	} else {
		BT_WARN("No BR/EDR link key found");
		conn->sec_level = BT_SECURITY_MEDIUM;
	}

	if (conn->required_sec_level > conn->sec_level) {
		BT_ERR("Failed to set required security level");
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
	}
}

static void synchronous_conn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_sync_conn_complete *evt = (void *)buf->data;
	struct bt_conn *sco_conn;
	u16_t handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status 0x%02x, handle %u, type 0x%02x", evt->status, handle,
	       evt->link_type);

	sco_conn = bt_conn_lookup_addr_sco(&evt->bdaddr);
	if (!sco_conn) {
		BT_ERR("Unable to find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	if (evt->status) {
		sco_conn->err = evt->status;
		bt_conn_set_state(sco_conn, BT_CONN_DISCONNECTED);
		bt_conn_unref(sco_conn);
		return;
	}

	sco_conn->handle = handle;
	bt_conn_set_state(sco_conn, BT_CONN_CONNECTED);
	bt_conn_unref(sco_conn);
}

static void conn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_conn_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	struct bt_hci_cp_read_remote_features *cp;
	u16_t handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status 0x%02x, handle %u, type 0x%02x", evt->status, handle,
	       evt->link_type);

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Unable to find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	if (evt->status) {
		conn->err = evt->status;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		bt_conn_unref(conn);
		return;
	}

	conn->handle = handle;
	conn->encrypt = evt->encr_enabled;
	update_sec_level_br(conn);
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_conn_unref(conn);

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_REMOTE_FEATURES, sizeof(*cp));
	if (!buf) {
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = evt->handle;

	bt_hci_cmd_send_sync(BT_HCI_OP_READ_REMOTE_FEATURES, buf, NULL);
}

static void pin_code_req(struct net_buf *buf)
{
	struct bt_hci_evt_pin_code_req *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("");

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	bt_conn_pin_code_req(conn);
	bt_conn_unref(conn);
}

static void link_key_notify(struct net_buf *buf)
{
	struct bt_hci_evt_link_key_notify *evt = (void *)buf->data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	BT_DBG("%s, link type 0x%02x", bt_addr_str(&evt->bdaddr), evt->key_type);

	if (!conn->br.link_key) {
		conn->br.link_key = bt_keys_get_link_key(&evt->bdaddr);
	}
	if (!conn->br.link_key) {
		BT_ERR("Can't update keys for %s", bt_addr_str(&evt->bdaddr));
		bt_conn_unref(conn);
		return;
	}

	/* clear any old Link Key flags */
	atomic_set(conn->br.link_key->flags, 0);

	switch (evt->key_type) {
	case BT_LK_COMBINATION:
		/*
		 * Setting Combination Link Key as AUTHENTICATED means it was
		 * successfully generated by 16 digits wide PIN code.
		 */
		if (atomic_test_and_clear_bit(conn->flags,
					      BT_CONN_BR_LEGACY_SECURE)) {
			atomic_set_bit(conn->br.link_key->flags,
				       BT_LINK_KEY_AUTHENTICATED);
		}
		memcpy(conn->br.link_key->val, evt->link_key, 16);
		break;
	case BT_LK_AUTH_COMBINATION_P192:
		atomic_set_bit(conn->br.link_key->flags,
			       BT_LINK_KEY_AUTHENTICATED);
		/* fall through */
	case BT_LK_UNAUTH_COMBINATION_P192:
		/* Mark no-bond so that link-key is removed on disconnection */
		if (bt_conn_ssp_get_auth(conn) < BT_HCI_DEDICATED_BONDING) {
			atomic_set_bit(conn->flags, BT_CONN_BR_NOBOND);
		}

		memcpy(conn->br.link_key->val, evt->link_key, 16);
		break;
	case BT_LK_AUTH_COMBINATION_P256:
		atomic_set_bit(conn->br.link_key->flags,
			       BT_LINK_KEY_AUTHENTICATED);
		/* fall through */
	case BT_LK_UNAUTH_COMBINATION_P256:
		atomic_set_bit(conn->br.link_key->flags, BT_LINK_KEY_SC);

		/* Mark no-bond so that link-key is removed on disconnection */
		if (bt_conn_ssp_get_auth(conn) < BT_HCI_DEDICATED_BONDING) {
			atomic_set_bit(conn->flags, BT_CONN_BR_NOBOND);
		}

		memcpy(conn->br.link_key->val, evt->link_key, 16);
		break;
	default:
		BT_WARN("Unsupported Link Key type %u", evt->key_type);
		memset(conn->br.link_key->val, 0,
		       sizeof(conn->br.link_key->val));
		break;
	}

	bt_conn_unref(conn);
}

static void link_key_neg_reply(const bt_addr_t *bdaddr)
{
	struct bt_hci_cp_link_key_neg_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_LINK_KEY_NEG_REPLY, sizeof(*cp));
	if (!buf) {
		BT_ERR("Out of command buffers");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	bt_hci_cmd_send_sync(BT_HCI_OP_LINK_KEY_NEG_REPLY, buf, NULL);
}

static void link_key_reply(const bt_addr_t *bdaddr, const u8_t *lk)
{
	struct bt_hci_cp_link_key_reply *cp;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_LINK_KEY_REPLY, sizeof(*cp));
	if (!buf) {
		BT_ERR("Out of command buffers");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	memcpy(cp->link_key, lk, 16);
	bt_hci_cmd_send_sync(BT_HCI_OP_LINK_KEY_REPLY, buf, NULL);
}

static void link_key_req(struct net_buf *buf)
{
	struct bt_hci_evt_link_key_req *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("%s", bt_addr_str(&evt->bdaddr));

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		link_key_neg_reply(&evt->bdaddr);
		return;
	}

	if (!conn->br.link_key) {
		conn->br.link_key = bt_keys_find_link_key(&evt->bdaddr);
	}

	if (!conn->br.link_key) {
		link_key_neg_reply(&evt->bdaddr);
		bt_conn_unref(conn);
		return;
	}

	/*
	 * Enforce regenerate by controller stronger link key since found one
	 * in database not covers requested security level.
	 */
	if (!atomic_test_bit(conn->br.link_key->flags,
			     BT_LINK_KEY_AUTHENTICATED) &&
	    conn->required_sec_level > BT_SECURITY_MEDIUM) {
		link_key_neg_reply(&evt->bdaddr);
		bt_conn_unref(conn);
		return;
	}

	link_key_reply(&evt->bdaddr, conn->br.link_key->val);
	bt_conn_unref(conn);
}

static void io_capa_neg_reply(const bt_addr_t *bdaddr, const u8_t reason)
{
	struct bt_hci_cp_io_capability_neg_reply *cp;
	struct net_buf *resp_buf;

	resp_buf = bt_hci_cmd_create(BT_HCI_OP_IO_CAPABILITY_NEG_REPLY,
				     sizeof(*cp));
	if (!resp_buf) {
		BT_ERR("Out of command buffers");
		return;
	}

	cp = net_buf_add(resp_buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, bdaddr);
	cp->reason = reason;
	bt_hci_cmd_send_sync(BT_HCI_OP_IO_CAPABILITY_NEG_REPLY, resp_buf, NULL);
}

static void io_capa_resp(struct net_buf *buf)
{
	struct bt_hci_evt_io_capa_resp *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("remote %s, IOcapa 0x%02x, auth 0x%02x",
	       bt_addr_str(&evt->bdaddr), evt->capability, evt->authentication);

	if (evt->authentication > BT_HCI_GENERAL_BONDING_MITM) {
		BT_ERR("Invalid remote authentication requirements");
		io_capa_neg_reply(&evt->bdaddr,
				  BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
		return;
	}

	if (evt->capability > BT_IO_NO_INPUT_OUTPUT) {
		BT_ERR("Invalid remote io capability requirements");
		io_capa_neg_reply(&evt->bdaddr,
				  BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
		return;
	}

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Unable to find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	conn->br.remote_io_capa = evt->capability;
	conn->br.remote_auth = evt->authentication;
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);
	bt_conn_unref(conn);
}

static void io_capa_req(struct net_buf *buf)
{
	struct bt_hci_evt_io_capa_req *evt = (void *)buf->data;
	struct net_buf *resp_buf;
	struct bt_conn *conn;
	struct bt_hci_cp_io_capability_reply *cp;
	u8_t auth;

	BT_DBG("");

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	resp_buf = bt_hci_cmd_create(BT_HCI_OP_IO_CAPABILITY_REPLY,
				     sizeof(*cp));
	if (!resp_buf) {
		BT_ERR("Out of command buffers");
		bt_conn_unref(conn);
		return;
	}

	/*
	 * Set authentication requirements when acting as pairing initiator to
	 * 'dedicated bond' with MITM protection set if local IO capa
	 * potentially allows it, and for acceptor, based on local IO capa and
	 * remote's authentication set.
	 */
	if (atomic_test_bit(conn->flags, BT_CONN_BR_PAIRING_INITIATOR)) {
		if (bt_conn_get_io_capa() != BT_IO_NO_INPUT_OUTPUT) {
			auth = BT_HCI_DEDICATED_BONDING_MITM;
		} else {
			auth = BT_HCI_DEDICATED_BONDING;
		}
	} else {
		auth = bt_conn_ssp_get_auth(conn);
	}

	cp = net_buf_add(resp_buf, sizeof(*cp));
	bt_addr_copy(&cp->bdaddr, &evt->bdaddr);
	cp->capability = bt_conn_get_io_capa();
	cp->authentication = auth;
	cp->oob_data = 0;
	bt_hci_cmd_send_sync(BT_HCI_OP_IO_CAPABILITY_REPLY, resp_buf, NULL);
	bt_conn_unref(conn);
}

static void ssp_complete(struct net_buf *buf)
{
	struct bt_hci_evt_ssp_complete *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("status %u", evt->status);

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	if (evt->status) {
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
	}

	bt_conn_unref(conn);
}

static void user_confirm_req(struct net_buf *buf)
{
	struct bt_hci_evt_user_confirm_req *evt = (void *)buf->data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	bt_conn_ssp_auth(conn, sys_le32_to_cpu(evt->passkey));
	bt_conn_unref(conn);
}

static void user_passkey_notify(struct net_buf *buf)
{
	struct bt_hci_evt_user_passkey_notify *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("");

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	bt_conn_ssp_auth(conn, sys_le32_to_cpu(evt->passkey));
	bt_conn_unref(conn);
}

static void user_passkey_req(struct net_buf *buf)
{
	struct bt_hci_evt_user_passkey_req *evt = (void *)buf->data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	bt_conn_ssp_auth(conn, 0);
	bt_conn_unref(conn);
}

struct discovery_priv {
	u16_t clock_offset;
	u8_t pscan_rep_mode;
	u8_t resolving;
} __packed;

static int request_name(const bt_addr_t *addr, u8_t pscan, u16_t offset)
{
	struct bt_hci_cp_remote_name_request *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_REMOTE_NAME_REQUEST, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	bt_addr_copy(&cp->bdaddr, addr);
	cp->pscan_rep_mode = pscan;
	cp->reserved = 0x00; /* reserver, should be set to 0x00 */
	cp->clock_offset = offset;

	return bt_hci_cmd_send_sync(BT_HCI_OP_REMOTE_NAME_REQUEST, buf, NULL);
}

#define EIR_SHORT_NAME		0x08
#define EIR_COMPLETE_NAME	0x09

static bool eir_has_name(const u8_t *eir)
{
	int len = 240;

	while (len) {
		if (len < 2) {
			break;
		};

		/* Look for early termination */
		if (!eir[0]) {
			break;
		}

		/* Check if field length is correct */
		if (eir[0] > len - 1) {
			break;
		}

		switch (eir[1]) {
		case EIR_SHORT_NAME:
		case EIR_COMPLETE_NAME:
			if (eir[0] > 1) {
				return true;
			}
			break;
		default:
			break;
		}

		/* Parse next AD Structure */
		len -= eir[0] + 1;
		eir += eir[0] + 1;
	}

	return false;
}

static void report_discovery_results(void)
{
	bool resolving_names = false;
	int i;

	for (i = 0; i < discovery_results_count; i++) {
		struct discovery_priv *priv;

		priv = (struct discovery_priv *)&discovery_results[i]._priv;

		if (eir_has_name(discovery_results[i].eir)) {
			continue;
		}

		if (request_name(&discovery_results[i].addr,
				 priv->pscan_rep_mode, priv->clock_offset)) {
			continue;
		}

		priv->resolving = 1;
		resolving_names = true;
	}

	if (resolving_names) {
		return;
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_INQUIRY);

	discovery_cb(discovery_results, discovery_results_count);

	discovery_cb = NULL;
	discovery_results = NULL;
	discovery_results_size = 0;
	discovery_results_count = 0;
}

static void inquiry_complete(struct net_buf *buf)
{
	struct bt_hci_evt_inquiry_complete *evt = (void *)buf->data;

	if (evt->status) {
		BT_ERR("Failed to complete inquiry");
	}

	report_discovery_results();
}

static struct bt_br_discovery_result *get_result_slot(const bt_addr_t *addr,
						      s8_t rssi)
{
	struct bt_br_discovery_result *result = NULL;
	size_t i;

	/* check if already present in results */
	for (i = 0; i < discovery_results_count; i++) {
		if (!bt_addr_cmp(addr, &discovery_results[i].addr)) {
			return &discovery_results[i];
		}
	}

	/* Pick a new slot (if available) */
	if (discovery_results_count < discovery_results_size) {
		bt_addr_copy(&discovery_results[discovery_results_count].addr,
			     addr);
		return &discovery_results[discovery_results_count++];
	}

	/* ignore if invalid RSSI */
	if (rssi == 0xff) {
		return NULL;
	}

	/*
	 * Pick slot with smallest RSSI that is smaller then passed RSSI
	 * TODO handle TX if present
	 */
	for (i = 0; i < discovery_results_size; i++) {
		if (discovery_results[i].rssi > rssi) {
			continue;
		}

		if (!result || result->rssi > discovery_results[i].rssi) {
			result = &discovery_results[i];
		}
	}

	if (result) {
		BT_DBG("Reusing slot (old %s rssi %d dBm)",
		       bt_addr_str(&result->addr), result->rssi);

		bt_addr_copy(&result->addr, addr);
	}

	return result;
}

static void inquiry_result_with_rssi(struct net_buf *buf)
{
	struct bt_hci_evt_inquiry_result_with_rssi *evt;
	u8_t num_reports = net_buf_pull_u8(buf);

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY)) {
		return;
	}

	BT_DBG("number of results: %u", num_reports);

	evt = (void *)buf->data;
	while (num_reports--) {
		struct bt_br_discovery_result *result;
		struct discovery_priv *priv;

		BT_DBG("%s rssi %d dBm", bt_addr_str(&evt->addr), evt->rssi);

		result = get_result_slot(&evt->addr, evt->rssi);
		if (!result) {
			return;
		}

		priv = (struct discovery_priv *)&result->_priv;
		priv->pscan_rep_mode = evt->pscan_rep_mode;
		priv->clock_offset = evt->clock_offset;

		memcpy(result->cod, evt->cod, 3);
		result->rssi = evt->rssi;

		/* we could reuse slot so make sure EIR is cleared */
		memset(result->eir, 0, sizeof(result->eir));

		/*
		 * Get next report iteration by moving pointer to right offset
		 * in buf according to spec 4.2, Vol 2, Part E, 7.7.33.
		 */
		evt = net_buf_pull(buf, sizeof(*evt));
	}
}

static void extended_inquiry_result(struct net_buf *buf)
{
	struct bt_hci_evt_extended_inquiry_result *evt = (void *)buf->data;
	struct bt_br_discovery_result *result;
	struct discovery_priv *priv;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY)) {
		return;
	}

	BT_DBG("%s rssi %d dBm", bt_addr_str(&evt->addr), evt->rssi);

	result = get_result_slot(&evt->addr, evt->rssi);
	if (!result) {
		return;
	}

	priv = (struct discovery_priv *)&result->_priv;
	priv->pscan_rep_mode = evt->pscan_rep_mode;
	priv->clock_offset = evt->clock_offset;

	result->rssi = evt->rssi;
	memcpy(result->cod, evt->cod, 3);
	memcpy(result->eir, evt->eir, sizeof(result->eir));
}

static void remote_name_request_complete(struct net_buf *buf)
{
	struct bt_hci_evt_remote_name_req_complete *evt = (void *)buf->data;
	struct bt_br_discovery_result *result;
	struct discovery_priv *priv;
	int eir_len = 240;
	u8_t *eir;
	int i;

	result = get_result_slot(&evt->bdaddr, 0xff);
	if (!result) {
		return;
	}

	priv = (struct discovery_priv *)&result->_priv;
	priv->resolving = 0;

	if (evt->status) {
		goto check_names;
	}

	eir = result->eir;

	while (eir_len) {
		if (eir_len < 2) {
			break;
		};

		/* Look for early termination */
		if (!eir[0]) {
			size_t name_len;

			eir_len -= 2;

			/* name is null terminated */
			name_len = strlen((const char *)evt->name);

			if (name_len > eir_len) {
				eir[0] = eir_len + 1;
				eir[1] = EIR_SHORT_NAME;
			} else {
				eir[0] = name_len + 1;
				eir[1] = EIR_SHORT_NAME;
			}

			memcpy(&eir[2], evt->name, eir[0] - 1);

			break;
		}

		/* Check if field length is correct */
		if (eir[0] > eir_len - 1) {
			break;
		}

		/* next EIR Structure */
		eir_len -= eir[0] + 1;
		eir += eir[0] + 1;
	}

check_names:
	/* if still waiting for names */
	for (i = 0; i < discovery_results_count; i++) {
		struct discovery_priv *priv;

		priv = (struct discovery_priv *)&discovery_results[i]._priv;

		if (priv->resolving) {
			return;
		}
	}

	/* all names resolved, report discovery results */
	atomic_clear_bit(bt_dev.flags, BT_DEV_INQUIRY);

	discovery_cb(discovery_results, discovery_results_count);

	discovery_cb = NULL;
	discovery_results = NULL;
	discovery_results_size = 0;
	discovery_results_count = 0;
}

static void link_encr(const u16_t handle)
{
	struct bt_hci_cp_set_conn_encrypt *encr;
	struct net_buf *buf;

	BT_DBG("");

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_CONN_ENCRYPT, sizeof(*encr));
	if (!buf) {
		BT_ERR("Out of command buffers");
		return;
	}

	encr = net_buf_add(buf, sizeof(*encr));
	encr->handle = sys_cpu_to_le16(handle);
	encr->encrypt = 0x01;

	bt_hci_cmd_send_sync(BT_HCI_OP_SET_CONN_ENCRYPT, buf, NULL);
}

static void auth_complete(struct net_buf *buf)
{
	struct bt_hci_evt_auth_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status %u, handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Can't find conn for handle %u", handle);
		return;
	}

	if (evt->status) {
		if (conn->state == BT_CONN_CONNECTED) {
			/*
			 * Inform layers above HCI about non-zero authentication
			 * status to make them able cleanup pending jobs.
			 */
			bt_l2cap_encrypt_change(conn, evt->status);
		}
		reset_pairing(conn);
	} else {
		link_encr(handle);
	}

	bt_conn_unref(conn);
}

static void read_remote_features_complete(struct net_buf *buf)
{
	struct bt_hci_evt_remote_features *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_hci_cp_read_remote_ext_features *cp;
	struct bt_conn *conn;

	BT_DBG("status %u handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Can't find conn for handle %u", handle);
		return;
	}

	if (evt->status) {
		goto done;
	}

	memcpy(conn->br.features[0], evt->features, sizeof(evt->features));

	if (!BT_FEAT_EXT_FEATURES(conn->br.features)) {
		goto done;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_REMOTE_EXT_FEATURES,
				sizeof(*cp));
	if (!buf) {
		goto done;
	}

	/* Read remote host features (page 1) */
	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = evt->handle;
	cp->page = 0x01;

	bt_hci_cmd_send_sync(BT_HCI_OP_READ_REMOTE_EXT_FEATURES, buf, NULL);

done:
	bt_conn_unref(conn);
}

static void read_remote_ext_features_complete(struct net_buf *buf)
{
	struct bt_hci_evt_remote_ext_features *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Can't find conn for handle %u", handle);
		return;
	}

	if (!evt->status && evt->page == 0x01) {
		memcpy(conn->br.features[1], evt->features,
		       sizeof(conn->br.features[1]));
	}

	bt_conn_unref(conn);
}

static void role_change(struct net_buf *buf)
{
	struct bt_hci_evt_role_change *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("status %u role %u addr %s", evt->status, evt->role,
	       bt_addr_str(&evt->bdaddr));

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	if (evt->role) {
		conn->role = BT_CONN_ROLE_SLAVE;
	} else {
		conn->role = BT_CONN_ROLE_MASTER;
	}

	bt_conn_unref(conn);
}
#endif /* CONFIG_BLUETOOTH_BREDR */

#if defined(CONFIG_BLUETOOTH_SMP)
static void update_sec_level(struct bt_conn *conn)
{
	if (!conn->encrypt) {
		conn->sec_level = BT_SECURITY_LOW;
		return;
	}

	if (conn->le.keys && atomic_test_bit(conn->le.keys->flags,
					     BT_KEYS_AUTHENTICATED)) {
		if (conn->le.keys->keys & BT_KEYS_LTK_P256) {
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
#endif /* CONFIG_BLUETOOTH_SMP */

#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
static void hci_encrypt_change(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_change *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
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
		if (conn->type == BT_CONN_TYPE_LE) {
			/* reset required security level in case of error */
			conn->required_sec_level = conn->sec_level;
#if defined(CONFIG_BLUETOOTH_BREDR)
		} else {
			bt_l2cap_encrypt_change(conn, evt->status);
			reset_pairing(conn);
#endif /* CONFIG_BLUETOOTH_BREDR */
		}
		bt_conn_unref(conn);
		return;
	}

	conn->encrypt = evt->encrypt;

#if defined(CONFIG_BLUETOOTH_SMP)
	if (conn->type == BT_CONN_TYPE_LE) {
		/*
		 * we update keys properties only on successful encryption to
		 * avoid losing valid keys if encryption was not successful.
		 *
		 * Update keys with last pairing info for proper sec level
		 * update. This is done only for LE transport, for BR/EDR keys
		 * are updated on HCI 'Link Key Notification Event'
		 */
		if (conn->encrypt) {
			bt_smp_update_keys(conn);
		}
		update_sec_level(conn);
	}
#endif /* CONFIG_BLUETOOTH_SMP */
#if defined(CONFIG_BLUETOOTH_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		update_sec_level_br(conn);

		if (IS_ENABLED(CONFIG_BLUETOOTH_SMP)) {
			/*
			 * Start SMP over BR/EDR if we are pairing and are
			 * master on the link
			 */
			if (atomic_test_bit(conn->flags, BT_CONN_BR_PAIRING) &&
			    conn->role == BT_CONN_ROLE_MASTER) {
				bt_smp_br_send_pairing_req(conn);
			}
		}

		reset_pairing(conn);
	}
#endif /* CONFIG_BLUETOOTH_BREDR */

	bt_l2cap_encrypt_change(conn, evt->status);
	bt_conn_security_changed(conn);

	bt_conn_unref(conn);
}

static void hci_encrypt_key_refresh_complete(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status %u handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	if (evt->status) {
		bt_l2cap_encrypt_change(conn, evt->status);
		return;
	}

	/*
	 * Update keys with last pairing info for proper sec level update.
	 * This is done only for LE transport. For BR/EDR transport keys are
	 * updated on HCI 'Link Key Notification Event', therefore update here
	 * only security level based on available keys and encryption state.
	 */
#if defined(CONFIG_BLUETOOTH_SMP)
	if (conn->type == BT_CONN_TYPE_LE) {
		bt_smp_update_keys(conn);
		update_sec_level(conn);
	}
#endif /* CONFIG_BLUETOOTH_SMP */
#if defined(CONFIG_BLUETOOTH_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		update_sec_level_br(conn);
	}
#endif /* CONFIG_BLUETOOTH_BREDR */

	bt_l2cap_encrypt_change(conn, evt->status);
	bt_conn_security_changed(conn);
	bt_conn_unref(conn);
}
#endif /* CONFIG_BLUETOOTH_SMP || CONFIG_BLUETOOTH_BREDR */

#if defined(CONFIG_BLUETOOTH_SMP)
static void le_ltk_request(struct net_buf *buf)
{
	struct bt_hci_evt_le_ltk_request *evt = (void *)buf->data;
	struct bt_hci_cp_le_ltk_req_neg_reply *cp;
	struct bt_conn *conn;
	u16_t handle;
	u8_t tk[16];

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

	if (!conn->le.keys) {
		conn->le.keys = bt_keys_find(BT_KEYS_LTK_P256, &conn->le.dst);
		if (!conn->le.keys) {
			conn->le.keys = bt_keys_find(BT_KEYS_SLAVE_LTK,
						     &conn->le.dst);
		}
	}

	if (conn->le.keys && (conn->le.keys->keys & BT_KEYS_LTK_P256) &&
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
		memcpy(cp->ltk, conn->le.keys->ltk.val,
		       conn->le.keys->enc_size);
		if (conn->le.keys->enc_size < sizeof(cp->ltk)) {
			memset(cp->ltk + conn->le.keys->enc_size, 0,
			       sizeof(cp->ltk) - conn->le.keys->enc_size);
		}

		bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_REPLY, buf);
		goto done;
	}

#if !defined(CONFIG_BLUETOOTH_SMP_SC_ONLY)
	if (conn->le.keys && (conn->le.keys->keys & BT_KEYS_SLAVE_LTK) &&
	    conn->le.keys->slave_ltk.rand == evt->rand &&
	    conn->le.keys->slave_ltk.ediv == evt->ediv) {
		struct bt_hci_cp_le_ltk_req_reply *cp;
		struct net_buf *buf;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_REPLY,
					sizeof(*cp));
		if (!buf) {
			BT_ERR("Out of command buffers");
			goto done;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->handle = evt->handle;

		/* use only enc_size bytes of key for encryption */
		memcpy(cp->ltk, conn->le.keys->slave_ltk.val,
		       conn->le.keys->enc_size);
		if (conn->le.keys->enc_size < sizeof(cp->ltk)) {
			memset(cp->ltk + conn->le.keys->enc_size, 0,
			       sizeof(cp->ltk) - conn->le.keys->enc_size);
		}

		bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_REPLY, buf);
		goto done;
	}
#endif /* !CONFIG_BLUETOOTH_SMP_SC_ONLY */

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY, sizeof(*cp));
	if (!buf) {
		BT_ERR("Out of command buffers");
		goto done;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = evt->handle;

	bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY, buf);

done:
	bt_conn_unref(conn);
}
#endif /* CONFIG_BLUETOOTH_SMP */

static void le_pkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_p256_public_key_complete *evt = (void *)buf->data;
	struct bt_pub_key_cb *cb;

	BT_DBG("status: 0x%x", evt->status);

	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	if (!evt->status) {
		memcpy(pub_key, evt->key, 64);
		atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	}

	for (cb = pub_key_cb; cb; cb = cb->_next) {
		cb->func(evt->status ? NULL : evt->key);
	}
}

static void le_dhkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_generate_dhkey_complete *evt = (void *)buf->data;

	BT_DBG("status: 0x%x", evt->status);

	if (dh_key_cb) {
		dh_key_cb(evt->status ? NULL : evt->dhkey);
		dh_key_cb = NULL;
	}
}

static void hci_reset_complete(struct net_buf *buf)
{
	u8_t status = buf->data[0];

	BT_DBG("status %u", status);

	if (status) {
		return;
	}

	scan_dev_found_cb = NULL;
#if defined(CONFIG_BLUETOOTH_BREDR)
	discovery_cb = NULL;
	discovery_results = NULL;
	discovery_results_size = 0;
	discovery_results_count = 0;
#endif /* CONFIG_BLUETOOTH_BREDR */

	/* we only allow to enable once so this bit must be keep set */
	atomic_set(bt_dev.flags, BIT(BT_DEV_ENABLE));
}

static void hci_cmd_done(u16_t opcode, u8_t status, struct net_buf *buf)
{
	BT_DBG("opcode 0x%04x status 0x%02x buf %p", opcode, status, buf);

	if (buf->pool != &hci_cmd_pool) {
		return;
	}

	if (cmd(buf)->opcode != opcode) {
		BT_WARN("OpCode 0x%04x completed instead of expected 0x%04x",
			opcode, cmd(buf)->opcode);
	}

	/* If the command was synchronous wake up bt_hci_cmd_send_sync() */
	if (cmd(buf)->sync) {
		cmd(buf)->status = status;
		k_sem_give(cmd(buf)->sync);
	}
}

static void hci_cmd_complete(struct net_buf *buf)
{
	struct bt_hci_evt_cmd_complete *evt = (void *)buf->data;
	u16_t opcode = sys_le16_to_cpu(evt->opcode);
	u8_t status, ncmd = evt->ncmd;

	BT_DBG("opcode 0x%04x", opcode);

	net_buf_pull(buf, sizeof(*evt));

	/* All command return parameters have a 1-byte status in the
	 * beginning, so we can safely make this generalization.
	 */
	status = buf->data[0];

	hci_cmd_done(opcode, status, buf);

	/* Allow next command to be sent */
	if (ncmd) {
		k_sem_give(&bt_dev.ncmd_sem);
	}
}

static void hci_cmd_status(struct net_buf *buf)
{
	struct bt_hci_evt_cmd_status *evt = (void *)buf->data;
	u16_t opcode = sys_le16_to_cpu(evt->opcode);
	u8_t ncmd = evt->ncmd;

	BT_DBG("opcode 0x%04x", opcode);

	net_buf_pull(buf, sizeof(*evt));

	hci_cmd_done(opcode, evt->status, buf);

	/* Allow next command to be sent */
	if (ncmd) {
		k_sem_give(&bt_dev.ncmd_sem);
	}
}

static int start_le_scan(u8_t scan_type, u16_t interval, u16_t window,
			 u8_t filter_dup)
{
	struct net_buf *buf, *rsp;
	struct bt_hci_cp_le_set_scan_param *set_param;
	struct bt_hci_cp_le_set_scan_enable *scan_enable;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_PARAM,
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

	if (IS_ENABLED(CONFIG_BLUETOOTH_PRIVACY)) {
		err = le_set_private_addr();
		if (err) {
			net_buf_unref(buf);
			return err;
		}

		set_param->addr_type = BT_ADDR_LE_RANDOM;
	} else {
		set_param->addr_type =  bt_dev.id_addr.type;

		/* only set NRPA if there is no advertising ongoing */
		if (scan_type == BT_HCI_LE_SCAN_ACTIVE &&
		    !atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
			err = le_set_private_addr();
			if (err) {
				net_buf_unref(buf);
				return err;
			}

			set_param->addr_type = BT_ADDR_LE_RANDOM;
		}
	}

	bt_hci_cmd_send(BT_HCI_OP_LE_SET_SCAN_PARAM, buf);
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_ENABLE,
				sizeof(*scan_enable));
	if (!buf) {
		return -ENOBUFS;
	}

	scan_enable = net_buf_add(buf, sizeof(*scan_enable));
	memset(scan_enable, 0, sizeof(*scan_enable));
	scan_enable->filter_dup = filter_dup;
	scan_enable->enable = BT_HCI_LE_SCAN_ENABLE;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_SCAN_ENABLE, buf, &rsp);
	if (err) {
		return err;
	}

	/* Update scan state in case of success (0) status */
	err = rsp->data[0];
	if (!err) {
		atomic_set_bit(bt_dev.flags, BT_DEV_SCANNING);
		if (scan_type == BT_HCI_LE_SCAN_ACTIVE) {
			atomic_set_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN);
		}
	}

	net_buf_unref(rsp);

	return err;
}

int bt_le_scan_update(bool fast_scan)
{
	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return 0;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		int err;

		err = bt_hci_stop_scanning();
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_CENTRAL)) {
		u16_t interval, window;
		struct bt_conn *conn;

		conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT_SCAN);
		if (!conn) {
			return 0;
		}

		bt_conn_unref(conn);

		if (fast_scan) {
			interval = BT_GAP_SCAN_FAST_INTERVAL;
			window = BT_GAP_SCAN_FAST_WINDOW;
		} else {
			interval = BT_GAP_SCAN_SLOW_INTERVAL_1;
			window = BT_GAP_SCAN_SLOW_WINDOW_1;
		}

		return start_le_scan(BT_HCI_LE_SCAN_PASSIVE, interval, window,
				     0x01);
	}

	return 0;
}

static void le_adv_report(struct net_buf *buf)
{
	u8_t num_reports = net_buf_pull_u8(buf);
	struct bt_hci_evt_le_advertising_info *info;

	BT_DBG("Adv number of reports %u",  num_reports);

	while (num_reports--) {
		const bt_addr_le_t *addr;
		s8_t rssi;

		info = (void *)buf->data;
		net_buf_pull(buf, sizeof(*info));

		rssi = info->data[info->length];

		BT_DBG("%s event %u, len %u, rssi %d dBm",
		       bt_addr_le_str(&info->addr),
		       info->evt_type, info->length, rssi);

		addr = find_id_addr(&info->addr);

		if (scan_dev_found_cb) {
			struct net_buf_simple_state state;

			net_buf_simple_save(&buf->b, &state);

			buf->len = info->length;
			scan_dev_found_cb(addr, rssi, info->evt_type, &buf->b);

			net_buf_simple_restore(&buf->b, &state);
		}

#if defined(CONFIG_BLUETOOTH_CONN)
		check_pending_conn(addr, &info->addr, info->evt_type);
#endif /* CONFIG_BLUETOOTH_CONN */

		/* Get next report iteration by moving pointer to right offset
		 * in buf according to spec 4.2, Vol 2, Part E, 7.7.65.2.
		 */
		net_buf_pull(buf, info->length + sizeof(rssi));
	}
}

static void hci_le_meta_event(struct net_buf *buf)
{
	struct bt_hci_evt_le_meta_event *evt = (void *)buf->data;

	BT_DBG("subevent 0x%02x", evt->subevent);

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
	case BT_HCI_EVT_LE_DATA_LEN_CHANGE:
		le_data_len_change(buf);
		break;
	case BT_HCI_EVT_LE_PHY_UPDATE_COMPLETE:
		le_phy_update_complete(buf);
		break;
#endif /* CONFIG_BLUETOOTH_CONN */
#if defined(CONFIG_BLUETOOTH_SMP)
	case BT_HCI_EVT_LE_LTK_REQUEST:
		le_ltk_request(buf);
		break;
#endif /* CONFIG_BLUETOOTH_SMP */
	case BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE:
		le_pkey_complete(buf);
		break;
	case BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE:
		le_dhkey_complete(buf);
		break;
	case BT_HCI_EVT_LE_ADVERTISING_REPORT:
		le_adv_report(buf);
		break;
	default:
		BT_WARN("Unhandled LE event 0x%02x len %u: %s",
			evt->subevent, buf->len, bt_hex(buf->data, buf->len));
		break;
	}
}

static void hci_event(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr = (void *)buf->data;

	BT_DBG("event 0x%02x", hdr->evt);

	BT_ASSERT(!bt_hci_evt_is_prio(hdr->evt));

	net_buf_pull(buf, sizeof(*hdr));

	switch (hdr->evt) {
#if defined(CONFIG_BLUETOOTH_BREDR)
	case BT_HCI_EVT_CONN_REQUEST:
		conn_req(buf);
		break;
	case BT_HCI_EVT_CONN_COMPLETE:
		conn_complete(buf);
		break;
	case BT_HCI_EVT_PIN_CODE_REQ:
		pin_code_req(buf);
		break;
	case BT_HCI_EVT_LINK_KEY_NOTIFY:
		link_key_notify(buf);
		break;
	case BT_HCI_EVT_LINK_KEY_REQ:
		link_key_req(buf);
		break;
	case BT_HCI_EVT_IO_CAPA_RESP:
		io_capa_resp(buf);
		break;
	case BT_HCI_EVT_IO_CAPA_REQ:
		io_capa_req(buf);
		break;
	case BT_HCI_EVT_SSP_COMPLETE:
		ssp_complete(buf);
		break;
	case BT_HCI_EVT_USER_CONFIRM_REQ:
		user_confirm_req(buf);
		break;
	case BT_HCI_EVT_USER_PASSKEY_NOTIFY:
		user_passkey_notify(buf);
		break;
	case BT_HCI_EVT_USER_PASSKEY_REQ:
		user_passkey_req(buf);
		break;
	case BT_HCI_EVT_INQUIRY_COMPLETE:
		inquiry_complete(buf);
		break;
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
		inquiry_result_with_rssi(buf);
		break;
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		extended_inquiry_result(buf);
		break;
	case BT_HCI_EVT_REMOTE_NAME_REQ_COMPLETE:
		remote_name_request_complete(buf);
		break;
	case BT_HCI_EVT_AUTH_COMPLETE:
		auth_complete(buf);
		break;
	case BT_HCI_EVT_REMOTE_FEATURES:
		read_remote_features_complete(buf);
		break;
	case BT_HCI_EVT_REMOTE_EXT_FEATURES:
		read_remote_ext_features_complete(buf);
		break;
	case BT_HCI_EVT_ROLE_CHANGE:
		role_change(buf);
		break;
	case BT_HCI_EVT_SYNC_CONN_COMPLETE:
		synchronous_conn_complete(buf);
		break;
#endif
#if defined(CONFIG_BLUETOOTH_CONN)
	case BT_HCI_EVT_DISCONN_COMPLETE:
		hci_disconn_complete(buf);
		break;
#endif /* CONFIG_BLUETOOTH_CONN */
#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
	case BT_HCI_EVT_ENCRYPT_CHANGE:
		hci_encrypt_change(buf);
		break;
	case BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE:
		hci_encrypt_key_refresh_complete(buf);
		break;
#endif /* CONFIG_BLUETOOTH_SMP || CONFIG_BLUETOOTH_BREDR */
	case BT_HCI_EVT_LE_META_EVENT:
		hci_le_meta_event(buf);
		break;
	default:
		BT_WARN("Unhandled event 0x%02x len %u: %s", hdr->evt,
			buf->len, bt_hex(buf->data, buf->len));
		break;

	}

	net_buf_unref(buf);
}

static void send_cmd(void)
{
	struct net_buf *buf;
	int err;

	/* Get next command */
	BT_DBG("calling net_buf_get");
	buf = net_buf_get(&bt_dev.cmd_tx_queue, K_NO_WAIT);
	BT_ASSERT(buf);

	/* Wait until ncmd > 0 */
	BT_DBG("calling sem_take_wait");
	k_sem_take(&bt_dev.ncmd_sem, K_FOREVER);

	/* Clear out any existing sent command */
	if (bt_dev.sent_cmd) {
		BT_ERR("Uncleared pending sent_cmd");
		net_buf_unref(bt_dev.sent_cmd);
		bt_dev.sent_cmd = NULL;
	}

	bt_dev.sent_cmd = net_buf_ref(buf);

	BT_DBG("Sending command 0x%04x (buf %p) to driver",
	       cmd(buf)->opcode, buf);

	err = bt_send(buf);
	if (err) {
		BT_ERR("Unable to send to driver (err %d)", err);
		k_sem_give(&bt_dev.ncmd_sem);
		hci_cmd_done(cmd(buf)->opcode, BT_HCI_ERR_UNSPECIFIED,
			     NULL);
		net_buf_unref(bt_dev.sent_cmd);
		bt_dev.sent_cmd = NULL;
		net_buf_unref(buf);
	}
}

static void process_events(struct k_poll_event *ev, int count)
{
	BT_DBG("count %d", count);

	for (; count; ev++, count--) {
		BT_DBG("ev->state %u", ev->state);

		switch (ev->state) {
		case K_POLL_STATE_SIGNALED:
			break;
		case K_POLL_STATE_FIFO_DATA_AVAILABLE:
			if (ev->tag == BT_EVENT_CMD_TX) {
				send_cmd();
			} else if (IS_ENABLED(CONFIG_BLUETOOTH_CONN)) {
				struct bt_conn *conn;

				if (ev->tag == BT_EVENT_CONN_TX_NOTIFY) {
					conn = CONTAINER_OF(ev->fifo,
							    struct bt_conn,
							    tx_notify);
					bt_conn_notify_tx(conn);
				} else if (ev->tag == BT_EVENT_CONN_TX_QUEUE) {
					conn = CONTAINER_OF(ev->fifo,
							    struct bt_conn,
							    tx_queue);
					bt_conn_process_tx(conn);
				}
			}
			break;
		case K_POLL_STATE_NOT_READY:
			break;
		default:
			BT_WARN("Unexpected k_poll event state %u", ev->state);
			break;
		}
	}
}

#if defined(CONFIG_BLUETOOTH_CONN)
/* command FIFO + conn_change signal + MAX_CONN * 2 (tx & tx_notify) */
#define EV_COUNT (2 + (CONFIG_BLUETOOTH_MAX_CONN * 2))
#else
/* command FIFO */
#define EV_COUNT 1
#endif

static void hci_tx_thread(void *p1, void *p2, void *p3)
{
	static struct k_poll_event events[EV_COUNT] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&bt_dev.cmd_tx_queue,
						BT_EVENT_CMD_TX),
	};

	BT_DBG("Started");

	while (1) {
		int ev_count, err;

		events[0].state = K_POLL_STATE_NOT_READY;
		ev_count = 1;

		if (IS_ENABLED(CONFIG_BLUETOOTH_CONN)) {
			ev_count += bt_conn_prepare_events(&events[1]);
		}

		BT_DBG("Calling k_poll with %d events", ev_count);

		err = k_poll(events, ev_count, K_FOREVER);
		BT_ASSERT(err == 0);

		process_events(events, ev_count);

		/* Make sure we don't hog the CPU if there's all the time
		 * some ready events.
		 */
		k_yield();
	}
}


static void read_local_ver_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_local_version_info *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	bt_dev.hci_version = rp->hci_version;
	bt_dev.hci_revision = sys_le16_to_cpu(rp->hci_revision);
	bt_dev.lmp_version = rp->lmp_version;
	bt_dev.lmp_subversion = sys_le16_to_cpu(rp->lmp_subversion);
	bt_dev.manufacturer = sys_le16_to_cpu(rp->manufacturer);
}

static void read_bdaddr_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_bd_addr *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	bt_addr_copy(&bt_dev.id_addr.a, &rp->bdaddr);
	bt_dev.id_addr.type = BT_ADDR_LE_PUBLIC;
}

static void read_le_features_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_local_features *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	memcpy(bt_dev.le.features, rp->features, sizeof(bt_dev.le.features));
}

#if defined(CONFIG_BLUETOOTH_BREDR)
static void read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;
	u16_t pkts;

	BT_DBG("status %u", rp->status);

	bt_dev.br.mtu = sys_le16_to_cpu(rp->acl_max_len);
	pkts = sys_le16_to_cpu(rp->acl_max_num);

	BT_DBG("ACL BR/EDR buffers: pkts %u mtu %u", pkts, bt_dev.br.mtu);

	k_sem_init(&bt_dev.br.pkts, pkts, pkts);
}
#elif defined(CONFIG_BLUETOOTH_CONN)
static void read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;
	u16_t pkts;

	BT_DBG("status %u", rp->status);

	/* If LE-side has buffers we can ignore the BR/EDR values */
	if (bt_dev.le.mtu) {
		return;
	}

	bt_dev.le.mtu = sys_le16_to_cpu(rp->acl_max_len);
	pkts = sys_le16_to_cpu(rp->acl_max_num);

	BT_DBG("ACL BR/EDR buffers: pkts %u mtu %u", pkts, bt_dev.le.mtu);

	pkts = min(pkts, CONFIG_BLUETOOTH_CONN_TX_MAX);

	k_sem_init(&bt_dev.le.pkts, pkts, pkts);
}
#endif

#if defined(CONFIG_BLUETOOTH_CONN)
static void le_read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_buffer_size *rp = (void *)buf->data;
	u8_t le_max_num;

	BT_DBG("status %u", rp->status);

	bt_dev.le.mtu = sys_le16_to_cpu(rp->le_max_len);
	if (!bt_dev.le.mtu) {
		return;
	}

	BT_DBG("ACL LE buffers: pkts %u mtu %u", rp->le_max_num, bt_dev.le.mtu);

	le_max_num = min(rp->le_max_num, CONFIG_BLUETOOTH_CONN_TX_MAX);
	k_sem_init(&bt_dev.le.pkts, le_max_num, le_max_num);
}
#endif

static void read_supported_commands_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_supported_commands *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	memcpy(bt_dev.supported_commands, rp->commands,
	       sizeof(bt_dev.supported_commands));

	/*
	 * Report "LE Read Local P-256 Public Key" and "LE Generate DH Key" as
	 * supported if TinyCrypt ECC is used for emulation.
	 */
	if (IS_ENABLED(CONFIG_BLUETOOTH_TINYCRYPT_ECC)) {
		bt_dev.supported_commands[34] |= 0x02;
		bt_dev.supported_commands[34] |= 0x04;
	}
}

static void read_local_features_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_local_features *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	memcpy(bt_dev.features[0], rp->features, sizeof(bt_dev.features[0]));
}

static void le_read_supp_states_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_supp_states *rp = (void *)buf->data;

	BT_DBG("status %u", rp->status);

	bt_dev.le.states = sys_get_le64(rp->le_states);
}

static int common_init(void)
{
	struct net_buf *rsp;
	int err;

	/* Send HCI_RESET */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_RESET, NULL, &rsp);
	if (err) {
		return err;
	}
	hci_reset_complete(rsp);
	net_buf_unref(rsp);

#if defined(CONFIG_BLUETOOTH_HOST_CRYPTO)
	/*
	 * initialize PRNG right after reset so that it is safe to use it later
	 * on in initialization process
	 */
	err = prng_init();
	if (err) {
		return err;
	}
#endif

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

	/* Read Local Supported Commands */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_SUPPORTED_COMMANDS, NULL,
				   &rsp);
	if (err) {
		return err;
	}
	read_supported_commands_complete(rsp);
	net_buf_unref(rsp);

#if defined(CONFIG_BLUETOOTH_HCI_ACL_FLOW_CONTROL)
	err = set_flow_control();
	if (err) {
		return err;
	}
#endif /* CONFIG_BLUETOOTH_CONN */

	return 0;
}

static int le_set_event_mask(void)
{
	struct bt_hci_cp_le_set_event_mask *cp_mask;
	struct net_buf *buf;
	u64_t mask = 0;

	/* Set LE event mask */
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_EVENT_MASK, sizeof(*cp_mask));
	if (!buf) {
		return -ENOBUFS;
	}

	cp_mask = net_buf_add(buf, sizeof(*cp_mask));

	mask |= BT_EVT_MASK_LE_ADVERTISING_REPORT;

	if (IS_ENABLED(CONFIG_BLUETOOTH_CONN)) {
		mask |= BT_EVT_MASK_LE_CONN_COMPLETE;
		mask |= BT_EVT_MASK_LE_CONN_UPDATE_COMPLETE;
		mask |= BT_EVT_MASK_LE_REMOTE_FEAT_COMPLETE;
		if (BT_FEAT_LE_CONN_PARAM_REQ_PROC(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_CONN_PARAM_REQ;
		}
		if (BT_FEAT_LE_DLE(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_DATA_LEN_CHANGE;
		}
		if (BT_FEAT_LE_PHY_2M(bt_dev.le.features) ||
		    BT_FEAT_LE_PHY_CODED(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_PHY_UPDATE_COMPLETE;
		}
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_SMP) &&
	    BT_FEAT_LE_ENCR(bt_dev.le.features)) {
		mask |= BT_EVT_MASK_LE_LTK_REQUEST;
	}

	/*
	 * If "LE Read Local P-256 Public Key" and "LE Generate DH Key" are
	 * supported we need to enable events generated by those commands.
	 */
	if ((bt_dev.supported_commands[34] & 0x02) &&
	    (bt_dev.supported_commands[34] & 0x04)) {
		mask |= BT_EVT_MASK_LE_P256_PUBLIC_KEY_COMPLETE;
		mask |= BT_EVT_MASK_LE_GENERATE_DHKEY_COMPLETE;
	}

	sys_put_le64(mask, cp_mask->events);
	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EVENT_MASK, buf, NULL);
}

static int le_init(void)
{
	struct bt_hci_cp_write_le_host_supp *cp_le;
	struct net_buf *buf;
	struct net_buf *rsp;
	int err;

	/* For now we only support LE capable controllers */
	if (!BT_FEAT_LE(bt_dev.features)) {
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

#if defined(CONFIG_BLUETOOTH_CONN)
	/* Read LE Buffer Size */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_BUFFER_SIZE,
				   NULL, &rsp);
	if (err) {
		return err;
	}
	le_read_buffer_size_complete(rsp);
	net_buf_unref(rsp);
#endif

	if (BT_FEAT_BREDR(bt_dev.features)) {
		buf = bt_hci_cmd_create(BT_HCI_OP_LE_WRITE_LE_HOST_SUPP,
					sizeof(*cp_le));
		if (!buf) {
			return -ENOBUFS;
		}

		cp_le = net_buf_add(buf, sizeof(*cp_le));

		/* Explicitly enable LE for dual-mode controllers */
		cp_le->le = 0x01;
		cp_le->simul = 0x00;
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_WRITE_LE_HOST_SUPP, buf,
					   NULL);
		if (err) {
			return err;
		}
	}

	/* Read LE Supported States */
	if (BT_CMD_LE_STATES(bt_dev.supported_commands)) {
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_SUPP_STATES, NULL,
					   &rsp);
		if (err) {
			return err;
		}
		le_read_supp_states_complete(rsp);
		net_buf_unref(rsp);
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_CONN) &&
	    BT_FEAT_LE_DLE(bt_dev.le.features)) {
		struct bt_hci_cp_le_write_default_data_len *cp;
		struct bt_hci_rp_le_read_max_data_len *rp;
		struct net_buf *buf, *rsp;
		u16_t tx_octets, tx_time;

		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_MAX_DATA_LEN, NULL,
					   &rsp);
		if (err) {
			return err;
		}

		rp = (void *)rsp->data;
		tx_octets = sys_le16_to_cpu(rp->max_tx_octets);
		tx_time = sys_le16_to_cpu(rp->max_tx_time);
		net_buf_unref(rsp);

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN,
					sizeof(*cp));
		if (!buf) {
			return -ENOBUFS;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->max_tx_octets = sys_cpu_to_le16(tx_octets);
		cp->max_tx_time = sys_cpu_to_le16(tx_time);

		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN,
					   buf, NULL);
		if (err) {
			return err;
		}
	}

	return  le_set_event_mask();
}

#if defined(CONFIG_BLUETOOTH_BREDR)
static int read_ext_features(void)
{
	int i;

	/* Read Local Supported Extended Features */
	for (i = 1; i < LMP_FEAT_PAGES_COUNT; i++) {
		struct bt_hci_cp_read_local_ext_features *cp;
		struct bt_hci_rp_read_local_ext_features *rp;
		struct net_buf *buf, *rsp;
		int err;

		buf = bt_hci_cmd_create(BT_HCI_OP_READ_LOCAL_EXT_FEATURES,
					sizeof(*cp));
		if (!buf) {
			return -ENOBUFS;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->page = i;

		err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_EXT_FEATURES,
					   buf, &rsp);
		if (err) {
			return err;
		}

		rp = (void *)rsp->data;

		memcpy(&bt_dev.features[i], rp->ext_features,
		       sizeof(bt_dev.features[i]));

		if (rp->max_page <= i) {
			net_buf_unref(rsp);
			break;
		}

		net_buf_unref(rsp);
	}

	return 0;
}

void device_supported_pkt_type(void)
{
	/* Device supported features and sco packet types */
	if (BT_FEAT_HV2_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_HV2);
	}

	if (BT_FEAT_HV3_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_HV3);
	}

	if (BT_FEAT_LMP_ESCO_CAPABLE(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_EV3);
	}

	if (BT_FEAT_EV4_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_EV4);
	}

	if (BT_FEAT_EV5_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_EV5);
	}

	if (BT_FEAT_2EV3_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_2EV3);
	}

	if (BT_FEAT_3EV3_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_3EV3);
	}

	if (BT_FEAT_3SLOT_PKT(bt_dev.features)) {
		bt_dev.br.esco_pkt_type |= (HCI_PKT_TYPE_ESCO_2EV5 |
					    HCI_PKT_TYPE_ESCO_3EV5);
	}
}

static int br_init(void)
{
	struct net_buf *buf;
	struct bt_hci_cp_write_ssp_mode *ssp_cp;
	struct bt_hci_cp_write_inquiry_mode *inq_cp;
	struct bt_hci_write_local_name *name_cp;
	int err;

	/* Read extended local features */
	if (BT_FEAT_EXT_FEATURES(bt_dev.features)) {
		err = read_ext_features();
		if (err) {
			return err;
		}
	}

	/* Add local supported packet types to bt_dev */
	device_supported_pkt_type();

	/* Get BR/EDR buffer size */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BUFFER_SIZE, NULL, &buf);
	if (err) {
		return err;
	}

	read_buffer_size_complete(buf);
	net_buf_unref(buf);

	/* Set SSP mode */
	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_SSP_MODE, sizeof(*ssp_cp));
	if (!buf) {
		return -ENOBUFS;
	}

	ssp_cp = net_buf_add(buf, sizeof(*ssp_cp));
	ssp_cp->mode = 0x01;
	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_SSP_MODE, buf, NULL);
	if (err) {
		return err;
	}

	/* Enable Inquiry results with RSSI or extended Inquiry */
	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_INQUIRY_MODE, sizeof(*inq_cp));
	if (!buf) {
		return -ENOBUFS;
	}

	inq_cp = net_buf_add(buf, sizeof(*inq_cp));
	inq_cp->mode = 0x02;
	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_INQUIRY_MODE, buf, NULL);
	if (err) {
		return err;
	}

	/* Set local name */
	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_LOCAL_NAME, sizeof(*name_cp));
	if (!buf) {
		return -ENOBUFS;
	}

	name_cp = net_buf_add(buf, sizeof(*name_cp));
	strncpy((char *)name_cp->local_name, CONFIG_BLUETOOTH_DEVICE_NAME,
		sizeof(name_cp->local_name));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_LOCAL_NAME, buf, NULL);
	if (err) {
		return err;
	}

	/* Set page timeout*/
	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_PAGE_TIMEOUT, sizeof(u16_t));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_le16(buf, CONFIG_BLUETOOTH_PAGE_TIMEOUT);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_PAGE_TIMEOUT, buf, NULL);
	if (err) {
		return err;
	}

	/* Enable BR/EDR SC if supported */
	if (BT_FEAT_SC(bt_dev.features)) {
		struct bt_hci_cp_write_sc_host_supp *sc_cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_SC_HOST_SUPP,
					sizeof(*sc_cp));
		if (!buf) {
			return -ENOBUFS;
		}

		sc_cp = net_buf_add(buf, sizeof(*sc_cp));
		sc_cp->sc_support = 0x01;

		err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_SC_HOST_SUPP, buf,
					   NULL);
		if (err) {
			return err;
		}
	}

	return 0;
}
#else
static int br_init(void)
{
#if defined(CONFIG_BLUETOOTH_CONN)
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
#endif /* CONFIG_BLUETOOTH_CONN */

	return 0;
}
#endif

static int set_event_mask(void)
{
	struct bt_hci_cp_set_event_mask *ev;
	struct net_buf *buf;
	u64_t mask = 0;

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_EVENT_MASK, sizeof(*ev));
	if (!buf) {
		return -ENOBUFS;
	}

	ev = net_buf_add(buf, sizeof(*ev));

	if (IS_ENABLED(CONFIG_BLUETOOTH_BREDR)) {
		/* Since we require LE support, we can count on a
		 * Bluetooth 4.0 feature set
		 */
		mask |= BT_EVT_MASK_INQUIRY_COMPLETE;
		mask |= BT_EVT_MASK_CONN_COMPLETE;
		mask |= BT_EVT_MASK_CONN_REQUEST;
		mask |= BT_EVT_MASK_AUTH_COMPLETE;
		mask |= BT_EVT_MASK_REMOTE_NAME_REQ_COMPLETE;
		mask |= BT_EVT_MASK_REMOTE_FEATURES;
		mask |= BT_EVT_MASK_ROLE_CHANGE;
		mask |= BT_EVT_MASK_PIN_CODE_REQ;
		mask |= BT_EVT_MASK_LINK_KEY_REQ;
		mask |= BT_EVT_MASK_LINK_KEY_NOTIFY;
		mask |= BT_EVT_MASK_INQUIRY_RESULT_WITH_RSSI;
		mask |= BT_EVT_MASK_REMOTE_EXT_FEATURES;
		mask |= BT_EVT_MASK_SYNC_CONN_COMPLETE;
		mask |= BT_EVT_MASK_EXTENDED_INQUIRY_RESULT;
		mask |= BT_EVT_MASK_IO_CAPA_REQ;
		mask |= BT_EVT_MASK_IO_CAPA_RESP;
		mask |= BT_EVT_MASK_USER_CONFIRM_REQ;
		mask |= BT_EVT_MASK_USER_PASSKEY_REQ;
		mask |= BT_EVT_MASK_SSP_COMPLETE;
		mask |= BT_EVT_MASK_USER_PASSKEY_NOTIFY;
	}

	mask |= BT_EVT_MASK_HARDWARE_ERROR;
	mask |= BT_EVT_MASK_DATA_BUFFER_OVERFLOW;
	mask |= BT_EVT_MASK_LE_META_EVENT;

	if (IS_ENABLED(CONFIG_BLUETOOTH_CONN)) {
		mask |= BT_EVT_MASK_DISCONN_COMPLETE;
		mask |= BT_EVT_MASK_REMOTE_VERSION_INFO;
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_SMP) &&
	    BT_FEAT_LE_ENCR(bt_dev.le.features)) {
		mask |= BT_EVT_MASK_ENCRYPT_CHANGE;
		mask |= BT_EVT_MASK_ENCRYPT_KEY_REFRESH_COMPLETE;
	}

	sys_put_le64(mask, ev->events);
	return bt_hci_cmd_send_sync(BT_HCI_OP_SET_EVENT_MASK, buf, NULL);
}

static inline int create_random_addr(bt_addr_le_t *addr)
{
	addr->type = BT_ADDR_LE_RANDOM;

	return bt_rand(addr->a.val, 6);
}

int bt_addr_le_create_nrpa(bt_addr_le_t *addr)
{
	int err;

	err = create_random_addr(addr);
	if (err) {
		return err;
	}

	BT_ADDR_SET_NRPA(&addr->a);

	return 0;
}

int bt_addr_le_create_static(bt_addr_le_t *addr)
{
	int err;

	err = create_random_addr(addr);
	if (err) {
		return err;
	}

	BT_ADDR_SET_STATIC(&addr->a);

	return 0;
}

static int set_static_addr(void)
{
	int err;

	if (bt_storage) {
		ssize_t ret;

		ret = bt_storage->read(NULL, BT_STORAGE_ID_ADDR,
				       &bt_dev.id_addr, sizeof(bt_dev.id_addr));
		if (ret == sizeof(bt_dev.id_addr)) {
			goto set_addr;
		}
	}

#if defined(CONFIG_SOC_FAMILY_NRF5)
	/* Read address from nRF5-specific storage
	 * Non-initialized FICR values default to 0xFF, skip if no address
	 * present. Also if a public address lives in FICR, do not use in this
	 * function.
	 */
	if (((NRF_FICR->DEVICEADDR[0] != UINT32_MAX) ||
	    ((NRF_FICR->DEVICEADDR[1] & UINT16_MAX) != UINT16_MAX)) &&
	      (NRF_FICR->DEVICEADDRTYPE & 0x01)) {

		bt_dev.id_addr.type = BT_ADDR_LE_RANDOM;
		sys_put_le32(NRF_FICR->DEVICEADDR[0], &bt_dev.id_addr.a.val[0]);
		sys_put_le16(NRF_FICR->DEVICEADDR[1], &bt_dev.id_addr.a.val[4]);
		/* The FICR value is a just a random number, with no knowledge
		 * of the Bluetooth Specification requirements for random
		 * static addresses.
		 */
		BT_ADDR_SET_STATIC(&bt_dev.id_addr.a);

		goto set_addr;
	}
#endif /* CONFIG_SOC_FAMILY_NRF5 */

	BT_DBG("Generating new static random address");

	err = bt_addr_le_create_static(&bt_dev.id_addr);
	if (err) {
		return err;
	}

	if (bt_storage) {
		ssize_t ret;

		ret = bt_storage->write(NULL, BT_STORAGE_ID_ADDR,
					&bt_dev.id_addr,
					sizeof(bt_dev.id_addr));
		if (ret != sizeof(bt_dev.id_addr)) {
			BT_ERR("Unable to store static address");
		}
	} else {
		BT_WARN("Using temporary static random address");
	}

set_addr:
	if (bt_dev.id_addr.type != BT_ADDR_LE_RANDOM ||
	    (bt_dev.id_addr.a.val[5] & 0xc0) != 0xc0) {
		BT_ERR("Only static random address supported as identity");
		return -EINVAL;
	}

	err = set_random_address(&bt_dev.id_addr.a);
	if (err) {
		return err;
	}

	atomic_set_bit(bt_dev.flags, BT_DEV_ID_STATIC_RANDOM);
	return 0;
}

#if defined(CONFIG_BLUETOOTH_DEBUG)
static const char *ver_str(u8_t ver)
{
	const char * const str[] = {
		"1.0b", "1.1", "1.2", "2.0", "2.1", "3.0", "4.0", "4.1", "4.2",
		"5.0",
	};

	if (ver < ARRAY_SIZE(str)) {
		return str[ver];
	}

	return "unknown";
}

static void show_dev_info(void)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&bt_dev.id_addr, addr, sizeof(addr));

	BT_INFO("Identity: %s", addr);
	BT_INFO("HCI: version %s (0x%02x) revision 0x%04x, manufacturer 0x%04x",
		ver_str(bt_dev.hci_version), bt_dev.hci_version,
		bt_dev.hci_revision, bt_dev.manufacturer);
	BT_INFO("LMP: version %s (0x%02x) subver 0x%04x",
		ver_str(bt_dev.lmp_version), bt_dev.lmp_version,
		bt_dev.lmp_subversion);
}
#else
static inline void show_dev_info(void)
{
}
#endif /* CONFIG_BLUETOOTH_DEBUG */

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

	if (BT_FEAT_BREDR(bt_dev.features)) {
		err = br_init();
		if (err) {
			return err;
		}
	} else if (IS_ENABLED(CONFIG_BLUETOOTH_BREDR)) {
		BT_ERR("Non-BR/EDR controller detected");
		return -EIO;
	}

	err = set_event_mask();
	if (err) {
		return err;
	}

	if (!bt_addr_le_cmp(&bt_dev.id_addr, BT_ADDR_LE_ANY) ||
	    !bt_addr_le_cmp(&bt_dev.id_addr, BT_ADDR_LE_NONE)) {
		BT_DBG("No public address. Trying to set static random.");
		err = set_static_addr();
		if (err) {
			BT_ERR("Unable to set identity address");
			return err;
		}
	}

	show_dev_info();

	return 0;
}

int bt_send(struct net_buf *buf)
{
	BT_DBG("buf %p len %u type %u", buf, buf->len, bt_buf_get_type(buf));

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	if (IS_ENABLED(CONFIG_BLUETOOTH_TINYCRYPT_ECC)) {
		return bt_hci_ecc_send(buf);
	}

	return bt_dev.drv->send(buf);
}

int bt_recv(struct net_buf *buf)
{
	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	BT_DBG("buf %p len %u", buf, buf->len);

	if (buf->pool->user_data_size < BT_BUF_USER_DATA_MIN) {
		BT_ERR("Too small user data size");
		net_buf_unref(buf);
		return -EINVAL;
	}

	switch (bt_buf_get_type(buf)) {
#if defined(CONFIG_BLUETOOTH_CONN)
	case BT_BUF_ACL_IN:
#if defined(CONFIG_BLUETOOTH_RECV_IS_RX_THREAD)
		hci_acl(buf);
#else
		net_buf_put(&bt_dev.rx_queue, buf);
#endif
		return 0;
#endif /* BLUETOOTH_CONN */
	case BT_BUF_EVT:
#if defined(CONFIG_BLUETOOTH_RECV_IS_RX_THREAD)
		hci_event(buf);
#else
		net_buf_put(&bt_dev.rx_queue, buf);
#endif
		return 0;
	default:
		BT_ERR("Invalid buf type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return -EINVAL;
	}
}

int bt_recv_prio(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr = (void *)buf->data;

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	BT_ASSERT(bt_buf_get_type(buf) == BT_BUF_EVT);
	BT_ASSERT(buf->len >= sizeof(*hdr));
	BT_ASSERT(bt_hci_evt_is_prio(hdr->evt));

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
		net_buf_unref(buf);
		BT_ASSERT(0);
		return -EINVAL;
	}

	net_buf_unref(buf);

	return 0;
}

int bt_hci_driver_register(const struct bt_hci_driver *drv)
{
	if (bt_dev.drv) {
		return -EALREADY;
	}

	if (!drv->open || !drv->send) {
		return -EINVAL;
	}

	bt_dev.drv = drv;

	BT_DBG("Registered %s", drv->name ? drv->name : "");

	bt_monitor_new_index(BT_MONITOR_TYPE_PRIMARY, drv->bus,
			     BT_ADDR_ANY, drv->name ? drv->name : "bt0");

	return 0;
}

#if defined(CONFIG_BLUETOOTH_PRIVACY)
static int irk_init(void)
{
	ssize_t err;

	if (bt_storage) {
		err = bt_storage->read(NULL, BT_STORAGE_LOCAL_IRK, &bt_dev.irk,
				       sizeof(bt_dev.irk));
		if (err == sizeof(bt_dev.irk)) {
			return 0;
		}
	}

	BT_DBG("Generating new IRK");

	err = bt_rand(bt_dev.irk, sizeof(bt_dev.irk));
	if (err) {
		return err;
	}

	if (bt_storage) {
		err = bt_storage->write(NULL, BT_STORAGE_LOCAL_IRK, bt_dev.irk,
					sizeof(bt_dev.irk));
		if (err != sizeof(bt_dev.irk)) {
			BT_ERR("Unable to store IRK");
		}
	} else {
		BT_WARN("Using temporary IRK");
	}

	return 0;
}
#endif /* CONFIG_BLUETOOTH_PRIVACY */

static int bt_init(void)
{
	int err;

	err = hci_init();
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_CONN)) {
		err = bt_conn_init();
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BLUETOOTH_PRIVACY)
	err = irk_init();
	if (err) {
		return err;
	}

	k_delayed_work_init(&bt_dev.rpa_update, rpa_timeout);
#endif

	bt_monitor_send(BT_MONITOR_OPEN_INDEX, NULL, 0);
	atomic_set_bit(bt_dev.flags, BT_DEV_READY);
	bt_le_scan_update(false);

	return 0;
}

static void init_work(struct k_work *work)
{
	int err;

	err = bt_init();
	if (ready_cb) {
		ready_cb(err);
	}
}

#if !defined(CONFIG_BLUETOOTH_RECV_IS_RX_THREAD)
static void hci_rx_thread(void)
{
	struct net_buf *buf;

	BT_DBG("started");

	while (1) {
		BT_DBG("calling fifo_get_wait");
		buf = net_buf_get(&bt_dev.rx_queue, K_FOREVER);

		BT_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf),
		       buf->len);

		switch (bt_buf_get_type(buf)) {
#if defined(CONFIG_BLUETOOTH_CONN)
		case BT_BUF_ACL_IN:
			hci_acl(buf);
			break;
#endif /* CONFIG_BLUETOOTH_CONN */
		case BT_BUF_EVT:
			hci_event(buf);
			break;
		default:
			BT_ERR("Unknown buf type %u", bt_buf_get_type(buf));
			net_buf_unref(buf);
			break;
		}

		/* Make sure we don't hog the CPU if the rx_queue never
		 * gets empty.
		 */
		k_yield();
	}
}
#endif /* !CONFIG_BLUETOOTH_RECV_IS_RX_THREAD */

int bt_enable(bt_ready_cb_t cb)
{
	int err;

	if (!bt_dev.drv) {
		BT_ERR("No HCI driver registered");
		return -ENODEV;
	}

	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_ENABLE)) {
		return -EALREADY;
	}

	ready_cb = cb;

	/* TX thread */
	k_thread_create(&tx_thread_data, tx_thread_stack,
			sizeof(tx_thread_stack), hci_tx_thread, NULL, NULL,
			NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

#if !defined(CONFIG_BLUETOOTH_RECV_IS_RX_THREAD)
	/* RX thread */
	k_thread_create(&rx_thread_data, rx_thread_stack,
			sizeof(rx_thread_stack),
			(k_thread_entry_t)hci_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(7), 0, K_NO_WAIT);
#endif

	if (IS_ENABLED(CONFIG_BLUETOOTH_TINYCRYPT_ECC)) {
		bt_hci_ecc_init();
	}

	err = bt_dev.drv->open();
	if (err) {
		BT_ERR("HCI driver open failed (%d)", err);
		return err;
	}

	if (!cb) {
		return bt_init();
	}

	k_work_submit(&bt_dev.init);
	return 0;
}

bool bt_addr_le_is_bonded(const bt_addr_le_t *addr)
{
	if (IS_ENABLED(CONFIG_BLUETOOTH_SMP)) {
		struct bt_keys *keys = bt_keys_find_addr(addr);

		/* if there are any keys stored then device is bonded */
		return keys && keys->keys;
	} else {
		return false;
	}
}

static bool valid_adv_param(const struct bt_le_adv_param *param)
{
	if (!(param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		/*
		 * BT Core 4.2 [Vol 2, Part E, 7.8.5]
		 * The Advertising_Interval_Min and Advertising_Interval_Max
		 * shall not be set to less than 0x00A0 (100 ms) if the
		 * Advertising_Type is set to ADV_SCAN_IND or ADV_NONCONN_IND.
		 */
		if (bt_dev.hci_version < BT_HCI_VERSION_5_0 &&
		    param->interval_min < 0x00a0) {
			return false;
		}
	}

	if (param->interval_min > param->interval_max ||
	    param->interval_min < 0x0020 || param->interval_max > 0x4000) {
		return false;
	}

	return true;
}

static int set_ad(u16_t hci_op, const struct bt_data *ad, size_t ad_len)
{
	struct bt_hci_cp_le_set_adv_data *set_data;
	struct net_buf *buf;
	int i;

	buf = bt_hci_cmd_create(hci_op, sizeof(*set_data));
	if (!buf) {
		return -ENOBUFS;
	}

	set_data = net_buf_add(buf, sizeof(*set_data));

	memset(set_data, 0, sizeof(*set_data));

	for (i = 0; i < ad_len; i++) {
		/* Check if ad fit in the remaining buffer */
		if (set_data->len + ad[i].data_len + 2 > 31) {
			net_buf_unref(buf);
			return -EINVAL;
		}

		set_data->data[set_data->len++] = ad[i].data_len + 1;
		set_data->data[set_data->len++] = ad[i].type;

		memcpy(&set_data->data[set_data->len], ad[i].data,
		       ad[i].data_len);
		set_data->len += ad[i].data_len;
	}

	return bt_hci_cmd_send_sync(hci_op, buf, NULL);
}

int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
	struct net_buf *buf;
	struct bt_hci_cp_le_set_adv_param *set_param;
	int err;

	if (!valid_adv_param(param)) {
		return -EINVAL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return -EALREADY;
	}

	err = set_ad(BT_HCI_OP_LE_SET_ADV_DATA, ad, ad_len);
	if (err) {
		return err;
	}

	/*
	 * We need to set SCAN_RSP when enabling advertising type that allows
	 * for Scan Requests.
	 *
	 * If sd was not provided but we enable connectable undirected
	 * advertising sd needs to be cleared from values set by previous calls.
	 * Clearing sd is done by calling set_ad() with NULL data and zero len.
	 * So following condition check is unusual but correct.
	 */
	if (sd || (param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		err = set_ad(BT_HCI_OP_LE_SET_SCAN_RSP_DATA, sd, sd_len);
		if (err) {
			return err;
		}
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_PARAM,
				sizeof(*set_param));
	if (!buf) {
		return -ENOBUFS;
	}

	set_param = net_buf_add(buf, sizeof(*set_param));

	memset(set_param, 0, sizeof(*set_param));
	set_param->min_interval = sys_cpu_to_le16(param->interval_min);
	set_param->max_interval = sys_cpu_to_le16(param->interval_max);
	set_param->channel_map  = 0x07;

	if (param->options & BT_LE_ADV_OPT_CONNECTABLE) {
		if (IS_ENABLED(CONFIG_BLUETOOTH_PRIVACY)) {
			err = le_set_private_addr();
			if (err) {
				net_buf_unref(buf);
				return err;
			}

			set_param->own_addr_type = BT_ADDR_LE_RANDOM;
		} else {
			/*
			 * If Static Random address is used as Identity
			 * address we need to restore it before advertising
			 * is enabled. Otherwise NRPA used for active scan
			 * could be used for advertising.
			 */
			if (atomic_test_bit(bt_dev.flags,
					    BT_DEV_ID_STATIC_RANDOM)) {
				set_random_address(&bt_dev.id_addr.a);
			}

			set_param->own_addr_type = bt_dev.id_addr.type;
		}

		set_param->type = BT_LE_ADV_IND;
	} else {
		if (param->own_addr) {
			/* Only NRPA is allowed */
			if (!BT_ADDR_IS_NRPA(param->own_addr)) {
				return -EINVAL;
			}

			err = set_random_address(param->own_addr);
		} else {
			err = le_set_private_addr();
		}

		if (err) {
			net_buf_unref(buf);
			return err;
		}

		set_param->own_addr_type = BT_ADDR_LE_RANDOM;

		if (sd) {
			set_param->type = BT_LE_ADV_SCAN_IND;
		} else {
			set_param->type = BT_LE_ADV_NONCONN_IND;
		}
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_PARAM, buf, NULL);
	if (err) {
		return err;
	}

	err = set_advertise_enable(true);
	if (err) {
		return err;
	}

	atomic_set_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING);

	return 0;
}

int bt_le_adv_stop(void)
{
	int err;

	/* Advertise disable may fail if slave connections are established,
	 * and advertising is not kept ON as the controller does not support
	 * simultaneous slave connections and connectable advertising state.
	 * Hence, we test and clear BT_DEV_KEEP_ADVERTISING flag before trying
	 * to disable advertising if BT_DEV_ADVERTISING is set.
	 */
	if (!atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING)) {
		return -EALREADY;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return 0;
	}

	err = set_advertise_enable(false);
	if (err) {
		return err;
	}

	if (!IS_ENABLED(CONFIG_BLUETOOTH_PRIVACY)) {
		/* If active scan is ongoing set NRPA */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN)) {
			le_set_private_addr();
		}
	}

	return 0;
}

static bool valid_le_scan_param(const struct bt_le_scan_param *param)
{
	if (param->type != BT_HCI_LE_SCAN_PASSIVE &&
	    param->type != BT_HCI_LE_SCAN_ACTIVE) {
		return false;
	}

	if (param->filter_dup != BT_HCI_LE_SCAN_FILTER_DUP_DISABLE &&
	    param->filter_dup != BT_HCI_LE_SCAN_FILTER_DUP_ENABLE) {
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

	return bt_le_scan_update(false);
}

struct net_buf *bt_buf_get_rx(enum bt_buf_type type, s32_t timeout)
{
	struct net_buf *buf;

	__ASSERT(type == BT_BUF_EVT || type == BT_BUF_ACL_IN,
		 "Invalid buffer type requested");

#if defined(CONFIG_BLUETOOTH_HCI_ACL_FLOW_CONTROL)
	if (type == BT_BUF_EVT) {
		buf = net_buf_alloc(&hci_rx_pool, timeout);
	} else {
		buf = net_buf_alloc(&acl_in_pool, timeout);
	}
#else
	buf = net_buf_alloc(&hci_rx_pool, timeout);
#endif

	if (buf) {
		net_buf_reserve(buf, CONFIG_BLUETOOTH_HCI_RESERVE);
		bt_buf_set_type(buf, type);
	}

	return buf;
}

struct net_buf *bt_buf_get_cmd_complete(s32_t timeout)
{
	struct net_buf *buf;
	unsigned int key;

	key = irq_lock();
	buf = bt_dev.sent_cmd;
	bt_dev.sent_cmd = NULL;
	irq_unlock(key);

	BT_DBG("sent_cmd %p", buf);

	if (buf) {
		bt_buf_set_type(buf, BT_BUF_EVT);
		buf->len = 0;
		net_buf_reserve(buf, CONFIG_BLUETOOTH_HCI_RESERVE);

		return buf;
	}

	return bt_buf_get_rx(BT_BUF_EVT, timeout);
}

#if defined(CONFIG_BLUETOOTH_BREDR)
static int br_start_inquiry(const struct bt_br_discovery_param *param)
{
	const u8_t iac[3] = { 0x33, 0x8b, 0x9e };
	struct bt_hci_op_inquiry *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_INQUIRY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	cp->length = param->length;
	cp->num_rsp = 0xff; /* we limit discovery only by time */

	memcpy(cp->lap, iac, 3);
	if (param->limited) {
		cp->lap[0] = 0x00;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_INQUIRY, buf, NULL);
}

static bool valid_br_discov_param(const struct bt_br_discovery_param *param,
				  size_t num_results)
{
	if (!num_results || num_results > 255) {
		return false;
	}

	if (!param->length || param->length > 0x30) {
		return false;
	}

	return true;
}

int bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  struct bt_br_discovery_result *results, size_t cnt,
			  bt_br_discovery_cb_t cb)
{
	int err;

	BT_DBG("");

	if (!valid_br_discov_param(param, cnt)) {
		return -EINVAL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY)) {
		return -EALREADY;
	}

	err = br_start_inquiry(param);
	if (err) {
		return err;
	}

	atomic_set_bit(bt_dev.flags, BT_DEV_INQUIRY);

	memset(results, 0, sizeof(*results) * cnt);

	discovery_cb = cb;
	discovery_results = results;
	discovery_results_size = cnt;
	discovery_results_count = 0;

	return 0;
}

int bt_br_discovery_stop(void)
{
	int err;
	int i;

	BT_DBG("");

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY)) {
		return -EALREADY;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_INQUIRY_CANCEL, NULL, NULL);
	if (err) {
		return err;
	}

	for (i = 0; i < discovery_results_count; i++) {
		struct discovery_priv *priv;
		struct bt_hci_cp_remote_name_cancel *cp;
		struct net_buf *buf;

		priv = (struct discovery_priv *)&discovery_results[i]._priv;

		if (!priv->resolving) {
			continue;
		}

		buf = bt_hci_cmd_create(BT_HCI_OP_REMOTE_NAME_CANCEL,
					sizeof(*cp));
		if (!buf) {
			continue;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		bt_addr_copy(&cp->bdaddr, &discovery_results[i].addr);

		bt_hci_cmd_send_sync(BT_HCI_OP_REMOTE_NAME_CANCEL, buf, NULL);
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_INQUIRY);

	discovery_cb = NULL;
	discovery_results = NULL;
	discovery_results_size = 0;
	discovery_results_count = 0;

	return 0;
}

static int write_scan_enable(u8_t scan)
{
	struct net_buf *buf;
	int err;

	BT_DBG("type %u", scan);

	buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_SCAN_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_u8(buf, scan);
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
#endif /* CONFIG_BLUETOOTH_BREDR */

void bt_storage_register(const struct bt_storage *storage)
{
	bt_storage = storage;
}

static int bt_storage_clear_all(void)
{
	if (IS_ENABLED(CONFIG_BLUETOOTH_CONN)) {
		bt_conn_disconnect_all();
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_SMP)) {
		bt_keys_clear_all();
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_BREDR)) {
		bt_keys_link_key_clear_addr(NULL);
	}

	if (bt_storage) {
		return bt_storage->clear(NULL);
	}

	return 0;
}

int bt_storage_clear(const bt_addr_le_t *addr)
{
	if (!addr) {
		return bt_storage_clear_all();
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_CONN)) {
		struct bt_conn *conn = bt_conn_lookup_addr_le(addr);
		if (conn) {
			bt_conn_disconnect(conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			bt_conn_unref(conn);
		}
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_BREDR)) {
		/* LE Public may indicate BR/EDR as well */
		if (addr->type == BT_ADDR_LE_PUBLIC) {
			bt_keys_link_key_clear_addr(&addr->a);
		}
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_SMP)) {
		struct bt_keys *keys = bt_keys_find_addr(addr);
		if (keys) {
			bt_keys_clear(keys);
		}
	}

	if (bt_storage) {
		return bt_storage->clear(addr);
	}

	return 0;
}

u16_t bt_hci_get_cmd_opcode(struct net_buf *buf)
{
	return cmd(buf)->opcode;
}

int bt_pub_key_gen(struct bt_pub_key_cb *new_cb)
{
	struct bt_pub_key_cb *cb;
	int err;

	/*
	 * We check for both "LE Read Local P-256 Public Key" and
	 * "LE Generate DH Key" support here since both commands are needed for
	 * ECC support. If "LE Generate DH Key" is not supported then there
	 * is no point in reading local public key.
	 */
	if (!(bt_dev.supported_commands[34] & 0x02) ||
	    !(bt_dev.supported_commands[34] & 0x04)) {
		BT_WARN("ECC HCI commands not available");
		return -ENOTSUP;
	}

	new_cb->_next = pub_key_cb;
	pub_key_cb = new_cb;

	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY)) {
		return 0;
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_P256_PUBLIC_KEY, NULL, NULL);
	if (err) {
		BT_ERR("Sending LE P256 Public Key command failed");
		atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);
		pub_key_cb = NULL;
		return err;
	}

	for (cb = pub_key_cb; cb; cb = cb->_next) {
		if (cb != new_cb) {
			cb->func(NULL);
		}
	}

	return 0;
}

const u8_t *bt_pub_key_get(void)
{
	if (atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY)) {
		return pub_key;
	}

	return NULL;
}

int bt_dh_key_gen(const u8_t remote_pk[64], bt_dh_key_cb_t cb)
{
	struct bt_hci_cp_le_generate_dhkey *cp;
	struct net_buf *buf;
	int err;

	if (dh_key_cb || atomic_test_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY)) {
		return -EBUSY;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY)) {
		return -EADDRNOTAVAIL;
	}

	dh_key_cb = cb;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_GENERATE_DHKEY, sizeof(*cp));
	if (!buf) {
		dh_key_cb = NULL;
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memcpy(cp->key, remote_pk, sizeof(cp->key));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_GENERATE_DHKEY, buf, NULL);
	if (err) {
		dh_key_cb = NULL;
		return err;
	}

	return 0;
}

#if defined(CONFIG_BLUETOOTH_BREDR)
int bt_br_oob_get_local(struct bt_br_oob *oob)
{
	bt_addr_copy(&oob->addr, &bt_dev.id_addr.a);

	return 0;
}
#endif /* CONFIG_BLUETOOTH_BREDR */

int bt_le_oob_get_local(struct bt_le_oob *oob)
{
	if (IS_ENABLED(CONFIG_BLUETOOTH_PRIVACY)) {
		int err;

		/* Invalidate RPA so a new one is generated */
		atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);

		err = le_set_private_addr();
		if (err) {
			return err;
		}

		bt_addr_le_copy(&oob->addr, &bt_dev.random_addr);
	} else {
		bt_addr_le_copy(&oob->addr, &bt_dev.id_addr);
	}

	return 0;
}
