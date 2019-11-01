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
#include <sys/atomic.h>
#include <sys/util.h>
#include <sys/slist.h>
#include <sys/byteorder.h>
#include <debug/stack.h>
#include <sys/__assert.h>
#include <soc.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_vs.h>
#include <bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_hci_core
#include "common/log.h"

#include "common/rpa.h"
#include "keys.h"
#include "monitor.h"
#include "hci_core.h"
#include "hci_ecc.h"
#include "ecc.h"

#include "conn_internal.h"
#include "l2cap_internal.h"
#include "gatt_internal.h"
#include "smp.h"
#include "crypto.h"
#include "settings.h"

/* Peripheral timeout to initialize Connection Parameter Update procedure */
#define CONN_UPDATE_TIMEOUT  K_MSEC(CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT)
#define RPA_TIMEOUT          K_SECONDS(CONFIG_BT_RPA_TIMEOUT)

#define HCI_CMD_TIMEOUT      K_SECONDS(10)

/* Stacks for the threads */
#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
static struct k_thread rx_thread_data;
static K_THREAD_STACK_DEFINE(rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);
#endif
static struct k_thread tx_thread_data;
static K_THREAD_STACK_DEFINE(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);

static void init_work(struct k_work *work);

struct bt_dev bt_dev = {
	.init          = Z_WORK_INITIALIZER(init_work),
	/* Give cmd_sem allowing to send first HCI_Reset cmd, the only
	 * exception is if the controller requests to wait for an
	 * initial Command Complete for NOP.
	 */
#if !defined(CONFIG_BT_WAIT_NOP)
	.ncmd_sem      = Z_SEM_INITIALIZER(bt_dev.ncmd_sem, 1, 1),
#else
	.ncmd_sem      = Z_SEM_INITIALIZER(bt_dev.ncmd_sem, 0, 1),
#endif
	.cmd_tx_queue  = Z_FIFO_INITIALIZER(bt_dev.cmd_tx_queue),
#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
	.rx_queue      = Z_FIFO_INITIALIZER(bt_dev.rx_queue),
#endif
};

static bt_ready_cb_t ready_cb;

static bt_le_scan_cb_t *scan_dev_found_cb;

#if defined(CONFIG_BT_HCI_VS_EVT_USER)
static bt_hci_vnd_evt_cb_t *hci_vnd_evt_cb;
#endif /* CONFIG_BT_HCI_VS_EVT_USER */

#if defined(CONFIG_BT_ECC)
static u8_t pub_key[64];
static struct bt_pub_key_cb *pub_key_cb;
static bt_dh_key_cb_t dh_key_cb;
#endif /* CONFIG_BT_ECC */

#if defined(CONFIG_BT_BREDR)
static bt_br_discovery_cb_t *discovery_cb;
struct bt_br_discovery_result *discovery_results;
static size_t discovery_results_size;
static size_t discovery_results_count;
#endif /* CONFIG_BT_BREDR */

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

	/* Index into the bt_conn storage array */
	u8_t  id;

	/** ACL connection handle */
	u16_t handle;
};

#define cmd(buf) ((struct cmd_data *)net_buf_user_data(buf))
#define acl(buf) ((struct acl_data *)net_buf_user_data(buf))

/* HCI command buffers. Derive the needed size from BT_BUF_RX_SIZE since
 * the same buffer is also used for the response.
 */
#define CMD_BUF_SIZE BT_BUF_RX_SIZE
NET_BUF_POOL_DEFINE(hci_cmd_pool, CONFIG_BT_HCI_CMD_COUNT,
		    CMD_BUF_SIZE, BT_BUF_USER_DATA_MIN, NULL);

NET_BUF_POOL_DEFINE(hci_rx_pool, CONFIG_BT_RX_BUF_COUNT,
		    BT_BUF_RX_SIZE, BT_BUF_USER_DATA_MIN, NULL);

#if defined(CONFIG_BT_CONN)
/* Dedicated pool for HCI_Number_of_Completed_Packets. This event is always
 * consumed synchronously by bt_recv_prio() so a single buffer is enough.
 * Having a dedicated pool for it ensures that exhaustion of the RX pool
 * cannot block the delivery of this priority event.
 */
NET_BUF_POOL_DEFINE(num_complete_pool, 1, BT_BUF_RX_SIZE,
		    BT_BUF_USER_DATA_MIN, NULL);
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
NET_BUF_POOL_DEFINE(discardable_pool, CONFIG_BT_DISCARDABLE_BUF_COUNT,
		    BT_BUF_RX_SIZE, BT_BUF_USER_DATA_MIN, NULL);
#endif /* CONFIG_BT_DISCARDABLE_BUF_COUNT */

struct event_handler {
	u8_t event;
	u8_t min_len;
	void (*handler)(struct net_buf *buf);
};

#define EVENT_HANDLER(_evt, _handler, _min_len) \
{ \
	.event = _evt, \
	.handler = _handler, \
	.min_len = _min_len, \
}

static inline void handle_event(u8_t event, struct net_buf *buf,
				const struct event_handler *handlers,
				size_t num_handlers)
{
	size_t i;

	for (i = 0; i < num_handlers; i++) {
		const struct event_handler *handler = &handlers[i];

		if (handler->event != event) {
			continue;
		}

		if (buf->len < handler->min_len) {
			BT_ERR("Too small (%u bytes) event 0x%02x",
			       buf->len, event);
			return;
		}

		handler->handler(buf);
		return;
	}

	BT_WARN("Unhandled event 0x%02x len %u: %s", event,
		buf->len, bt_hex(buf->data, buf->len));
}

static inline bool is_wl_empty(void)
{
#if defined(CONFIG_BT_WHITELIST)
	return !bt_dev.le.wl_entries;
#else
	return true;
#endif /* defined(CONFIG_BT_WHITELIST) */
}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static void report_completed_packet(struct net_buf *buf)
{

	struct bt_hci_cp_host_num_completed_packets *cp;
	u16_t handle = acl(buf)->handle;
	struct bt_hci_handle_count *hc;
	struct bt_conn *conn;

	net_buf_destroy(buf);

	/* Do nothing if controller to host flow control is not supported */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 10, 5)) {
		return;
	}

	conn = bt_conn_lookup_id(acl(buf)->id);
	if (!conn) {
		BT_WARN("Unable to look up conn with id 0x%02x", acl(buf)->id);
		return;
	}

	if (conn->state != BT_CONN_CONNECTED &&
	    conn->state != BT_CONN_DISCONNECT) {
		BT_WARN("Not reporting packet for non-connected conn");
		bt_conn_unref(conn);
		return;
	}

	bt_conn_unref(conn);

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

#define ACL_IN_SIZE BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_RX_MTU)
NET_BUF_POOL_DEFINE(acl_in_pool, CONFIG_BT_ACL_RX_COUNT, ACL_IN_SIZE,
		    BT_BUF_USER_DATA_MIN, report_completed_packet);
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

struct net_buf *bt_hci_cmd_create(u16_t opcode, u8_t param_len)
{
	struct bt_hci_cmd_hdr *hdr;
	struct net_buf *buf;

	BT_DBG("opcode 0x%04x param_len %u", opcode, param_len);

	buf = net_buf_alloc(&hci_cmd_pool, K_FOREVER);
	__ASSERT_NO_MSG(buf);

	BT_DBG("buf %p", buf);

	net_buf_reserve(buf, CONFIG_BT_HCI_RESERVE);

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
		switch (cmd(buf)->status) {
		case BT_HCI_ERR_CONN_LIMIT_EXCEEDED:
			err = -ECONNREFUSED;
			break;
		default:
			err = -EIO;
			break;
		}

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

#if defined(CONFIG_BT_OBSERVER) || defined(CONFIG_BT_CONN)
const bt_addr_le_t *bt_lookup_id_addr(u8_t id, const bt_addr_le_t *addr)
{
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		struct bt_keys *keys;

		keys = bt_keys_find_irk(id, addr);
		if (keys) {
			BT_DBG("Identity %s matched RPA %s",
			       bt_addr_le_str(&keys->addr),
			       bt_addr_le_str(addr));
			return &keys->addr;
		}
	}

	return addr;
}
#endif /* CONFIG_BT_OBSERVER || CONFIG_BT_CONN */

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

	atomic_set_bit_to(bt_dev.flags, BT_DEV_ADVERTISING, enable);

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

int bt_addr_from_str(const char *str, bt_addr_t *addr)
{
	int i, j;
	u8_t tmp;

	if (strlen(str) != 17U) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		addr->val[i] = addr->val[i] << 4;

		if (char2hex(*str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
	}

	return 0;
}

int bt_addr_le_from_str(const char *str, const char *type, bt_addr_le_t *addr)
{
	int err;

	err = bt_addr_from_str(str, &addr->a);
	if (err < 0) {
		return err;
	}

	if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
		addr->type = BT_ADDR_LE_RANDOM;
	} else if (!strcmp(type, "public-id") || !strcmp(type, "(public-id)")) {
		addr->type = BT_ADDR_LE_PUBLIC_ID;
	} else if (!strcmp(type, "random-id") || !strcmp(type, "(random-id)")) {
		addr->type = BT_ADDR_LE_RANDOM_ID;
	} else {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_PRIVACY)
/* this function sets new RPA only if current one is no longer valid */
static int le_set_private_addr(u8_t id)
{
	bt_addr_t rpa;
	int err;

	/* check if RPA is valid */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_RPA_VALID)) {
		return 0;
	}

	err = bt_rpa_create(bt_dev.irk[id], &rpa);
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
	int err_adv = 0, err_scan = 0;

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
		err_adv = le_set_private_addr(bt_dev.adv_id);
		set_advertise_enable(true);
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN)) {
		/* TODO do we need to toggle scan? */
		err_scan = le_set_private_addr(BT_ID_DEFAULT);
	}

	/* If both advertising and scanning is active, le_set_private_addr
	 * will fail. In this case, set back RPA_VALID so that if either of
	 * advertising or scanning was restarted by application then
	 * le_set_private_addr in the public API call path will not retry
	 * set_random_address. This is needed so as to be able to stop and
	 * restart either of the role by the application after rpa_timeout.
	 */
	if (err_adv || err_scan) {
		atomic_set_bit(bt_dev.flags, BT_DEV_RPA_VALID);
	}
}
#else
static int le_set_private_addr(u8_t id)
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

#if defined(CONFIG_BT_OBSERVER)
static int set_le_scan_enable(u8_t enable)
{
	struct bt_hci_cp_le_set_scan_enable *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_ENABLE, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	if (enable == BT_HCI_LE_SCAN_ENABLE) {
		cp->filter_dup = atomic_test_bit(bt_dev.flags,
						 BT_DEV_SCAN_FILTER_DUP);
	} else {
		cp->filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE;
	}

	cp->enable = enable;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_SCAN_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	atomic_set_bit_to(bt_dev.flags, BT_DEV_SCANNING,
			  enable == BT_HCI_LE_SCAN_ENABLE);

	return 0;
}
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
static void hci_acl(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	u16_t handle, len;
	struct bt_conn *conn;
	u8_t flags;

	BT_DBG("buf %p", buf);

	BT_ASSERT(buf->len >= sizeof(*hdr));

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = sys_le16_to_cpu(hdr->len);
	handle = sys_le16_to_cpu(hdr->handle);
	flags = bt_acl_flags(handle);

	acl(buf)->handle = bt_acl_handle(handle);
	acl(buf)->id = BT_CONN_ID_INVALID;

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

	acl(buf)->id = bt_conn_index(conn);

	bt_conn_recv(conn, buf, flags);
	bt_conn_unref(conn);
}

static void hci_data_buf_overflow(struct net_buf *buf)
{
	struct bt_hci_evt_data_buf_overflow *evt = (void *)buf->data;

	BT_WARN("Data buffer overflow (link type 0x%02x)", evt->link_type);
}

static void hci_num_completed_packets(struct net_buf *buf)
{
	struct bt_hci_evt_num_completed_packets *evt = (void *)buf->data;
	int i;

	BT_DBG("num_handles %u", evt->num_handles);

	for (i = 0; i < evt->num_handles; i++) {
		u16_t handle, count;
		struct bt_conn *conn;
		unsigned int key;

		handle = sys_le16_to_cpu(evt->h[i].handle);
		count = sys_le16_to_cpu(evt->h[i].count);

		BT_DBG("handle %u count %u", handle, count);

		key = irq_lock();

		conn = bt_conn_lookup_handle(handle);
		if (!conn) {
			irq_unlock(key);
			BT_ERR("No connection for handle %u", handle);
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

#if defined(CONFIG_BT_CENTRAL)
#if defined(CONFIG_BT_WHITELIST)
int bt_le_auto_conn(const struct bt_le_conn_param *conn_param)
{
	struct net_buf *buf;
	struct bt_hci_cp_le_create_conn *cp;
	u8_t own_addr_type;
	int err;

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		err = set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		err = le_set_private_addr(BT_ID_DEFAULT);
		if (err) {
			return err;
		}
		if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			own_addr_type = BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
		} else {
			own_addr_type = BT_ADDR_LE_RANDOM;
		}
	} else {
		const bt_addr_le_t *addr = &bt_dev.id_addr[BT_ID_DEFAULT];

		/* If Static Random address is used as Identity address we
		 * need to restore it before creating connection. Otherwise
		 * NRPA used for active scan could be used for connection.
		 */
		if (addr->type == BT_ADDR_LE_RANDOM) {
			err = set_random_address(&addr->a);
			if (err) {
				return err;
			}
		}

		own_addr_type = addr->type;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_CONN, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->filter_policy = BT_HCI_LE_CREATE_CONN_FP_WHITELIST;
	cp->own_addr_type = own_addr_type;

	/* User Initiated procedure use fast scan parameters. */
	cp->scan_interval = sys_cpu_to_le16(BT_GAP_SCAN_FAST_INTERVAL);
	cp->scan_window = sys_cpu_to_le16(BT_GAP_SCAN_FAST_WINDOW);

	cp->conn_interval_min = sys_cpu_to_le16(conn_param->interval_min);
	cp->conn_interval_max = sys_cpu_to_le16(conn_param->interval_max);
	cp->conn_latency = sys_cpu_to_le16(conn_param->latency);
	cp->supervision_timeout = sys_cpu_to_le16(conn_param->timeout);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CONN, buf, NULL);
}
#endif /* defined(CONFIG_BT_WHITELIST) */

static int hci_le_create_conn(const struct bt_conn *conn)
{
	struct net_buf *buf;
	struct bt_hci_cp_le_create_conn *cp;
	u8_t own_addr_type;
	const bt_addr_le_t *peer_addr;
	int err;

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		err = le_set_private_addr(conn->id);
		if (err) {
			return err;
		}

		if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			own_addr_type = BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
		} else {
			own_addr_type = BT_ADDR_LE_RANDOM;
		}
	} else {
		/* If Static Random address is used as Identity address we
		 * need to restore it before creating connection. Otherwise
		 * NRPA used for active scan could be used for connection.
		 */
		const bt_addr_le_t *own_addr = &bt_dev.id_addr[conn->id];

		if (own_addr->type == BT_ADDR_LE_RANDOM) {
			err = set_random_address(&own_addr->a);
			if (err) {
				return err;
			}
		}

		own_addr_type = own_addr->type;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_CONN, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	/* Interval == window for continuous scanning */
	cp->scan_interval = sys_cpu_to_le16(BT_GAP_SCAN_FAST_INTERVAL);
	cp->scan_window = cp->scan_interval;

	peer_addr = &conn->le.dst;
#if defined(CONFIG_BT_SMP)
	if (!bt_dev.le.rl_size || bt_dev.le.rl_entries > bt_dev.le.rl_size) {
		/* Host resolving is used, use the RPA directly. */
		peer_addr = &conn->le.resp_addr;
	}
#endif
	bt_addr_le_copy(&cp->peer_addr, peer_addr);
	cp->own_addr_type = own_addr_type;
	cp->conn_interval_min = sys_cpu_to_le16(conn->le.interval_min);
	cp->conn_interval_max = sys_cpu_to_le16(conn->le.interval_max);
	cp->conn_latency = sys_cpu_to_le16(conn->le.latency);
	cp->supervision_timeout = sys_cpu_to_le16(conn->le.timeout);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CONN, buf, NULL);
}
#endif /* CONFIG_BT_CENTRAL */

static void hci_disconn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_disconn_complete *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status 0x%02x handle %u reason 0x%02x", evt->status, handle,
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

	/* Check stacks usage */
#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
	STACK_ANALYZE("rx stack", rx_thread_stack);
#endif
	STACK_ANALYZE("tx stack", tx_thread_stack);

	bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
	conn->handle = 0U;

	if (conn->type != BT_CONN_TYPE_LE) {
#if defined(CONFIG_BT_BREDR)
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

#if defined(CONFIG_BT_CENTRAL) && !defined(CONFIG_BT_WHITELIST)
	if (atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT)) {
		bt_conn_set_state(conn, BT_CONN_CONNECT_SCAN);
		bt_le_scan_update(false);
	}
#endif /* defined(CONFIG_BT_CENTRAL) && !defined(CONFIG_BT_WHITELIST) */

	bt_conn_unref(conn);

advertise:
	if (atomic_test_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING) &&
	    !atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
			le_set_private_addr(bt_dev.adv_id);
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

/* LE Data Length Change Event is optional so this function just ignore
 * error and stack will continue to use default values.
 */
static void hci_le_set_data_len(struct bt_conn *conn)
{
	struct bt_hci_rp_le_read_max_data_len *rp;
	struct bt_hci_cp_le_set_data_len *cp;
	struct net_buf *buf, *rsp;
	u16_t tx_octets, tx_time;
	int err;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_MAX_DATA_LEN, NULL, &rsp);
	if (err) {
		BT_ERR("Failed to read DLE max data len");
		return;
	}

	rp = (void *)rsp->data;
	tx_octets = sys_le16_to_cpu(rp->max_tx_octets);
	tx_time = sys_le16_to_cpu(rp->max_tx_time);
	net_buf_unref(rsp);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_DATA_LEN, sizeof(*cp));
	if (!buf) {
		BT_ERR("Failed to create LE Set Data Length Command");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->tx_octets = sys_cpu_to_le16(tx_octets);
	cp->tx_time = sys_cpu_to_le16(tx_time);
	err = bt_hci_cmd_send(BT_HCI_OP_LE_SET_DATA_LEN, buf);
	if (err) {
		BT_ERR("Failed to send LE Set Data Length Command");
	}
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
	cp->all_phys = 0U;
	cp->tx_phys = BT_HCI_LE_PHY_PREFER_2M;
	cp->rx_phys = BT_HCI_LE_PHY_PREFER_2M;
	cp->phy_opts = BT_HCI_LE_PHY_CODED_ANY;
	bt_hci_cmd_send(BT_HCI_OP_LE_SET_PHY, buf);

	return 0;
}

static void slave_update_conn_param(struct bt_conn *conn)
{
	if (!IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		return;
	}

	/* don't start timer again on PHY update etc */
	if (atomic_test_bit(conn->flags, BT_CONN_SLAVE_PARAM_UPDATE)) {
		return;
	}

	/*
	 * Core 4.2 Vol 3, Part C, 9.3.12.2
	 * The Peripheral device should not perform a Connection Parameter
	 * Update procedure within 5 s after establishing a connection.
	 */
	k_delayed_work_submit(&conn->update_work, CONN_UPDATE_TIMEOUT);
}

#if defined(CONFIG_BT_SMP)
static void update_pending_id(struct bt_keys *keys, void *data)
{
	if (keys->flags & BT_KEYS_ID_PENDING_ADD) {
		keys->flags &= ~BT_KEYS_ID_PENDING_ADD;
		bt_id_add(keys);
		return;
	}

	if (keys->flags & BT_KEYS_ID_PENDING_DEL) {
		keys->flags &= ~BT_KEYS_ID_PENDING_DEL;
		bt_id_del(keys);
		return;
	}
}
#endif

static struct bt_conn *find_pending_connect(bt_addr_le_t *peer_addr)
{
	struct bt_conn *conn;

	/*
	 * Make lookup to check if there's a connection object in
	 * CONNECT or DIR_ADV state associated with passed peer LE address.
	 */
	conn = bt_conn_lookup_state_le(peer_addr, BT_CONN_CONNECT);
	if (conn) {
		return conn;
	}

	return bt_conn_lookup_state_le(peer_addr, BT_CONN_CONNECT_DIR_ADV);
}

static void enh_conn_complete(struct bt_hci_evt_le_enh_conn_complete *evt)
{
	u16_t handle = sys_le16_to_cpu(evt->handle);
	bt_addr_le_t peer_addr, id_addr;
	struct bt_conn *conn;
	int err;

	BT_DBG("status 0x%02x handle %u role %u %s", evt->status, handle,
	       evt->role, bt_addr_le_str(&evt->peer_addr));

#if defined(CONFIG_BT_SMP)
	if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_ID_PENDING)) {
		bt_keys_foreach(BT_KEYS_IRK, update_pending_id, NULL);
	}
#endif

	if (evt->status) {
		/*
		 * If there was an error we are only interested in pending
		 * connection. There is no need to check ID address as
		 * only one connection can be in that state.
		 *
		 * Depending on error code address might not be valid anyway.
		 */
		conn = find_pending_connect(NULL);
		if (!conn) {
			return;
		}

		conn->err = evt->status;

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			/*
			 * Handle advertising timeout after high duty directed
			 * advertising.
			 */
			if (conn->err == BT_HCI_ERR_ADV_TIMEOUT) {
				atomic_clear_bit(bt_dev.flags,
						 BT_DEV_ADVERTISING);
				bt_conn_set_state(conn, BT_CONN_DISCONNECTED);

				goto done;
			}
		}

		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			/*
			 * Handle cancellation of outgoing connection attempt.
			 */
			if (conn->err == BT_HCI_ERR_UNKNOWN_CONN_ID) {
				/* We notify before checking autoconnect flag
				 * as application may choose to change it from
				 * callback.
				 */
				bt_conn_set_state(conn, BT_CONN_DISCONNECTED);

#if !defined(CONFIG_BT_WHITELIST)
				/* Check if device is marked for autoconnect. */
				if (atomic_test_bit(conn->flags,
						    BT_CONN_AUTO_CONNECT)) {
					bt_conn_set_state(conn,
							  BT_CONN_CONNECT_SCAN);
				}
#endif /* !defined(CONFIG_BT_WHITELIST) */
				goto done;
			}
		}

		BT_WARN("Unexpected status 0x%02x", evt->status);

		bt_conn_unref(conn);

		return;
	}

	bt_addr_le_copy(&id_addr, &evt->peer_addr);

	/* Translate "enhanced" identity address type to normal one */
	if (id_addr.type == BT_ADDR_LE_PUBLIC_ID ||
	    id_addr.type == BT_ADDR_LE_RANDOM_ID) {
		id_addr.type -= BT_ADDR_LE_PUBLIC_ID;
		bt_addr_copy(&peer_addr.a, &evt->peer_rpa);
		peer_addr.type = BT_ADDR_LE_RANDOM;
	} else {
		bt_addr_le_copy(&peer_addr, &evt->peer_addr);
	}

	conn = find_pending_connect(&id_addr);

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    evt->role == BT_HCI_ROLE_SLAVE) {
		/*
		 * clear advertising even if we are not able to add connection
		 * object to keep host in sync with controller state
		 */
		atomic_clear_bit(bt_dev.flags, BT_DEV_ADVERTISING);

		/* for slave we may need to add new connection */
		if (!conn) {
			conn = bt_conn_add_le(bt_dev.adv_id, &id_addr);
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    IS_ENABLED(CONFIG_BT_WHITELIST) &&
	    evt->role == BT_HCI_ROLE_MASTER) {
		/* for whitelist initiator me may need to add new connection. */
		if (!conn) {
			conn = bt_conn_add_le(BT_ID_DEFAULT, &id_addr);
		}
	}

	if (!conn) {
		BT_ERR("Unable to add new conn for handle %u", handle);
		return;
	}

	conn->handle = handle;
	bt_addr_le_copy(&conn->le.dst, &id_addr);
	conn->le.interval = sys_le16_to_cpu(evt->interval);
	conn->le.latency = sys_le16_to_cpu(evt->latency);
	conn->le.timeout = sys_le16_to_cpu(evt->supv_timeout);
	conn->role = evt->role;
	conn->err = 0U;

	/*
	 * Use connection address (instead of identity address) as initiator
	 * or responder address. Only slave needs to be updated. For master all
	 * was set during outgoing connection creation.
	 */
	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    conn->role == BT_HCI_ROLE_SLAVE) {
		bt_addr_le_copy(&conn->le.init_addr, &peer_addr);

		if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
			bt_addr_copy(&conn->le.resp_addr.a, &evt->local_rpa);
			conn->le.resp_addr.type = BT_ADDR_LE_RANDOM;
		} else {
			bt_addr_le_copy(&conn->le.resp_addr,
					&bt_dev.id_addr[conn->id]);
		}

		/* if the controller supports, lets advertise for another
		 * slave connection.
		 * check for connectable advertising state is sufficient as
		 * this is how this le connection complete for slave occurred.
		 */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING) &&
		    BT_LE_STATES_SLAVE_CONN_ADV(bt_dev.le.states)) {
			if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
				le_set_private_addr(bt_dev.adv_id);
			}

			set_advertise_enable(true);
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_MASTER) {
		if (IS_ENABLED(CONFIG_BT_WHITELIST) &&
		    atomic_test_bit(bt_dev.flags, BT_DEV_AUTO_CONN)) {
			conn->id = BT_ID_DEFAULT;
			atomic_clear_bit(bt_dev.flags, BT_DEV_AUTO_CONN);
		}

		bt_addr_le_copy(&conn->le.resp_addr, &peer_addr);

		if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
			bt_addr_copy(&conn->le.init_addr.a, &evt->local_rpa);
			conn->le.init_addr.type = BT_ADDR_LE_RANDOM;
		} else {
			bt_addr_le_copy(&conn->le.init_addr,
					&bt_dev.id_addr[conn->id]);
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

	if (IS_ENABLED(CONFIG_BT_AUTO_PHY_UPDATE) &&
	    BT_FEAT_LE_PHY_2M(bt_dev.le.features)) {
		err = hci_le_set_phy(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_PHY_UPDATE);
			goto done;
		}
	}

	if (IS_ENABLED(CONFIG_BT_DATA_LEN_UPDATE) &&
	    BT_FEAT_LE_DLE(bt_dev.le.features)) {
		hci_le_set_data_len(conn);
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    conn->role == BT_CONN_ROLE_SLAVE) {
		slave_update_conn_param(conn);
	}

done:
	bt_conn_unref(conn);
	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		bt_le_scan_update(false);
	}
}

static void le_enh_conn_complete(struct net_buf *buf)
{
	enh_conn_complete((void *)buf->data);
}

static void le_legacy_conn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *evt = (void *)buf->data;
	struct bt_hci_evt_le_enh_conn_complete enh;
	const bt_addr_le_t *id_addr;

	BT_DBG("status 0x%02x role %u %s", evt->status, evt->role,
	       bt_addr_le_str(&evt->peer_addr));

	enh.status         = evt->status;
	enh.handle         = evt->handle;
	enh.role           = evt->role;
	enh.interval       = evt->interval;
	enh.latency        = evt->latency;
	enh.supv_timeout   = evt->supv_timeout;
	enh.clock_accuracy = evt->clock_accuracy;

	bt_addr_le_copy(&enh.peer_addr, &evt->peer_addr);

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		bt_addr_copy(&enh.local_rpa, &bt_dev.random_addr.a);
	} else {
		bt_addr_copy(&enh.local_rpa, BT_ADDR_ANY);
	}

	if (evt->role == BT_HCI_ROLE_SLAVE) {
		id_addr = bt_lookup_id_addr(bt_dev.adv_id, &enh.peer_addr);
	} else {
		id_addr = bt_lookup_id_addr(BT_ID_DEFAULT, &enh.peer_addr);
	}

	if (id_addr != &enh.peer_addr) {
		bt_addr_copy(&enh.peer_rpa, &enh.peer_addr.a);
		bt_addr_le_copy(&enh.peer_addr, id_addr);
		enh.peer_addr.type += BT_ADDR_LE_PUBLIC_ID;
	} else {
		bt_addr_copy(&enh.peer_rpa, BT_ADDR_ANY);
	}

	enh_conn_complete(&enh);
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

	if (IS_ENABLED(CONFIG_BT_AUTO_PHY_UPDATE) &&
	    BT_FEAT_LE_PHY_2M(bt_dev.le.features) &&
	    BT_FEAT_LE_PHY_2M(conn->le.features)) {
		int err;

		err = hci_le_set_phy(conn);
		if (!err) {
			atomic_set_bit(conn->flags, BT_CONN_AUTO_PHY_UPDATE);
			goto done;
		}
	}

	if (IS_ENABLED(CONFIG_BT_DATA_LEN_UPDATE) &&
	    BT_FEAT_LE_DLE(bt_dev.le.features) &&
	    BT_FEAT_LE_DLE(conn->le.features)) {
		hci_le_set_data_len(conn);
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    conn->role == BT_CONN_ROLE_SLAVE) {
		slave_update_conn_param(conn);
	}

done:
	bt_conn_unref(conn);
}

#if defined(CONFIG_BT_DATA_LEN_UPDATE)
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

	/* TODO use those */

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_DATA_LEN_UPDATE */

#if defined(CONFIG_BT_PHY_UPDATE)
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

	BT_DBG("PHY updated: status: 0x%02x, tx: %u, rx: %u",
	       evt->status, evt->tx_phy, evt->rx_phy);

	if (!IS_ENABLED(CONFIG_BT_AUTO_PHY_UPDATE) ||
	    !atomic_test_and_clear_bit(conn->flags, BT_CONN_AUTO_PHY_UPDATE)) {
		goto done;
	}

	if (IS_ENABLED(CONFIG_BT_DATA_LEN_UPDATE) &&
	    BT_FEAT_LE_DLE(bt_dev.le.features) &&
	    BT_FEAT_LE_DLE(conn->le.features)) {
		hci_le_set_data_len(conn);
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    conn->role == BT_CONN_ROLE_SLAVE) {
		slave_update_conn_param(conn);
	}

done:
	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_PHY_UPDATE */

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
	    ((param->timeout * 4U) <=
	     ((1 + param->latency) * param->interval_max))) {
		return false;
	}

	return true;
}

static void le_conn_param_neg_reply(u16_t handle, u8_t reason)
{
	struct bt_hci_cp_le_conn_param_req_neg_reply *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY,
				sizeof(*cp));
	if (!buf) {
		BT_ERR("Unable to allocate buffer");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->reason = sys_cpu_to_le16(reason);

	bt_hci_cmd_send(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY, buf);
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
	(void)memset(cp, 0, sizeof(*cp));

	cp->handle = sys_cpu_to_le16(handle);
	cp->interval_min = sys_cpu_to_le16(param->interval_min);
	cp->interval_max = sys_cpu_to_le16(param->interval_max);
	cp->latency = sys_cpu_to_le16(param->latency);
	cp->timeout = sys_cpu_to_le16(param->timeout);

	return bt_hci_cmd_send(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY, buf);
}

static void le_conn_param_req(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_param_req *evt = (void *)buf->data;
	struct bt_le_conn_param param;
	struct bt_conn *conn;
	u16_t handle;

	handle = sys_le16_to_cpu(evt->handle);
	param.interval_min = sys_le16_to_cpu(evt->interval_min);
	param.interval_max = sys_le16_to_cpu(evt->interval_max);
	param.latency = sys_le16_to_cpu(evt->latency);
	param.timeout = sys_le16_to_cpu(evt->timeout);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		le_conn_param_neg_reply(handle, BT_HCI_ERR_UNKNOWN_CONN_ID);
		return;
	}

	if (!le_param_req(conn, &param)) {
		le_conn_param_neg_reply(handle, BT_HCI_ERR_INVALID_LL_PARAM);
	} else {
		le_conn_param_req_reply(handle, &param);
	}

	bt_conn_unref(conn);
}

static void le_conn_update_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_update_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status 0x%02x, handle %u", evt->status, handle);

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
	} else if (evt->status == BT_HCI_ERR_UNSUPP_REMOTE_FEATURE &&
		   conn->role == BT_HCI_ROLE_SLAVE &&
		   !atomic_test_and_set_bit(conn->flags,
					    BT_CONN_SLAVE_PARAM_L2CAP)) {
		/* CPR not supported, let's try L2CAP CPUP instead */
		struct bt_le_conn_param param;

		param.interval_min = conn->le.interval_min;
		param.interval_max = conn->le.interval_max;
		param.latency = conn->le.pending_latency;
		param.timeout = conn->le.pending_timeout;

		bt_l2cap_update_conn_param(conn, &param);
	}

	bt_conn_unref(conn);
}

#if defined(CONFIG_BT_CENTRAL)
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

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING) &&
	    set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE)) {
		goto failed;
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
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static int set_flow_control(void)
{
	struct bt_hci_cp_host_buffer_size *hbs;
	struct net_buf *buf;
	int err;

	/* Check if host flow control is actually supported */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 10, 5)) {
		BT_WARN("Controller to host flow control not supported");
		return 0;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_HOST_BUFFER_SIZE,
				sizeof(*hbs));
	if (!buf) {
		return -ENOBUFS;
	}

	hbs = net_buf_add(buf, sizeof(*hbs));
	(void)memset(hbs, 0, sizeof(*hbs));
	hbs->acl_mtu = sys_cpu_to_le16(CONFIG_BT_L2CAP_RX_MTU +
				       sizeof(struct bt_l2cap_hdr));
	hbs->acl_pkts = sys_cpu_to_le16(CONFIG_BT_ACL_RX_COUNT);

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
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

static int bt_clear_all_pairings(u8_t id)
{
	bt_conn_disconnect_all(id);

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		bt_keys_clear_all(id);
	}

	if (IS_ENABLED(CONFIG_BT_BREDR)) {
		bt_keys_link_key_clear_addr(NULL);
	}

	return 0;
}

int bt_unpair(u8_t id, const bt_addr_le_t *addr)
{
	struct bt_keys *keys = NULL;
	struct bt_conn *conn;

	if (id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	if (!addr || !bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
		return bt_clear_all_pairings(id);
	}

	conn = bt_conn_lookup_addr_le(id, addr);
	if (conn) {
		/* Clear the conn->le.keys pointer since we'll invalidate it,
		 * and don't want any subsequent code (like disconnected
		 * callbacks) accessing it.
		 */
		if (conn->type == BT_CONN_TYPE_LE) {
			keys = conn->le.keys;
			conn->le.keys = NULL;
		}

		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		bt_conn_unref(conn);
	}

	if (IS_ENABLED(CONFIG_BT_BREDR)) {
		/* LE Public may indicate BR/EDR as well */
		if (addr->type == BT_ADDR_LE_PUBLIC) {
			bt_keys_link_key_clear_addr(&addr->a);
		}
	}

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		if (!keys) {
			keys = bt_keys_find_addr(id, addr);
		}

		if (keys) {
			bt_keys_clear(keys);
		}
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_gatt_clear(id, addr);
	}

	return 0;
}

#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
static enum bt_security_err security_err_get(u8_t hci_err)
{
	switch (hci_err) {
	case BT_HCI_ERR_SUCCESS:
		return BT_SECURITY_ERR_SUCCESS;
	case BT_HCI_ERR_AUTH_FAIL:
		return BT_SECURITY_ERR_AUTH_FAIL;
	case BT_HCI_ERR_PIN_OR_KEY_MISSING:
		return BT_SECURITY_ERR_PIN_OR_KEY_MISSING;
	case BT_HCI_ERR_PAIRING_NOT_SUPPORTED:
		return BT_SECURITY_ERR_PAIR_NOT_SUPPORTED;
	case BT_HCI_ERR_PAIRING_NOT_ALLOWED:
		return BT_SECURITY_ERR_PAIR_NOT_ALLOWED;
	case BT_HCI_ERR_INVALID_PARAM:
		return BT_SECURITY_ERR_INVALID_PARAM;
	default:
		return BT_SECURITY_ERR_UNSPECIFIED;
	}
}

static void reset_pairing(struct bt_conn *conn)
{
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING);
		atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING_INITIATOR);
		atomic_clear_bit(conn->flags, BT_CONN_BR_LEGACY_SECURE);
	}
#endif /* CONFIG_BT_BREDR */

	/* Reset required security level to current operational */
	conn->required_sec_level = conn->sec_level;
}
#endif /* defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR) */

#if defined(CONFIG_BT_BREDR)
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

static bool br_sufficient_key_size(struct bt_conn *conn)
{
	struct bt_hci_cp_read_encryption_key_size *cp;
	struct bt_hci_rp_read_encryption_key_size *rp;
	struct net_buf *buf, *rsp;
	u8_t key_size;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE,
				sizeof(*cp));
	if (!buf) {
		BT_ERR("Failed to allocate command buffer");
		return false;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE,
				   buf, &rsp);
	if (err) {
		BT_ERR("Failed to read encryption key size (err %d)", err);
		return false;
	}

	if (rsp->len < sizeof(*rp)) {
		BT_ERR("Too small command complete for encryption key size");
		net_buf_unref(rsp);
		return false;
	}

	rp = (void *)rsp->data;
	key_size = rp->key_size;
	net_buf_unref(rsp);

	BT_DBG("Encryption key size is %u", key_size);

	if (conn->sec_level == BT_SECURITY_L4) {
		return key_size == BT_HCI_ENCRYPTION_KEY_SIZE_MAX;
	}

	return key_size >= BT_HCI_ENCRYPTION_KEY_SIZE_MIN;
}

static bool update_sec_level_br(struct bt_conn *conn)
{
	if (!conn->encrypt) {
		conn->sec_level = BT_SECURITY_L1;
		return true;
	}

	if (conn->br.link_key) {
		if (conn->br.link_key->flags & BT_LINK_KEY_AUTHENTICATED) {
			if (conn->encrypt == 0x02) {
				conn->sec_level = BT_SECURITY_L4;
			} else {
				conn->sec_level = BT_SECURITY_L3;
			}
		} else {
			conn->sec_level = BT_SECURITY_L2;
		}
	} else {
		BT_WARN("No BR/EDR link key found");
		conn->sec_level = BT_SECURITY_L2;
	}

	if (!br_sufficient_key_size(conn)) {
		BT_ERR("Encryption key size is not sufficient");
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
		return false;
	}

	if (conn->required_sec_level > conn->sec_level) {
		BT_ERR("Failed to set required security level");
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
		return false;
	}

	return true;
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
	conn->err = 0U;
	conn->encrypt = evt->encr_enabled;

	if (!update_sec_level_br(conn)) {
		bt_conn_unref(conn);
		return;
	}

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
	conn->br.link_key->flags = 0U;

	switch (evt->key_type) {
	case BT_LK_COMBINATION:
		/*
		 * Setting Combination Link Key as AUTHENTICATED means it was
		 * successfully generated by 16 digits wide PIN code.
		 */
		if (atomic_test_and_clear_bit(conn->flags,
					      BT_CONN_BR_LEGACY_SECURE)) {
			conn->br.link_key->flags |= BT_LINK_KEY_AUTHENTICATED;
		}
		memcpy(conn->br.link_key->val, evt->link_key, 16);
		break;
	case BT_LK_AUTH_COMBINATION_P192:
		conn->br.link_key->flags |= BT_LINK_KEY_AUTHENTICATED;
		/* fall through */
	case BT_LK_UNAUTH_COMBINATION_P192:
		/* Mark no-bond so that link-key is removed on disconnection */
		if (bt_conn_ssp_get_auth(conn) < BT_HCI_DEDICATED_BONDING) {
			atomic_set_bit(conn->flags, BT_CONN_BR_NOBOND);
		}

		memcpy(conn->br.link_key->val, evt->link_key, 16);
		break;
	case BT_LK_AUTH_COMBINATION_P256:
		conn->br.link_key->flags |= BT_LINK_KEY_AUTHENTICATED;
		/* fall through */
	case BT_LK_UNAUTH_COMBINATION_P256:
		conn->br.link_key->flags |= BT_LINK_KEY_SC;

		/* Mark no-bond so that link-key is removed on disconnection */
		if (bt_conn_ssp_get_auth(conn) < BT_HCI_DEDICATED_BONDING) {
			atomic_set_bit(conn->flags, BT_CONN_BR_NOBOND);
		}

		memcpy(conn->br.link_key->val, evt->link_key, 16);
		break;
	default:
		BT_WARN("Unsupported Link Key type %u", evt->key_type);
		(void)memset(conn->br.link_key->val, 0,
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
	if (!(conn->br.link_key->flags & BT_LINK_KEY_AUTHENTICATED) &&
	    conn->required_sec_level > BT_SECURITY_L2) {
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
	cp->oob_data = 0U;
	bt_hci_cmd_send_sync(BT_HCI_OP_IO_CAPABILITY_REPLY, resp_buf, NULL);
	bt_conn_unref(conn);
}

static void ssp_complete(struct net_buf *buf)
{
	struct bt_hci_evt_ssp_complete *evt = (void *)buf->data;
	struct bt_conn *conn;

	BT_DBG("status 0x%02x", evt->status);

	conn = bt_conn_lookup_addr_br(&evt->bdaddr);
	if (!conn) {
		BT_ERR("Can't find conn for %s", bt_addr_str(&evt->bdaddr));
		return;
	}

	bt_conn_ssp_auth_complete(conn, security_err_get(evt->status));
	if (evt->status) {
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
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

		priv->resolving = 1U;
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
	u8_t num_reports = net_buf_pull_u8(buf);

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_INQUIRY)) {
		return;
	}

	BT_DBG("number of results: %u", num_reports);

	while (num_reports--) {
		struct bt_hci_evt_inquiry_result_with_rssi *evt;
		struct bt_br_discovery_result *result;
		struct discovery_priv *priv;

		if (buf->len < sizeof(*evt)) {
			BT_ERR("Unexpected end to buffer");
			return;
		}

		evt = net_buf_pull_mem(buf, sizeof(*evt));
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
		(void)memset(result->eir, 0, sizeof(result->eir));
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
	priv->resolving = 0U;

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

	BT_DBG("status 0x%02x, handle %u", evt->status, handle);

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

	BT_DBG("status 0x%02x handle %u", evt->status, handle);

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

	BT_DBG("status 0x%02x handle %u", evt->status, handle);

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

	BT_DBG("status 0x%02x role %u addr %s", evt->status, evt->role,
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
#endif /* CONFIG_BT_BREDR */

#if defined(CONFIG_BT_SMP)
static int le_set_privacy_mode(const bt_addr_le_t *addr, u8_t mode)
{
	struct bt_hci_cp_le_set_privacy_mode cp;
	struct net_buf *buf;
	int err;

	/* Check if set privacy mode command is supported */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 39, 2)) {
		BT_WARN("Set privacy mode command is not supported");
		return 0;
	}

	BT_DBG("addr %s mode 0x%02x", bt_addr_le_str(addr), mode);

	bt_addr_le_copy(&cp.id_addr, addr);
	cp.mode = mode;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PRIVACY_MODE, sizeof(cp));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_mem(buf, &cp, sizeof(cp));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_PRIVACY_MODE, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int addr_res_enable(u8_t enable)
{
	struct net_buf *buf;

	BT_DBG("%s", enable ? "enabled" : "disabled");

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_u8(buf, enable);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE,
				    buf, NULL);
}

static int hci_id_add(const bt_addr_le_t *addr, u8_t val[16])
{
	struct bt_hci_cp_le_add_dev_to_rl *cp;
	struct net_buf *buf;

	BT_DBG("addr %s", bt_addr_le_str(addr));

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_ADD_DEV_TO_RL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->peer_id_addr, addr);
	memcpy(cp->peer_irk, val, 16);

#if defined(CONFIG_BT_PRIVACY)
	memcpy(cp->local_irk, bt_dev.irk, 16);
#else
	(void)memset(cp->local_irk, 0, 16);
#endif

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_ADD_DEV_TO_RL, buf, NULL);
}

void bt_id_add(struct bt_keys *keys)
{
	bool adv_enabled;
#if defined(CONFIG_BT_OBSERVER)
	bool scan_enabled;
#endif /* CONFIG_BT_OBSERVER */
	struct bt_conn *conn;
	int err;

	BT_DBG("addr %s", bt_addr_le_str(&keys->addr));

	/* Nothing to be done if host-side resolving is used */
	if (!bt_dev.le.rl_size || bt_dev.le.rl_entries > bt_dev.le.rl_size) {
		bt_dev.le.rl_entries++;
		return;
	}

	conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT);
	if (conn) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ID_PENDING);
		keys->flags |= BT_KEYS_ID_PENDING_ADD;
		bt_conn_unref(conn);
		return;
	}

	adv_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING);
	if (adv_enabled) {
		set_advertise_enable(false);
	}

#if defined(CONFIG_BT_OBSERVER)
	scan_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING);
	if (scan_enabled) {
		set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	/* If there are any existing entries address resolution will be on */
	if (bt_dev.le.rl_entries) {
		err = addr_res_enable(BT_HCI_ADDR_RES_DISABLE);
		if (err) {
			BT_WARN("Failed to disable address resolution");
			goto done;
		}
	}

	if (bt_dev.le.rl_entries == bt_dev.le.rl_size) {
		BT_WARN("Resolving list size exceeded. Switching to host.");

		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_CLEAR_RL, NULL, NULL);
		if (err) {
			BT_ERR("Failed to clear resolution list");
			goto done;
		}

		bt_dev.le.rl_entries++;

		goto done;
	}

	err = hci_id_add(&keys->addr, keys->irk.val);
	if (err) {
		BT_ERR("Failed to add IRK to controller");
		goto done;
	}

	bt_dev.le.rl_entries++;

	/*
	 * According to Core Spec. 5.0 Vol 1, Part A 5.4.5 Privacy Feature
	 *
	 * By default, network privacy mode is used when private addresses are
	 * resolved and generated by the Controller, so advertising packets from
	 * peer devices that contain private addresses will only be accepted.
	 * By changing to the device privacy mode device is only concerned about
	 * its privacy and will accept advertising packets from peer devices
	 * that contain their identity address as well as ones that contain
	 * a private address, even if the peer device has distributed its IRK in
	 * the past.
	 */
	err = le_set_privacy_mode(&keys->addr, BT_HCI_LE_PRIVACY_MODE_DEVICE);
	if (err) {
		BT_ERR("Failed to set privacy mode");
		goto done;
	}

done:
	addr_res_enable(BT_HCI_ADDR_RES_ENABLE);

#if defined(CONFIG_BT_OBSERVER)
	if (scan_enabled) {
		set_le_scan_enable(BT_HCI_LE_SCAN_ENABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	if (adv_enabled) {
		set_advertise_enable(true);
	}
}

static void keys_add_id(struct bt_keys *keys, void *data)
{
	hci_id_add(&keys->addr, keys->irk.val);
}

void bt_id_del(struct bt_keys *keys)
{
	struct bt_hci_cp_le_rem_dev_from_rl *cp;
	bool adv_enabled;
#if defined(CONFIG_BT_OBSERVER)
	bool scan_enabled;
#endif /* CONFIG_BT_OBSERVER */
	struct bt_conn *conn;
	struct net_buf *buf;
	int err;

	BT_DBG("addr %s", bt_addr_le_str(&keys->addr));

	if (!bt_dev.le.rl_size ||
	    bt_dev.le.rl_entries > bt_dev.le.rl_size + 1) {
		bt_dev.le.rl_entries--;
		return;
	}

	conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT);
	if (conn) {
		atomic_set_bit(bt_dev.flags, BT_DEV_ID_PENDING);
		keys->flags |= BT_KEYS_ID_PENDING_DEL;
		bt_conn_unref(conn);
		return;
	}

	adv_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING);
	if (adv_enabled) {
		set_advertise_enable(false);
	}

#if defined(CONFIG_BT_OBSERVER)
	scan_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING);
	if (scan_enabled) {
		set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	err = addr_res_enable(BT_HCI_ADDR_RES_DISABLE);
	if (err) {
		BT_ERR("Disabling address resolution failed (err %d)", err);
		goto done;
	}

	/* We checked size + 1 earlier, so here we know we can fit again */
	if (bt_dev.le.rl_entries > bt_dev.le.rl_size) {
		bt_dev.le.rl_entries--;
		keys->keys &= ~BT_KEYS_IRK;
		bt_keys_foreach(BT_KEYS_IRK, keys_add_id, NULL);
		goto done;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REM_DEV_FROM_RL, sizeof(*cp));
	if (!buf) {
		goto done;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->peer_id_addr, &keys->addr);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REM_DEV_FROM_RL, buf, NULL);
	if (err) {
		BT_ERR("Failed to remove IRK from controller");
		goto done;
	}

	bt_dev.le.rl_entries--;

done:
	/* Only re-enable if there are entries to do resolving with */
	if (bt_dev.le.rl_entries) {
		addr_res_enable(BT_HCI_ADDR_RES_ENABLE);
	}

#if defined(CONFIG_BT_OBSERVER)
	if (scan_enabled) {
		set_le_scan_enable(BT_HCI_LE_SCAN_ENABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	if (adv_enabled) {
		set_advertise_enable(true);
	}
}

static void update_sec_level(struct bt_conn *conn)
{
	if (!conn->encrypt) {
		conn->sec_level = BT_SECURITY_L1;
		return;
	}

	if (conn->le.keys && (conn->le.keys->flags & BT_KEYS_AUTHENTICATED)) {
		if (conn->le.keys->flags & BT_KEYS_SC &&
		    conn->le.keys->enc_size == BT_SMP_MAX_ENC_KEY_SIZE) {
			conn->sec_level = BT_SECURITY_L4;
		} else {
			conn->sec_level = BT_SECURITY_L3;
		}
	} else {
		conn->sec_level = BT_SECURITY_L2;
	}

	if (conn->required_sec_level > conn->sec_level) {
		BT_ERR("Failed to set required security level");
		bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
	}
}
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
static void hci_encrypt_change(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_change *evt = (void *)buf->data;
	u16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status 0x%02x handle %u encrypt 0x%02x", evt->status, handle,
	       evt->encrypt);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	if (evt->status) {
		reset_pairing(conn);
		bt_l2cap_encrypt_change(conn, evt->status);
		bt_conn_security_changed(conn, security_err_get(evt->status));
		bt_conn_unref(conn);
		return;
	}

	conn->encrypt = evt->encrypt;

#if defined(CONFIG_BT_SMP)
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
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		if (!update_sec_level_br(conn)) {
			bt_conn_unref(conn);
			return;
		}

		if (IS_ENABLED(CONFIG_BT_SMP)) {
			/*
			 * Start SMP over BR/EDR if we are pairing and are
			 * master on the link
			 */
			if (atomic_test_bit(conn->flags, BT_CONN_BR_PAIRING) &&
			    conn->role == BT_CONN_ROLE_MASTER) {
				bt_smp_br_send_pairing_req(conn);
			}
		}
	}
#endif /* CONFIG_BT_BREDR */
	reset_pairing(conn);

	bt_l2cap_encrypt_change(conn, evt->status);
	bt_conn_security_changed(conn, BT_SECURITY_ERR_SUCCESS);

	bt_conn_unref(conn);
}

static void hci_encrypt_key_refresh_complete(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("status 0x%02x handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	if (evt->status) {
		reset_pairing(conn);
		bt_l2cap_encrypt_change(conn, evt->status);
		bt_conn_security_changed(conn, security_err_get(evt->status));
		bt_conn_unref(conn);
		return;
	}

	/*
	 * Update keys with last pairing info for proper sec level update.
	 * This is done only for LE transport. For BR/EDR transport keys are
	 * updated on HCI 'Link Key Notification Event', therefore update here
	 * only security level based on available keys and encryption state.
	 */
#if defined(CONFIG_BT_SMP)
	if (conn->type == BT_CONN_TYPE_LE) {
		bt_smp_update_keys(conn);
		update_sec_level(conn);
	}
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_BREDR)
	if (conn->type == BT_CONN_TYPE_BR) {
		if (!update_sec_level_br(conn)) {
			bt_conn_unref(conn);
			return;
		}
	}
#endif /* CONFIG_BT_BREDR */

	reset_pairing(conn);
	bt_l2cap_encrypt_change(conn, evt->status);
	bt_conn_security_changed(conn, BT_SECURITY_ERR_SUCCESS);
	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */

#if defined(CONFIG_BT_SMP)
static void le_ltk_neg_reply(u16_t handle)
{
	struct bt_hci_cp_le_ltk_req_neg_reply *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY, sizeof(*cp));
	if (!buf) {
		BT_ERR("Out of command buffers");

		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);

	bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY, buf);
}

static void le_ltk_reply(u16_t handle, u8_t *ltk)
{
	struct bt_hci_cp_le_ltk_req_reply *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_REPLY,
				sizeof(*cp));
	if (!buf) {
		BT_ERR("Out of command buffers");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	memcpy(cp->ltk, ltk, sizeof(cp->ltk));

	bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_REPLY, buf);
}

static void le_ltk_request(struct net_buf *buf)
{
	struct bt_hci_evt_le_ltk_request *evt = (void *)buf->data;
	struct bt_conn *conn;
	u16_t handle;
	u8_t ltk[16];

	handle = sys_le16_to_cpu(evt->handle);

	BT_DBG("handle %u", handle);

	conn = bt_conn_lookup_handle(handle);
	if (!conn) {
		BT_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	if (bt_smp_request_ltk(conn, evt->rand, evt->ediv, ltk)) {
		le_ltk_reply(handle, ltk);
	} else {
		le_ltk_neg_reply(handle);
	}

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_ECC)
static void le_pkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_p256_public_key_complete *evt = (void *)buf->data;
	struct bt_pub_key_cb *cb;

	BT_DBG("status: 0x%02x", evt->status);

	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	if (!evt->status) {
		memcpy(pub_key, evt->key, 64);
		atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	}

	for (cb = pub_key_cb; cb; cb = cb->_next) {
		cb->func(evt->status ? NULL : pub_key);
	}

	pub_key_cb = NULL;
}

static void le_dhkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_generate_dhkey_complete *evt = (void *)buf->data;

	BT_DBG("status: 0x%02x", evt->status);

	if (dh_key_cb) {
		dh_key_cb(evt->status ? NULL : evt->dhkey);
		dh_key_cb = NULL;
	}
}
#endif /* CONFIG_BT_ECC */

static void hci_reset_complete(struct net_buf *buf)
{
	u8_t status = buf->data[0];
	atomic_t flags;

	BT_DBG("status 0x%02x", status);

	if (status) {
		return;
	}

	scan_dev_found_cb = NULL;
#if defined(CONFIG_BT_BREDR)
	discovery_cb = NULL;
	discovery_results = NULL;
	discovery_results_size = 0;
	discovery_results_count = 0;
#endif /* CONFIG_BT_BREDR */

	flags = (atomic_get(bt_dev.flags) & BT_DEV_PERSISTENT_FLAGS);
	atomic_set(bt_dev.flags, flags);
}

static void hci_cmd_done(u16_t opcode, u8_t status, struct net_buf *buf)
{
	BT_DBG("opcode 0x%04x status 0x%02x buf %p", opcode, status, buf);

	if (net_buf_pool_get(buf->pool_id) != &hci_cmd_pool) {
		BT_WARN("opcode 0x%04x pool id %u pool %p != &hci_cmd_pool %p",
			opcode, buf->pool_id, net_buf_pool_get(buf->pool_id),
			&hci_cmd_pool);
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
	struct bt_hci_evt_cmd_complete *evt;
	u8_t status, ncmd;
	u16_t opcode;

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	ncmd = evt->ncmd;
	opcode = sys_le16_to_cpu(evt->opcode);

	BT_DBG("opcode 0x%04x", opcode);

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
	struct bt_hci_evt_cmd_status *evt;
	u16_t opcode;
	u8_t ncmd;

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	opcode = sys_le16_to_cpu(evt->opcode);
	ncmd = evt->ncmd;

	BT_DBG("opcode 0x%04x", opcode);

	hci_cmd_done(opcode, evt->status, buf);

	/* Allow next command to be sent */
	if (ncmd) {
		k_sem_give(&bt_dev.ncmd_sem);
	}
}

#if defined(CONFIG_BT_OBSERVER)
static int start_le_scan(u8_t scan_type, u16_t interval, u16_t window)
{
	struct bt_hci_cp_le_set_scan_param set_param;
	struct net_buf *buf;
	int err;

	(void)memset(&set_param, 0, sizeof(set_param));

	set_param.scan_type = scan_type;

	/* for the rest parameters apply default values according to
	 *  spec 4.2, vol2, part E, 7.8.10
	 */
	set_param.interval = sys_cpu_to_le16(interval);
	set_param.window = sys_cpu_to_le16(window);

	if (IS_ENABLED(CONFIG_BT_WHITELIST) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_SCAN_WL)) {
		set_param.filter_policy = BT_HCI_LE_SCAN_FP_USE_WHITELIST;
	} else {
		set_param.filter_policy = BT_HCI_LE_SCAN_FP_NO_WHITELIST;
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		err = le_set_private_addr(BT_ID_DEFAULT);
		if (err) {
			return err;
		}

		if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			set_param.addr_type = BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
		} else {
			set_param.addr_type = BT_ADDR_LE_RANDOM;
		}
	} else {
		set_param.addr_type =  bt_dev.id_addr[0].type;

		/* Use NRPA unless identity has been explicitly requested
		 * (through Kconfig), or if there is no advertising ongoing.
		 */
		if (!IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY) &&
		    scan_type == BT_HCI_LE_SCAN_ACTIVE &&
		    !atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
			err = le_set_private_addr(BT_ID_DEFAULT);
			if (err) {
				return err;
			}

			set_param.addr_type = BT_ADDR_LE_RANDOM;
		} else if (set_param.addr_type == BT_ADDR_LE_RANDOM) {
			err = set_random_address(&bt_dev.id_addr[0].a);
			if (err) {
				return err;
			}
		}
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_PARAM, sizeof(set_param));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_mem(buf, &set_param, sizeof(set_param));

	bt_hci_cmd_send(BT_HCI_OP_LE_SET_SCAN_PARAM, buf);

	err = set_le_scan_enable(BT_HCI_LE_SCAN_ENABLE);
	if (err) {
		return err;
	}

	atomic_set_bit_to(bt_dev.flags, BT_DEV_ACTIVE_SCAN,
			  scan_type == BT_HCI_LE_SCAN_ACTIVE);

	return 0;
}

int bt_le_scan_update(bool fast_scan)
{
	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return 0;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		int err;

		err = set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		u16_t interval, window;
		struct bt_conn *conn;

		/* don't restart scan if we have pending connection */
		conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT);
		if (conn) {
			bt_conn_unref(conn);
			return 0;
		}

		conn = bt_conn_lookup_state_le(NULL, BT_CONN_CONNECT_SCAN);
		if (!conn) {
			return 0;
		}

		atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP);

		bt_conn_unref(conn);

		if (fast_scan) {
			interval = BT_GAP_SCAN_FAST_INTERVAL;
			window = BT_GAP_SCAN_FAST_WINDOW;
		} else {
			interval = CONFIG_BT_BACKGROUND_SCAN_INTERVAL;
			window = CONFIG_BT_BACKGROUND_SCAN_WINDOW;
		}

		return start_le_scan(BT_HCI_LE_SCAN_PASSIVE, interval, window);
	}

	return 0;
}

void bt_data_parse(struct net_buf_simple *ad,
		   bool (*func)(struct bt_data *data, void *user_data),
		   void *user_data)
{
	while (ad->len > 1) {
		struct bt_data data;
		u8_t len;

		len = net_buf_simple_pull_u8(ad);
		if (len == 0U) {
			/* Early termination */
			return;
		}

		if (len > ad->len) {
			BT_WARN("Malformed data");
			return;
		}

		data.type = net_buf_simple_pull_u8(ad);
		data.data_len = len - 1;
		data.data = ad->data;

		if (!func(&data, user_data)) {
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

static void le_adv_report(struct net_buf *buf)
{
	u8_t num_reports = net_buf_pull_u8(buf);
	struct bt_hci_evt_le_advertising_info *info;

	BT_DBG("Adv number of reports %u",  num_reports);

	while (num_reports--) {
		bt_addr_le_t id_addr;
		s8_t rssi;

		if (buf->len < sizeof(*info)) {
			BT_ERR("Unexpected end of buffer");
			break;
		}

		info = net_buf_pull_mem(buf, sizeof(*info));
		rssi = info->data[info->length];

		BT_DBG("%s event %u, len %u, rssi %d dBm",
		       bt_addr_le_str(&info->addr),
		       info->evt_type, info->length, rssi);

		if (info->addr.type == BT_ADDR_LE_PUBLIC_ID ||
		    info->addr.type == BT_ADDR_LE_RANDOM_ID) {
			bt_addr_le_copy(&id_addr, &info->addr);
			id_addr.type -= BT_ADDR_LE_PUBLIC_ID;
		} else {
			bt_addr_le_copy(&id_addr,
					bt_lookup_id_addr(bt_dev.adv_id,
							  &info->addr));
		}

		if (scan_dev_found_cb) {
			struct net_buf_simple_state state;

			net_buf_simple_save(&buf->b, &state);

			buf->len = info->length;
			scan_dev_found_cb(&id_addr, rssi, info->evt_type,
					  &buf->b);

			net_buf_simple_restore(&buf->b, &state);
		}

#if defined(CONFIG_BT_CENTRAL)
		check_pending_conn(&id_addr, &info->addr, info->evt_type);
#endif /* CONFIG_BT_CENTRAL */

		net_buf_pull(buf, info->length + sizeof(rssi));
	}
}
#endif /* CONFIG_BT_OBSERVER */

int bt_hci_get_conn_handle(const struct bt_conn *conn, u16_t *conn_handle)
{
	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	*conn_handle = conn->handle;
	return 0;
}

#if defined(CONFIG_BT_HCI_VS_EVT_USER)
int bt_hci_register_vnd_evt_cb(bt_hci_vnd_evt_cb_t cb)
{
	hci_vnd_evt_cb = cb;
	return 0;
}
#endif /* CONFIG_BT_HCI_VS_EVT_USER */

static void hci_vendor_event(struct net_buf *buf)
{
	bool handled = false;

#if defined(CONFIG_BT_HCI_VS_EVT_USER)
	if (hci_vnd_evt_cb) {
		struct net_buf_simple_state state;

		net_buf_simple_save(&buf->b, &state);

		handled = hci_vnd_evt_cb(&buf->b);

		net_buf_simple_restore(&buf->b, &state);
	}
#endif /* CONFIG_BT_HCI_VS_EVT_USER */

	if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT) && !handled) {
		/* do nothing at present time */
		BT_WARN("Unhandled vendor-specific event: %s",
			bt_hex(buf->data, buf->len));
	}
}

static const struct event_handler meta_events[] = {
#if defined(CONFIG_BT_OBSERVER)
	EVENT_HANDLER(BT_HCI_EVT_LE_ADVERTISING_REPORT, le_adv_report,
		      sizeof(struct bt_hci_evt_le_advertising_report)),
#endif /* CONFIG_BT_OBSERVER */
#if defined(CONFIG_BT_CONN)
	EVENT_HANDLER(BT_HCI_EVT_LE_CONN_COMPLETE, le_legacy_conn_complete,
		      sizeof(struct bt_hci_evt_le_conn_complete)),
	EVENT_HANDLER(BT_HCI_EVT_LE_ENH_CONN_COMPLETE, le_enh_conn_complete,
		      sizeof(struct bt_hci_evt_le_enh_conn_complete)),
	EVENT_HANDLER(BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE,
		      le_conn_update_complete,
		      sizeof(struct bt_hci_evt_le_conn_update_complete)),
	EVENT_HANDLER(BT_HCI_EV_LE_REMOTE_FEAT_COMPLETE,
		      le_remote_feat_complete,
		      sizeof(struct bt_hci_evt_le_remote_feat_complete)),
	EVENT_HANDLER(BT_HCI_EVT_LE_CONN_PARAM_REQ, le_conn_param_req,
		      sizeof(struct bt_hci_evt_le_conn_param_req)),
#if defined(CONFIG_BT_DATA_LEN_UPDATE)
	EVENT_HANDLER(BT_HCI_EVT_LE_DATA_LEN_CHANGE, le_data_len_change,
		      sizeof(struct bt_hci_evt_le_data_len_change)),
#endif /* CONFIG_BT_DATA_LEN_UPDATE */
#if defined(CONFIG_BT_PHY_UPDATE)
	EVENT_HANDLER(BT_HCI_EVT_LE_PHY_UPDATE_COMPLETE,
		      le_phy_update_complete,
		      sizeof(struct bt_hci_evt_le_phy_update_complete)),
#endif /* CONFIG_BT_PHY_UPDATE */
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_SMP)
	EVENT_HANDLER(BT_HCI_EVT_LE_LTK_REQUEST, le_ltk_request,
		      sizeof(struct bt_hci_evt_le_ltk_request)),
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_ECC)
	EVENT_HANDLER(BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE, le_pkey_complete,
		      sizeof(struct bt_hci_evt_le_p256_public_key_complete)),
	EVENT_HANDLER(BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE, le_dhkey_complete,
		      sizeof(struct bt_hci_evt_le_generate_dhkey_complete)),
#endif /* CONFIG_BT_SMP */
};

static void hci_le_meta_event(struct net_buf *buf)
{
	struct bt_hci_evt_le_meta_event *evt;

	evt = net_buf_pull_mem(buf, sizeof(*evt));

	BT_DBG("subevent 0x%02x", evt->subevent);

	handle_event(evt->subevent, buf, meta_events, ARRAY_SIZE(meta_events));
}

static const struct event_handler normal_events[] = {
	EVENT_HANDLER(BT_HCI_EVT_VENDOR, hci_vendor_event,
		      sizeof(struct bt_hci_evt_vs)),
	EVENT_HANDLER(BT_HCI_EVT_LE_META_EVENT, hci_le_meta_event,
		      sizeof(struct bt_hci_evt_le_meta_event)),
#if defined(CONFIG_BT_BREDR)
	EVENT_HANDLER(BT_HCI_EVT_CONN_REQUEST, conn_req,
		      sizeof(struct bt_hci_evt_conn_request)),
	EVENT_HANDLER(BT_HCI_EVT_CONN_COMPLETE, conn_complete,
		      sizeof(struct bt_hci_evt_conn_complete)),
	EVENT_HANDLER(BT_HCI_EVT_PIN_CODE_REQ, pin_code_req,
		      sizeof(struct bt_hci_evt_pin_code_req)),
	EVENT_HANDLER(BT_HCI_EVT_LINK_KEY_NOTIFY, link_key_notify,
		      sizeof(struct bt_hci_evt_link_key_notify)),
	EVENT_HANDLER(BT_HCI_EVT_LINK_KEY_REQ, link_key_req,
		      sizeof(struct bt_hci_evt_link_key_req)),
	EVENT_HANDLER(BT_HCI_EVT_IO_CAPA_RESP, io_capa_resp,
		      sizeof(struct bt_hci_evt_io_capa_resp)),
	EVENT_HANDLER(BT_HCI_EVT_IO_CAPA_REQ, io_capa_req,
		      sizeof(struct bt_hci_evt_io_capa_req)),
	EVENT_HANDLER(BT_HCI_EVT_SSP_COMPLETE, ssp_complete,
		      sizeof(struct bt_hci_evt_ssp_complete)),
	EVENT_HANDLER(BT_HCI_EVT_USER_CONFIRM_REQ, user_confirm_req,
		      sizeof(struct bt_hci_evt_user_confirm_req)),
	EVENT_HANDLER(BT_HCI_EVT_USER_PASSKEY_NOTIFY, user_passkey_notify,
		      sizeof(struct bt_hci_evt_user_passkey_notify)),
	EVENT_HANDLER(BT_HCI_EVT_USER_PASSKEY_REQ, user_passkey_req,
		      sizeof(struct bt_hci_evt_user_passkey_req)),
	EVENT_HANDLER(BT_HCI_EVT_INQUIRY_COMPLETE, inquiry_complete,
		      sizeof(struct bt_hci_evt_inquiry_complete)),
	EVENT_HANDLER(BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI,
		      inquiry_result_with_rssi,
		      sizeof(struct bt_hci_evt_inquiry_result_with_rssi)),
	EVENT_HANDLER(BT_HCI_EVT_EXTENDED_INQUIRY_RESULT,
		      extended_inquiry_result,
		      sizeof(struct bt_hci_evt_extended_inquiry_result)),
	EVENT_HANDLER(BT_HCI_EVT_REMOTE_NAME_REQ_COMPLETE,
		      remote_name_request_complete,
		      sizeof(struct bt_hci_evt_remote_name_req_complete)),
	EVENT_HANDLER(BT_HCI_EVT_AUTH_COMPLETE, auth_complete,
		      sizeof(struct bt_hci_evt_auth_complete)),
	EVENT_HANDLER(BT_HCI_EVT_REMOTE_FEATURES,
		      read_remote_features_complete,
		      sizeof(struct bt_hci_evt_remote_features)),
	EVENT_HANDLER(BT_HCI_EVT_REMOTE_EXT_FEATURES,
		      read_remote_ext_features_complete,
		      sizeof(struct bt_hci_evt_remote_ext_features)),
	EVENT_HANDLER(BT_HCI_EVT_ROLE_CHANGE, role_change,
		      sizeof(struct bt_hci_evt_role_change)),
	EVENT_HANDLER(BT_HCI_EVT_SYNC_CONN_COMPLETE, synchronous_conn_complete,
		      sizeof(struct bt_hci_evt_sync_conn_complete)),
#endif /* CONFIG_BT_BREDR */
#if defined(CONFIG_BT_CONN)
	EVENT_HANDLER(BT_HCI_EVT_DISCONN_COMPLETE, hci_disconn_complete,
		      sizeof(struct bt_hci_evt_disconn_complete)),
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	EVENT_HANDLER(BT_HCI_EVT_ENCRYPT_CHANGE, hci_encrypt_change,
		      sizeof(struct bt_hci_evt_encrypt_change)),
	EVENT_HANDLER(BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE,
		      hci_encrypt_key_refresh_complete,
		      sizeof(struct bt_hci_evt_encrypt_key_refresh_complete)),
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */
};

static void hci_event(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr;

	BT_ASSERT(buf->len >= sizeof(*hdr));

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	BT_DBG("event 0x%02x", hdr->evt);
	BT_ASSERT(!bt_hci_evt_is_prio(hdr->evt));

	handle_event(hdr->evt, buf, normal_events, ARRAY_SIZE(normal_events));

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
			} else if (IS_ENABLED(CONFIG_BT_CONN)) {
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

#if defined(CONFIG_BT_CONN)
/* command FIFO + conn_change signal + MAX_CONN * 2 (tx & tx_notify) */
#define EV_COUNT (2 + (CONFIG_BT_MAX_CONN * 2))
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

		if (IS_ENABLED(CONFIG_BT_CONN)) {
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

	BT_DBG("status 0x%02x", rp->status);

	bt_dev.hci_version = rp->hci_version;
	bt_dev.hci_revision = sys_le16_to_cpu(rp->hci_revision);
	bt_dev.lmp_version = rp->lmp_version;
	bt_dev.lmp_subversion = sys_le16_to_cpu(rp->lmp_subversion);
	bt_dev.manufacturer = sys_le16_to_cpu(rp->manufacturer);
}

static void read_bdaddr_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_bd_addr *rp = (void *)buf->data;

	BT_DBG("status 0x%02x", rp->status);

	if (!bt_addr_cmp(&rp->bdaddr, BT_ADDR_ANY) ||
	    !bt_addr_cmp(&rp->bdaddr, BT_ADDR_NONE)) {
		BT_DBG("Controller has no public address");
		return;
	}

	bt_addr_copy(&bt_dev.id_addr[0].a, &rp->bdaddr);
	bt_dev.id_addr[0].type = BT_ADDR_LE_PUBLIC;
	bt_dev.id_count = 1U;
}

static void read_le_features_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_local_features *rp = (void *)buf->data;

	BT_DBG("status 0x%02x", rp->status);

	memcpy(bt_dev.le.features, rp->features, sizeof(bt_dev.le.features));
}

#if defined(CONFIG_BT_BREDR)
static void read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;
	u16_t pkts;

	BT_DBG("status 0x%02x", rp->status);

	bt_dev.br.mtu = sys_le16_to_cpu(rp->acl_max_len);
	pkts = sys_le16_to_cpu(rp->acl_max_num);

	BT_DBG("ACL BR/EDR buffers: pkts %u mtu %u", pkts, bt_dev.br.mtu);

	k_sem_init(&bt_dev.br.pkts, pkts, pkts);
}
#elif defined(CONFIG_BT_CONN)
static void read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;
	u16_t pkts;

	BT_DBG("status 0x%02x", rp->status);

	/* If LE-side has buffers we can ignore the BR/EDR values */
	if (bt_dev.le.mtu) {
		return;
	}

	bt_dev.le.mtu = sys_le16_to_cpu(rp->acl_max_len);
	pkts = sys_le16_to_cpu(rp->acl_max_num);

	BT_DBG("ACL BR/EDR buffers: pkts %u mtu %u", pkts, bt_dev.le.mtu);

	pkts = MIN(pkts, CONFIG_BT_CONN_TX_MAX);

	k_sem_init(&bt_dev.le.pkts, pkts, pkts);
}
#endif

#if defined(CONFIG_BT_CONN)
static void le_read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_buffer_size *rp = (void *)buf->data;
	u8_t le_max_num;

	BT_DBG("status 0x%02x", rp->status);

	bt_dev.le.mtu = sys_le16_to_cpu(rp->le_max_len);
	if (!bt_dev.le.mtu) {
		return;
	}

	BT_DBG("ACL LE buffers: pkts %u mtu %u", rp->le_max_num, bt_dev.le.mtu);

	le_max_num = MIN(rp->le_max_num, CONFIG_BT_CONN_TX_MAX);
	k_sem_init(&bt_dev.le.pkts, le_max_num, le_max_num);
}
#endif

static void read_supported_commands_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_supported_commands *rp = (void *)buf->data;

	BT_DBG("status 0x%02x", rp->status);

	memcpy(bt_dev.supported_commands, rp->commands,
	       sizeof(bt_dev.supported_commands));

	/*
	 * Report "LE Read Local P-256 Public Key" and "LE Generate DH Key" as
	 * supported if TinyCrypt ECC is used for emulation.
	 */
	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		bt_dev.supported_commands[34] |= 0x02;
		bt_dev.supported_commands[34] |= 0x04;
	}
}

static void read_local_features_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_local_features *rp = (void *)buf->data;

	BT_DBG("status 0x%02x", rp->status);

	memcpy(bt_dev.features[0], rp->features, sizeof(bt_dev.features[0]));
}

static void le_read_supp_states_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_supp_states *rp = (void *)buf->data;

	BT_DBG("status 0x%02x", rp->status);

	bt_dev.le.states = sys_get_le64(rp->le_states);
}

#if defined(CONFIG_BT_SMP)
static void le_read_resolving_list_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_rl_size *rp = (void *)buf->data;

	BT_DBG("Resolving List size %u", rp->rl_size);

	bt_dev.le.rl_size = rp->rl_size;
}
#endif /* defined(CONFIG_BT_SMP) */

#if defined(CONFIG_BT_WHITELIST)
static void le_read_wl_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_wl_size *rp =
		(struct bt_hci_rp_le_read_wl_size *)buf->data;

	BT_DBG("Whitelist size %u", rp->wl_size);

	bt_dev.le.wl_size = rp->wl_size;
}
#endif

static int common_init(void)
{
	struct net_buf *rsp;
	int err;

	if (!(bt_dev.drv->quirks & BT_QUIRK_NO_RESET)) {
		/* Send HCI_RESET */
		err = bt_hci_cmd_send_sync(BT_HCI_OP_RESET, NULL, &rsp);
		if (err) {
			return err;
		}
		hci_reset_complete(rsp);
		net_buf_unref(rsp);
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
	if (!atomic_test_bit(bt_dev.flags, BT_DEV_USER_ID_ADDR)) {
		err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR, NULL, &rsp);
		if (err) {
			return err;
		}
		read_bdaddr_complete(rsp);
		net_buf_unref(rsp);
	}

	/* Read Local Supported Commands */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_SUPPORTED_COMMANDS, NULL,
				   &rsp);
	if (err) {
		return err;
	}
	read_supported_commands_complete(rsp);
	net_buf_unref(rsp);

	if (IS_ENABLED(CONFIG_BT_HOST_CRYPTO)) {
		/* Initialize the PRNG so that it is safe to use it later
		 * on in the initialization process.
		 */
		err = prng_init();
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	err = set_flow_control();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

	return 0;
}

static int le_set_event_mask(void)
{
	struct bt_hci_cp_le_set_event_mask *cp_mask;
	struct net_buf *buf;
	u64_t mask = 0U;

	/* Set LE event mask */
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_EVENT_MASK, sizeof(*cp_mask));
	if (!buf) {
		return -ENOBUFS;
	}

	cp_mask = net_buf_add(buf, sizeof(*cp_mask));

	mask |= BT_EVT_MASK_LE_ADVERTISING_REPORT;

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		if (IS_ENABLED(CONFIG_BT_SMP) &&
		    BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_ENH_CONN_COMPLETE;
		} else {
			mask |= BT_EVT_MASK_LE_CONN_COMPLETE;
		}

		mask |= BT_EVT_MASK_LE_CONN_UPDATE_COMPLETE;
		mask |= BT_EVT_MASK_LE_REMOTE_FEAT_COMPLETE;

		if (BT_FEAT_LE_CONN_PARAM_REQ_PROC(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_CONN_PARAM_REQ;
		}

		if (IS_ENABLED(CONFIG_BT_DATA_LEN_UPDATE) &&
		    BT_FEAT_LE_DLE(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_DATA_LEN_CHANGE;
		}

		if (IS_ENABLED(CONFIG_BT_PHY_UPDATE) &&
		    (BT_FEAT_LE_PHY_2M(bt_dev.le.features) ||
		     BT_FEAT_LE_PHY_CODED(bt_dev.le.features))) {
			mask |= BT_EVT_MASK_LE_PHY_UPDATE_COMPLETE;
		}
	}

	if (IS_ENABLED(CONFIG_BT_SMP) &&
	    BT_FEAT_LE_ENCR(bt_dev.le.features)) {
		mask |= BT_EVT_MASK_LE_LTK_REQUEST;
	}

	/*
	 * If "LE Read Local P-256 Public Key" and "LE Generate DH Key" are
	 * supported we need to enable events generated by those commands.
	 */
	if (IS_ENABLED(CONFIG_BT_ECC) &&
	    (BT_CMD_TEST(bt_dev.supported_commands, 34, 1)) &&
	    (BT_CMD_TEST(bt_dev.supported_commands, 34, 2))) {
		mask |= BT_EVT_MASK_LE_P256_PUBLIC_KEY_COMPLETE;
		mask |= BT_EVT_MASK_LE_GENERATE_DHKEY_COMPLETE;
	}

	sys_put_le64(mask, cp_mask->events);
	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EVENT_MASK, buf, NULL);
}

static int le_init(void)
{
	struct bt_hci_cp_write_le_host_supp *cp_le;
	struct net_buf *buf, *rsp;
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

#if defined(CONFIG_BT_CONN)
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

	if (IS_ENABLED(CONFIG_BT_CONN) &&
	    IS_ENABLED(CONFIG_BT_DATA_LEN_UPDATE) &&
	    BT_FEAT_LE_DLE(bt_dev.le.features)) {
		struct bt_hci_cp_le_write_default_data_len *cp;
		struct bt_hci_rp_le_read_max_data_len *rp;
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

#if defined(CONFIG_BT_SMP)
	if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
#if defined(CONFIG_BT_PRIVACY)
		struct bt_hci_cp_le_set_rpa_timeout *cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_RPA_TIMEOUT,
					sizeof(*cp));
		if (!buf) {
			return -ENOBUFS;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->rpa_timeout = sys_cpu_to_le16(CONFIG_BT_RPA_TIMEOUT);
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_RPA_TIMEOUT, buf,
					   NULL);
		if (err) {
			return err;
		}
#endif /* defined(CONFIG_BT_PRIVACY) */

		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_RL_SIZE, NULL,
					   &rsp);
		if (err) {
			return err;
		}
		le_read_resolving_list_size_complete(rsp);
		net_buf_unref(rsp);
	}
#endif

#if defined(CONFIG_BT_WHITELIST)
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_WL_SIZE, NULL,
				   &rsp);
	if (err) {
		return err;
	}

	le_read_wl_size_complete(rsp);
	net_buf_unref(rsp);
#endif /* defined(CONFIG_BT_WHITELIST) */

	return  le_set_event_mask();
}

#if defined(CONFIG_BT_BREDR)
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
	strncpy((char *)name_cp->local_name, CONFIG_BT_DEVICE_NAME,
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

	net_buf_add_le16(buf, CONFIG_BT_PAGE_TIMEOUT);

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
#if defined(CONFIG_BT_CONN)
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
#endif /* CONFIG_BT_CONN */

	return 0;
}
#endif

static int set_event_mask(void)
{
	struct bt_hci_cp_set_event_mask *ev;
	struct net_buf *buf;
	u64_t mask = 0U;

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_EVENT_MASK, sizeof(*ev));
	if (!buf) {
		return -ENOBUFS;
	}

	ev = net_buf_add(buf, sizeof(*ev));

	if (IS_ENABLED(CONFIG_BT_BREDR)) {
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

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		mask |= BT_EVT_MASK_DISCONN_COMPLETE;
		mask |= BT_EVT_MASK_REMOTE_VERSION_INFO;
	}

	if (IS_ENABLED(CONFIG_BT_SMP) &&
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

#if defined(CONFIG_BT_DEBUG)
static const char *ver_str(u8_t ver)
{
	const char * const str[] = {
		"1.0b", "1.1", "1.2", "2.0", "2.1", "3.0", "4.0", "4.1", "4.2",
		"5.0", "5.1",
	};

	if (ver < ARRAY_SIZE(str)) {
		return str[ver];
	}

	return "unknown";
}

static void bt_dev_show_info(void)
{
	int i;

	BT_INFO("Identity%s: %s", bt_dev.id_count > 1 ? "[0]" : "",
		bt_addr_le_str(&bt_dev.id_addr[0]));

	for (i = 1; i < bt_dev.id_count; i++) {
		BT_INFO("Identity[%d]: %s",
			i, bt_addr_le_str(&bt_dev.id_addr[i]));
	}

	BT_INFO("HCI: version %s (0x%02x) revision 0x%04x, manufacturer 0x%04x",
		ver_str(bt_dev.hci_version), bt_dev.hci_version,
		bt_dev.hci_revision, bt_dev.manufacturer);
	BT_INFO("LMP: version %s (0x%02x) subver 0x%04x",
		ver_str(bt_dev.lmp_version), bt_dev.lmp_version,
		bt_dev.lmp_subversion);
}
#else
static inline void bt_dev_show_info(void)
{
}
#endif /* CONFIG_BT_DEBUG */

#if defined(CONFIG_BT_HCI_VS_EXT)
#if defined(CONFIG_BT_DEBUG)
static const char *vs_hw_platform(u16_t platform)
{
	static const char * const plat_str[] = {
		"reserved", "Intel Corporation", "Nordic Semiconductor",
		"NXP Semiconductors" };

	if (platform < ARRAY_SIZE(plat_str)) {
		return plat_str[platform];
	}

	return "unknown";
}

static const char *vs_hw_variant(u16_t platform, u16_t variant)
{
	static const char * const nordic_str[] = {
		"reserved", "nRF51x", "nRF52x"
	};

	if (platform != BT_HCI_VS_HW_PLAT_NORDIC) {
		return "unknown";
	}

	if (variant < ARRAY_SIZE(nordic_str)) {
		return nordic_str[variant];
	}

	return "unknown";
}

static const char *vs_fw_variant(u8_t variant)
{
	static const char * const var_str[] = {
		"Standard Bluetooth controller",
		"Vendor specific controller",
		"Firmware loader",
		"Rescue image",
	};

	if (variant < ARRAY_SIZE(var_str)) {
		return var_str[variant];
	}

	return "unknown";
}
#endif /* CONFIG_BT_DEBUG */

static void hci_vs_init(void)
{
	union {
		struct bt_hci_rp_vs_read_version_info *info;
		struct bt_hci_rp_vs_read_supported_commands *cmds;
		struct bt_hci_rp_vs_read_supported_features *feat;
	} rp;
	struct net_buf *rsp;
	int err;

	/* If heuristics is enabled, try to guess HCI VS support by looking
	 * at the HCI version and identity address. We haven't tried to set
	 * a static random address yet at this point, so the identity will
	 * either be zeroes or a valid public address.
	 */
	if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT_DETECT) &&
	    (bt_dev.hci_version < BT_HCI_VERSION_5_0 ||
	     (!atomic_test_bit(bt_dev.flags, BT_DEV_USER_ID_ADDR) &&
	      bt_addr_le_cmp(&bt_dev.id_addr[0], BT_ADDR_LE_ANY)))) {
		BT_WARN("Controller doesn't seem to support Zephyr vendor HCI");
		return;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_VERSION_INFO, NULL, &rsp);
	if (err) {
		BT_WARN("Vendor HCI extensions not available");
		return;
	}

#if defined(CONFIG_BT_DEBUG)
	rp.info = (void *)rsp->data;
	BT_INFO("HW Platform: %s (0x%04x)",
		vs_hw_platform(sys_le16_to_cpu(rp.info->hw_platform)),
		sys_le16_to_cpu(rp.info->hw_platform));
	BT_INFO("HW Variant: %s (0x%04x)",
		vs_hw_variant(sys_le16_to_cpu(rp.info->hw_platform),
			      sys_le16_to_cpu(rp.info->hw_variant)),
		sys_le16_to_cpu(rp.info->hw_variant));
	BT_INFO("Firmware: %s (0x%02x) Version %u.%u Build %u",
		vs_fw_variant(rp.info->fw_variant), rp.info->fw_variant,
		rp.info->fw_version, sys_le16_to_cpu(rp.info->fw_revision),
		sys_le32_to_cpu(rp.info->fw_build));
#endif /* CONFIG_BT_DEBUG */

	net_buf_unref(rsp);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS,
				   NULL, &rsp);
	if (err) {
		BT_WARN("Failed to read supported vendor features");
		return;
	}

	rp.cmds = (void *)rsp->data;
	memcpy(bt_dev.vs_commands, rp.cmds->commands, BT_DEV_VS_CMDS_MAX);
	net_buf_unref(rsp);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_SUPPORTED_FEATURES,
				   NULL, &rsp);
	if (err) {
		BT_WARN("Failed to read supported vendor commands");
		return;
	}

	rp.feat = (void *)rsp->data;
	memcpy(bt_dev.vs_features, rp.feat->features, BT_DEV_VS_FEAT_MAX);
	net_buf_unref(rsp);
}
#endif /* CONFIG_BT_HCI_VS_EXT */

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
	} else if (IS_ENABLED(CONFIG_BT_BREDR)) {
		BT_ERR("Non-BR/EDR controller detected");
		return -EIO;
	}

	err = set_event_mask();
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_HCI_VS_EXT)
	hci_vs_init();
#endif

	if (!IS_ENABLED(CONFIG_BT_SETTINGS) && !bt_dev.id_count) {
		BT_DBG("No public address. Trying to set static random.");
		err = bt_setup_id_addr();
		if (err) {
			BT_ERR("Unable to set identity address");
			return err;
		}
	}

	return 0;
}

int bt_send(struct net_buf *buf)
{
	BT_DBG("buf %p len %u type %u", buf, buf->len, bt_buf_get_type(buf));

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		return bt_hci_ecc_send(buf);
	}

	return bt_dev.drv->send(buf);
}

int bt_recv(struct net_buf *buf)
{
	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	BT_DBG("buf %p len %u", buf, buf->len);

	switch (bt_buf_get_type(buf)) {
#if defined(CONFIG_BT_CONN)
	case BT_BUF_ACL_IN:
#if defined(CONFIG_BT_RECV_IS_RX_THREAD)
		hci_acl(buf);
#else
		net_buf_put(&bt_dev.rx_queue, buf);
#endif
		return 0;
#endif /* BT_CONN */
	case BT_BUF_EVT:
#if defined(CONFIG_BT_RECV_IS_RX_THREAD)
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

static const struct event_handler prio_events[] = {
	EVENT_HANDLER(BT_HCI_EVT_CMD_COMPLETE, hci_cmd_complete,
		      sizeof(struct bt_hci_evt_cmd_complete)),
	EVENT_HANDLER(BT_HCI_EVT_CMD_STATUS, hci_cmd_status,
		      sizeof(struct bt_hci_evt_cmd_status)),
#if defined(CONFIG_BT_CONN)
	EVENT_HANDLER(BT_HCI_EVT_DATA_BUF_OVERFLOW,
		      hci_data_buf_overflow,
		      sizeof(struct bt_hci_evt_data_buf_overflow)),
	EVENT_HANDLER(BT_HCI_EVT_NUM_COMPLETED_PACKETS,
		      hci_num_completed_packets,
		      sizeof(struct bt_hci_evt_num_completed_packets)),
#endif /* CONFIG_BT_CONN */
};

int bt_recv_prio(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr;

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	BT_ASSERT(bt_buf_get_type(buf) == BT_BUF_EVT);
	BT_ASSERT(buf->len >= sizeof(*hdr));

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	BT_ASSERT(bt_hci_evt_is_prio(hdr->evt));

	handle_event(hdr->evt, buf, prio_events, ARRAY_SIZE(prio_events));

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

#if defined(CONFIG_BT_PRIVACY)
static int irk_init(void)
{
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		BT_DBG("Expecting settings to handle local IRK");
	} else {
		int err;

		err = bt_rand(&bt_dev.irk[0], 16);
		if (err) {
			return err;
		}

		BT_WARN("Using temporary IRK");
	}

	return 0;
}
#endif /* CONFIG_BT_PRIVACY */

void bt_finalize_init(void)
{
	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		bt_le_scan_update(false);
	}

	bt_dev_show_info();
}

static int bt_init(void)
{
	int err;

	err = hci_init();
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		err = bt_conn_init();
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BT_PRIVACY)
	err = irk_init();
	if (err) {
		return err;
	}

	k_delayed_work_init(&bt_dev.rpa_update, rpa_timeout);
#endif

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		if (!bt_dev.id_count) {
			BT_WARN("No ID address. App must call settings_load()");
			return 0;
		}

		atomic_set_bit(bt_dev.flags, BT_DEV_PRESET_ID);
	}

	bt_finalize_init();
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

#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
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
#if defined(CONFIG_BT_CONN)
		case BT_BUF_ACL_IN:
			hci_acl(buf);
			break;
#endif /* CONFIG_BT_CONN */
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
#endif /* !CONFIG_BT_RECV_IS_RX_THREAD */

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

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		err = bt_settings_init();
		if (err) {
			return err;
		}
	} else {
		bt_set_name(CONFIG_BT_DEVICE_NAME);
	}

	ready_cb = cb;

	/* TX thread */
	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_THREAD_STACK_SIZEOF(tx_thread_stack),
			hci_tx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_HCI_TX_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&tx_thread_data, "BT TX");

#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
	/* RX thread */
	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack),
			(k_thread_entry_t)hci_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&rx_thread_data, "BT RX");
#endif

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		bt_hci_ecc_init();
	}

	err = bt_dev.drv->open();
	if (err) {
		BT_ERR("HCI driver open failed (%d)", err);
		return err;
	}

	bt_monitor_send(BT_MONITOR_OPEN_INDEX, NULL, 0);

	if (!cb) {
		return bt_init();
	}

	k_work_submit(&bt_dev.init);
	return 0;
}

struct bt_ad {
	const struct bt_data *data;
	size_t len;
};

static int set_ad(u16_t hci_op, const struct bt_ad *ad, size_t ad_len)
{
	struct bt_hci_cp_le_set_adv_data *set_data;
	struct net_buf *buf;
	int c, i;

	buf = bt_hci_cmd_create(hci_op, sizeof(*set_data));
	if (!buf) {
		return -ENOBUFS;
	}

	set_data = net_buf_add(buf, sizeof(*set_data));

	(void)memset(set_data, 0, sizeof(*set_data));

	for (c = 0; c < ad_len; c++) {
		const struct bt_data *data = ad[c].data;

		for (i = 0; i < ad[c].len; i++) {
			int len = data[i].data_len;
			u8_t type = data[i].type;

			/* Check if ad fit in the remaining buffer */
			if (set_data->len + len + 2 > 31) {
				len = 31 - (set_data->len + 2);
				if (type != BT_DATA_NAME_COMPLETE || !len) {
					net_buf_unref(buf);
					BT_ERR("Too big advertising data");
					return -EINVAL;
				}
				type = BT_DATA_NAME_SHORTENED;
			}

			set_data->data[set_data->len++] = len + 1;
			set_data->data[set_data->len++] = type;

			memcpy(&set_data->data[set_data->len], data[i].data,
			       len);
			set_data->len += len;
		}
	}

	return bt_hci_cmd_send_sync(hci_op, buf, NULL);
}

int bt_set_name(const char *name)
{
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	size_t len = strlen(name);
	int err;

	if (len >= sizeof(bt_dev.name)) {
		return -ENOMEM;
	}

	if (!strcmp(bt_dev.name, name)) {
		return 0;
	}

	strncpy(bt_dev.name, name, sizeof(bt_dev.name));

	/* Update advertising name if in use */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING_NAME)) {
		struct bt_data data[] = { BT_DATA(BT_DATA_NAME_COMPLETE, name,
						strlen(name)) };
		struct bt_ad sd = { data, ARRAY_SIZE(data) };

		set_ad(BT_HCI_OP_LE_SET_SCAN_RSP_DATA, &sd, 1);

		/* Make sure the new name is set */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
			set_advertise_enable(false);
			set_advertise_enable(true);
		}
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		err = settings_save_one("bt/name", bt_dev.name, len);
		if (err) {
			BT_WARN("Unable to store name");
		}
	}

	return 0;
#else
	return -ENOMEM;
#endif
}

const char *bt_get_name(void)
{
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	return bt_dev.name;
#else
	return CONFIG_BT_DEVICE_NAME;
#endif
}

int bt_set_id_addr(const bt_addr_le_t *addr)
{
	bt_addr_le_t non_const_addr;

	if (atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		BT_ERR("Setting identity not allowed after bt_enable()");
		return -EBUSY;
	}

	bt_addr_le_copy(&non_const_addr, addr);

	return bt_id_create(&non_const_addr, NULL);
}

void bt_id_get(bt_addr_le_t *addrs, size_t *count)
{
	size_t to_copy = MIN(*count, bt_dev.id_count);

	memcpy(addrs, bt_dev.id_addr, to_copy * sizeof(bt_addr_le_t));
	*count = to_copy;
}

static int id_find(const bt_addr_le_t *addr)
{
	u8_t id;

	for (id = 0U; id < bt_dev.id_count; id++) {
		if (!bt_addr_le_cmp(addr, &bt_dev.id_addr[id])) {
			return id;
		}
	}

	return -ENOENT;
}

static void id_create(u8_t id, bt_addr_le_t *addr, u8_t *irk)
{
	if (addr && bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
		bt_addr_le_copy(&bt_dev.id_addr[id], addr);
	} else {
		bt_addr_le_t new_addr;

		do {
			bt_addr_le_create_static(&new_addr);
			/* Make sure we didn't generate a duplicate */
		} while (id_find(&new_addr) >= 0);

		bt_addr_le_copy(&bt_dev.id_addr[id], &new_addr);

		if (addr) {
			bt_addr_le_copy(addr, &bt_dev.id_addr[id]);
		}
	}

#if defined(CONFIG_BT_PRIVACY)
	{
		u8_t zero_irk[16] = { 0 };

		if (irk && memcmp(irk, zero_irk, 16)) {
			memcpy(&bt_dev.irk[id], irk, 16);
		} else {
			bt_rand(&bt_dev.irk[id], 16);
			if (irk) {
				memcpy(irk, &bt_dev.irk[id], 16);
			}
		}
	}
#endif
	/* Only store if stack was already initialized. Before initialization
	 * we don't know the flash content, so it's potentially harmful to
	 * try to write anything there.
	 */
	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		bt_settings_save_id();
	}
}

int bt_id_create(bt_addr_le_t *addr, u8_t *irk)
{
	int new_id;

	if (addr && bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
		if (addr->type != BT_ADDR_LE_RANDOM ||
		    !BT_ADDR_IS_STATIC(&addr->a)) {
			BT_ERR("Only static random identity address supported");
			return -EINVAL;
		}

		if (id_find(addr) >= 0) {
			return -EALREADY;
		}
	}

	if (!IS_ENABLED(CONFIG_BT_PRIVACY) && irk) {
		return -EINVAL;
	}

	if (bt_dev.id_count == ARRAY_SIZE(bt_dev.id_addr)) {
		return -ENOMEM;
	}

	new_id = bt_dev.id_count++;
	if (new_id == BT_ID_DEFAULT &&
	    !atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		atomic_set_bit(bt_dev.flags, BT_DEV_USER_ID_ADDR);
	}

	id_create(new_id, addr, irk);

	return new_id;
}

int bt_id_reset(u8_t id, bt_addr_le_t *addr, u8_t *irk)
{
	if (addr && bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
		if (addr->type != BT_ADDR_LE_RANDOM ||
		    !BT_ADDR_IS_STATIC(&addr->a)) {
			BT_ERR("Only static random identity address supported");
			return -EINVAL;
		}

		if (id_find(addr) >= 0) {
			return -EALREADY;
		}
	}

	if (!IS_ENABLED(CONFIG_BT_PRIVACY) && irk) {
		return -EINVAL;
	}

	if (id == BT_ID_DEFAULT || id >= bt_dev.id_count) {
		return -EINVAL;
	}

	if (id == bt_dev.adv_id && atomic_test_bit(bt_dev.flags,
						   BT_DEV_ADVERTISING)) {
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_BT_CONN) &&
	    bt_addr_le_cmp(&bt_dev.id_addr[id], BT_ADDR_LE_ANY)) {
		int err;

		err = bt_unpair(id, NULL);
		if (err) {
			return err;
		}
	}

	id_create(id, addr, irk);

	return id;
}

int bt_id_delete(u8_t id)
{
	if (id == BT_ID_DEFAULT || id >= bt_dev.id_count) {
		return -EINVAL;
	}

	if (!bt_addr_le_cmp(&bt_dev.id_addr[id], BT_ADDR_LE_ANY)) {
		return -EALREADY;
	}

	if (id == bt_dev.adv_id && atomic_test_bit(bt_dev.flags,
						   BT_DEV_ADVERTISING)) {
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		int err;

		err = bt_unpair(id, NULL);
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BT_PRIVACY)
	(void)memset(bt_dev.irk[id], 0, 16);
#endif
	bt_addr_le_copy(&bt_dev.id_addr[id], BT_ADDR_LE_ANY);

	if (id == bt_dev.id_count - 1) {
		bt_dev.id_count--;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		bt_settings_save_id();
	}

	return 0;
}

#if defined(CONFIG_BT_HCI_VS_EXT)
static uint8_t bt_read_static_addr(bt_addr_le_t *addr)
{
	struct bt_hci_rp_vs_read_static_addrs *rp;
	struct net_buf *rsp;
	int err, i;
	u8_t cnt;
	if (!(bt_dev.vs_commands[1] & BIT(0))) {
		BT_WARN("Read Static Addresses command not available");
		return 0;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS, NULL, &rsp);
	if (err) {
		BT_WARN("Failed to read static addresses");
		return 0;
	}
	rp = (void *)rsp->data;
	cnt = MIN(rp->num_addrs, CONFIG_BT_ID_MAX);

	for (i = 0; i < cnt; i++) {
		addr[i].type = BT_ADDR_LE_RANDOM;
		bt_addr_copy(&addr[i].a, &rp->a[i].bdaddr);
	}
	net_buf_unref(rsp);
	if (!cnt) {
		BT_WARN("No static addresses stored in controller");
	}
	return cnt;
}
#elif defined(CONFIG_BT_CTLR)
uint8_t bt_read_static_addr(bt_addr_le_t *addr);
#endif /* CONFIG_BT_HCI_VS_EXT */

int bt_setup_id_addr(void)
{
#if defined(CONFIG_BT_HCI_VS_EXT) || defined(CONFIG_BT_CTLR)
	/* Only read the addresses if the user has not already configured one or
	 * more identities (!bt_dev.id_count).
	 */
	if (!bt_dev.id_count) {
		bt_addr_le_t addrs[CONFIG_BT_ID_MAX];

		bt_dev.id_count = bt_read_static_addr(addrs);
		if (bt_dev.id_count) {
			int i;

			for (i = 0; i < bt_dev.id_count; i++) {
				id_create(i, &addrs[i], NULL);
			}

			return set_random_address(&bt_dev.id_addr[0].a);
		}
	}
#endif
	return bt_id_create(NULL, NULL);
}

bool bt_addr_le_is_bonded(u8_t id, const bt_addr_le_t *addr)
{
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		struct bt_keys *keys = bt_keys_find_addr(id, addr);

		/* if there are any keys stored then device is bonded */
		return keys && keys->keys;
	} else {
		return false;
	}
}

static bool valid_adv_param(const struct bt_le_adv_param *param, bool dir_adv)
{
	if (param->id >= bt_dev.id_count ||
	    !bt_addr_le_cmp(&bt_dev.id_addr[param->id], BT_ADDR_LE_ANY)) {
		return false;
	}

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

	if (is_wl_empty() &&
	    ((param->options & BT_LE_ADV_OPT_FILTER_SCAN_REQ) ||
	     (param->options & BT_LE_ADV_OPT_FILTER_CONN))) {
		return false;
	}


	if ((param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY) || !dir_adv) {
		if (param->interval_min > param->interval_max ||
		    param->interval_min < 0x0020 ||
		    param->interval_max > 0x4000) {
			return false;
		}
	}

	return true;
}

static inline bool ad_has_name(const struct bt_data *ad, size_t ad_len)
{
	int i;

	for (i = 0; i < ad_len; i++) {
		if (ad[i].type == BT_DATA_NAME_COMPLETE ||
		    ad[i].type == BT_DATA_NAME_SHORTENED) {
			return true;
		}
	}

	return false;
}

static int le_adv_update(const struct bt_data *ad, size_t ad_len,
			 const struct bt_data *sd, size_t sd_len,
			 bool connectable, bool use_name)
{
	struct bt_ad d[2] = {};
	struct bt_data data;
	int err;

	d[0].data = ad;
	d[0].len = ad_len;

	err = set_ad(BT_HCI_OP_LE_SET_ADV_DATA, d, 1);
	if (err) {
		return err;
	}

	d[0].data = sd;
	d[0].len = sd_len;

	if (use_name) {
		const char *name;

		if (sd) {
			/* Cannot use name if name is already set */
			if (ad_has_name(sd, sd_len)) {
				return -EINVAL;
			}
		}

		name = bt_get_name();
		data = (struct bt_data)BT_DATA(
			BT_DATA_NAME_COMPLETE,
			name, strlen(name));

		d[1].data = &data;
		d[1].len = 1;
	}

	/*
	 * We need to set SCAN_RSP when enabling advertising type that
	 * allows for Scan Requests.
	 *
	 * If any data was not provided but we enable connectable
	 * undirected advertising sd needs to be cleared from values set
	 * by previous calls.
	 * Clearing sd is done by calling set_ad() with NULL data and
	 * zero len.
	 * So following condition check is unusual but correct.
	 */
	if (d[0].data || d[1].data || connectable) {
		err = set_ad(BT_HCI_OP_LE_SET_SCAN_RSP_DATA, d, 2);
		if (err) {
			return err;
		}
	}

	return 0;
}

int bt_le_adv_update_data(const struct bt_data *ad, size_t ad_len,
			  const struct bt_data *sd, size_t sd_len)
{
	bool connectable, use_name;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return -EAGAIN;
	}

	connectable = atomic_test_bit(bt_dev.flags,
				      BT_DEV_ADVERTISING_CONNECTABLE);
	use_name = atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING_NAME);

	return le_adv_update(ad, ad_len, sd, sd_len, connectable, use_name);
}

int bt_le_adv_start_internal(const struct bt_le_adv_param *param,
			     const struct bt_data *ad, size_t ad_len,
			     const struct bt_data *sd, size_t sd_len,
			     const bt_addr_le_t *peer)
{
	struct bt_hci_cp_le_set_adv_param set_param;
	const bt_addr_le_t *id_addr;
	struct net_buf *buf;
	bool dir_adv = (peer != NULL);
	int err = 0;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (!valid_adv_param(param, dir_adv)) {
		return -EINVAL;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return -EALREADY;
	}

	(void)memset(&set_param, 0, sizeof(set_param));

	set_param.min_interval = sys_cpu_to_le16(param->interval_min);
	set_param.max_interval = sys_cpu_to_le16(param->interval_max);
	set_param.channel_map  = 0x07;

	if (bt_dev.adv_id != param->id) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);
	}

#if defined(CONFIG_BT_WHITELIST)
	if ((param->options & BT_LE_ADV_OPT_FILTER_SCAN_REQ) &&
	    (param->options & BT_LE_ADV_OPT_FILTER_CONN)) {
		set_param.filter_policy = BT_LE_ADV_FP_WHITELIST_BOTH;
	} else if (param->options & BT_LE_ADV_OPT_FILTER_SCAN_REQ) {
		set_param.filter_policy = BT_LE_ADV_FP_WHITELIST_SCAN_REQ;
	} else if (param->options & BT_LE_ADV_OPT_FILTER_CONN) {
		set_param.filter_policy = BT_LE_ADV_FP_WHITELIST_CONN_IND;
	} else {
#else
	{
#endif /* defined(CONFIG_BT_WHITELIST) */
		set_param.filter_policy = BT_LE_ADV_FP_NO_WHITELIST;
	}

	/* Set which local identity address we're advertising with */
	bt_dev.adv_id = param->id;
	id_addr = &bt_dev.id_addr[param->id];

	if (param->options & BT_LE_ADV_OPT_CONNECTABLE) {
		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
		    !(param->options & BT_LE_ADV_OPT_USE_IDENTITY)) {
			err = le_set_private_addr(param->id);
			if (err) {
				return err;
			}

			if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
				set_param.own_addr_type =
					BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
			} else {
				set_param.own_addr_type = BT_ADDR_LE_RANDOM;
			}
		} else {
			/*
			 * If Static Random address is used as Identity
			 * address we need to restore it before advertising
			 * is enabled. Otherwise NRPA used for active scan
			 * could be used for advertising.
			 */
			if (id_addr->type == BT_ADDR_LE_RANDOM) {
				err = set_random_address(&id_addr->a);
				if (err) {
					return err;
				}
			}

			set_param.own_addr_type = id_addr->type;
		}

		if (dir_adv) {
			if (param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY) {
				set_param.type = BT_LE_ADV_DIRECT_IND_LOW_DUTY;
			} else {
				set_param.type = BT_LE_ADV_DIRECT_IND;
			}

			bt_addr_le_copy(&set_param.direct_addr, peer);

			if (IS_ENABLED(CONFIG_BT_SMP) &&
			    !IS_ENABLED(CONFIG_BT_PRIVACY) &&
			    BT_FEAT_LE_PRIVACY(bt_dev.le.features) &&
			    (param->options & BT_LE_ADV_OPT_DIR_ADDR_RPA)) {
				/* This will not use RPA for our own address
				 * since we have set zeroed out the local IRK.
				 */
				set_param.own_addr_type |=
					BT_HCI_OWN_ADDR_RPA_MASK;
			}
		} else {
			set_param.type = BT_LE_ADV_IND;
		}
	} else {
		if (param->options & BT_LE_ADV_OPT_USE_IDENTITY) {
			if (id_addr->type == BT_ADDR_LE_RANDOM) {
				err = set_random_address(&id_addr->a);
			}

			set_param.own_addr_type = id_addr->type;
		} else {
			err = le_set_private_addr(param->id);
			set_param.own_addr_type = BT_ADDR_LE_RANDOM;
		}

		if (err) {
			return err;
		}

		if (sd) {
			set_param.type = BT_LE_ADV_SCAN_IND;
		} else {
			set_param.type = BT_LE_ADV_NONCONN_IND;
		}
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_PARAM, sizeof(set_param));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_mem(buf, &set_param, sizeof(set_param));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_PARAM, buf, NULL);
	if (err) {
		return err;
	}

	if (!dir_adv) {
		err = le_adv_update(ad, ad_len, sd, sd_len,
				    param->options & BT_LE_ADV_OPT_CONNECTABLE,
				    param->options & BT_LE_ADV_OPT_USE_NAME);
		if (err) {
			return err;
		}
	}

	err = set_advertise_enable(true);
	if (err) {
		return err;
	}

	atomic_set_bit_to(bt_dev.flags, BT_DEV_KEEP_ADVERTISING,
			  !(param->options & BT_LE_ADV_OPT_ONE_TIME));

	atomic_set_bit_to(bt_dev.flags, BT_DEV_ADVERTISING_NAME,
			  param->options & BT_LE_ADV_OPT_USE_NAME);

	atomic_set_bit_to(bt_dev.flags, BT_DEV_ADVERTISING_CONNECTABLE,
			  param->options & BT_LE_ADV_OPT_CONNECTABLE);

	return 0;
}

int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
	if (param->options & BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY) {
		return -EINVAL;
	}

	return bt_le_adv_start_internal(param, ad, ad_len, sd, sd_len, NULL);
}

int bt_le_adv_stop(void)
{
	int err;

	/* Make sure advertising is not re-enabled later even if it's not
	 * currently enabled (i.e. BT_DEV_ADVERTISING is not set).
	 */
	atomic_clear_bit(bt_dev.flags, BT_DEV_KEEP_ADVERTISING);

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_ADVERTISING)) {
		return 0;
	}

	err = set_advertise_enable(false);
	if (err) {
		return err;
	}

	if (!IS_ENABLED(CONFIG_BT_PRIVACY)) {
		/* If active scan is ongoing set NRPA */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING) &&
		    atomic_test_bit(bt_dev.flags, BT_DEV_ACTIVE_SCAN)) {
			le_set_private_addr(bt_dev.adv_id);
		}
	}

	return 0;
}

#if defined(CONFIG_BT_OBSERVER)
static bool valid_le_scan_param(const struct bt_le_scan_param *param)
{
	if (param->type != BT_HCI_LE_SCAN_PASSIVE &&
	    param->type != BT_HCI_LE_SCAN_ACTIVE) {
		return false;
	}

	if (param->filter_dup &
	    ~(BT_LE_SCAN_FILTER_DUPLICATE | BT_LE_SCAN_FILTER_WHITELIST)) {
		return false;
	}

	if (is_wl_empty() &&
	    param->filter_dup & BT_LE_SCAN_FILTER_WHITELIST) {
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

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	/* Check that the parameters have valid values */
	if (!valid_le_scan_param(param)) {
		return -EINVAL;
	}

	/* Return if active scan is already enabled */
	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return -EALREADY;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		err = set_le_scan_enable(BT_HCI_LE_SCAN_DISABLE);
		if (err) {
			atomic_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN);
			return err;
		}
	}

	atomic_set_bit_to(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP,
			  param->filter_dup & BT_LE_SCAN_FILTER_DUPLICATE);

#if defined(CONFIG_BT_WHITELIST)
	atomic_set_bit_to(bt_dev.flags, BT_DEV_SCAN_WL,
			  param->filter_dup & BT_LE_SCAN_FILTER_WHITELIST);
#endif /* defined(CONFIG_BT_WHITELIST) */

	err = start_le_scan(param->type, param->interval, param->window);
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
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_WHITELIST)
int bt_le_whitelist_add(const bt_addr_le_t *addr)
{
	struct bt_hci_cp_le_add_dev_to_wl *cp;
	struct net_buf *buf;
	int err;

	if (!(bt_dev.le.wl_entries < bt_dev.le.wl_size)) {
		return -ENOMEM;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_ADD_DEV_TO_WL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->addr, addr);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_ADD_DEV_TO_WL, buf, NULL);
	if (err) {
		BT_ERR("Failed to add device to whitelist");

		return err;
	}

	bt_dev.le.wl_entries++;

	return 0;
}

int bt_le_whitelist_rem(const bt_addr_le_t *addr)
{
	struct bt_hci_cp_le_rem_dev_from_wl *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REM_DEV_FROM_WL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->addr, addr);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REM_DEV_FROM_WL, buf, NULL);
	if (err) {
		BT_ERR("Failed to remove device from whitelist");
		return err;
	}

	bt_dev.le.wl_entries--;
	return 0;
}

int bt_le_whitelist_clear(void)
{
	int err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_CLEAR_WL, NULL, NULL);

	if (err) {
		BT_ERR("Failed to clear whitelist");
		return err;
	}

	bt_dev.le.wl_entries = 0;
	return 0;
}
#endif /* defined(CONFIG_BT_WHITELIST) */

int bt_le_set_chan_map(u8_t chan_map[5])
{
	struct bt_hci_cp_le_set_host_chan_classif *cp;
	struct net_buf *buf;

	if (!IS_ENABLED(CONFIG_BT_CENTRAL)) {
		return -ENOTSUP;
	}

	if (!BT_CMD_TEST(bt_dev.supported_commands, 27, 3)) {
		BT_WARN("Set Host Channel Classification command is "
			"not supported");
		return -ENOTSUP;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_HOST_CHAN_CLASSIF,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	memcpy(&cp->ch_map[0], &chan_map[0], 4);
	cp->ch_map[4] = chan_map[4] & BIT_MASK(5);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_HOST_CHAN_CLASSIF,
				    buf, NULL);
}

struct net_buf *bt_buf_get_rx(enum bt_buf_type type, s32_t timeout)
{
	struct net_buf *buf;

	__ASSERT(type == BT_BUF_EVT || type == BT_BUF_ACL_IN,
		 "Invalid buffer type requested");

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	if (type == BT_BUF_EVT) {
		buf = net_buf_alloc(&hci_rx_pool, timeout);
	} else {
		buf = net_buf_alloc(&acl_in_pool, timeout);
	}
#else
	buf = net_buf_alloc(&hci_rx_pool, timeout);
#endif

	if (buf) {
		net_buf_reserve(buf, CONFIG_BT_HCI_RESERVE);
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
		buf->len = 0U;
		net_buf_reserve(buf, CONFIG_BT_HCI_RESERVE);

		return buf;
	}

	return bt_buf_get_rx(BT_BUF_EVT, timeout);
}

struct net_buf *bt_buf_get_evt(u8_t evt, bool discardable, s32_t timeout)
{
	switch (evt) {
#if defined(CONFIG_BT_CONN)
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
		{
			struct net_buf *buf;

			buf = net_buf_alloc(&num_complete_pool, timeout);
			if (buf) {
				net_buf_reserve(buf, CONFIG_BT_HCI_RESERVE);
				bt_buf_set_type(buf, BT_BUF_EVT);
			}

			return buf;
		}
#endif /* CONFIG_BT_CONN */
	case BT_HCI_EVT_CMD_COMPLETE:
	case BT_HCI_EVT_CMD_STATUS:
		return bt_buf_get_cmd_complete(timeout);
	default:
#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
		if (discardable) {
			struct net_buf *buf;

			buf = net_buf_alloc(&discardable_pool, timeout);
			if (buf) {
				net_buf_reserve(buf, CONFIG_BT_HCI_RESERVE);
				bt_buf_set_type(buf, BT_BUF_EVT);
			}

			return buf;
		}
#endif /* CONFIG_BT_DISCARDABLE_BUF_COUNT */

		return bt_buf_get_rx(BT_BUF_EVT, timeout);
	}
}

#if defined(CONFIG_BT_BREDR)
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

	(void)memset(results, 0, sizeof(*results) * cnt);

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

	atomic_set_bit_to(bt_dev.flags, BT_DEV_ISCAN,
			  (scan & BT_BREDR_SCAN_INQUIRY));
	atomic_set_bit_to(bt_dev.flags, BT_DEV_PSCAN,
			  (scan & BT_BREDR_SCAN_PAGE));

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
#endif /* CONFIG_BT_BREDR */

#if defined(CONFIG_BT_ECC)
int bt_pub_key_gen(struct bt_pub_key_cb *new_cb)
{
	int err;

	/*
	 * We check for both "LE Read Local P-256 Public Key" and
	 * "LE Generate DH Key" support here since both commands are needed for
	 * ECC support. If "LE Generate DH Key" is not supported then there
	 * is no point in reading local public key.
	 */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 34, 1) ||
	    !BT_CMD_TEST(bt_dev.supported_commands, 34, 2)) {
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
#endif /* CONFIG_BT_ECC */

#if defined(CONFIG_BT_BREDR)
int bt_br_oob_get_local(struct bt_br_oob *oob)
{
	bt_addr_copy(&oob->addr, &bt_dev.id_addr[0].a);

	return 0;
}
#endif /* CONFIG_BT_BREDR */

int bt_le_oob_get_local(u8_t id, struct bt_le_oob *oob)
{
	int err;

	if (id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		/* Invalidate RPA so a new one is generated */
		atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);

		err = le_set_private_addr(id);
		if (err) {
			return err;
		}

		bt_addr_le_copy(&oob->addr, &bt_dev.random_addr);
	} else {
		bt_addr_le_copy(&oob->addr, &bt_dev.id_addr[id]);
	}


	if (IS_ENABLED(CONFIG_BT_SMP)) {
		err = bt_smp_le_oob_generate_sc_data(&oob->le_sc_data);
		if (err) {
			return err;
		}
	}

	return 0;
}

#if defined(CONFIG_BT_SMP)
int bt_le_oob_set_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data *oobd_local,
			  const struct bt_le_oob_sc_data *oobd_remote)
{
	return bt_smp_le_oob_set_sc_data(conn, oobd_local, oobd_remote);
}

int bt_le_oob_get_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data **oobd_local,
			  const struct bt_le_oob_sc_data **oobd_remote)
{
	return bt_smp_le_oob_get_sc_data(conn, oobd_local, oobd_remote);
}
#endif
