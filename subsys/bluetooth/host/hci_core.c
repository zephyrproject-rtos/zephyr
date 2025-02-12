/* hci_core.c - HCI core Bluetooth handling */

/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/debug/stack.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>
#if DT_HAS_CHOSEN(zephyr_bt_hci)
#include <zephyr/drivers/bluetooth.h>
#else
#include <zephyr/drivers/bluetooth/hci_driver.h>
#endif

#include "common/bt_str.h"
#include "common/assert.h"

#include "common/rpa.h"
#include "keys.h"
#include "monitor.h"
#include "hci_core.h"
#include "hci_ecc.h"
#include "ecc.h"
#include "id.h"
#include "adv.h"
#include "scan.h"

#include "addr_internal.h"
#include "conn_internal.h"
#include "iso_internal.h"
#include "l2cap_internal.h"
#include "gatt_internal.h"
#include "smp.h"
#include "crypto.h"
#include "settings.h"

#if defined(CONFIG_BT_CLASSIC)
#include "classic/br.h"
#endif

#if defined(CONFIG_BT_DF)
#include "direction_internal.h"
#endif /* CONFIG_BT_DF */

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_core);

#define BT_HCI_DEV  DT_CHOSEN(zephyr_bt_hci)
#define BT_HCI_BUS  BT_DT_HCI_BUS_GET(BT_HCI_DEV)
#define BT_HCI_NAME BT_DT_HCI_NAME_GET(BT_HCI_DEV)

void bt_tx_irq_raise(void);

#define HCI_CMD_TIMEOUT      K_SECONDS(10)

/* Stacks for the threads */
static void rx_work_handler(struct k_work *work);
static K_WORK_DEFINE(rx_work, rx_work_handler);
#if defined(CONFIG_BT_RECV_WORKQ_BT)
static struct k_work_q bt_workq;
static K_KERNEL_STACK_DEFINE(rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);
#endif /* CONFIG_BT_RECV_WORKQ_BT */

static void init_work(struct k_work *work);

struct bt_dev bt_dev = {
	.init          = Z_WORK_INITIALIZER(init_work),
#if defined(CONFIG_BT_PRIVACY)
	.rpa_timeout   = CONFIG_BT_RPA_TIMEOUT,
#endif
#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
	.appearance = CONFIG_BT_DEVICE_APPEARANCE,
#endif
#if DT_HAS_CHOSEN(zephyr_bt_hci)
	.hci = DEVICE_DT_GET(BT_HCI_DEV),
#endif
};

static bt_ready_cb_t ready_cb;

#if defined(CONFIG_BT_HCI_VS_EVT_USER)
static bt_hci_vnd_evt_cb_t *hci_vnd_evt_cb;
#endif /* CONFIG_BT_HCI_VS_EVT_USER */

struct cmd_data {
	/** HCI status of the command completion */
	uint8_t  status;

	/** The command OpCode that the buffer contains */
	uint16_t opcode;

	/** The state to update when command completes with success. */
	struct bt_hci_cmd_state_set *state;

	/** Used by bt_hci_cmd_send_sync. */
	struct k_sem *sync;
};

static struct cmd_data cmd_data[CONFIG_BT_BUF_CMD_TX_COUNT];

#define cmd(buf) (&cmd_data[net_buf_id(buf)])
#define acl(buf) ((struct acl_data *)net_buf_user_data(buf))

#if DT_HAS_CHOSEN(zephyr_bt_hci)
static bool drv_quirk_no_reset(void)
{
	return  ((BT_DT_HCI_QUIRKS_GET(DT_CHOSEN(zephyr_bt_hci)) & BT_HCI_QUIRK_NO_RESET) != 0);
}

__maybe_unused static bool drv_quirk_no_auto_dle(void)
{
	return  ((BT_DT_HCI_QUIRKS_GET(DT_CHOSEN(zephyr_bt_hci)) & BT_HCI_QUIRK_NO_AUTO_DLE) != 0);
}
#else
static bool drv_quirk_no_reset(void)
{
	return  ((bt_dev.drv->quirks & BT_QUIRK_NO_RESET) != 0);
}

__maybe_unused static bool drv_quirk_no_auto_dle(void)
{
	return  ((bt_dev.drv->quirks & BT_QUIRK_NO_AUTO_DLE) != 0);
}
#endif

void bt_hci_cmd_state_set_init(struct net_buf *buf,
			       struct bt_hci_cmd_state_set *state,
			       atomic_t *target, int bit, bool val)
{
	state->target = target;
	state->bit = bit;
	state->val = val;
	cmd(buf)->state = state;
}

/* HCI command buffers. Derive the needed size from both Command and Event
 * buffer length since the buffer is also used for the response event i.e
 * command complete or command status.
 */
#define CMD_BUF_SIZE MAX(BT_BUF_EVT_RX_SIZE, BT_BUF_CMD_TX_SIZE)
NET_BUF_POOL_FIXED_DEFINE(hci_cmd_pool, CONFIG_BT_BUF_CMD_TX_COUNT,
			  CMD_BUF_SIZE, sizeof(struct bt_buf_data), NULL);

struct event_handler {
	uint8_t event;
	uint8_t min_len;
	void (*handler)(struct net_buf *buf);
};

#define EVENT_HANDLER(_evt, _handler, _min_len) \
{ \
	.event = _evt, \
	.handler = _handler, \
	.min_len = _min_len, \
}

static int handle_event_common(uint8_t event, struct net_buf *buf,
			       const struct event_handler *handlers, size_t num_handlers)
{
	size_t i;

	for (i = 0; i < num_handlers; i++) {
		const struct event_handler *handler = &handlers[i];

		if (handler->event != event) {
			continue;
		}

		if (buf->len < handler->min_len) {
			LOG_ERR("Too small (%u bytes) event 0x%02x", buf->len, event);
			return -EINVAL;
		}

		handler->handler(buf);
		return 0;
	}

	return -EOPNOTSUPP;
}

static void handle_event(uint8_t event, struct net_buf *buf, const struct event_handler *handlers,
			 size_t num_handlers)
{
	int err;

	err = handle_event_common(event, buf, handlers, num_handlers);
	if (err == -EOPNOTSUPP) {
		LOG_WRN("Unhandled event 0x%02x len %u: %s", event, buf->len,
			bt_hex(buf->data, buf->len));
	}

	/* Other possible errors are handled by handle_event_common function */
}

static void handle_vs_event(uint8_t event, struct net_buf *buf,
			    const struct event_handler *handlers, size_t num_handlers)
{
	int err;

	err = handle_event_common(event, buf, handlers, num_handlers);
	if (err == -EOPNOTSUPP) {
		LOG_WRN("Unhandled vendor-specific event: %s", bt_hex(buf->data, buf->len));
	}

	/* Other possible errors are handled by handle_event_common function */
}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
void bt_hci_host_num_completed_packets(struct net_buf *buf)
{

	struct bt_hci_cp_host_num_completed_packets *cp;
	uint16_t handle = acl(buf)->handle;
	struct bt_hci_handle_count *hc;
	struct bt_conn *conn;
	uint8_t index = acl(buf)->index;

	net_buf_destroy(buf);

	/* Do nothing if controller to host flow control is not supported */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 10, 5)) {
		return;
	}

	conn = bt_conn_lookup_index(index);
	if (!conn) {
		LOG_WRN("Unable to look up conn with index 0x%02x", index);
		return;
	}

	if (conn->state != BT_CONN_CONNECTED &&
	    conn->state != BT_CONN_DISCONNECTING) {
		LOG_WRN("Not reporting packet for non-connected conn");
		bt_conn_unref(conn);
		return;
	}

	bt_conn_unref(conn);

	LOG_DBG("Reporting completed packet for handle %u", handle);

	buf = bt_hci_cmd_create(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS,
				sizeof(*cp) + sizeof(*hc));
	if (!buf) {
		LOG_ERR("Unable to allocate new HCI command");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->num_handles = sys_cpu_to_le16(1);

	hc = net_buf_add(buf, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(handle);
	hc->count  = sys_cpu_to_le16(1);

	bt_hci_cmd_send(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS, buf);
}
#endif /* defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL) */

struct net_buf *bt_hci_cmd_create(uint16_t opcode, uint8_t param_len)
{
	struct bt_hci_cmd_hdr *hdr;
	struct net_buf *buf;

	LOG_DBG("opcode 0x%04x param_len %u", opcode, param_len);

	/* net_buf_alloc(K_FOREVER) can fail when run from the syswq */
	buf = net_buf_alloc(&hci_cmd_pool, K_FOREVER);
	if (!buf) {
		LOG_DBG("Unable to allocate a command buffer");
		return NULL;
	}

	LOG_DBG("buf %p", buf);

	net_buf_reserve(buf, BT_BUF_RESERVE);

	bt_buf_set_type(buf, BT_BUF_CMD);

	cmd(buf)->opcode = opcode;
	cmd(buf)->sync = NULL;
	cmd(buf)->state = NULL;

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

	LOG_DBG("opcode 0x%04x len %u", opcode, buf->len);

	/* Host Number of Completed Packets can ignore the ncmd value
	 * and does not generate any cmd complete/status events.
	 */
	if (opcode == BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS) {
		int err;

		err = bt_send(buf);
		if (err) {
			LOG_ERR("Unable to send to driver (err %d)", err);
			net_buf_unref(buf);
		}

		return err;
	}

	net_buf_put(&bt_dev.cmd_tx_queue, buf);
	bt_tx_irq_raise();

	return 0;
}

static bool process_pending_cmd(k_timeout_t timeout);
int bt_hci_cmd_send_sync(uint16_t opcode, struct net_buf *buf,
			 struct net_buf **rsp)
{
	struct k_sem sync_sem;
	uint8_t status;
	int err;

	if (!buf) {
		buf = bt_hci_cmd_create(opcode, 0);
		if (!buf) {
			return -ENOBUFS;
		}
	} else {
		/* `cmd(buf)` depends on this  */
		if (net_buf_pool_get(buf->pool_id) != &hci_cmd_pool) {
			__ASSERT_NO_MSG(false);
			return -EINVAL;
		}
	}

	LOG_DBG("buf %p opcode 0x%04x len %u", buf, opcode, buf->len);

	/* This local sem is just for suspending the current thread until the
	 * command is processed by the LL. It is given (and we are awaken) by
	 * the cmd_complete/status handlers.
	 */
	k_sem_init(&sync_sem, 0, 1);
	cmd(buf)->sync = &sync_sem;

	net_buf_put(&bt_dev.cmd_tx_queue, net_buf_ref(buf));
	bt_tx_irq_raise();

	/* TODO: disallow sending sync commands from syswq altogether */

	/* Since the commands are now processed in the syswq, we cannot suspend
	 * and wait. We have to send the command from the current context.
	 */
	if (k_current_get() == &k_sys_work_q.thread) {
		/* drain the command queue until we get to send the command of interest. */
		struct net_buf *cmd = NULL;

		do {
			cmd = k_fifo_peek_head(&bt_dev.cmd_tx_queue);
			LOG_DBG("process cmd %p want %p", cmd, buf);

			/* Wait for a response from the Bluetooth Controller.
			 * The Controller may fail to respond if:
			 *  - It was never programmed or connected.
			 *  - There was a fatal error.
			 *
			 * See the `BT_HCI_OP_` macros in hci_types.h or
			 * Core_v5.4, Vol 4, Part E, Section 5.4.1 and Section 7
			 * to map the opcode to the HCI command documentation.
			 * Example: 0x0c03 represents HCI_Reset command.
			 */
			__maybe_unused bool success = process_pending_cmd(HCI_CMD_TIMEOUT);

			BT_ASSERT_MSG(success, "command opcode 0x%04x timeout", opcode);
		} while (buf != cmd);
	}

	/* Now that we have sent the command, suspend until the LL replies */
	err = k_sem_take(&sync_sem, HCI_CMD_TIMEOUT);
	BT_ASSERT_MSG(err == 0,
		      "Controller unresponsive, command opcode 0x%04x timeout with err %d",
		      opcode, err);

	status = cmd(buf)->status;
	if (status) {
		LOG_WRN("opcode 0x%04x status 0x%02x", opcode, status);
		net_buf_unref(buf);

		switch (status) {
		case BT_HCI_ERR_CONN_LIMIT_EXCEEDED:
			return -ECONNREFUSED;
		case BT_HCI_ERR_INSUFFICIENT_RESOURCES:
			return -ENOMEM;
		case BT_HCI_ERR_INVALID_PARAM:
			return -EINVAL;
		case BT_HCI_ERR_CMD_DISALLOWED:
			return -EACCES;
		default:
			return -EIO;
		}
	}

	LOG_DBG("rsp %p opcode 0x%04x len %u", buf, opcode, buf->len);

	if (rsp) {
		*rsp = buf;
	} else {
		net_buf_unref(buf);
	}

	return 0;
}

int bt_hci_le_rand(void *buffer, size_t len)
{
	struct bt_hci_rp_le_rand *rp;
	struct net_buf *rsp;
	size_t count;
	int err;

	/* Check first that HCI_LE_Rand is supported */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 27, 7)) {
		return -ENOTSUP;
	}

	while (len > 0) {
		/* Number of bytes to fill on this iteration */
		count = MIN(len, sizeof(rp->rand));
		/* Request the next 8 bytes over HCI */
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_RAND, NULL, &rsp);
		if (err) {
			return err;
		}
		/* Copy random data into buffer */
		rp = (void *)rsp->data;
		memcpy(buffer, rp->rand, count);

		net_buf_unref(rsp);
		buffer = (uint8_t *)buffer + count;
		len -= count;
	}

	return 0;
}

static int hci_le_read_max_data_len(uint16_t *tx_octets, uint16_t *tx_time)
{
	struct bt_hci_rp_le_read_max_data_len *rp;
	struct net_buf *rsp;
	int err;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_MAX_DATA_LEN, NULL, &rsp);
	if (err) {
		LOG_ERR("Failed to read DLE max data len");
		return err;
	}

	rp = (void *)rsp->data;
	*tx_octets = sys_le16_to_cpu(rp->max_tx_octets);
	*tx_time = sys_le16_to_cpu(rp->max_tx_time);
	net_buf_unref(rsp);

	if (!IN_RANGE(*tx_octets, BT_HCI_LE_MAX_TX_OCTETS_MIN, BT_HCI_LE_MAX_TX_OCTETS_MAX)) {
		LOG_WRN("tx_octets exceeds the valid range %u", *tx_octets);
	}
	if (!IN_RANGE(*tx_time, BT_HCI_LE_MAX_TX_TIME_MIN, BT_HCI_LE_MAX_TX_TIME_MAX)) {
		LOG_WRN("tx_time exceeds the valid range %u", *tx_time);
	}

	return 0;
}

uint8_t bt_get_phy(uint8_t hci_phy)
{
	switch (hci_phy) {
	case BT_HCI_LE_PHY_1M:
		return BT_GAP_LE_PHY_1M;
	case BT_HCI_LE_PHY_2M:
		return BT_GAP_LE_PHY_2M;
	case BT_HCI_LE_PHY_CODED:
		return BT_GAP_LE_PHY_CODED;
	default:
		return 0;
	}
}

int bt_get_df_cte_type(uint8_t hci_cte_type)
{
	switch (hci_cte_type) {
	case BT_HCI_LE_AOA_CTE:
		return BT_DF_CTE_TYPE_AOA;
	case BT_HCI_LE_AOD_CTE_1US:
		return BT_DF_CTE_TYPE_AOD_1US;
	case BT_HCI_LE_AOD_CTE_2US:
		return BT_DF_CTE_TYPE_AOD_2US;
	case BT_HCI_LE_NO_CTE:
		return BT_DF_CTE_TYPE_NONE;
	default:
		return BT_DF_CTE_TYPE_NONE;
	}
}

#if defined(CONFIG_BT_CONN_TX)
static void hci_num_completed_packets(struct net_buf *buf)
{
	struct bt_hci_evt_num_completed_packets *evt = (void *)buf->data;
	int i;

	if (sizeof(*evt) + sizeof(evt->h[0]) * evt->num_handles > buf->len) {
		LOG_ERR("evt num_handles (=%u) too large (%u > %u)",
			evt->num_handles,
			sizeof(*evt) + sizeof(evt->h[0]) * evt->num_handles,
			buf->len);
		return;
	}

	LOG_DBG("num_handles %u", evt->num_handles);

	for (i = 0; i < evt->num_handles; i++) {
		uint16_t handle, count;
		struct bt_conn *conn;

		handle = sys_le16_to_cpu(evt->h[i].handle);
		count = sys_le16_to_cpu(evt->h[i].count);

		LOG_DBG("handle %u count %u", handle, count);

		conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_ALL);
		if (!conn) {
			LOG_ERR("No connection for handle %u", handle);
			continue;
		}

		while (count--) {
			sys_snode_t *node;

			k_sem_give(bt_conn_get_pkts(conn));

			/* move the next TX context from the `pending` list to
			 * the `complete` list.
			 */
			node = sys_slist_get(&conn->tx_pending);

			if (!node) {
				LOG_ERR("packets count mismatch");
				__ASSERT_NO_MSG(0);
				break;
			}

			sys_slist_append(&conn->tx_complete, node);

			/* align the `pending` value */
			__ASSERT_NO_MSG(atomic_get(&conn->in_ll));
			atomic_dec(&conn->in_ll);

			/* TX context free + callback happens in there */
			k_work_submit(&conn->tx_complete_work);
		}

		bt_conn_unref(conn);
	}
}
#endif /* CONFIG_BT_CONN_TX */

#if defined(CONFIG_BT_CONN)
static void hci_acl(struct net_buf *buf)
{
	struct bt_hci_acl_hdr *hdr;
	uint16_t handle, len;
	struct bt_conn *conn;
	uint8_t flags;

	LOG_DBG("buf %p", buf);
	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Invalid HCI ACL packet size (%u)", buf->len);
		net_buf_unref(buf);
		return;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = sys_le16_to_cpu(hdr->len);
	handle = sys_le16_to_cpu(hdr->handle);
	flags = bt_acl_flags(handle);

	acl(buf)->handle = bt_acl_handle(handle);
	acl(buf)->index = BT_CONN_INDEX_INVALID;

	LOG_DBG("handle %u len %u flags %u", acl(buf)->handle, len, flags);

	if (buf->len != len) {
		LOG_ERR("ACL data length mismatch (%u != %u)", buf->len, len);
		net_buf_unref(buf);
		return;
	}

	conn = bt_conn_lookup_handle(acl(buf)->handle, BT_CONN_TYPE_ALL);
	if (!conn) {
		LOG_ERR("Unable to find conn for handle %u", acl(buf)->handle);
		net_buf_unref(buf);
		return;
	}

	acl(buf)->index = bt_conn_index(conn);

	bt_conn_recv(conn, buf, flags);
	bt_conn_unref(conn);
}

static void hci_data_buf_overflow(struct net_buf *buf)
{
	struct bt_hci_evt_data_buf_overflow *evt = (void *)buf->data;

	LOG_WRN("Data buffer overflow (link type 0x%02x)", evt->link_type);
}

#if defined(CONFIG_BT_CENTRAL)
static void set_phy_conn_param(const struct bt_conn *conn,
			       struct bt_hci_ext_conn_phy *phy)
{
	phy->conn_interval_min = sys_cpu_to_le16(conn->le.interval_min);
	phy->conn_interval_max = sys_cpu_to_le16(conn->le.interval_max);
	phy->conn_latency = sys_cpu_to_le16(conn->le.latency);
	phy->supervision_timeout = sys_cpu_to_le16(conn->le.timeout);

	phy->min_ce_len = 0;
	phy->max_ce_len = 0;
}

int bt_le_create_conn_ext(const struct bt_conn *conn)
{
	struct bt_hci_cp_le_ext_create_conn *cp;
	struct bt_hci_ext_conn_phy *phy;
	struct bt_hci_cmd_state_set state;
	bool use_filter = false;
	struct net_buf *buf;
	uint8_t own_addr_type;
	uint8_t num_phys;
	int err;

	if (IS_ENABLED(CONFIG_BT_FILTER_ACCEPT_LIST)) {
		use_filter = atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT);
	}

	err = bt_id_set_create_conn_own_addr(use_filter, &own_addr_type);
	if (err) {
		return err;
	}

	num_phys = (!(bt_dev.create_param.options &
		      BT_CONN_LE_OPT_NO_1M) ? 1 : 0) +
		   ((bt_dev.create_param.options &
		      BT_CONN_LE_OPT_CODED) ? 1 : 0);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_EXT_CREATE_CONN, sizeof(*cp) +
				num_phys * sizeof(*phy));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	if (use_filter) {
		/* User Initiated procedure use fast scan parameters. */
		bt_addr_le_copy(&cp->peer_addr, BT_ADDR_LE_ANY);
		cp->filter_policy = BT_HCI_LE_CREATE_CONN_FP_FILTER;
	} else {
		const bt_addr_le_t *peer_addr = &conn->le.dst;

#if defined(CONFIG_BT_SMP)
		if (bt_dev.le.rl_entries > bt_dev.le.rl_size) {
			/* Host resolving is used, use the RPA directly. */
			peer_addr = &conn->le.resp_addr;
		}
#endif
		bt_addr_le_copy(&cp->peer_addr, peer_addr);
		cp->filter_policy = BT_HCI_LE_CREATE_CONN_FP_NO_FILTER;
	}

	cp->own_addr_type = own_addr_type;
	cp->phys = 0;

	if (!(bt_dev.create_param.options & BT_CONN_LE_OPT_NO_1M)) {
		cp->phys |= BT_HCI_LE_EXT_SCAN_PHY_1M;
		phy = net_buf_add(buf, sizeof(*phy));
		phy->scan_interval = sys_cpu_to_le16(
			bt_dev.create_param.interval);
		phy->scan_window = sys_cpu_to_le16(
			bt_dev.create_param.window);
		set_phy_conn_param(conn, phy);
	}

	if (bt_dev.create_param.options & BT_CONN_LE_OPT_CODED) {
		cp->phys |= BT_HCI_LE_EXT_SCAN_PHY_CODED;
		phy = net_buf_add(buf, sizeof(*phy));
		phy->scan_interval = sys_cpu_to_le16(
			bt_dev.create_param.interval_coded);
		phy->scan_window = sys_cpu_to_le16(
			bt_dev.create_param.window_coded);
		set_phy_conn_param(conn, phy);
	}

	bt_hci_cmd_state_set_init(buf, &state, bt_dev.flags,
				  BT_DEV_INITIATING, true);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_EXT_CREATE_CONN, buf, NULL);
}

int bt_le_create_conn_synced(const struct bt_conn *conn, const struct bt_le_ext_adv *adv,
			     uint8_t subevent)
{
	struct bt_hci_cp_le_ext_create_conn_v2 *cp;
	struct bt_hci_ext_conn_phy *phy;
	struct bt_hci_cmd_state_set state;
	struct net_buf *buf;
	uint8_t own_addr_type;
	int err;

	err = bt_id_set_create_conn_own_addr(false, &own_addr_type);
	if (err) {
		return err;
	}

	/* There shall only be one Initiating_PHYs */
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_EXT_CREATE_CONN_V2, sizeof(*cp) + sizeof(*phy));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->subevent = subevent;
	cp->adv_handle = adv->handle;
	bt_addr_le_copy(&cp->peer_addr, &conn->le.dst);
	cp->filter_policy = BT_HCI_LE_CREATE_CONN_FP_NO_FILTER;
	cp->own_addr_type = own_addr_type;

	/* The Initiating_PHY is the secondary phy of the corresponding ext adv set */
	if (adv->options & BT_LE_ADV_OPT_CODED) {
		cp->phys = BT_HCI_LE_EXT_SCAN_PHY_CODED;
	} else if (adv->options & BT_LE_ADV_OPT_NO_2M) {
		cp->phys = BT_HCI_LE_EXT_SCAN_PHY_1M;
	} else {
		cp->phys = BT_HCI_LE_EXT_SCAN_PHY_2M;
	}

	phy = net_buf_add(buf, sizeof(*phy));
	(void)memset(phy, 0, sizeof(*phy));
	set_phy_conn_param(conn, phy);

	bt_hci_cmd_state_set_init(buf, &state, bt_dev.flags, BT_DEV_INITIATING, true);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_EXT_CREATE_CONN_V2, buf, NULL);
}

static int bt_le_create_conn_legacy(const struct bt_conn *conn)
{
	struct bt_hci_cp_le_create_conn *cp;
	struct bt_hci_cmd_state_set state;
	bool use_filter = false;
	struct net_buf *buf;
	uint8_t own_addr_type;
	int err;

	if (IS_ENABLED(CONFIG_BT_FILTER_ACCEPT_LIST)) {
		use_filter = atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT);
	}

	err = bt_id_set_create_conn_own_addr(use_filter, &own_addr_type);
	if (err) {
		return err;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_CONN, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memset(cp, 0, sizeof(*cp));
	cp->own_addr_type = own_addr_type;

	if (use_filter) {
		/* User Initiated procedure use fast scan parameters. */
		bt_addr_le_copy(&cp->peer_addr, BT_ADDR_LE_ANY);
		cp->filter_policy = BT_HCI_LE_CREATE_CONN_FP_FILTER;
	} else {
		const bt_addr_le_t *peer_addr = &conn->le.dst;

#if defined(CONFIG_BT_SMP)
		if (bt_dev.le.rl_entries > bt_dev.le.rl_size) {
			/* Host resolving is used, use the RPA directly. */
			peer_addr = &conn->le.resp_addr;
		}
#endif
		bt_addr_le_copy(&cp->peer_addr, peer_addr);
		cp->filter_policy = BT_HCI_LE_CREATE_CONN_FP_NO_FILTER;
	}

	cp->scan_interval = sys_cpu_to_le16(bt_dev.create_param.interval);
	cp->scan_window = sys_cpu_to_le16(bt_dev.create_param.window);

	cp->conn_interval_min = sys_cpu_to_le16(conn->le.interval_min);
	cp->conn_interval_max = sys_cpu_to_le16(conn->le.interval_max);
	cp->conn_latency = sys_cpu_to_le16(conn->le.latency);
	cp->supervision_timeout = sys_cpu_to_le16(conn->le.timeout);

	bt_hci_cmd_state_set_init(buf, &state, bt_dev.flags,
				  BT_DEV_INITIATING, true);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CONN, buf, NULL);
}

int bt_le_create_conn(const struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		return bt_le_create_conn_ext(conn);
	}

	return bt_le_create_conn_legacy(conn);
}

int bt_le_create_conn_cancel(void)
{
	struct net_buf *buf;
	struct bt_hci_cmd_state_set state;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CREATE_CONN_CANCEL, 0);

	bt_hci_cmd_state_set_init(buf, &state, bt_dev.flags,
				  BT_DEV_INITIATING, false);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_CREATE_CONN_CANCEL, buf, NULL);
}
#endif /* CONFIG_BT_CENTRAL */

int bt_hci_disconnect(uint16_t handle, uint8_t reason)
{
	struct net_buf *buf;
	struct bt_hci_cp_disconnect *disconn;

	buf = bt_hci_cmd_create(BT_HCI_OP_DISCONNECT, sizeof(*disconn));
	if (!buf) {
		return -ENOBUFS;
	}

	disconn = net_buf_add(buf, sizeof(*disconn));
	disconn->handle = sys_cpu_to_le16(handle);
	disconn->reason = reason;

	return bt_hci_cmd_send_sync(BT_HCI_OP_DISCONNECT, buf, NULL);
}

static uint16_t disconnected_handles[CONFIG_BT_MAX_CONN];
static uint8_t disconnected_handles_reason[CONFIG_BT_MAX_CONN];

static void disconnected_handles_reset(void)
{
	(void)memset(disconnected_handles, 0, sizeof(disconnected_handles));
}

static void conn_handle_disconnected(uint16_t handle, uint8_t disconnect_reason)
{
	for (int i = 0; i < ARRAY_SIZE(disconnected_handles); i++) {
		if (!disconnected_handles[i]) {
			/* Use invalid connection handle bits so that connection
			 * handle 0 can be used as a valid non-zero handle.
			 */
			disconnected_handles[i] = ~BT_ACL_HANDLE_MASK | handle;
			disconnected_handles_reason[i] = disconnect_reason;
		}
	}
}

/** @returns the disconnect reason. */
static uint8_t conn_handle_is_disconnected(uint16_t handle)
{
	handle |= ~BT_ACL_HANDLE_MASK;

	for (int i = 0; i < ARRAY_SIZE(disconnected_handles); i++) {
		if (disconnected_handles[i] == handle) {
			disconnected_handles[i] = 0;
			return disconnected_handles_reason[i];
		}
	}

	return 0;
}

static void hci_disconn_complete_prio(struct net_buf *buf)
{
	struct bt_hci_evt_disconn_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	LOG_DBG("status 0x%02x handle %u reason 0x%02x", evt->status, handle, evt->reason);

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_ALL);
	if (!conn) {
		/* Priority disconnect complete event received before normal
		 * connection complete event.
		 */
		conn_handle_disconnected(handle, evt->reason);
		return;
	}

	conn->err = evt->reason;

	bt_conn_set_state(conn, BT_CONN_DISCONNECT_COMPLETE);
	bt_conn_unref(conn);
}

static void hci_disconn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_disconn_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	LOG_DBG("status 0x%02x handle %u reason 0x%02x", evt->status, handle, evt->reason);

	if (evt->status) {
		return;
	}

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_ALL);
	if (!conn) {
		LOG_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	bt_conn_set_state(conn, BT_CONN_DISCONNECTED);

	if (conn->type != BT_CONN_TYPE_LE) {
#if defined(CONFIG_BT_CLASSIC)
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

#if defined(CONFIG_BT_CENTRAL) && !defined(CONFIG_BT_FILTER_ACCEPT_LIST)
	if (atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT)) {
		bt_conn_set_state(conn, BT_CONN_SCAN_BEFORE_INITIATING);
		bt_le_scan_update(false);
	}
#endif /* defined(CONFIG_BT_CENTRAL) && !defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

	bt_conn_unref(conn);
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
	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_REMOTE_FEATURES, buf, NULL);
}

static int hci_read_remote_version(struct bt_conn *conn)
{
	struct bt_hci_cp_read_remote_version_info *cp;
	struct net_buf *buf;

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	/* Remote version cannot change. */
	if (atomic_test_bit(conn->flags, BT_CONN_AUTO_VERSION_INFO)) {
		return 0;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_REMOTE_VERSION_INFO,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);

	return bt_hci_cmd_send_sync(BT_HCI_OP_READ_REMOTE_VERSION_INFO, buf,
				    NULL);
}

/* LE Data Length Change Event is optional so this function just ignore
 * error and stack will continue to use default values.
 */
int bt_le_set_data_len(struct bt_conn *conn, uint16_t tx_octets, uint16_t tx_time)
{
	struct bt_hci_cp_le_set_data_len *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_DATA_LEN, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->tx_octets = sys_cpu_to_le16(tx_octets);
	cp->tx_time = sys_cpu_to_le16(tx_time);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_DATA_LEN, buf, NULL);
}

#if defined(CONFIG_BT_USER_PHY_UPDATE)
static int hci_le_read_phy(struct bt_conn *conn)
{
	struct bt_hci_cp_le_read_phy *cp;
	struct bt_hci_rp_le_read_phy *rp;
	struct net_buf *buf, *rsp;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_READ_PHY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_PHY, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	conn->le.phy.tx_phy = bt_get_phy(rp->tx_phy);
	conn->le.phy.rx_phy = bt_get_phy(rp->rx_phy);
	net_buf_unref(rsp);

	return 0;
}
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

int bt_le_set_phy(struct bt_conn *conn, uint8_t all_phys,
		  uint8_t pref_tx_phy, uint8_t pref_rx_phy, uint8_t phy_opts)
{
	struct bt_hci_cp_le_set_phy *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PHY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->all_phys = all_phys;
	cp->tx_phys = pref_tx_phy;
	cp->rx_phys = pref_rx_phy;
	cp->phy_opts = phy_opts;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_PHY, buf, NULL);
}

static struct bt_conn *find_pending_connect(uint8_t role, bt_addr_le_t *peer_addr)
{
	struct bt_conn *conn;

	/*
	 * Make lookup to check if there's a connection object in
	 * CONNECT or CONNECT_AUTO state associated with passed peer LE address.
	 */
	if (IS_ENABLED(CONFIG_BT_CENTRAL) && role == BT_HCI_ROLE_CENTRAL) {
		conn = bt_conn_lookup_state_le(BT_ID_DEFAULT, peer_addr,
					       BT_CONN_INITIATING);
		if (IS_ENABLED(CONFIG_BT_FILTER_ACCEPT_LIST) && !conn) {
			conn = bt_conn_lookup_state_le(BT_ID_DEFAULT,
						       BT_ADDR_LE_NONE,
						       BT_CONN_INITIATING_FILTER_LIST);
		}

		return conn;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && role == BT_HCI_ROLE_PERIPHERAL) {
		conn = bt_conn_lookup_state_le(bt_dev.adv_conn_id, peer_addr,
					       BT_CONN_ADV_DIR_CONNECTABLE);
		if (!conn) {
			conn = bt_conn_lookup_state_le(bt_dev.adv_conn_id,
						       BT_ADDR_LE_NONE,
						       BT_CONN_ADV_CONNECTABLE);
		}

		return conn;
	}

	return NULL;
}

/* We don't want the application to get a PHY update callback upon connection
 * establishment on 2M PHY. Therefore we must prevent issuing LE Set PHY
 * in this scenario.
 */
static bool skip_auto_phy_update_on_conn_establishment(struct bt_conn *conn)
{
#if defined(CONFIG_BT_USER_PHY_UPDATE)
	if (IS_ENABLED(CONFIG_BT_AUTO_PHY_UPDATE) &&
	    IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		if (conn->le.phy.tx_phy == BT_HCI_LE_PHY_2M &&
		    conn->le.phy.rx_phy == BT_HCI_LE_PHY_2M) {
			return true;
		}
	}
#else
	ARG_UNUSED(conn);
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

	return false;
}

static void conn_auto_initiate(struct bt_conn *conn)
{
	int err;

	if (conn->state != BT_CONN_CONNECTED) {
		/* It is possible that connection was disconnected directly from
		 * connected callback so we must check state before doing
		 * connection parameters update.
		 */
		return;
	}

	if (!atomic_test_bit(conn->flags, BT_CONN_AUTO_FEATURE_EXCH) &&
	    ((conn->role == BT_HCI_ROLE_CENTRAL) ||
	     BT_FEAT_LE_PER_INIT_FEAT_XCHG(bt_dev.le.features))) {
		err = hci_le_read_remote_features(conn);
		if (err) {
			LOG_ERR("Failed read remote features (%d)", err);
		}
	}

	if (IS_ENABLED(CONFIG_BT_REMOTE_VERSION) &&
	    !atomic_test_bit(conn->flags, BT_CONN_AUTO_VERSION_INFO)) {
		err = hci_read_remote_version(conn);
		if (err) {
			LOG_ERR("Failed read remote version (%d)", err);
		}
	}

	if (IS_ENABLED(CONFIG_BT_AUTO_PHY_UPDATE) &&
	    BT_FEAT_LE_PHY_2M(bt_dev.le.features) &&
	    !skip_auto_phy_update_on_conn_establishment(conn)) {
		err = bt_le_set_phy(conn, 0U, BT_HCI_LE_PHY_PREFER_2M,
				    BT_HCI_LE_PHY_PREFER_2M,
				    BT_HCI_LE_PHY_CODED_ANY);
		if (err) {
			LOG_ERR("Failed LE Set PHY (%d)", err);
		}
	}

	if (IS_ENABLED(CONFIG_BT_AUTO_DATA_LEN_UPDATE) &&
	    BT_FEAT_LE_DLE(bt_dev.le.features)) {
		if (drv_quirk_no_auto_dle()) {
			uint16_t tx_octets, tx_time;

			err = hci_le_read_max_data_len(&tx_octets, &tx_time);
			if (!err) {
				err = bt_le_set_data_len(conn,
						tx_octets, tx_time);
				if (err) {
					LOG_ERR("Failed to set data len (%d)", err);
				}
			}
		} else {
			/* No need to auto-initiate DLE procedure.
			 * It is done by the controller.
			 */
		}
	}
}

static void le_conn_complete_cancel(uint8_t err)
{
	int ret;
	struct bt_conn *conn;

	/* Handle create connection cancel.
	 *
	 * There is no need to check ID address as only one
	 * connection in central role can be in pending state.
	 */
	conn = find_pending_connect(BT_HCI_ROLE_CENTRAL, NULL);
	if (!conn) {
		LOG_ERR("No pending central connection");
		return;
	}

	if (atomic_test_bit(conn->flags, BT_CONN_AUTO_CONNECT)) {
		if (!IS_ENABLED(CONFIG_BT_FILTER_ACCEPT_LIST)) {
			/* Restart passive scanner for device */
			bt_conn_set_state(conn, BT_CONN_SCAN_BEFORE_INITIATING);
		} else {
			/* Restart FAL initiator after RPA timeout. */
			ret = bt_le_create_conn(conn);
			if (ret) {
				LOG_ERR("Failed to restart initiator");
			}
		}
	} else {
		int busy_status = k_work_delayable_busy_get(&conn->deferred_work);

		if (!(busy_status & (K_WORK_QUEUED | K_WORK_DELAYED))) {
			LOG_WRN("Connection creation timeout triggered");
			conn->err = err;
			bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		} else {
			/* Restart initiator after RPA timeout. */
			ret = bt_le_create_conn(conn);
			if (ret) {
				LOG_ERR("Failed to restart initiator");
			}
		}
	}

	bt_conn_unref(conn);
}

static void le_conn_complete_adv_timeout(void)
{
	if (!(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	      BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
		struct bt_le_ext_adv *adv = bt_le_adv_lookup_legacy();
		struct bt_conn *conn;

		/* Handle advertising timeout after high duty cycle directed
		 * advertising.
		 */

		atomic_clear_bit(adv->flags, BT_ADV_ENABLED);

		if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
		    !BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
			/* No advertising set terminated event, must be a
			 * legacy advertiser set.
			 */
			bt_le_adv_delete_legacy();
		}

		/* There is no need to check ID address as only one
		 * connection in peripheral role can be in pending state.
		 */
		conn = find_pending_connect(BT_HCI_ROLE_PERIPHERAL, NULL);
		if (!conn) {
			LOG_ERR("No pending peripheral connection");
			return;
		}

		conn->err = BT_HCI_ERR_ADV_TIMEOUT;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);

		bt_conn_unref(conn);
	}
}

static void enh_conn_complete(struct bt_hci_evt_le_enh_conn_complete *evt)
{
#if defined(CONFIG_BT_CONN) && (CONFIG_BT_EXT_ADV_MAX_ADV_SET > 1)
	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
		evt->role == BT_HCI_ROLE_PERIPHERAL &&
		evt->status == BT_HCI_ERR_SUCCESS &&
		(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
				BT_FEAT_LE_EXT_ADV(bt_dev.le.features))) {

		/* Cache the connection complete event. Process it later.
		 * See bt_dev.cached_conn_complete.
		 */
		for (int i = 0; i < ARRAY_SIZE(bt_dev.cached_conn_complete); i++) {
			if (!bt_dev.cached_conn_complete[i].valid) {
				(void)memcpy(&bt_dev.cached_conn_complete[i].evt,
					evt,
					sizeof(struct bt_hci_evt_le_enh_conn_complete));
				bt_dev.cached_conn_complete[i].valid = true;
				return;
			}
		}

		__ASSERT(false, "No more cache entries available."
				"This should not happen by design");

		return;
	}
#endif
	bt_hci_le_enh_conn_complete(evt);
}

static void translate_addrs(bt_addr_le_t *peer_addr, bt_addr_le_t *id_addr,
			    const struct bt_hci_evt_le_enh_conn_complete *evt, uint8_t id)
{
	if (bt_addr_le_is_resolved(&evt->peer_addr)) {
		bt_addr_le_copy_resolved(id_addr, &evt->peer_addr);

		bt_addr_copy(&peer_addr->a, &evt->peer_rpa);
		peer_addr->type = BT_ADDR_LE_RANDOM;
	} else {
		bt_addr_le_copy(id_addr, bt_lookup_id_addr(id, &evt->peer_addr));
		bt_addr_le_copy(peer_addr, &evt->peer_addr);
	}
}

static void update_conn(struct bt_conn *conn, const bt_addr_le_t *id_addr,
			const struct bt_hci_evt_le_enh_conn_complete *evt)
{
	conn->handle = sys_le16_to_cpu(evt->handle);
	bt_addr_le_copy(&conn->le.dst, id_addr);
	conn->le.interval = sys_le16_to_cpu(evt->interval);
	conn->le.latency = sys_le16_to_cpu(evt->latency);
	conn->le.timeout = sys_le16_to_cpu(evt->supv_timeout);
	conn->role = evt->role;
	conn->err = 0U;

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	conn->le.data_len.tx_max_len = BT_GAP_DATA_LEN_DEFAULT;
	conn->le.data_len.tx_max_time = BT_GAP_DATA_TIME_DEFAULT;
	conn->le.data_len.rx_max_len = BT_GAP_DATA_LEN_DEFAULT;
	conn->le.data_len.rx_max_time = BT_GAP_DATA_TIME_DEFAULT;
#endif
}

void bt_hci_le_enh_conn_complete(struct bt_hci_evt_le_enh_conn_complete *evt)
{
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	uint8_t disconnect_reason = conn_handle_is_disconnected(handle);
	bt_addr_le_t peer_addr, id_addr;
	struct bt_conn *conn;
	uint8_t id;

	LOG_DBG("status 0x%02x handle %u role %u peer %s peer RPA %s", evt->status, handle,
		evt->role, bt_addr_le_str(&evt->peer_addr), bt_addr_str(&evt->peer_rpa));
	LOG_DBG("local RPA %s", bt_addr_str(&evt->local_rpa));

#if defined(CONFIG_BT_SMP)
	bt_id_pending_keys_update();
#endif

	if (evt->status) {
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
		    evt->status == BT_HCI_ERR_ADV_TIMEOUT) {
			le_conn_complete_adv_timeout();
			return;
		}

		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    evt->status == BT_HCI_ERR_UNKNOWN_CONN_ID) {
			le_conn_complete_cancel(evt->status);
			bt_le_scan_update(false);
			return;
		}

		if (IS_ENABLED(CONFIG_BT_CENTRAL) && IS_ENABLED(CONFIG_BT_PER_ADV_RSP) &&
		    evt->status == BT_HCI_ERR_CONN_FAIL_TO_ESTAB) {
			le_conn_complete_cancel(evt->status);

			atomic_clear_bit(bt_dev.flags, BT_DEV_INITIATING);

			return;
		}

		LOG_WRN("Unexpected status 0x%02x", evt->status);

		return;
	}

	id = evt->role == BT_HCI_ROLE_PERIPHERAL ? bt_dev.adv_conn_id : BT_ID_DEFAULT;
	translate_addrs(&peer_addr, &id_addr, evt, id);

	conn = find_pending_connect(evt->role, &id_addr);

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    evt->role == BT_HCI_ROLE_PERIPHERAL &&
	    !(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	      BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
		struct bt_le_ext_adv *adv = bt_le_adv_lookup_legacy();
		/* Clear advertising even if we are not able to add connection
		 * object to keep host in sync with controller state.
		 */
		atomic_clear_bit(adv->flags, BT_ADV_ENABLED);
		(void)bt_le_lim_adv_cancel_timeout(adv);
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    evt->role == BT_HCI_ROLE_CENTRAL) {
		/* Clear initiating even if we are not able to add connection
		 * object to keep the host in sync with controller state.
		 */
		atomic_clear_bit(bt_dev.flags, BT_DEV_INITIATING);
	}

	if (!conn) {
		LOG_ERR("No pending conn for peer %s", bt_addr_le_str(&evt->peer_addr));
		bt_hci_disconnect(handle, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	update_conn(conn, &id_addr, evt);

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	conn->le.phy.tx_phy = BT_GAP_LE_PHY_1M;
	conn->le.phy.rx_phy = BT_GAP_LE_PHY_1M;
#endif
	/*
	 * Use connection address (instead of identity address) as initiator
	 * or responder address. Only peripheral needs to be updated. For central all
	 * was set during outgoing connection creation.
	 */
	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    conn->role == BT_HCI_ROLE_PERIPHERAL) {
		bt_addr_le_copy(&conn->le.init_addr, &peer_addr);

		if (!(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
		      BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
			struct bt_le_ext_adv *adv = bt_le_adv_lookup_legacy();

			if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
			    !atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
				conn->le.resp_addr.type = BT_ADDR_LE_RANDOM;
				if (!bt_addr_eq(&evt->local_rpa, BT_ADDR_ANY)) {
					bt_addr_copy(&conn->le.resp_addr.a,
						     &evt->local_rpa);
				} else {
					bt_addr_copy(&conn->le.resp_addr.a,
						     &bt_dev.random_addr.a);
				}
			} else {
				bt_addr_le_copy(&conn->le.resp_addr,
						&bt_dev.id_addr[conn->id]);
			}
		} else {
			/* Copy the local RPA and handle this in advertising set
			 * terminated event.
			 */
			bt_addr_copy(&conn->le.resp_addr.a, &evt->local_rpa);
		}

		/* if the controller supports, lets advertise for another
		 * peripheral connection.
		 * check for connectable advertising state is sufficient as
		 * this is how this le connection complete for peripheral occurred.
		 */
		if (BT_LE_STATES_PER_CONN_ADV(bt_dev.le.states)) {
			bt_le_adv_resume();
		}

		if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
		    !BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
			struct bt_le_ext_adv *adv = bt_le_adv_lookup_legacy();
			/* No advertising set terminated event, must be a
			 * legacy advertiser set.
			 */
			if (!atomic_test_bit(adv->flags, BT_ADV_PERSIST)) {
				bt_le_adv_delete_legacy();
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL) {
		bt_addr_le_copy(&conn->le.resp_addr, &peer_addr);

		if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
			conn->le.init_addr.type = BT_ADDR_LE_RANDOM;
			if (!bt_addr_eq(&evt->local_rpa, BT_ADDR_ANY)) {
				bt_addr_copy(&conn->le.init_addr.a,
					     &evt->local_rpa);
			} else {
				bt_addr_copy(&conn->le.init_addr.a,
					     &bt_dev.random_addr.a);
			}
		} else {
			bt_addr_le_copy(&conn->le.init_addr,
					&bt_dev.id_addr[conn->id]);
		}
	}

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		int err;

		err = hci_le_read_phy(conn);
		if (err) {
			LOG_WRN("Failed to read PHY (%d)", err);
		}
	}
#endif /* defined(CONFIG_BT_USER_PHY_UPDATE) */

	bt_conn_set_state(conn, BT_CONN_CONNECTED);

	if (disconnect_reason) {
		/* Mark the connection as already disconnected before calling
		 * the connected callback, so that the application cannot
		 * start sending packets
		 */
		conn->err = disconnect_reason;
		bt_conn_set_state(conn, BT_CONN_DISCONNECT_COMPLETE);
	}

	bt_conn_connected(conn);

	/* Start auto-initiated procedures */
	conn_auto_initiate(conn);

	bt_conn_unref(conn);

	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    conn->role == BT_HCI_ROLE_CENTRAL) {
		bt_le_scan_update(false);
	}
}

#if defined(CONFIG_BT_PER_ADV_SYNC_RSP)
void bt_hci_le_enh_conn_complete_sync(struct bt_hci_evt_le_enh_conn_complete_v2 *evt,
				      struct bt_le_per_adv_sync *sync)
{
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	uint8_t disconnect_reason = conn_handle_is_disconnected(handle);
	bt_addr_le_t peer_addr, id_addr;
	struct bt_conn *conn;

	if (!sync->num_subevents) {
		LOG_ERR("Unexpected connection complete event");

		return;
	}

	conn = bt_conn_add_le(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (!conn) {
		LOG_ERR("Unable to allocate connection");
		/* Tell the controller to disconnect to keep it in sync with
		 * the host state and avoid a "rogue" connection.
		 */
		bt_hci_disconnect(handle, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

		return;
	}

	LOG_DBG("status 0x%02x handle %u role %u peer %s peer RPA %s", evt->status, handle,
		evt->role, bt_addr_le_str(&evt->peer_addr), bt_addr_str(&evt->peer_rpa));
	LOG_DBG("local RPA %s", bt_addr_str(&evt->local_rpa));

	if (evt->role != BT_HCI_ROLE_PERIPHERAL) {
		LOG_ERR("PAwR sync always becomes peripheral");

		return;
	}

#if defined(CONFIG_BT_SMP)
	bt_id_pending_keys_update();
#endif

	if (evt->status) {
		LOG_ERR("Unexpected status 0x%02x", evt->status);

		return;
	}

	translate_addrs(&peer_addr, &id_addr, (const struct bt_hci_evt_le_enh_conn_complete *)evt,
			BT_ID_DEFAULT);
	update_conn(conn, &id_addr, (const struct bt_hci_evt_le_enh_conn_complete *)evt);

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	/* The connection is always initiated on the same phy as the PAwR advertiser */
	conn->le.phy.tx_phy = sync->phy;
	conn->le.phy.rx_phy = sync->phy;
#endif

	bt_addr_le_copy(&conn->le.init_addr, &peer_addr);

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		conn->le.resp_addr.type = BT_ADDR_LE_RANDOM;
		bt_addr_copy(&conn->le.resp_addr.a, &evt->local_rpa);
	} else {
		bt_addr_le_copy(&conn->le.resp_addr, &bt_dev.id_addr[conn->id]);
	}

	bt_conn_set_state(conn, BT_CONN_CONNECTED);

	if (disconnect_reason) {
		/* Mark the connection as already disconnected before calling
		 * the connected callback, so that the application cannot
		 * start sending packets
		 */
		conn->err = disconnect_reason;
		bt_conn_set_state(conn, BT_CONN_DISCONNECT_COMPLETE);
	}

	bt_conn_connected(conn);

	/* Since we don't give the application a reference to manage
	 * for peripheral connections, we need to release this reference here.
	 */
	bt_conn_unref(conn);

	/* Start auto-initiated procedures */
	conn_auto_initiate(conn);
}
#endif /* CONFIG_BT_PER_ADV_SYNC_RSP */

static void le_enh_conn_complete(struct net_buf *buf)
{
	enh_conn_complete((void *)buf->data);
}

#if defined(CONFIG_BT_PER_ADV_RSP) || defined(CONFIG_BT_PER_ADV_SYNC_RSP)
static void le_enh_conn_complete_v2(struct net_buf *buf)
{
	struct bt_hci_evt_le_enh_conn_complete_v2 *evt =
		(struct bt_hci_evt_le_enh_conn_complete_v2 *)buf->data;

	if (evt->adv_handle == BT_HCI_ADV_HANDLE_INVALID &&
	    evt->sync_handle == BT_HCI_SYNC_HANDLE_INVALID) {
		/* The connection was not created via PAwR, handle the event like v1 */
		enh_conn_complete((struct bt_hci_evt_le_enh_conn_complete *)evt);
	}
#if defined(CONFIG_BT_PER_ADV_RSP)
	else if (evt->adv_handle != BT_HCI_ADV_HANDLE_INVALID &&
		 evt->sync_handle == BT_HCI_SYNC_HANDLE_INVALID) {
		/* The connection was created via PAwR advertiser, it can be handled like v1 */
		enh_conn_complete((struct bt_hci_evt_le_enh_conn_complete *)evt);
	}
#endif /* CONFIG_BT_PER_ADV_RSP */
#if defined(CONFIG_BT_PER_ADV_SYNC_RSP)
	else if (evt->adv_handle == BT_HCI_ADV_HANDLE_INVALID &&
		 evt->sync_handle != BT_HCI_SYNC_HANDLE_INVALID) {
		/* Created via PAwR sync, no adv set terminated event, needs separate handling */
		struct bt_le_per_adv_sync *sync;

		sync = bt_hci_get_per_adv_sync(evt->sync_handle);
		if (!sync) {
			LOG_ERR("Unknown sync handle %d", evt->sync_handle);

			return;
		}

		bt_hci_le_enh_conn_complete_sync(evt, sync);
	}
#endif /* CONFIG_BT_PER_ADV_SYNC_RSP */
	else {
		LOG_ERR("Invalid connection complete event");
	}
}
#endif /* CONFIG_BT_PER_ADV_RSP || CONFIG_BT_PER_ADV_SYNC_RSP */

static void le_legacy_conn_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *evt = (void *)buf->data;
	struct bt_hci_evt_le_enh_conn_complete enh;

	LOG_DBG("status 0x%02x role %u %s", evt->status, evt->role,
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

	bt_addr_copy(&enh.peer_rpa, BT_ADDR_ANY);

	enh_conn_complete(&enh);
}

static void le_remote_feat_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_remote_feat_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	if (!evt->status) {
		memcpy(conn->le.features, evt->features,
		       sizeof(conn->le.features));
	}

	atomic_set_bit(conn->flags, BT_CONN_AUTO_FEATURE_EXCH);

	if (IS_ENABLED(CONFIG_BT_REMOTE_INFO) &&
	    !IS_ENABLED(CONFIG_BT_REMOTE_VERSION)) {
		notify_remote_info(conn);
	}

	bt_conn_unref(conn);
}

#if defined(CONFIG_BT_DATA_LEN_UPDATE)
static void le_data_len_change(struct net_buf *buf)
{
	struct bt_hci_evt_le_data_len_change *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	uint16_t max_tx_octets = sys_le16_to_cpu(evt->max_tx_octets);
	uint16_t max_rx_octets = sys_le16_to_cpu(evt->max_rx_octets);
	uint16_t max_tx_time = sys_le16_to_cpu(evt->max_tx_time);
	uint16_t max_rx_time = sys_le16_to_cpu(evt->max_rx_time);

	if (!IN_RANGE(max_tx_octets, BT_HCI_LE_MAX_TX_OCTETS_MIN, BT_HCI_LE_MAX_TX_OCTETS_MAX)) {
		LOG_WRN("max_tx_octets exceeds the valid range %u", max_tx_octets);
	}
	if (!IN_RANGE(max_rx_octets, BT_HCI_LE_MAX_RX_OCTETS_MIN, BT_HCI_LE_MAX_RX_OCTETS_MAX)) {
		LOG_WRN("max_rx_octets exceeds the valid range %u", max_rx_octets);
	}
	if (!IN_RANGE(max_tx_time, BT_HCI_LE_MAX_TX_TIME_MIN, BT_HCI_LE_MAX_TX_TIME_MAX)) {
		LOG_WRN("max_tx_time exceeds the valid range %u", max_tx_time);
	}
	if (!IN_RANGE(max_rx_time, BT_HCI_LE_MAX_RX_TIME_MIN, BT_HCI_LE_MAX_RX_TIME_MAX)) {
		LOG_WRN("max_rx_time exceeds the valid range %u", max_rx_time);
	}

	LOG_DBG("max. tx: %u (%uus), max. rx: %u (%uus)", max_tx_octets, max_tx_time, max_rx_octets,
		max_rx_time);

	conn->le.data_len.tx_max_len = max_tx_octets;
	conn->le.data_len.tx_max_time = max_tx_time;
	conn->le.data_len.rx_max_len = max_rx_octets;
	conn->le.data_len.rx_max_time = max_rx_time;
	notify_le_data_len_updated(conn);
#endif

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_DATA_LEN_UPDATE */

#if defined(CONFIG_BT_PHY_UPDATE)
static void le_phy_update_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_phy_update_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	LOG_DBG("PHY updated: status: 0x%02x, tx: %u, rx: %u", evt->status, evt->tx_phy,
		evt->rx_phy);

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	conn->le.phy.tx_phy = bt_get_phy(evt->tx_phy);
	conn->le.phy.rx_phy = bt_get_phy(evt->rx_phy);
	notify_le_phy_updated(conn);
#endif

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_PHY_UPDATE */

bool bt_le_conn_params_valid(const struct bt_le_conn_param *param)
{
	if (IS_ENABLED(CONFIG_BT_CONN_PARAM_ANY)) {
		return true;
	}

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
	     ((1U + param->latency) * param->interval_max))) {
		return false;
	}

	return true;
}

static void le_conn_param_neg_reply(uint16_t handle, uint8_t reason)
{
	struct bt_hci_cp_le_conn_param_req_neg_reply *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY,
				sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate buffer");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->reason = sys_cpu_to_le16(reason);

	bt_hci_cmd_send(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY, buf);
}

static int le_conn_param_req_reply(uint16_t handle,
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
	uint16_t handle;

	handle = sys_le16_to_cpu(evt->handle);
	param.interval_min = sys_le16_to_cpu(evt->interval_min);
	param.interval_max = sys_le16_to_cpu(evt->interval_max);
	param.latency = sys_le16_to_cpu(evt->latency);
	param.timeout = sys_le16_to_cpu(evt->timeout);

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Unable to lookup conn for handle %u", handle);
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
	uint16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	LOG_DBG("status 0x%02x, handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Unable to lookup conn for handle %u", handle);
		return;
	}

	if (evt->status == BT_HCI_ERR_UNSUPP_REMOTE_FEATURE &&
	    conn->role == BT_HCI_ROLE_PERIPHERAL &&
	    !atomic_test_and_set_bit(conn->flags,
				     BT_CONN_PERIPHERAL_PARAM_L2CAP)) {
		/* CPR not supported, let's try L2CAP CPUP instead */
		struct bt_le_conn_param param;

		param.interval_min = conn->le.interval_min;
		param.interval_max = conn->le.interval_max;
		param.latency = conn->le.pending_latency;
		param.timeout = conn->le.pending_timeout;

		bt_l2cap_update_conn_param(conn, &param);
	} else {
		if (!evt->status) {
			conn->le.interval = sys_le16_to_cpu(evt->interval);
			conn->le.latency = sys_le16_to_cpu(evt->latency);
			conn->le.timeout = sys_le16_to_cpu(evt->supv_timeout);

#if defined(CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS)
			atomic_clear_bit(conn->flags,
					 BT_CONN_PERIPHERAL_PARAM_AUTO_UPDATE);
		} else if (atomic_test_bit(conn->flags,
					   BT_CONN_PERIPHERAL_PARAM_AUTO_UPDATE) &&
			   evt->status == BT_HCI_ERR_UNSUPP_LL_PARAM_VAL &&
			   conn->le.conn_param_retry_countdown) {
			conn->le.conn_param_retry_countdown--;
			k_work_schedule(&conn->deferred_work,
					K_MSEC(CONFIG_BT_CONN_PARAM_RETRY_TIMEOUT));
		} else {
			atomic_clear_bit(conn->flags,
					 BT_CONN_PERIPHERAL_PARAM_AUTO_UPDATE);
#endif /* CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS */

		}

		notify_le_param_updated(conn);
	}

	bt_conn_unref(conn);
}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static int set_flow_control(void)
{
	struct bt_hci_cp_host_buffer_size *hbs;
	struct net_buf *buf;
	int err;

	/* Check if host flow control is actually supported */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 10, 5)) {
		LOG_WRN("Controller to host flow control not supported");
		return 0;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_HOST_BUFFER_SIZE,
				sizeof(*hbs));
	if (!buf) {
		return -ENOBUFS;
	}

	hbs = net_buf_add(buf, sizeof(*hbs));
	(void)memset(hbs, 0, sizeof(*hbs));
	hbs->acl_mtu = sys_cpu_to_le16(CONFIG_BT_BUF_ACL_RX_SIZE);
	hbs->acl_pkts = sys_cpu_to_le16(CONFIG_BT_BUF_ACL_RX_COUNT);

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

static void unpair(uint8_t id, const bt_addr_le_t *addr)
{
	struct bt_keys *keys = NULL;
	struct bt_conn *conn = bt_conn_lookup_addr_le(id, addr);

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

	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
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

	bt_gatt_clear(id, addr);

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
	struct bt_conn_auth_info_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bt_auth_info_cbs, listener,
					  next, node) {
		if (listener->bond_deleted) {
			listener->bond_deleted(id, addr);
		}
	}
#endif /* defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC) */
}

static void unpair_remote(const struct bt_bond_info *info, void *data)
{
	uint8_t *id = (uint8_t *) data;

	unpair(*id, &info->addr);
}

int bt_unpair(uint8_t id, const bt_addr_le_t *addr)
{
	if (id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		if (!addr || bt_addr_le_eq(addr, BT_ADDR_LE_ANY)) {
			bt_foreach_bond(id, unpair_remote, &id);
		} else {
			unpair(id, addr);
		}
	} else {
		CHECKIF(addr == NULL) {
			LOG_DBG("addr is NULL");
			return -EINVAL;
		}

		unpair(id, addr);
	}

	return 0;
}

#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
enum bt_security_err bt_security_err_get(uint8_t hci_err)
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
#endif /* defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC) */

#if defined(CONFIG_BT_SMP)
static bool update_sec_level(struct bt_conn *conn)
{
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

	return !(conn->required_sec_level > conn->sec_level);
}
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
static void hci_encrypt_change(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_change *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	uint8_t status = evt->status;
	struct bt_conn *conn;

	LOG_DBG("status 0x%02x handle %u encrypt 0x%02x", evt->status, handle, evt->encrypt);

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_ALL);
	if (!conn) {
		LOG_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	if (status) {
		bt_conn_security_changed(conn, status,
					 bt_security_err_get(status));
		bt_conn_unref(conn);
		return;
	}

	if (conn->encrypt == evt->encrypt) {
		LOG_WRN("No change to encryption state (encrypt 0x%02x)", evt->encrypt);
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

		if (!update_sec_level(conn)) {
			status = BT_HCI_ERR_AUTH_FAIL;
		}
	}
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_CLASSIC)
	if (conn->type == BT_CONN_TYPE_BR) {
		if (!bt_br_update_sec_level(conn)) {
			bt_conn_unref(conn);
			return;
		}

		if (IS_ENABLED(CONFIG_BT_SMP)) {
			/*
			 * Start SMP over BR/EDR if we are pairing and are
			 * central on the link
			 */
			if (atomic_test_bit(conn->flags, BT_CONN_BR_PAIRING) &&
			    conn->role == BT_CONN_ROLE_CENTRAL) {
				bt_smp_br_send_pairing_req(conn);
			}
		}
	}
#endif /* CONFIG_BT_CLASSIC */

	bt_conn_security_changed(conn, status, bt_security_err_get(status));

	if (status) {
		LOG_ERR("Failed to set required security level");
		bt_conn_disconnect(conn, status);
	}

	bt_conn_unref(conn);
}

static void hci_encrypt_key_refresh_complete(struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *evt = (void *)buf->data;
	uint8_t status = evt->status;
	struct bt_conn *conn;
	uint16_t handle;

	handle = sys_le16_to_cpu(evt->handle);

	LOG_DBG("status 0x%02x handle %u", evt->status, handle);

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_ALL);
	if (!conn) {
		LOG_ERR("Unable to look up conn with handle %u", handle);
		return;
	}

	if (status) {
		bt_conn_security_changed(conn, status,
					 bt_security_err_get(status));
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

		if (!update_sec_level(conn)) {
			status = BT_HCI_ERR_AUTH_FAIL;
		}
	}
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_CLASSIC)
	if (conn->type == BT_CONN_TYPE_BR) {
		if (!bt_br_update_sec_level(conn)) {
			bt_conn_unref(conn);
			return;
		}
	}
#endif /* CONFIG_BT_CLASSIC */

	bt_conn_security_changed(conn, status, bt_security_err_get(status));
	if (status) {
		LOG_ERR("Failed to set required security level");
		bt_conn_disconnect(conn, status);
	}

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC */

#if defined(CONFIG_BT_REMOTE_VERSION)
static void bt_hci_evt_read_remote_version_complete(struct net_buf *buf)
{
	struct bt_hci_evt_remote_version_info *evt;
	struct bt_conn *conn;
	uint16_t handle;

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	handle = sys_le16_to_cpu(evt->handle);
	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_ALL);
	if (!conn) {
		LOG_ERR("No connection for handle %u", handle);
		return;
	}

	if (!evt->status) {
		conn->rv.version = evt->version;
		conn->rv.manufacturer = sys_le16_to_cpu(evt->manufacturer);
		conn->rv.subversion = sys_le16_to_cpu(evt->subversion);
	}

	atomic_set_bit(conn->flags, BT_CONN_AUTO_VERSION_INFO);

	if (IS_ENABLED(CONFIG_BT_REMOTE_INFO)) {
		/* Remote features is already present */
		notify_remote_info(conn);
	}

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_REMOTE_VERSION */

static void hci_hardware_error(struct net_buf *buf)
{
	struct bt_hci_evt_hardware_error *evt;

	evt = net_buf_pull_mem(buf, sizeof(*evt));

	LOG_ERR("Hardware error, hardware code: %d", evt->hardware_code);
}

#if defined(CONFIG_BT_SMP)
static void le_ltk_neg_reply(uint16_t handle)
{
	struct bt_hci_cp_le_ltk_req_neg_reply *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Out of command buffers");

		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);

	bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY, buf);
}

static void le_ltk_reply(uint16_t handle, uint8_t *ltk)
{
	struct bt_hci_cp_le_ltk_req_reply *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_REPLY,
				sizeof(*cp));
	if (!buf) {
		LOG_ERR("Out of command buffers");
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
	uint16_t handle;
	uint8_t ltk[16];

	handle = sys_le16_to_cpu(evt->handle);

	LOG_DBG("handle %u", handle);

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Unable to lookup conn for handle %u", handle);
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

static void hci_reset_complete(struct net_buf *buf)
{
	uint8_t status = buf->data[0];
	atomic_t flags;

	LOG_DBG("status 0x%02x", status);

	if (status) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		bt_scan_reset();
	}

#if defined(CONFIG_BT_CLASSIC)
	bt_br_discovery_reset();
#endif /* CONFIG_BT_CLASSIC */

	flags = (atomic_get(bt_dev.flags) & BT_DEV_PERSISTENT_FLAGS);
	atomic_set(bt_dev.flags, flags);
}

static void hci_cmd_done(uint16_t opcode, uint8_t status, struct net_buf *evt_buf)
{
	/* Original command buffer. */
	struct net_buf *buf = NULL;

	LOG_DBG("opcode 0x%04x status 0x%02x buf %p", opcode, status, evt_buf);

	/* Unsolicited cmd complete. This does not complete a command.
	 * The controller can send these for effect of the `ncmd` field.
	 */
	if (opcode == 0) {
		goto exit;
	}

	/* Take the original command buffer reference. */
	buf = atomic_ptr_clear((atomic_ptr_t *)&bt_dev.sent_cmd);

	if (!buf) {
		LOG_ERR("No command sent for cmd complete 0x%04x", opcode);
		goto exit;
	}

	if (cmd(buf)->opcode != opcode) {
		LOG_ERR("OpCode 0x%04x completed instead of expected 0x%04x", opcode,
			cmd(buf)->opcode);
		buf = atomic_ptr_set((atomic_ptr_t *)&bt_dev.sent_cmd, buf);
		__ASSERT_NO_MSG(!buf);
		goto exit;
	}

	/* Response data is to be delivered in the original command
	 * buffer.
	 */
	if (evt_buf != buf) {
		net_buf_reset(buf);
		bt_buf_set_type(buf, BT_BUF_EVT);
		net_buf_reserve(buf, BT_BUF_RESERVE);
		net_buf_add_mem(buf, evt_buf->data, evt_buf->len);
	}

	if (cmd(buf)->state && !status) {
		struct bt_hci_cmd_state_set *update = cmd(buf)->state;

		atomic_set_bit_to(update->target, update->bit, update->val);
	}

	/* If the command was synchronous wake up bt_hci_cmd_send_sync() */
	if (cmd(buf)->sync) {
		LOG_DBG("sync cmd released");
		cmd(buf)->status = status;
		k_sem_give(cmd(buf)->sync);
	}

exit:
	if (buf) {
		net_buf_unref(buf);
	}
}

static void hci_cmd_complete(struct net_buf *buf)
{
	struct bt_hci_evt_cmd_complete *evt;
	uint8_t status, ncmd;
	uint16_t opcode;

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	ncmd = evt->ncmd;
	opcode = sys_le16_to_cpu(evt->opcode);

	LOG_DBG("opcode 0x%04x", opcode);

	/* All command return parameters have a 1-byte status in the
	 * beginning, so we can safely make this generalization.
	 */
	status = buf->data[0];

	/* HOST_NUM_COMPLETED_PACKETS should not generate a response under normal operation.
	 * The generation of this command ignores `ncmd_sem`, so should not be given here.
	 */
	if (opcode == BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS) {
		LOG_WRN("Unexpected HOST_NUM_COMPLETED_PACKETS (status 0x%02x)", status);
		return;
	}

	hci_cmd_done(opcode, status, buf);

	/* Allow next command to be sent */
	if (ncmd) {
		k_sem_give(&bt_dev.ncmd_sem);
		bt_tx_irq_raise();
	}
}

static void hci_cmd_status(struct net_buf *buf)
{
	struct bt_hci_evt_cmd_status *evt;
	uint16_t opcode;
	uint8_t ncmd;

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	opcode = sys_le16_to_cpu(evt->opcode);
	ncmd = evt->ncmd;

	LOG_DBG("opcode 0x%04x", opcode);

	hci_cmd_done(opcode, evt->status, buf);

	/* Allow next command to be sent */
	if (ncmd) {
		k_sem_give(&bt_dev.ncmd_sem);
		bt_tx_irq_raise();
	}
}

int bt_hci_get_conn_handle(const struct bt_conn *conn, uint16_t *conn_handle)
{
	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	*conn_handle = conn->handle;
	return 0;
}

#if defined(CONFIG_BT_EXT_ADV)
int bt_hci_get_adv_handle(const struct bt_le_ext_adv *adv, uint8_t *adv_handle)
{
	if (!atomic_test_bit(adv->flags, BT_ADV_CREATED)) {
		return -EINVAL;
	}

	*adv_handle = adv->handle;
	return 0;
}
#endif /* CONFIG_BT_EXT_ADV */

#if defined(CONFIG_BT_PER_ADV_SYNC)
int bt_hci_get_adv_sync_handle(const struct bt_le_per_adv_sync *sync, uint16_t *sync_handle)
{
	if (!atomic_test_bit(sync->flags, BT_PER_ADV_SYNC_CREATED)) {
		return -EINVAL;
	}

	*sync_handle = sync->handle;

	return 0;
}
#endif

#if defined(CONFIG_BT_HCI_VS_EVT_USER)
int bt_hci_register_vnd_evt_cb(bt_hci_vnd_evt_cb_t cb)
{
	hci_vnd_evt_cb = cb;
	return 0;
}
#endif /* CONFIG_BT_HCI_VS_EVT_USER */

#if defined(CONFIG_BT_TRANSMIT_POWER_CONTROL)
void bt_hci_le_transmit_power_report(struct net_buf *buf)
{
	struct bt_hci_evt_le_transmit_power_report *evt;
	struct bt_conn_le_tx_power_report report;
	struct bt_conn *conn;

	evt = net_buf_pull_mem(buf, sizeof(*evt));
	conn = bt_conn_lookup_handle(sys_le16_to_cpu(evt->handle), BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Unknown conn handle 0x%04X for transmit power report",
		       sys_le16_to_cpu(evt->handle));
		return;
	}

	report.reason = evt->reason;
	report.phy = evt->phy;
	report.tx_power_level = evt->tx_power_level;
	report.tx_power_level_flag = evt->tx_power_level_flag;
	report.delta = evt->delta;

	notify_tx_power_report(conn, report);

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_TRANSMIT_POWER_CONTROL */

#if defined(CONFIG_BT_PATH_LOSS_MONITORING)
void bt_hci_le_path_loss_threshold_event(struct net_buf *buf)
{
	struct bt_hci_evt_le_path_loss_threshold *evt;
	struct bt_conn_le_path_loss_threshold_report report;
	struct bt_conn *conn;

	evt = net_buf_pull_mem(buf, sizeof(*evt));

	if (evt->zone_entered > BT_CONN_LE_PATH_LOSS_ZONE_ENTERED_HIGH) {
		LOG_ERR("Invalid zone %u in bt_hci_evt_le_path_loss_threshold",
			evt->zone_entered);
		return;
	}

	conn = bt_conn_lookup_handle(sys_le16_to_cpu(evt->handle), BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Unknown conn handle 0x%04X for path loss threshold report",
		       sys_le16_to_cpu(evt->handle));
		return;
	}

	if (evt->current_path_loss == BT_HCI_LE_PATH_LOSS_UNAVAILABLE) {
		report.zone = BT_CONN_LE_PATH_LOSS_ZONE_UNAVAILABLE;
		report.path_loss = BT_HCI_LE_PATH_LOSS_UNAVAILABLE;
	} else {
		report.zone = evt->zone_entered;
		report.path_loss = evt->current_path_loss;
	}

	notify_path_loss_threshold_report(conn, report);

	bt_conn_unref(conn);
}
#endif /* CONFIG_BT_PATH_LOSS_MONITORING */

static const struct event_handler vs_events[] = {
#if defined(CONFIG_BT_DF_VS_CL_IQ_REPORT_16_BITS_IQ_SAMPLES)
	EVENT_HANDLER(BT_HCI_EVT_VS_LE_CONNECTIONLESS_IQ_REPORT,
		      bt_hci_le_vs_df_connectionless_iq_report,
		      sizeof(struct bt_hci_evt_vs_le_connectionless_iq_report)),
#endif /* CONFIG_BT_DF_VS_CL_IQ_REPORT_16_BITS_IQ_SAMPLES */
#if defined(CONFIG_BT_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES)
	EVENT_HANDLER(BT_HCI_EVT_VS_LE_CONNECTION_IQ_REPORT, bt_hci_le_vs_df_connection_iq_report,
		      sizeof(struct bt_hci_evt_vs_le_connection_iq_report)),
#endif /* CONFIG_BT_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES */
};

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

	if (IS_ENABLED(CONFIG_BT_HCI_VS) && !handled) {
		struct bt_hci_evt_vs *evt;

		evt = net_buf_pull_mem(buf, sizeof(*evt));

		LOG_DBG("subevent 0x%02x", evt->subevent);

		handle_vs_event(evt->subevent, buf, vs_events, ARRAY_SIZE(vs_events));
	}
}

static const struct event_handler meta_events[] = {
#if defined(CONFIG_BT_OBSERVER)
	EVENT_HANDLER(BT_HCI_EVT_LE_ADVERTISING_REPORT, bt_hci_le_adv_report,
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
	EVENT_HANDLER(BT_HCI_EVT_LE_REMOTE_FEAT_COMPLETE,
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
	EVENT_HANDLER(BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE,
		      bt_hci_evt_le_pkey_complete,
		      sizeof(struct bt_hci_evt_le_p256_public_key_complete)),
	EVENT_HANDLER(BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE,
		      bt_hci_evt_le_dhkey_complete,
		      sizeof(struct bt_hci_evt_le_generate_dhkey_complete)),
#endif /* CONFIG_BT_SMP */
#if defined(CONFIG_BT_EXT_ADV)
#if defined(CONFIG_BT_BROADCASTER)
	EVENT_HANDLER(BT_HCI_EVT_LE_ADV_SET_TERMINATED, bt_hci_le_adv_set_terminated,
		      sizeof(struct bt_hci_evt_le_adv_set_terminated)),
	EVENT_HANDLER(BT_HCI_EVT_LE_SCAN_REQ_RECEIVED, bt_hci_le_scan_req_received,
		      sizeof(struct bt_hci_evt_le_scan_req_received)),
#endif
#if defined(CONFIG_BT_OBSERVER)
	EVENT_HANDLER(BT_HCI_EVT_LE_SCAN_TIMEOUT, bt_hci_le_scan_timeout,
		      0),
	EVENT_HANDLER(BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT, bt_hci_le_adv_ext_report,
		      sizeof(struct bt_hci_evt_le_ext_advertising_report)),
#endif /* defined(CONFIG_BT_OBSERVER) */
#if defined(CONFIG_BT_PER_ADV_SYNC)
	EVENT_HANDLER(BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED,
		      bt_hci_le_per_adv_sync_established,
		      sizeof(struct bt_hci_evt_le_per_adv_sync_established)),
	EVENT_HANDLER(BT_HCI_EVT_LE_PER_ADVERTISING_REPORT, bt_hci_le_per_adv_report,
		      sizeof(struct bt_hci_evt_le_per_advertising_report)),
	EVENT_HANDLER(BT_HCI_EVT_LE_PER_ADV_SYNC_LOST, bt_hci_le_per_adv_sync_lost,
		      sizeof(struct bt_hci_evt_le_per_adv_sync_lost)),
#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)
	EVENT_HANDLER(BT_HCI_EVT_LE_PAST_RECEIVED, bt_hci_le_past_received,
		      sizeof(struct bt_hci_evt_le_past_received)),
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER */
#endif /* defined(CONFIG_BT_PER_ADV_SYNC) */
#endif /* defined(CONFIG_BT_EXT_ADV) */
#if defined(CONFIG_BT_ISO_UNICAST)
	EVENT_HANDLER(BT_HCI_EVT_LE_CIS_ESTABLISHED, hci_le_cis_established,
		      sizeof(struct bt_hci_evt_le_cis_established)),
#if defined(CONFIG_BT_ISO_PERIPHERAL)
	EVENT_HANDLER(BT_HCI_EVT_LE_CIS_REQ, hci_le_cis_req,
		      sizeof(struct bt_hci_evt_le_cis_req)),
#endif /* (CONFIG_BT_ISO_PERIPHERAL) */
#endif /* (CONFIG_BT_ISO_UNICAST) */
#if defined(CONFIG_BT_ISO_BROADCASTER)
	EVENT_HANDLER(BT_HCI_EVT_LE_BIG_COMPLETE,
		      hci_le_big_complete,
		      sizeof(struct bt_hci_evt_le_big_complete)),
	EVENT_HANDLER(BT_HCI_EVT_LE_BIG_TERMINATE,
		      hci_le_big_terminate,
		      sizeof(struct bt_hci_evt_le_big_terminate)),
#endif /* CONFIG_BT_ISO_BROADCASTER */
#if defined(CONFIG_BT_ISO_SYNC_RECEIVER)
	EVENT_HANDLER(BT_HCI_EVT_LE_BIG_SYNC_ESTABLISHED,
		      hci_le_big_sync_established,
		      sizeof(struct bt_hci_evt_le_big_sync_established)),
	EVENT_HANDLER(BT_HCI_EVT_LE_BIG_SYNC_LOST,
		      hci_le_big_sync_lost,
		      sizeof(struct bt_hci_evt_le_big_sync_lost)),
	EVENT_HANDLER(BT_HCI_EVT_LE_BIGINFO_ADV_REPORT,
		      bt_hci_le_biginfo_adv_report,
		      sizeof(struct bt_hci_evt_le_biginfo_adv_report)),
#endif /* CONFIG_BT_ISO_SYNC_RECEIVER */
#if defined(CONFIG_BT_DF_CONNECTIONLESS_CTE_RX)
	EVENT_HANDLER(BT_HCI_EVT_LE_CONNECTIONLESS_IQ_REPORT, bt_hci_le_df_connectionless_iq_report,
		      sizeof(struct bt_hci_evt_le_connectionless_iq_report)),
#endif /* CONFIG_BT_DF_CONNECTIONLESS_CTE_RX */
#if defined(CONFIG_BT_DF_CONNECTION_CTE_RX)
	EVENT_HANDLER(BT_HCI_EVT_LE_CONNECTION_IQ_REPORT, bt_hci_le_df_connection_iq_report,
		      sizeof(struct bt_hci_evt_le_connection_iq_report)),
#endif /* CONFIG_BT_DF_CONNECTION_CTE_RX */
#if defined(CONFIG_BT_DF_CONNECTION_CTE_REQ)
	EVENT_HANDLER(BT_HCI_EVT_LE_CTE_REQUEST_FAILED, bt_hci_le_df_cte_req_failed,
		      sizeof(struct bt_hci_evt_le_cte_req_failed)),
#endif /* CONFIG_BT_DF_CONNECTION_CTE_REQ */
#if defined(CONFIG_BT_TRANSMIT_POWER_CONTROL)
	EVENT_HANDLER(BT_HCI_EVT_LE_TRANSMIT_POWER_REPORT, bt_hci_le_transmit_power_report,
		      sizeof(struct bt_hci_evt_le_transmit_power_report)),
#endif /* CONFIG_BT_TRANSMIT_POWER_CONTROL */
#if defined(CONFIG_BT_PATH_LOSS_MONITORING)
	EVENT_HANDLER(BT_HCI_EVT_LE_PATH_LOSS_THRESHOLD, bt_hci_le_path_loss_threshold_event,
		      sizeof(struct bt_hci_evt_le_path_loss_threshold)),
#endif /* CONFIG_BT_PATH_LOSS_MONITORING */
#if defined(CONFIG_BT_PER_ADV_SYNC_RSP)
	EVENT_HANDLER(BT_HCI_EVT_LE_PER_ADVERTISING_REPORT_V2, bt_hci_le_per_adv_report_v2,
		      sizeof(struct bt_hci_evt_le_per_advertising_report_v2)),
#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)
	EVENT_HANDLER(BT_HCI_EVT_LE_PAST_RECEIVED_V2, bt_hci_le_past_received_v2,
		      sizeof(struct bt_hci_evt_le_past_received_v2)),
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER */
	EVENT_HANDLER(BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED_V2,
		      bt_hci_le_per_adv_sync_established_v2,
		      sizeof(struct bt_hci_evt_le_per_adv_sync_established_v2)),
#endif /* CONFIG_BT_PER_ADV_SYNC_RSP */
#if defined(CONFIG_BT_PER_ADV_RSP)
	EVENT_HANDLER(BT_HCI_EVT_LE_PER_ADV_SUBEVENT_DATA_REQUEST,
		      bt_hci_le_per_adv_subevent_data_request,
		      sizeof(struct bt_hci_evt_le_per_adv_subevent_data_request)),
	EVENT_HANDLER(BT_HCI_EVT_LE_PER_ADV_RESPONSE_REPORT, bt_hci_le_per_adv_response_report,
		      sizeof(struct bt_hci_evt_le_per_adv_response_report)),
#endif /* CONFIG_BT_PER_ADV_RSP */
#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_PER_ADV_RSP) || defined(CONFIG_BT_PER_ADV_SYNC_RSP)
	EVENT_HANDLER(BT_HCI_EVT_LE_ENH_CONN_COMPLETE_V2, le_enh_conn_complete_v2,
		      sizeof(struct bt_hci_evt_le_enh_conn_complete_v2)),
#endif /* CONFIG_BT_PER_ADV_RSP || CONFIG_BT_PER_ADV_SYNC_RSP */
#endif /* CONFIG_BT_CONN */

};

static void hci_le_meta_event(struct net_buf *buf)
{
	struct bt_hci_evt_le_meta_event *evt;

	evt = net_buf_pull_mem(buf, sizeof(*evt));

	LOG_DBG("subevent 0x%02x", evt->subevent);

	handle_event(evt->subevent, buf, meta_events, ARRAY_SIZE(meta_events));
}

static const struct event_handler normal_events[] = {
	EVENT_HANDLER(BT_HCI_EVT_VENDOR, hci_vendor_event,
		      sizeof(struct bt_hci_evt_vs)),
	EVENT_HANDLER(BT_HCI_EVT_LE_META_EVENT, hci_le_meta_event,
		      sizeof(struct bt_hci_evt_le_meta_event)),
#if defined(CONFIG_BT_CLASSIC)
	EVENT_HANDLER(BT_HCI_EVT_CONN_REQUEST, bt_hci_conn_req,
		      sizeof(struct bt_hci_evt_conn_request)),
	EVENT_HANDLER(BT_HCI_EVT_CONN_COMPLETE, bt_hci_conn_complete,
		      sizeof(struct bt_hci_evt_conn_complete)),
	EVENT_HANDLER(BT_HCI_EVT_PIN_CODE_REQ, bt_hci_pin_code_req,
		      sizeof(struct bt_hci_evt_pin_code_req)),
	EVENT_HANDLER(BT_HCI_EVT_LINK_KEY_NOTIFY, bt_hci_link_key_notify,
		      sizeof(struct bt_hci_evt_link_key_notify)),
	EVENT_HANDLER(BT_HCI_EVT_LINK_KEY_REQ, bt_hci_link_key_req,
		      sizeof(struct bt_hci_evt_link_key_req)),
	EVENT_HANDLER(BT_HCI_EVT_IO_CAPA_RESP, bt_hci_io_capa_resp,
		      sizeof(struct bt_hci_evt_io_capa_resp)),
	EVENT_HANDLER(BT_HCI_EVT_IO_CAPA_REQ, bt_hci_io_capa_req,
		      sizeof(struct bt_hci_evt_io_capa_req)),
	EVENT_HANDLER(BT_HCI_EVT_SSP_COMPLETE, bt_hci_ssp_complete,
		      sizeof(struct bt_hci_evt_ssp_complete)),
	EVENT_HANDLER(BT_HCI_EVT_USER_CONFIRM_REQ, bt_hci_user_confirm_req,
		      sizeof(struct bt_hci_evt_user_confirm_req)),
	EVENT_HANDLER(BT_HCI_EVT_USER_PASSKEY_NOTIFY,
		      bt_hci_user_passkey_notify,
		      sizeof(struct bt_hci_evt_user_passkey_notify)),
	EVENT_HANDLER(BT_HCI_EVT_USER_PASSKEY_REQ, bt_hci_user_passkey_req,
		      sizeof(struct bt_hci_evt_user_passkey_req)),
	EVENT_HANDLER(BT_HCI_EVT_INQUIRY_COMPLETE, bt_hci_inquiry_complete,
		      sizeof(struct bt_hci_evt_inquiry_complete)),
	EVENT_HANDLER(BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI,
		      bt_hci_inquiry_result_with_rssi,
		      sizeof(struct bt_hci_evt_inquiry_result_with_rssi)),
	EVENT_HANDLER(BT_HCI_EVT_EXTENDED_INQUIRY_RESULT,
		      bt_hci_extended_inquiry_result,
		      sizeof(struct bt_hci_evt_extended_inquiry_result)),
	EVENT_HANDLER(BT_HCI_EVT_REMOTE_NAME_REQ_COMPLETE,
		      bt_hci_remote_name_request_complete,
		      sizeof(struct bt_hci_evt_remote_name_req_complete)),
	EVENT_HANDLER(BT_HCI_EVT_AUTH_COMPLETE, bt_hci_auth_complete,
		      sizeof(struct bt_hci_evt_auth_complete)),
	EVENT_HANDLER(BT_HCI_EVT_REMOTE_FEATURES,
		      bt_hci_read_remote_features_complete,
		      sizeof(struct bt_hci_evt_remote_features)),
	EVENT_HANDLER(BT_HCI_EVT_REMOTE_EXT_FEATURES,
		      bt_hci_read_remote_ext_features_complete,
		      sizeof(struct bt_hci_evt_remote_ext_features)),
	EVENT_HANDLER(BT_HCI_EVT_ROLE_CHANGE, bt_hci_role_change,
		      sizeof(struct bt_hci_evt_role_change)),
	EVENT_HANDLER(BT_HCI_EVT_SYNC_CONN_COMPLETE, bt_hci_synchronous_conn_complete,
		      sizeof(struct bt_hci_evt_sync_conn_complete)),
#endif /* CONFIG_BT_CLASSIC */
#if defined(CONFIG_BT_CONN)
	EVENT_HANDLER(BT_HCI_EVT_DISCONN_COMPLETE, hci_disconn_complete,
		      sizeof(struct bt_hci_evt_disconn_complete)),
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
	EVENT_HANDLER(BT_HCI_EVT_ENCRYPT_CHANGE, hci_encrypt_change,
		      sizeof(struct bt_hci_evt_encrypt_change)),
	EVENT_HANDLER(BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE,
		      hci_encrypt_key_refresh_complete,
		      sizeof(struct bt_hci_evt_encrypt_key_refresh_complete)),
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC */
#if defined(CONFIG_BT_REMOTE_VERSION)
	EVENT_HANDLER(BT_HCI_EVT_REMOTE_VERSION_INFO,
		      bt_hci_evt_read_remote_version_complete,
		      sizeof(struct bt_hci_evt_remote_version_info)),
#endif /* CONFIG_BT_REMOTE_VERSION */
	EVENT_HANDLER(BT_HCI_EVT_HARDWARE_ERROR, hci_hardware_error,
		      sizeof(struct bt_hci_evt_hardware_error)),
};


#define BT_HCI_EVT_FLAG_RECV_PRIO BIT(0)
#define BT_HCI_EVT_FLAG_RECV      BIT(1)

/** @brief Get HCI event flags.
 *
 * Helper for the HCI driver to get HCI event flags that describes rules that.
 * must be followed.
 *
 * @param evt HCI event code.
 *
 * @return HCI event flags for the specified event.
 */
static inline uint8_t bt_hci_evt_get_flags(uint8_t evt)
{
	switch (evt) {
	case BT_HCI_EVT_DISCONN_COMPLETE:
		return BT_HCI_EVT_FLAG_RECV | BT_HCI_EVT_FLAG_RECV_PRIO;
		/* fallthrough */
#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_ISO)
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
#if defined(CONFIG_BT_CONN)
	case BT_HCI_EVT_DATA_BUF_OVERFLOW:
		__fallthrough;
#endif /* defined(CONFIG_BT_CONN) */
#endif /* CONFIG_BT_CONN ||  CONFIG_BT_ISO */
	case BT_HCI_EVT_CMD_COMPLETE:
	case BT_HCI_EVT_CMD_STATUS:
		return BT_HCI_EVT_FLAG_RECV_PRIO;
	default:
		return BT_HCI_EVT_FLAG_RECV;
	}
}

static void hci_event(struct net_buf *buf)
{
	struct bt_hci_evt_hdr *hdr;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Invalid HCI event size (%u)", buf->len);
		net_buf_unref(buf);
		return;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	LOG_DBG("event 0x%02x", hdr->evt);
	BT_ASSERT(bt_hci_evt_get_flags(hdr->evt) & BT_HCI_EVT_FLAG_RECV);

	handle_event(hdr->evt, buf, normal_events, ARRAY_SIZE(normal_events));

	net_buf_unref(buf);
}

static void hci_core_send_cmd(void)
{
	struct net_buf *buf;
	int err;

	/* Get next command */
	LOG_DBG("fetch cmd");
	buf = net_buf_get(&bt_dev.cmd_tx_queue, K_NO_WAIT);
	BT_ASSERT(buf);

	/* Clear out any existing sent command */
	if (bt_dev.sent_cmd) {
		LOG_ERR("Uncleared pending sent_cmd");
		net_buf_unref(bt_dev.sent_cmd);
		bt_dev.sent_cmd = NULL;
	}

	bt_dev.sent_cmd = net_buf_ref(buf);

	LOG_DBG("Sending command 0x%04x (buf %p) to driver", cmd(buf)->opcode, buf);

	err = bt_send(buf);
	if (err) {
		LOG_ERR("Unable to send to driver (err %d)", err);
		k_sem_give(&bt_dev.ncmd_sem);
		hci_cmd_done(cmd(buf)->opcode, BT_HCI_ERR_UNSPECIFIED, buf);
		net_buf_unref(buf);
		bt_tx_irq_raise();
	}
}

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_ISO)
/* command FIFO + conn_change signal + MAX_CONN + ISO_MAX_CHAN */
#define EV_COUNT (2 + CONFIG_BT_MAX_CONN + CONFIG_BT_ISO_MAX_CHAN)
#else
/* command FIFO + conn_change signal + MAX_CONN */
#define EV_COUNT (2 + CONFIG_BT_MAX_CONN)
#endif /* CONFIG_BT_ISO */
#else
#if defined(CONFIG_BT_ISO)
/* command FIFO + conn_change signal + ISO_MAX_CHAN */
#define EV_COUNT (2 + CONFIG_BT_ISO_MAX_CHAN)
#else
/* command FIFO */
#define EV_COUNT 1
#endif /* CONFIG_BT_ISO */
#endif /* CONFIG_BT_CONN */

static void read_local_ver_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_local_version_info *rp = (void *)buf->data;

	LOG_DBG("status 0x%02x", rp->status);

	bt_dev.hci_version = rp->hci_version;
	bt_dev.hci_revision = sys_le16_to_cpu(rp->hci_revision);
	bt_dev.lmp_version = rp->lmp_version;
	bt_dev.lmp_subversion = sys_le16_to_cpu(rp->lmp_subversion);
	bt_dev.manufacturer = sys_le16_to_cpu(rp->manufacturer);
}

static void read_le_features_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_local_features *rp = (void *)buf->data;

	LOG_DBG("status 0x%02x", rp->status);

	memcpy(bt_dev.le.features, rp->features, sizeof(bt_dev.le.features));
}

#if defined(CONFIG_BT_CONN)
#if !defined(CONFIG_BT_CLASSIC)
static void read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;
	uint16_t pkts;

	LOG_DBG("status 0x%02x", rp->status);

	/* If LE-side has buffers we can ignore the BR/EDR values */
	if (bt_dev.le.acl_mtu) {
		return;
	}

	bt_dev.le.acl_mtu = sys_le16_to_cpu(rp->acl_max_len);
	pkts = sys_le16_to_cpu(rp->acl_max_num);

	LOG_DBG("ACL BR/EDR buffers: pkts %u mtu %u", pkts, bt_dev.le.acl_mtu);

	k_sem_init(&bt_dev.le.acl_pkts, pkts, pkts);
}
#endif /* !defined(CONFIG_BT_CLASSIC) */
#endif /* CONFIG_BT_CONN */

static void le_read_buffer_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_buffer_size *rp = (void *)buf->data;

	LOG_DBG("status 0x%02x", rp->status);

#if defined(CONFIG_BT_CONN)
	uint16_t acl_mtu = sys_le16_to_cpu(rp->le_max_len);

	if (!acl_mtu || !rp->le_max_num) {
		return;
	}

	bt_dev.le.acl_mtu = acl_mtu;

	LOG_DBG("ACL LE buffers: pkts %u mtu %u", rp->le_max_num, bt_dev.le.acl_mtu);

	k_sem_init(&bt_dev.le.acl_pkts, rp->le_max_num, rp->le_max_num);
#endif /* CONFIG_BT_CONN */
}

static void read_buffer_size_v2_complete(struct net_buf *buf)
{
#if defined(CONFIG_BT_ISO)
	struct bt_hci_rp_le_read_buffer_size_v2 *rp = (void *)buf->data;

	LOG_DBG("status %u", rp->status);

#if defined(CONFIG_BT_CONN)
	uint16_t acl_mtu = sys_le16_to_cpu(rp->acl_max_len);

	if (acl_mtu && rp->acl_max_num) {
		bt_dev.le.acl_mtu = acl_mtu;
		LOG_DBG("ACL LE buffers: pkts %u mtu %u", rp->acl_max_num, bt_dev.le.acl_mtu);

		k_sem_init(&bt_dev.le.acl_pkts, rp->acl_max_num, rp->acl_max_num);
	}
#endif /* CONFIG_BT_CONN */

	uint16_t iso_mtu = sys_le16_to_cpu(rp->iso_max_len);

	if (!iso_mtu || !rp->iso_max_num) {
		LOG_ERR("ISO buffer size not set");
		return;
	}

	bt_dev.le.iso_mtu = iso_mtu;

	LOG_DBG("ISO buffers: pkts %u mtu %u", rp->iso_max_num, bt_dev.le.iso_mtu);

	k_sem_init(&bt_dev.le.iso_pkts, rp->iso_max_num, rp->iso_max_num);
	bt_dev.le.iso_limit = rp->iso_max_num;
#endif /* CONFIG_BT_ISO */
}

static int le_set_host_feature(uint8_t bit_number, uint8_t bit_value)
{
	struct bt_hci_cp_le_set_host_feature *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_HOST_FEATURE, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->bit_number = bit_number;
	cp->bit_value = bit_value;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_HOST_FEATURE, buf, NULL);
}

static void read_supported_commands_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_supported_commands *rp = (void *)buf->data;

	LOG_DBG("status 0x%02x", rp->status);

	memcpy(bt_dev.supported_commands, rp->commands,
	       sizeof(bt_dev.supported_commands));

	/* Report additional HCI commands used for ECDH as
	 * supported if TinyCrypt ECC is used for emulation.
	 */
	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		bt_hci_ecc_supported_commands(bt_dev.supported_commands);
	}
}

static void read_local_features_complete(struct net_buf *buf)
{
	struct bt_hci_rp_read_local_features *rp = (void *)buf->data;

	LOG_DBG("status 0x%02x", rp->status);

	memcpy(bt_dev.features[0], rp->features, sizeof(bt_dev.features[0]));
}

static void le_read_supp_states_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_supp_states *rp = (void *)buf->data;

	LOG_DBG("status 0x%02x", rp->status);

	bt_dev.le.states = sys_get_le64(rp->le_states);
}

#if defined(CONFIG_BT_BROADCASTER)
static void le_read_maximum_adv_data_len_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_max_adv_data_len *rp = (void *)buf->data;

	LOG_DBG("status 0x%02x", rp->status);

	bt_dev.le.max_adv_data_len = sys_le16_to_cpu(rp->max_adv_data_len);
}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_SMP)
static void le_read_resolving_list_size_complete(struct net_buf *buf)
{
	struct bt_hci_rp_le_read_rl_size *rp = (void *)buf->data;

	LOG_DBG("Resolving List size %u", rp->rl_size);

	bt_dev.le.rl_size = rp->rl_size;
}
#endif /* defined(CONFIG_BT_SMP) */

static int common_init(void)
{
	struct net_buf *rsp;
	int err;

	if (!drv_quirk_no_reset()) {
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

	/* Read Local Supported Commands */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_SUPPORTED_COMMANDS, NULL,
				   &rsp);
	if (err) {
		return err;
	}
	read_supported_commands_complete(rsp);
	net_buf_unref(rsp);

	if (IS_ENABLED(CONFIG_BT_HOST_CRYPTO_PRNG)) {
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
	uint64_t mask = 0U;

	/* Set LE event mask */
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_EVENT_MASK, sizeof(*cp_mask));
	if (!buf) {
		return -ENOBUFS;
	}

	cp_mask = net_buf_add(buf, sizeof(*cp_mask));

	mask |= BT_EVT_MASK_LE_ADVERTISING_REPORT;

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		mask |= BT_EVT_MASK_LE_ADV_SET_TERMINATED;
		mask |= BT_EVT_MASK_LE_SCAN_REQ_RECEIVED;
		mask |= BT_EVT_MASK_LE_EXT_ADVERTISING_REPORT;
		mask |= BT_EVT_MASK_LE_SCAN_TIMEOUT;
		if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC)) {
			mask |= BT_EVT_MASK_LE_PER_ADV_SYNC_ESTABLISHED;
			mask |= BT_EVT_MASK_LE_PER_ADVERTISING_REPORT;
			mask |= BT_EVT_MASK_LE_PER_ADV_SYNC_LOST;
			mask |= BT_EVT_MASK_LE_PAST_RECEIVED;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		if ((IS_ENABLED(CONFIG_BT_SMP) &&
		     BT_FEAT_LE_PRIVACY(bt_dev.le.features)) ||
		    (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
		     BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
			/* C24:
			 * Mandatory if the LE Controller supports Connection
			 * State and either LE Feature (LL Privacy) or
			 * LE Feature (Extended Advertising) is supported, ...
			 */
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
		if (IS_ENABLED(CONFIG_BT_TRANSMIT_POWER_CONTROL)) {
			mask |= BT_EVT_MASK_LE_TRANSMIT_POWER_REPORTING;
		}

		if (IS_ENABLED(CONFIG_BT_PATH_LOSS_MONITORING)) {
			mask |= BT_EVT_MASK_LE_PATH_LOSS_THRESHOLD;
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

	/*
	 * Enable CIS events only if ISO connections are enabled and controller
	 * support them.
	 */
	if (IS_ENABLED(CONFIG_BT_ISO) &&
	    BT_FEAT_LE_CIS(bt_dev.le.features)) {
		mask |= BT_EVT_MASK_LE_CIS_ESTABLISHED;
		if (BT_FEAT_LE_CIS_PERIPHERAL(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_CIS_REQ;
		}
	}

	/* Enable BIS events for broadcaster and/or receiver */
	if (IS_ENABLED(CONFIG_BT_ISO) && BT_FEAT_LE_BIS(bt_dev.le.features)) {
		if (IS_ENABLED(CONFIG_BT_ISO_BROADCASTER) &&
		    BT_FEAT_LE_ISO_BROADCASTER(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_BIG_COMPLETE;
			mask |= BT_EVT_MASK_LE_BIG_TERMINATED;
		}
		if (IS_ENABLED(CONFIG_BT_ISO_SYNC_RECEIVER) &&
		    BT_FEAT_LE_SYNC_RECEIVER(bt_dev.le.features)) {
			mask |= BT_EVT_MASK_LE_BIG_SYNC_ESTABLISHED;
			mask |= BT_EVT_MASK_LE_BIG_SYNC_LOST;
			mask |= BT_EVT_MASK_LE_BIGINFO_ADV_REPORT;
		}
	}

	/* Enable IQ samples report events receiver */
	if (IS_ENABLED(CONFIG_BT_DF_CONNECTIONLESS_CTE_RX)) {
		mask |= BT_EVT_MASK_LE_CONNECTIONLESS_IQ_REPORT;
	}

	if (IS_ENABLED(CONFIG_BT_DF_CONNECTION_CTE_RX)) {
		mask |= BT_EVT_MASK_LE_CONNECTION_IQ_REPORT;
		mask |= BT_EVT_MASK_LE_CTE_REQUEST_FAILED;
	}

	if (IS_ENABLED(CONFIG_BT_PER_ADV_RSP)) {
		mask |= BT_EVT_MASK_LE_PER_ADV_SUBEVENT_DATA_REQ;
		mask |= BT_EVT_MASK_LE_PER_ADV_RESPONSE_REPORT;
	}

	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_RSP)) {
		mask |= BT_EVT_MASK_LE_PER_ADVERTISING_REPORT_V2;
		mask |= BT_EVT_MASK_LE_PER_ADV_SYNC_ESTABLISHED_V2;
		mask |= BT_EVT_MASK_LE_PAST_RECEIVED_V2;
	}

	if (IS_ENABLED(CONFIG_BT_CONN) &&
	    (IS_ENABLED(CONFIG_BT_PER_ADV_RSP) || IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_RSP))) {
		mask |= BT_EVT_MASK_LE_ENH_CONN_COMPLETE_V2;
	}

	sys_put_le64(mask, cp_mask->events);
	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EVENT_MASK, buf, NULL);
}

static int le_init_iso(void)
{
	int err;
	struct net_buf *rsp;

	if (IS_ENABLED(CONFIG_BT_ISO_UNICAST)) {
		/* Set Connected Isochronous Streams - Host support */
		err = le_set_host_feature(BT_LE_FEAT_BIT_ISO_CHANNELS, 1);
		if (err) {
			return err;
		}
	}

	/* Octet 41, bit 5 is read buffer size V2 */
	if (BT_CMD_TEST(bt_dev.supported_commands, 41, 5)) {
		/* Read ISO Buffer Size V2 */
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_BUFFER_SIZE_V2,
					   NULL, &rsp);
		if (err) {
			return err;
		}

		read_buffer_size_v2_complete(rsp);

		net_buf_unref(rsp);
	} else if (IS_ENABLED(CONFIG_BT_CONN_TX)) {
		if (IS_ENABLED(CONFIG_BT_ISO_TX)) {
			LOG_WRN("Read Buffer Size V2 command is not supported. "
				"No ISO TX buffers will be available");
		}

		/* Read LE Buffer Size in the case that we support ACL without TX ISO (e.g. if we
		 * only support ISO sync receiver).
		 */
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_BUFFER_SIZE,
					   NULL, &rsp);
		if (err) {
			return err;
		}

		le_read_buffer_size_complete(rsp);

		net_buf_unref(rsp);
	}

	return 0;
}

static int le_init(void)
{
	struct bt_hci_cp_write_le_host_supp *cp_le;
	struct net_buf *buf, *rsp;
	int err;

	/* For now we only support LE capable controllers */
	if (!BT_FEAT_LE(bt_dev.features)) {
		LOG_ERR("Non-LE capable controller detected!");
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

	if (IS_ENABLED(CONFIG_BT_ISO) &&
	    BT_FEAT_LE_ISO(bt_dev.le.features)) {
		err = le_init_iso();
		if (err) {
			return err;
		}
	} else if (IS_ENABLED(CONFIG_BT_CONN)) {
		/* Read LE Buffer Size */
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_BUFFER_SIZE,
					   NULL, &rsp);
		if (err) {
			return err;
		}

		le_read_buffer_size_complete(rsp);

		net_buf_unref(rsp);
	}

#if defined(CONFIG_BT_BROADCASTER)
	if (IS_ENABLED(CONFIG_BT_EXT_ADV) && BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		/* Read LE Max Adv Data Len */
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN, NULL, &rsp);
		if (err == 0) {
			le_read_maximum_adv_data_len_complete(rsp);
			net_buf_unref(rsp);
		} else if (err == -EIO) {
			LOG_WRN("Controller does not support 'LE_READ_MAX_ADV_DATA_LEN'. "
				"Assuming maximum length is 31 bytes.");
			bt_dev.le.max_adv_data_len = 31;
		} else {
			return err;
		}
	} else {
		bt_dev.le.max_adv_data_len = 31;
	}
#endif /* CONFIG_BT_BROADCASTER */

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
	    IS_ENABLED(CONFIG_BT_AUTO_DATA_LEN_UPDATE) &&
	    BT_FEAT_LE_DLE(bt_dev.le.features)) {
		struct bt_hci_cp_le_write_default_data_len *cp;
		uint16_t tx_octets, tx_time;

		err = hci_le_read_max_data_len(&tx_octets, &tx_time);
		if (err) {
			return err;
		}

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
		cp->rpa_timeout = sys_cpu_to_le16(bt_dev.rpa_timeout);
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

#if defined(CONFIG_BT_DF)
	if (BT_FEAT_LE_CONNECTIONLESS_CTE_TX(bt_dev.le.features) ||
	    BT_FEAT_LE_CONNECTIONLESS_CTE_RX(bt_dev.le.features) ||
	    BT_FEAT_LE_RX_CTE(bt_dev.le.features)) {
		err = le_df_init();
		if (err) {
			return err;
		}
	}
#endif /* CONFIG_BT_DF */

	return  le_set_event_mask();
}

#if !defined(CONFIG_BT_CLASSIC)
static int bt_br_init(void)
{
#if defined(CONFIG_BT_CONN)
	struct net_buf *rsp;
	int err;

	if (bt_dev.le.acl_mtu) {
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
#endif /* !defined(CONFIG_BT_CLASSIC) */

static int set_event_mask(void)
{
	struct bt_hci_cp_set_event_mask *ev;
	struct net_buf *buf;
	uint64_t mask = 0U;

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_EVENT_MASK, sizeof(*ev));
	if (!buf) {
		return -ENOBUFS;
	}

	ev = net_buf_add(buf, sizeof(*ev));

	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
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

const char *bt_hci_get_ver_str(uint8_t core_version)
{
	const char * const str[] = {
		"1.0b", "1.1", "1.2", "2.0", "2.1", "3.0", "4.0", "4.1", "4.2",
		"5.0", "5.1", "5.2", "5.3", "5.4"
	};

	if (core_version < ARRAY_SIZE(str)) {
		return str[core_version];
	}

	return "unknown";
}

static void bt_dev_show_info(void)
{
	int i;

	LOG_INF("Identity%s: %s", bt_dev.id_count > 1 ? "[0]" : "",
		bt_addr_le_str(&bt_dev.id_addr[0]));

	if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
#if defined(CONFIG_BT_PRIVACY)
		uint8_t irk[16];

		sys_memcpy_swap(irk, bt_dev.irk[0], 16);
		LOG_INF("IRK%s: 0x%s", bt_dev.id_count > 1 ? "[0]" : "", bt_hex(irk, 16));
#endif
	}

	for (i = 1; i < bt_dev.id_count; i++) {
		LOG_INF("Identity[%d]: %s", i, bt_addr_le_str(&bt_dev.id_addr[i]));

		if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
#if defined(CONFIG_BT_PRIVACY)
			uint8_t irk[16];

			sys_memcpy_swap(irk, bt_dev.irk[i], 16);
			LOG_INF("IRK[%d]: 0x%s", i, bt_hex(irk, 16));
#endif
		}
	}

	if (IS_ENABLED(CONFIG_BT_SMP) &&
	    IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
		bt_keys_foreach_type(BT_KEYS_ALL, bt_keys_show_sniffer_info, NULL);
	}

	LOG_INF("HCI: version %s (0x%02x) revision 0x%04x, manufacturer 0x%04x",
		bt_hci_get_ver_str(bt_dev.hci_version), bt_dev.hci_version, bt_dev.hci_revision,
		bt_dev.manufacturer);
	LOG_INF("LMP: version %s (0x%02x) subver 0x%04x", bt_hci_get_ver_str(bt_dev.lmp_version),
		bt_dev.lmp_version, bt_dev.lmp_subversion);
}

#if defined(CONFIG_BT_HCI_VS)
static const char *vs_hw_platform(uint16_t platform)
{
	static const char * const plat_str[] = {
		"reserved", "Intel Corporation", "Nordic Semiconductor",
		"NXP Semiconductors" };

	if (platform < ARRAY_SIZE(plat_str)) {
		return plat_str[platform];
	}

	return "unknown";
}

static const char *vs_hw_variant(uint16_t platform, uint16_t variant)
{
	static const char * const nordic_str[] = {
		"reserved", "nRF51x", "nRF52x", "nRF53x", "nRF54Hx", "nRF54Lx"
	};

	if (platform != BT_HCI_VS_HW_PLAT_NORDIC) {
		return "unknown";
	}

	if (variant < ARRAY_SIZE(nordic_str)) {
		return nordic_str[variant];
	}

	return "unknown";
}

static const char *vs_fw_variant(uint8_t variant)
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
	 * at the HCI version and identity address. We haven't set any addresses
	 * at this point. So we need to read the public address.
	 */
	if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT_DETECT)) {
		bt_addr_le_t addr;

		if ((bt_dev.hci_version < BT_HCI_VERSION_5_0) ||
		    bt_id_read_public_addr(&addr)) {
			LOG_WRN("Controller doesn't seem to support "
				"Zephyr vendor HCI");
			return;
		}
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_VERSION_INFO, NULL, &rsp);
	if (err) {
		LOG_WRN("Vendor HCI extensions not available");
		return;
	}

	if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT_DETECT) &&
	    rsp->len != sizeof(struct bt_hci_rp_vs_read_version_info)) {
		LOG_WRN("Invalid Vendor HCI extensions");
		net_buf_unref(rsp);
		return;
	}

	rp.info = (void *)rsp->data;
	LOG_INF("HW Platform: %s (0x%04x)", vs_hw_platform(sys_le16_to_cpu(rp.info->hw_platform)),
		sys_le16_to_cpu(rp.info->hw_platform));
	LOG_INF("HW Variant: %s (0x%04x)",
		vs_hw_variant(sys_le16_to_cpu(rp.info->hw_platform),
			      sys_le16_to_cpu(rp.info->hw_variant)),
		sys_le16_to_cpu(rp.info->hw_variant));
	LOG_INF("Firmware: %s (0x%02x) Version %u.%u Build %u", vs_fw_variant(rp.info->fw_variant),
		rp.info->fw_variant, rp.info->fw_version, sys_le16_to_cpu(rp.info->fw_revision),
		sys_le32_to_cpu(rp.info->fw_build));

	net_buf_unref(rsp);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS,
				   NULL, &rsp);
	if (err) {
		LOG_WRN("Failed to read supported vendor commands");
		return;
	}

	if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT_DETECT) &&
	    rsp->len != sizeof(struct bt_hci_rp_vs_read_supported_commands)) {
		LOG_WRN("Invalid Vendor HCI extensions");
		net_buf_unref(rsp);
		return;
	}

	rp.cmds = (void *)rsp->data;
	memcpy(bt_dev.vs_commands, rp.cmds->commands, BT_DEV_VS_CMDS_MAX);
	net_buf_unref(rsp);

	if (BT_VS_CMD_SUP_FEAT(bt_dev.vs_commands)) {
		err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_SUPPORTED_FEATURES,
					   NULL, &rsp);
		if (err) {
			LOG_WRN("Failed to read supported vendor features");
			return;
		}

		if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT_DETECT) &&
		    rsp->len !=
		    sizeof(struct bt_hci_rp_vs_read_supported_features)) {
			LOG_WRN("Invalid Vendor HCI extensions");
			net_buf_unref(rsp);
			return;
		}

		rp.feat = (void *)rsp->data;
		memcpy(bt_dev.vs_features, rp.feat->features,
		       BT_DEV_VS_FEAT_MAX);
		net_buf_unref(rsp);
	}
}
#endif /* CONFIG_BT_HCI_VS */

static int hci_init(void)
{
	int err;

#if defined(CONFIG_BT_HCI_SETUP)
	struct bt_hci_setup_params setup_params = { 0 };

	bt_addr_copy(&setup_params.public_addr, BT_ADDR_ANY);
#if defined(CONFIG_BT_HCI_SET_PUBLIC_ADDR)
	if (bt_dev.id_count > 0 && bt_dev.id_addr[BT_ID_DEFAULT].type == BT_ADDR_LE_PUBLIC) {
		bt_addr_copy(&setup_params.public_addr, &bt_dev.id_addr[BT_ID_DEFAULT].a);
	}
#endif /* defined(CONFIG_BT_HCI_SET_PUBLIC_ADDR) */

#if DT_HAS_CHOSEN(zephyr_bt_hci)
	err = bt_hci_setup(bt_dev.hci, &setup_params);
	if (err && err != -ENOSYS) {
		return err;
	}
#else
	if (bt_dev.drv->setup) {
		err = bt_dev.drv->setup(&setup_params);
		if (err) {
			return err;
		}
	}
#endif
#endif /* defined(CONFIG_BT_HCI_SETUP) */

	err = common_init();
	if (err) {
		return err;
	}

	err = le_init();
	if (err) {
		return err;
	}

	if (BT_FEAT_BREDR(bt_dev.features)) {
		err = bt_br_init();
		if (err) {
			return err;
		}
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		LOG_ERR("Non-BR/EDR controller detected");
		return -EIO;
	}
#if defined(CONFIG_BT_CONN)
	else if (!bt_dev.le.acl_mtu) {
		LOG_ERR("ACL BR/EDR buffers not initialized");
		return -EIO;
	}
#endif

	err = set_event_mask();
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_HCI_VS)
	hci_vs_init();
#endif
	err = bt_id_init();
	if (err) {
		return err;
	}

	return 0;
}

int bt_send(struct net_buf *buf)
{
	LOG_DBG("buf %p len %u type %u", buf, buf->len, bt_buf_get_type(buf));

	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	if (IS_ENABLED(CONFIG_BT_TINYCRYPT_ECC)) {
		return bt_hci_ecc_send(buf);
	}

#if DT_HAS_CHOSEN(zephyr_bt_hci)
	return bt_hci_send(bt_dev.hci, buf);
#else
	return bt_dev.drv->send(buf);
#endif
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
	EVENT_HANDLER(BT_HCI_EVT_DISCONN_COMPLETE, hci_disconn_complete_prio,
		      sizeof(struct bt_hci_evt_disconn_complete)),
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_CONN_TX)
	EVENT_HANDLER(BT_HCI_EVT_NUM_COMPLETED_PACKETS,
		      hci_num_completed_packets,
		      sizeof(struct bt_hci_evt_num_completed_packets)),
#endif /* CONFIG_BT_CONN_TX */
};

void hci_event_prio(struct net_buf *buf)
{
	struct net_buf_simple_state state;
	struct bt_hci_evt_hdr *hdr;
	uint8_t evt_flags;

	net_buf_simple_save(&buf->b, &state);

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Invalid HCI event size (%u)", buf->len);
		net_buf_unref(buf);
		return;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	evt_flags = bt_hci_evt_get_flags(hdr->evt);
	BT_ASSERT(evt_flags & BT_HCI_EVT_FLAG_RECV_PRIO);

	handle_event(hdr->evt, buf, prio_events, ARRAY_SIZE(prio_events));

	if (evt_flags & BT_HCI_EVT_FLAG_RECV) {
		net_buf_simple_restore(&buf->b, &state);
	} else {
		net_buf_unref(buf);
	}
}

static void rx_queue_put(struct net_buf *buf)
{
	net_buf_slist_put(&bt_dev.rx_queue, buf);

#if defined(CONFIG_BT_RECV_WORKQ_SYS)
	const int err = k_work_submit(&rx_work);
#elif defined(CONFIG_BT_RECV_WORKQ_BT)
	const int err = k_work_submit_to_queue(&bt_workq, &rx_work);
#endif /* CONFIG_BT_RECV_WORKQ_SYS */
	if (err < 0) {
		LOG_ERR("Could not submit rx_work: %d", err);
	}
}

static int bt_recv_unsafe(struct net_buf *buf)
{
	bt_monitor_send(bt_monitor_opcode(buf), buf->data, buf->len);

	LOG_DBG("buf %p len %u", buf, buf->len);

	switch (bt_buf_get_type(buf)) {
#if defined(CONFIG_BT_CONN)
	case BT_BUF_ACL_IN:
		rx_queue_put(buf);
		return 0;
#endif /* BT_CONN */
	case BT_BUF_EVT:
	{
		struct bt_hci_evt_hdr *hdr = (void *)buf->data;
		uint8_t evt_flags = bt_hci_evt_get_flags(hdr->evt);

		if (evt_flags & BT_HCI_EVT_FLAG_RECV_PRIO) {
			hci_event_prio(buf);
		}

		if (evt_flags & BT_HCI_EVT_FLAG_RECV) {
			rx_queue_put(buf);
		}

		return 0;
	}
#if defined(CONFIG_BT_ISO)
	case BT_BUF_ISO_IN:
		rx_queue_put(buf);
		return 0;
#endif /* CONFIG_BT_ISO */
	default:
		LOG_ERR("Invalid buf type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return -EINVAL;
	}
}

#if DT_HAS_CHOSEN(zephyr_bt_hci)
int bt_hci_recv(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);
#else
int bt_recv(struct net_buf *buf)
{
#endif
	int err;

	k_sched_lock();
	err = bt_recv_unsafe(buf);
	k_sched_unlock();

	return err;
}

/* Old-style HCI driver registration */
#if !DT_HAS_CHOSEN(zephyr_bt_hci)
int bt_hci_driver_register(const struct bt_hci_driver *drv)
{
	if (bt_dev.drv) {
		return -EALREADY;
	}

	if (!drv->open || !drv->send) {
		return -EINVAL;
	}

	bt_dev.drv = drv;

	LOG_DBG("Registered %s", drv->name ? drv->name : "");

	bt_monitor_new_index(BT_MONITOR_TYPE_PRIMARY, drv->bus,
			     BT_ADDR_ANY, drv->name ? drv->name : "bt0");

	return 0;
}
#endif /* !DT_HAS_CHOSEN(zephyr_bt_hci) */

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

	if (IS_ENABLED(CONFIG_BT_ISO)) {
		err = bt_conn_iso_init();
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		if (!bt_dev.id_count) {
			LOG_INF("No ID address. App must call settings_load()");
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

static void rx_work_handler(struct k_work *work)
{
	int err;

	struct net_buf *buf;

	LOG_DBG("Getting net_buf from queue");
	buf = net_buf_slist_get(&bt_dev.rx_queue);
	if (!buf) {
		return;
	}

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
#if defined(CONFIG_BT_CONN)
	case BT_BUF_ACL_IN:
		hci_acl(buf);
		break;
#endif /* CONFIG_BT_CONN */
#if defined(CONFIG_BT_ISO)
	case BT_BUF_ISO_IN:
		hci_iso(buf);
		break;
#endif /* CONFIG_BT_ISO */
	case BT_BUF_EVT:
		hci_event(buf);
		break;
	default:
		LOG_ERR("Unknown buf type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		break;
	}

	/* Schedule the work handler to be executed again if there are
	 * additional items in the queue. This allows for other users of the
	 * work queue to get a chance at running, which wouldn't be possible if
	 * we used a while() loop with a k_yield() statement.
	 */
	if (!sys_slist_is_empty(&bt_dev.rx_queue)) {

#if defined(CONFIG_BT_RECV_WORKQ_SYS)
		err = k_work_submit(&rx_work);
#elif defined(CONFIG_BT_RECV_WORKQ_BT)
		err = k_work_submit_to_queue(&bt_workq, &rx_work);
#endif
		if (err < 0) {
			LOG_ERR("Could not submit rx_work: %d", err);
		}
	}
}

#if defined(CONFIG_BT_TESTING)
k_tid_t bt_testing_tx_tid_get(void)
{
	/* We now TX everything from the syswq */
	return &k_sys_work_q.thread;
}

#if defined(CONFIG_BT_ISO)
void bt_testing_set_iso_mtu(uint16_t mtu)
{
	bt_dev.le.iso_mtu = mtu;
}
#endif /* CONFIG_BT_ISO */
#endif /* CONFIG_BT_TESTING */

int bt_enable(bt_ready_cb_t cb)
{
	int err;

#if DT_HAS_CHOSEN(zephyr_bt_hci)
	if (!device_is_ready(bt_dev.hci)) {
		LOG_ERR("HCI driver is not ready");
		return -ENODEV;
	}

	bt_monitor_new_index(BT_MONITOR_TYPE_PRIMARY, BT_HCI_BUS, BT_ADDR_ANY, BT_HCI_NAME);
#else /* !DT_HAS_CHONSEN(zephyr_bt_hci) */
	if (!bt_dev.drv) {
		LOG_ERR("No HCI driver registered");
		return -ENODEV;
	}
#endif

	atomic_clear_bit(bt_dev.flags, BT_DEV_DISABLE);

	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_ENABLE)) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		err = bt_settings_init();
		if (err) {
			return err;
		}
	} else if (IS_ENABLED(CONFIG_BT_DEVICE_NAME_DYNAMIC)) {
		err = bt_set_name(CONFIG_BT_DEVICE_NAME);
		if (err) {
			LOG_WRN("Failed to set device name (%d)", err);
		}
	}

	ready_cb = cb;

	/* Give cmd_sem allowing to send first HCI_Reset cmd, the only
	 * exception is if the controller requests to wait for an
	 * initial Command Complete for NOP.
	 */
	if (!IS_ENABLED(CONFIG_BT_WAIT_NOP)) {
		k_sem_init(&bt_dev.ncmd_sem, 1, 1);
	} else {
		k_sem_init(&bt_dev.ncmd_sem, 0, 1);
	}
	k_fifo_init(&bt_dev.cmd_tx_queue);

#if defined(CONFIG_BT_RECV_WORKQ_BT)
	/* RX thread */
	k_work_queue_init(&bt_workq);
	k_work_queue_start(&bt_workq, rx_thread_stack,
			   CONFIG_BT_RX_STACK_SIZE,
			   K_PRIO_COOP(CONFIG_BT_RX_PRIO), NULL);
	k_thread_name_set(&bt_workq.thread, "BT RX WQ");
#endif

#if DT_HAS_CHOSEN(zephyr_bt_hci)
	err = bt_hci_open(bt_dev.hci, bt_hci_recv);
#else
	err = bt_dev.drv->open();
#endif
	if (err) {
		LOG_ERR("HCI driver open failed (%d)", err);
		return err;
	}

	bt_monitor_send(BT_MONITOR_OPEN_INDEX, NULL, 0);

	if (!cb) {
		return bt_init();
	}

	k_work_submit(&bt_dev.init);
	return 0;
}

int bt_disable(void)
{
	int err;

#if !DT_HAS_CHOSEN(zephyr_bt_hci)
	if (!bt_dev.drv) {
		LOG_ERR("No HCI driver registered");
		return -ENODEV;
	}

	if (!bt_dev.drv->close) {
		return -ENOTSUP;
	}
#endif

	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_DISABLE)) {
		return -EALREADY;
	}

	/* Clear BT_DEV_READY before disabling HCI link */
	atomic_clear_bit(bt_dev.flags, BT_DEV_READY);

#if defined(CONFIG_BT_BROADCASTER)
	bt_adv_reset_adv_pool();
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_PRIVACY)
	k_work_cancel_delayable(&bt_dev.rpa_update);
#endif /* CONFIG_BT_PRIVACY */

#if defined(CONFIG_BT_PER_ADV_SYNC)
	bt_periodic_sync_disable();
#endif /* CONFIG_BT_PER_ADV_SYNC */

#if defined(CONFIG_BT_CONN)
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		bt_pub_key_hci_disrupted();
	}
	bt_conn_cleanup_all();
	disconnected_handles_reset();
#endif /* CONFIG_BT_CONN */

#if DT_HAS_CHOSEN(zephyr_bt_hci)
	err = bt_hci_close(bt_dev.hci);
	if (err == -ENOSYS) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_DISABLE);
		atomic_set_bit(bt_dev.flags, BT_DEV_READY);
		return -ENOTSUP;
	}
#else
	err = bt_dev.drv->close();
#endif
	if (err) {
		LOG_ERR("HCI driver close failed (%d)", err);

		/* Re-enable BT_DEV_READY to avoid inconsistent stack state */
		atomic_set_bit(bt_dev.flags, BT_DEV_READY);

		return err;
	}

#if defined(CONFIG_BT_RECV_WORKQ_BT)
	/* Abort RX thread */
	k_thread_abort(&bt_workq.thread);
#endif

	/* Some functions rely on checking this bitfield */
	memset(bt_dev.supported_commands, 0x00, sizeof(bt_dev.supported_commands));

	/* Reset IDs and corresponding keys. */
	bt_dev.id_count = 0;
#if defined(CONFIG_BT_SMP)
	bt_dev.le.rl_entries = 0;
	bt_keys_reset();
#endif

	/* If random address was set up - clear it */
	bt_addr_le_copy(&bt_dev.random_addr, BT_ADDR_LE_ANY);

	if (IS_ENABLED(CONFIG_BT_ISO)) {
		bt_iso_reset();
	}

	bt_monitor_send(BT_MONITOR_CLOSE_INDEX, NULL, 0);

	/* Clear BT_DEV_ENABLE here to prevent early bt_enable() calls, before disable is
	 * completed.
	 */
	atomic_clear_bit(bt_dev.flags, BT_DEV_ENABLE);

	return 0;
}

bool bt_is_ready(void)
{
	return atomic_test_bit(bt_dev.flags, BT_DEV_READY);
}

#define DEVICE_NAME_LEN (sizeof(CONFIG_BT_DEVICE_NAME) - 1)
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
BUILD_ASSERT(DEVICE_NAME_LEN < CONFIG_BT_DEVICE_NAME_MAX);
#else
BUILD_ASSERT(DEVICE_NAME_LEN < 248);
#endif

int bt_set_name(const char *name)
{
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	size_t len = strlen(name);
	int err;

	if (len > CONFIG_BT_DEVICE_NAME_MAX) {
		return -ENOMEM;
	}

	if (!strcmp(bt_dev.name, name)) {
		return 0;
	}

	memcpy(bt_dev.name, name, len);
	bt_dev.name[len] = '\0';

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		err = bt_settings_store_name(bt_dev.name, len);
		if (err) {
			LOG_WRN("Unable to store name");
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

uint16_t bt_get_appearance(void)
{
#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
	return bt_dev.appearance;
#else
	return CONFIG_BT_DEVICE_APPEARANCE;
#endif
}

#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
int bt_set_appearance(uint16_t appearance)
{
	if (bt_dev.appearance != appearance) {
		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			int err = bt_settings_store_appearance(&appearance, sizeof(appearance));
			if (err) {
				LOG_ERR("Unable to save setting 'bt/appearance' (err %d).", err);
				return err;
			}
		}

		bt_dev.appearance = appearance;
	}

	return 0;
}
#endif

bool bt_addr_le_is_bonded(uint8_t id, const bt_addr_le_t *addr)
{
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		struct bt_keys *keys = bt_keys_find_addr(id, addr);

		/* if there are any keys stored then device is bonded */
		return keys && keys->keys;
	} else {
		return false;
	}
}

#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
int bt_le_filter_accept_list_add(const bt_addr_le_t *addr)
{
	struct bt_hci_cp_le_add_dev_to_fal *cp;
	struct net_buf *buf;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_ADD_DEV_TO_FAL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->addr, addr);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_ADD_DEV_TO_FAL, buf, NULL);
	if (err) {
		LOG_ERR("Failed to add device to filter accept list");

		return err;
	}

	return 0;
}

int bt_le_filter_accept_list_remove(const bt_addr_le_t *addr)
{
	struct bt_hci_cp_le_rem_dev_from_fal *cp;
	struct net_buf *buf;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REM_DEV_FROM_FAL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->addr, addr);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REM_DEV_FROM_FAL, buf, NULL);
	if (err) {
		LOG_ERR("Failed to remove device from filter accept list");
		return err;
	}

	return 0;
}

int bt_le_filter_accept_list_clear(void)
{
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_CLEAR_FAL, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to clear filter accept list");
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_BT_FILTER_ACCEPT_LIST) */

int bt_le_set_chan_map(uint8_t chan_map[5])
{
	struct bt_hci_cp_le_set_host_chan_classif *cp;
	struct net_buf *buf;

	if (!(IS_ENABLED(CONFIG_BT_CENTRAL) || IS_ENABLED(CONFIG_BT_BROADCASTER))) {
		return -ENOTSUP;
	}

	if (!BT_CMD_TEST(bt_dev.supported_commands, 27, 3)) {
		LOG_WRN("Set Host Channel Classification command is "
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

#if defined(CONFIG_BT_RPA_TIMEOUT_DYNAMIC)
int bt_le_set_rpa_timeout(uint16_t new_rpa_timeout)
{
	if ((new_rpa_timeout == 0) || (new_rpa_timeout > 3600)) {
		return -EINVAL;
	}

	if (new_rpa_timeout == bt_dev.rpa_timeout) {
		return 0;
	}

	bt_dev.rpa_timeout = new_rpa_timeout;
	atomic_set_bit(bt_dev.flags, BT_DEV_RPA_TIMEOUT_CHANGED);

	return 0;
}
#endif

int bt_configure_data_path(uint8_t dir, uint8_t id, uint8_t vs_config_len,
			   const uint8_t *vs_config)
{
	struct bt_hci_rp_configure_data_path *rp;
	struct bt_hci_cp_configure_data_path *cp;
	struct net_buf *rsp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_CONFIGURE_DATA_PATH, sizeof(*cp) +
				vs_config_len);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->data_path_dir = dir;
	cp->data_path_id  = id;
	cp->vs_config_len = vs_config_len;
	if (vs_config_len) {
		(void)memcpy(cp->vs_config, vs_config, vs_config_len);
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_CONFIGURE_DATA_PATH, buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	if (rp->status) {
		err = -EIO;
	}
	net_buf_unref(rsp);

	return err;
}

/* Return `true` if a command was processed/sent */
static bool process_pending_cmd(k_timeout_t timeout)
{
	if (!k_fifo_is_empty(&bt_dev.cmd_tx_queue)) {
		if (k_sem_take(&bt_dev.ncmd_sem, timeout) == 0) {
			hci_core_send_cmd();
			return true;
		}
	}

	return false;
}

static void tx_processor(struct k_work *item)
{
	LOG_DBG("TX process start");
	if (process_pending_cmd(K_NO_WAIT)) {
		/* If we processed a command, let the scheduler run before
		 * processing another command (or data).
		 */
		bt_tx_irq_raise();
		return;
	}

	/* Hand over control to conn to process pending data */
	if (IS_ENABLED(CONFIG_BT_CONN_TX)) {
		bt_conn_tx_processor();
	}
}

static K_WORK_DEFINE(tx_work, tx_processor);

void bt_tx_irq_raise(void)
{
	LOG_DBG("kick TX");
	k_work_submit(&tx_work);
}
