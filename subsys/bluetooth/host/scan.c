/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/types.h>

#include <sys/byteorder.h>
#include <sys/check.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/iso.h>
#include <bluetooth/buf.h>
#include <bluetooth/direction.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "direction_internal.h"
#include "id.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_scan
#include "common/log.h"

static bt_le_scan_cb_t *scan_dev_found_cb;
static sys_slist_t scan_cbs = SYS_SLIST_STATIC_INIT(&scan_cbs);

#if defined(CONFIG_BT_EXT_ADV)
#if defined(CONFIG_BT_PER_ADV_SYNC)
static struct bt_le_per_adv_sync *get_pending_per_adv_sync(void);
static struct bt_le_per_adv_sync per_adv_sync_pool[CONFIG_BT_PER_ADV_SYNC_MAX];
static sys_slist_t pa_sync_cbs = SYS_SLIST_STATIC_INIT(&pa_sync_cbs);
#endif /* defined(CONFIG_BT_PER_ADV_SYNC) */
#endif /* defined(CONFIG_BT_EXT_ADV) */

void bt_scan_reset(void)
{
	scan_dev_found_cb = NULL;
}

static int set_le_ext_scan_enable(uint8_t enable, uint16_t duration)
{
	struct bt_hci_cp_le_set_ext_scan_enable *cp;
	struct bt_hci_cmd_state_set state;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_EXT_SCAN_ENABLE, sizeof(*cp));
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
	cp->duration = sys_cpu_to_le16(duration);
	cp->period = 0;

	bt_hci_cmd_state_set_init(buf, &state, bt_dev.flags, BT_DEV_SCANNING,
				  enable == BT_HCI_LE_SCAN_ENABLE);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EXT_SCAN_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int bt_le_scan_set_enable_legacy(uint8_t enable)
{
	struct bt_hci_cp_le_set_scan_enable *cp;
	struct bt_hci_cmd_state_set state;
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

	bt_hci_cmd_state_set_init(buf, &state, bt_dev.flags, BT_DEV_SCANNING,
				  enable == BT_HCI_LE_SCAN_ENABLE);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_SCAN_ENABLE, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

int bt_le_scan_set_enable(uint8_t enable)
{
	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		return set_le_ext_scan_enable(enable, 0);
	}

	return bt_le_scan_set_enable_legacy(enable);
}

static int start_le_scan_ext(struct bt_hci_ext_scan_phy *phy_1m,
			     struct bt_hci_ext_scan_phy *phy_coded,
			     uint16_t duration)
{
	struct bt_hci_cp_le_set_ext_scan_param *set_param;
	struct net_buf *buf;
	uint8_t own_addr_type;
	bool active_scan;
	int err;

	active_scan = (phy_1m && phy_1m->type == BT_HCI_LE_SCAN_ACTIVE) ||
		      (phy_coded && phy_coded->type == BT_HCI_LE_SCAN_ACTIVE);

	if (duration > 0) {
		atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_LIMITED);

		/* Allow bt_le_oob_get_local to be called directly before
		 * starting a scan limited by timeout.
		 */
		if (IS_ENABLED(CONFIG_BT_PRIVACY) && !bt_id_rpa_is_new()) {
			atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);
		}
	}

	err = bt_id_set_scan_own_addr(active_scan, &own_addr_type);
	if (err) {
		return err;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_EXT_SCAN_PARAM,
				sizeof(*set_param) +
				(phy_1m ? sizeof(*phy_1m) : 0) +
				(phy_coded ? sizeof(*phy_coded) : 0));
	if (!buf) {
		return -ENOBUFS;
	}

	set_param = net_buf_add(buf, sizeof(*set_param));
	set_param->own_addr_type = own_addr_type;
	set_param->phys = 0;

	if (IS_ENABLED(CONFIG_BT_WHITELIST) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_SCAN_WL)) {
		set_param->filter_policy = BT_HCI_LE_SCAN_FP_USE_WHITELIST;
	} else {
		set_param->filter_policy = BT_HCI_LE_SCAN_FP_NO_WHITELIST;
	}

	if (phy_1m) {
		set_param->phys |= BT_HCI_LE_EXT_SCAN_PHY_1M;
		net_buf_add_mem(buf, phy_1m, sizeof(*phy_1m));
	}

	if (phy_coded) {
		set_param->phys |= BT_HCI_LE_EXT_SCAN_PHY_CODED;
		net_buf_add_mem(buf, phy_coded, sizeof(*phy_coded));
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_EXT_SCAN_PARAM, buf, NULL);
	if (err) {
		return err;
	}

	err = set_le_ext_scan_enable(BT_HCI_LE_SCAN_ENABLE, duration);
	if (err) {
		return err;
	}

	atomic_set_bit_to(bt_dev.flags, BT_DEV_ACTIVE_SCAN, active_scan);

	return 0;
}

static int start_le_scan_legacy(uint8_t scan_type, uint16_t interval, uint16_t window)
{
	struct bt_hci_cp_le_set_scan_param set_param;
	struct net_buf *buf;
	int err;
	bool active_scan;

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

	active_scan = scan_type == BT_HCI_LE_SCAN_ACTIVE;
	err = bt_id_set_scan_own_addr(active_scan, &set_param.addr_type);
	if (err) {
		return err;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_PARAM, sizeof(set_param));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_mem(buf, &set_param, sizeof(set_param));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_SCAN_PARAM, buf, NULL);
	if (err) {
		return err;
	}

	err = bt_le_scan_set_enable(BT_HCI_LE_SCAN_ENABLE);
	if (err) {
		return err;
	}

	atomic_set_bit_to(bt_dev.flags, BT_DEV_ACTIVE_SCAN, active_scan);

	return 0;
}

static int start_passive_scan(bool fast_scan)
{
	uint16_t interval, window;

	if (fast_scan) {
		interval = BT_GAP_SCAN_FAST_INTERVAL;
		window = BT_GAP_SCAN_FAST_WINDOW;
	} else {
		interval = CONFIG_BT_BACKGROUND_SCAN_INTERVAL;
		window = CONFIG_BT_BACKGROUND_SCAN_WINDOW;
	}

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		struct bt_hci_ext_scan_phy scan;

		scan.type = BT_HCI_LE_SCAN_PASSIVE;
		scan.interval = sys_cpu_to_le16(interval);
		scan.window = sys_cpu_to_le16(window);

		return start_le_scan_ext(&scan, NULL, 0);
	}

	return start_le_scan_legacy(BT_HCI_LE_SCAN_PASSIVE, interval, window);
}

int bt_le_scan_update(bool fast_scan)
{
	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return 0;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		int err;

		err = bt_le_scan_set_enable(BT_HCI_LE_SCAN_DISABLE);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		struct bt_conn *conn;

		/* don't restart scan if we have pending connection */
		conn = bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL,
					       BT_CONN_CONNECT);
		if (conn) {
			bt_conn_unref(conn);
			return 0;
		}

		conn = bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL,
					       BT_CONN_CONNECT_SCAN);
		if (conn) {
			atomic_set_bit(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP);

			bt_conn_unref(conn);

			return start_passive_scan(fast_scan);
		}
	}

#if defined(CONFIG_BT_PER_ADV_SYNC)
	if (get_pending_per_adv_sync()) {
		return start_passive_scan(fast_scan);
	}
#endif

	return 0;
}

#if defined(CONFIG_BT_CENTRAL)
static void check_pending_conn(const bt_addr_le_t *id_addr,
			       const bt_addr_le_t *addr, uint8_t adv_props)
{
	struct bt_conn *conn;

	/* No connections are allowed during explicit scanning */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return;
	}

	/* Return if event is not connectable */
	if (!(adv_props & BT_HCI_LE_ADV_EVT_TYPE_CONN)) {
		return;
	}

	conn = bt_conn_lookup_state_le(BT_ID_DEFAULT, id_addr,
				       BT_CONN_CONNECT_SCAN);
	if (!conn) {
		return;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING) &&
	    bt_le_scan_set_enable(BT_HCI_LE_SCAN_DISABLE)) {
		goto failed;
	}

	bt_addr_le_copy(&conn->le.resp_addr, addr);
	if (bt_le_create_conn(conn)) {
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

/* Convert Legacy adv report evt_type field to adv props */
static uint8_t get_adv_props(uint8_t evt_type)
{
	switch (evt_type) {
	case BT_GAP_ADV_TYPE_ADV_IND:
		return BT_GAP_ADV_PROP_CONNECTABLE |
		       BT_GAP_ADV_PROP_SCANNABLE;

	case BT_GAP_ADV_TYPE_ADV_DIRECT_IND:
		return BT_GAP_ADV_PROP_CONNECTABLE |
		       BT_GAP_ADV_PROP_DIRECTED;

	case BT_GAP_ADV_TYPE_ADV_SCAN_IND:
		return BT_GAP_ADV_PROP_SCANNABLE;

	case BT_GAP_ADV_TYPE_ADV_NONCONN_IND:
		return 0;

	/* In legacy advertising report, we don't know if the scan
	 * response come from a connectable advertiser, so don't
	 * set connectable property bit.
	 */
	case BT_GAP_ADV_TYPE_SCAN_RSP:
		return BT_GAP_ADV_PROP_SCAN_RESPONSE |
		       BT_GAP_ADV_PROP_SCANNABLE;

	default:
		return 0;
	}
}

static void le_adv_recv(bt_addr_le_t *addr, struct bt_le_scan_recv_info *info,
			struct net_buf *buf, uint8_t len)
{
	struct bt_le_scan_cb *listener, *next;
	struct net_buf_simple_state state;
	bt_addr_le_t id_addr;

	BT_DBG("%s event %u, len %u, rssi %d dBm", bt_addr_le_str(addr),
	       info->adv_type, len, info->rssi);

	if (!IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    !IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN) &&
	    (info->adv_props & BT_HCI_LE_ADV_PROP_DIRECT)) {
		BT_DBG("Dropped direct adv report");
		return;
	}

	if (addr->type == BT_ADDR_LE_PUBLIC_ID ||
	    addr->type == BT_ADDR_LE_RANDOM_ID) {
		bt_addr_le_copy(&id_addr, addr);
		id_addr.type -= BT_ADDR_LE_PUBLIC_ID;
	} else if (addr->type == BT_HCI_PEER_ADDR_ANONYMOUS) {
		bt_addr_le_copy(&id_addr, BT_ADDR_LE_ANY);
	} else {
		bt_addr_le_copy(&id_addr,
				bt_lookup_id_addr(BT_ID_DEFAULT, addr));
	}

	info->addr = &id_addr;

	if (scan_dev_found_cb) {
		net_buf_simple_save(&buf->b, &state);

		buf->len = len;
		scan_dev_found_cb(&id_addr, info->rssi, info->adv_type,
				  &buf->b);

		net_buf_simple_restore(&buf->b, &state);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&scan_cbs, listener, next, node) {
		if (listener->recv) {
			net_buf_simple_save(&buf->b, &state);

			buf->len = len;
			listener->recv(info, &buf->b);

			net_buf_simple_restore(&buf->b, &state);
		}
	}

#if defined(CONFIG_BT_CENTRAL)
	check_pending_conn(&id_addr, addr, info->adv_props);
#endif /* CONFIG_BT_CENTRAL */
}

#if defined(CONFIG_BT_EXT_ADV)
void bt_hci_le_scan_timeout(struct net_buf *buf)
{
	struct bt_le_scan_cb *listener, *next;

	atomic_clear_bit(bt_dev.flags, BT_DEV_SCANNING);
	atomic_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN);

	atomic_clear_bit(bt_dev.flags, BT_DEV_SCAN_LIMITED);
	atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);

#if defined(CONFIG_BT_SMP)
	bt_id_pending_keys_update();
#endif

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&scan_cbs, listener, next, node) {
		if (listener->timeout) {
			listener->timeout();
		}
	}
}

/* Convert Extended adv report evt_type field into adv type */
static uint8_t get_adv_type(uint8_t evt_type)
{
	switch (evt_type) {
	case (BT_HCI_LE_ADV_EVT_TYPE_CONN |
	      BT_HCI_LE_ADV_EVT_TYPE_SCAN |
	      BT_HCI_LE_ADV_EVT_TYPE_LEGACY):
		return BT_GAP_ADV_TYPE_ADV_IND;

	case (BT_HCI_LE_ADV_EVT_TYPE_CONN |
	      BT_HCI_LE_ADV_EVT_TYPE_DIRECT |
	      BT_HCI_LE_ADV_EVT_TYPE_LEGACY):
		return BT_GAP_ADV_TYPE_ADV_DIRECT_IND;

	case (BT_HCI_LE_ADV_EVT_TYPE_SCAN |
	      BT_HCI_LE_ADV_EVT_TYPE_LEGACY):
		return BT_GAP_ADV_TYPE_ADV_SCAN_IND;

	case BT_HCI_LE_ADV_EVT_TYPE_LEGACY:
		return BT_GAP_ADV_TYPE_ADV_NONCONN_IND;

	case (BT_HCI_LE_ADV_EVT_TYPE_SCAN_RSP |
	      BT_HCI_LE_ADV_EVT_TYPE_CONN |
	      BT_HCI_LE_ADV_EVT_TYPE_SCAN |
	      BT_HCI_LE_ADV_EVT_TYPE_LEGACY):
	case (BT_HCI_LE_ADV_EVT_TYPE_SCAN_RSP |
	      BT_HCI_LE_ADV_EVT_TYPE_SCAN |
	      BT_HCI_LE_ADV_EVT_TYPE_LEGACY):
		/* Scan response from connectable or non-connectable advertiser.
		 */
		return BT_GAP_ADV_TYPE_SCAN_RSP;

	default:
		return BT_GAP_ADV_TYPE_EXT_ADV;
	}
}

void bt_hci_le_adv_ext_report(struct net_buf *buf)
{
	uint8_t num_reports = net_buf_pull_u8(buf);

	BT_DBG("Adv number of reports %u",  num_reports);

	while (num_reports--) {
		struct bt_hci_evt_le_ext_advertising_info *evt;
		struct bt_le_scan_recv_info adv_info;

		if (buf->len < sizeof(*evt)) {
			BT_ERR("Unexpected end of buffer");
			break;
		}

		evt = net_buf_pull_mem(buf, sizeof(*evt));

		adv_info.primary_phy = bt_get_phy(evt->prim_phy);
		adv_info.secondary_phy = bt_get_phy(evt->sec_phy);
		adv_info.tx_power = evt->tx_power;
		adv_info.rssi = evt->rssi;
		adv_info.sid = evt->sid;
		adv_info.interval = sys_le16_to_cpu(evt->interval);

		adv_info.adv_type = get_adv_type(evt->evt_type);
		/* Convert "Legacy" property to Extended property. */
		adv_info.adv_props = evt->evt_type ^ BT_HCI_LE_ADV_PROP_LEGACY;

		le_adv_recv(&evt->addr, &adv_info, buf, evt->length);

		net_buf_pull(buf, evt->length);
	}
}


#if defined(CONFIG_BT_PER_ADV_SYNC)
static void per_adv_sync_delete(struct bt_le_per_adv_sync *per_adv_sync)
{
	atomic_clear(per_adv_sync->flags);
}

static struct bt_le_per_adv_sync *per_adv_sync_new(void)
{
	struct bt_le_per_adv_sync *per_adv_sync = NULL;

	for (int i = 0; i < ARRAY_SIZE(per_adv_sync_pool); i++) {
		if (!atomic_test_bit(per_adv_sync_pool[i].flags,
				     BT_PER_ADV_SYNC_CREATED)) {
			per_adv_sync = &per_adv_sync_pool[i];
			break;
		}
	}

	if (!per_adv_sync) {
		return NULL;
	}

	(void)memset(per_adv_sync, 0, sizeof(*per_adv_sync));
	atomic_set_bit(per_adv_sync->flags, BT_PER_ADV_SYNC_CREATED);

	return per_adv_sync;
}

static struct bt_le_per_adv_sync *get_pending_per_adv_sync(void)
{
	for (int i = 0; i < ARRAY_SIZE(per_adv_sync_pool); i++) {
		if (atomic_test_bit(per_adv_sync_pool[i].flags,
				    BT_PER_ADV_SYNC_SYNCING)) {
			return &per_adv_sync_pool[i];
		}
	}

	return NULL;
}

struct bt_le_per_adv_sync *bt_hci_get_per_adv_sync(uint16_t handle)
{
	for (int i = 0; i < ARRAY_SIZE(per_adv_sync_pool); i++) {
		if (per_adv_sync_pool[i].handle == handle &&
		    atomic_test_bit(per_adv_sync_pool[i].flags,
				    BT_PER_ADV_SYNC_SYNCED)) {
			return &per_adv_sync_pool[i];
		}
	}

	return NULL;
}

void bt_hci_le_per_adv_report(struct net_buf *buf)
{
	struct bt_hci_evt_le_per_advertising_report *evt;
	struct bt_le_per_adv_sync *per_adv_sync;
	struct bt_le_per_adv_sync_recv_info info;
	struct bt_le_per_adv_sync_cb *listener;
	struct net_buf_simple_state state;

	if (buf->len < sizeof(*evt)) {
		BT_ERR("Unexpected end of buffer");
		return;
	}

	evt = net_buf_pull_mem(buf, sizeof(*evt));

	per_adv_sync = bt_hci_get_per_adv_sync(sys_le16_to_cpu(evt->handle));

	if (!per_adv_sync) {
		BT_ERR("Unknown handle 0x%04X for periodic advertising report",
		       sys_le16_to_cpu(evt->handle));
		return;
	}

	if (atomic_test_bit(per_adv_sync->flags,
			    BT_PER_ADV_SYNC_RECV_DISABLED)) {
		BT_ERR("Received PA adv report when receive disabled");
		return;
	}

	info.tx_power = evt->tx_power;
	info.rssi = evt->rssi;
	info.cte_type = BIT(evt->cte_type);
	info.addr = &per_adv_sync->addr;

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->recv) {
			net_buf_simple_save(&buf->b, &state);

			buf->len = evt->length;
			listener->recv(per_adv_sync, &info, &buf->b);

			net_buf_simple_restore(&buf->b, &state);
		}
	}
}

static int per_adv_sync_terminate(uint16_t handle)
{
	struct bt_hci_cp_le_per_adv_terminate_sync *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_PER_ADV_TERMINATE_SYNC,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->handle = sys_cpu_to_le16(handle);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_PER_ADV_TERMINATE_SYNC, buf,
				    NULL);
}

void bt_hci_le_per_adv_sync_established(struct net_buf *buf)
{
	struct bt_hci_evt_le_per_adv_sync_established *evt =
		(struct bt_hci_evt_le_per_adv_sync_established *)buf->data;
	struct bt_le_per_adv_sync_synced_info sync_info;
	struct bt_le_per_adv_sync *pending_per_adv_sync;
	struct bt_le_per_adv_sync_cb *listener;
	int err;

	pending_per_adv_sync = get_pending_per_adv_sync();

	if (pending_per_adv_sync) {
		atomic_clear_bit(pending_per_adv_sync->flags,
				 BT_PER_ADV_SYNC_SYNCING);
		err = bt_le_scan_update(false);

		if (err) {
			BT_ERR("Could not update scan (%d)", err);
		}
	}

	if (evt->status == BT_HCI_ERR_OP_CANCELLED_BY_HOST) {
		/* Cancelled locally, don't call CB */
		if (pending_per_adv_sync) {
			per_adv_sync_delete(pending_per_adv_sync);
		} else {
			BT_ERR("Unexpected per adv sync cancelled event");
		}

		return;
	}

	if (!pending_per_adv_sync ||
	    pending_per_adv_sync->sid != evt->sid ||
	    bt_addr_le_cmp(&pending_per_adv_sync->addr, &evt->adv_addr)) {
		struct bt_le_per_adv_sync_term_info term_info;

		BT_ERR("Unexpected per adv sync established event");
		per_adv_sync_terminate(sys_le16_to_cpu(evt->handle));

		if (pending_per_adv_sync) {
			/* Terminate the pending PA sync and notify app */
			term_info.addr = &pending_per_adv_sync->addr;
			term_info.sid = pending_per_adv_sync->sid;

			/* Deleting before callback, so the caller will be able
			 * to restart sync in the callback.
			 */
			per_adv_sync_delete(pending_per_adv_sync);


			SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs,
						     listener,
						     node) {
				if (listener->term) {
					listener->term(pending_per_adv_sync,
						       &term_info);
				}
			}
		}
		return;
	}

	atomic_set_bit(pending_per_adv_sync->flags, BT_PER_ADV_SYNC_SYNCED);

	pending_per_adv_sync->handle = sys_le16_to_cpu(evt->handle);
	pending_per_adv_sync->interval = sys_le16_to_cpu(evt->interval);
	pending_per_adv_sync->clock_accuracy =
		sys_le16_to_cpu(evt->clock_accuracy);
	pending_per_adv_sync->phy = evt->phy;

	memset(&sync_info, 0, sizeof(sync_info));
	sync_info.interval = pending_per_adv_sync->interval;
	sync_info.phy = bt_get_phy(pending_per_adv_sync->phy);
	sync_info.addr = &pending_per_adv_sync->addr;
	sync_info.sid = pending_per_adv_sync->sid;

	sync_info.recv_enabled =
		!atomic_test_bit(pending_per_adv_sync->flags,
				 BT_PER_ADV_SYNC_RECV_DISABLED);

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->synced) {
			listener->synced(pending_per_adv_sync, &sync_info);
		}
	}
}

void bt_hci_le_per_adv_sync_lost(struct net_buf *buf)
{
	struct bt_hci_evt_le_per_adv_sync_lost *evt =
		(struct bt_hci_evt_le_per_adv_sync_lost *)buf->data;
	struct bt_le_per_adv_sync_term_info term_info;
	struct bt_le_per_adv_sync *per_adv_sync;
	struct bt_le_per_adv_sync_cb *listener;

	per_adv_sync = bt_hci_get_per_adv_sync(sys_le16_to_cpu(evt->handle));

	if (!per_adv_sync) {
		BT_ERR("Unknown handle 0x%04Xfor periodic adv sync lost",
		       sys_le16_to_cpu(evt->handle));
		return;
	}

	term_info.addr = &per_adv_sync->addr;
	term_info.sid = per_adv_sync->sid;

	/* Deleting before callback, so the caller will be able to restart
	 * sync in the callback
	 */
	per_adv_sync_delete(per_adv_sync);


	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->term) {
			listener->term(per_adv_sync, &term_info);
		}
	}
}

#if defined(CONFIG_BT_CONN)
void bt_hci_le_past_received(struct net_buf *buf)
{
	struct bt_hci_evt_le_past_received *evt =
		(struct bt_hci_evt_le_past_received *)buf->data;
	struct bt_le_per_adv_sync_synced_info sync_info;
	struct bt_le_per_adv_sync_cb *listener;
	struct bt_le_per_adv_sync *per_adv_sync;

	if (evt->status) {
		/* No sync created, don't notify app */
		BT_DBG("PAST receive failed with status 0x%02X", evt->status);
		return;
	}

	sync_info.conn = bt_conn_lookup_handle(
				sys_le16_to_cpu(evt->conn_handle));

	if (!sync_info.conn) {
		BT_ERR("Could not lookup connection handle from PAST");
		per_adv_sync_terminate(sys_le16_to_cpu(evt->sync_handle));
		return;
	}

	per_adv_sync = per_adv_sync_new();
	if (!per_adv_sync) {
		BT_WARN("Could not allocate new PA sync from PAST");
		per_adv_sync_terminate(sys_le16_to_cpu(evt->sync_handle));
		return;
	}

	atomic_set_bit(per_adv_sync->flags, BT_PER_ADV_SYNC_SYNCED);

	per_adv_sync->handle = sys_le16_to_cpu(evt->sync_handle);
	per_adv_sync->interval = sys_le16_to_cpu(evt->interval);
	per_adv_sync->clock_accuracy = sys_le16_to_cpu(evt->clock_accuracy);
	per_adv_sync->phy = evt->phy;
	bt_addr_le_copy(&per_adv_sync->addr, &evt->addr);
	per_adv_sync->sid = evt->adv_sid;

	sync_info.interval = per_adv_sync->interval;
	sync_info.phy = bt_get_phy(per_adv_sync->phy);
	sync_info.addr = &per_adv_sync->addr;
	sync_info.sid = per_adv_sync->sid;
	sync_info.service_data = sys_le16_to_cpu(evt->service_data);

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->synced) {
			listener->synced(per_adv_sync, &sync_info);
		}
	}
}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_ISO_BROADCAST)
void bt_hci_le_biginfo_adv_report(struct net_buf *buf)
{
	struct bt_hci_evt_le_biginfo_adv_report *evt;
	struct bt_le_per_adv_sync *per_adv_sync;
	struct bt_le_per_adv_sync_cb *listener;
	struct bt_iso_biginfo biginfo;

	evt = net_buf_pull_mem(buf, sizeof(*evt));

	per_adv_sync = bt_hci_get_per_adv_sync(sys_le16_to_cpu(evt->sync_handle));

	if (!per_adv_sync) {
		BT_ERR("Unknown handle 0x%04X for periodic advertising report",
		       sys_le16_to_cpu(evt->sync_handle));
		return;
	}

	biginfo.addr = &per_adv_sync->addr;
	biginfo.sid = per_adv_sync->sid;
	biginfo.num_bis = evt->num_bis;
	biginfo.sub_evt_count = evt->nse;
	biginfo.iso_interval = sys_le16_to_cpu(evt->iso_interval);
	biginfo.burst_number = evt->bn;
	biginfo.offset = evt->pto;
	biginfo.rep_count = evt->irc;
	biginfo.max_pdu = sys_le16_to_cpu(evt->max_pdu);
	biginfo.sdu_interval = sys_get_le24(evt->sdu_interval);
	biginfo.max_sdu = sys_le16_to_cpu(evt->max_sdu);
	biginfo.phy = evt->phy;
	biginfo.framing = evt->framing;
	biginfo.encryption = evt->encryption ? true : false;

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->biginfo) {
			listener->biginfo(per_adv_sync, &biginfo);
		}
	}
}
#endif /* defined(CONFIG_BT_ISO_BROADCAST) */
#if defined(CONFIG_BT_DF_CONNECTIONLESS_CTE_RX)
void bt_hci_le_df_connectionless_iq_report(struct net_buf *buf)
{
	struct bt_df_per_adv_sync_iq_samples_report cte_report;
	struct bt_le_per_adv_sync *per_adv_sync;
	struct bt_le_per_adv_sync_cb *listener;

	hci_df_prepare_connectionless_iq_report(buf, &cte_report, &per_adv_sync);

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->cte_report_cb) {
			listener->cte_report_cb(per_adv_sync, &cte_report);
		}
	}
}
#endif /* CONFIG_BT_DF_CONNECTIONLESS_CTE_RX */
#endif /* defined(CONFIG_BT_PER_ADV_SYNC) */
#endif /* defined(CONFIG_BT_EXT_ADV) */

void bt_hci_le_adv_report(struct net_buf *buf)
{
	uint8_t num_reports = net_buf_pull_u8(buf);
	struct bt_hci_evt_le_advertising_info *evt;

	BT_DBG("Adv number of reports %u",  num_reports);

	while (num_reports--) {
		struct bt_le_scan_recv_info adv_info;

		if (buf->len < sizeof(*evt)) {
			BT_ERR("Unexpected end of buffer");
			break;
		}

		evt = net_buf_pull_mem(buf, sizeof(*evt));

		adv_info.primary_phy = BT_GAP_LE_PHY_1M;
		adv_info.secondary_phy = 0;
		adv_info.tx_power = BT_GAP_TX_POWER_INVALID;
		adv_info.rssi = evt->data[evt->length];
		adv_info.sid = BT_GAP_SID_INVALID;
		adv_info.interval = 0U;

		adv_info.adv_type = evt->evt_type;
		adv_info.adv_props = get_adv_props(evt->evt_type);

		le_adv_recv(&evt->addr, &adv_info, buf, evt->length);

		net_buf_pull(buf, evt->length + sizeof(adv_info.rssi));
	}
}

static bool valid_le_scan_param(const struct bt_le_scan_param *param)
{
	if (param->type != BT_HCI_LE_SCAN_PASSIVE &&
	    param->type != BT_HCI_LE_SCAN_ACTIVE) {
		return false;
	}

	if (param->options & ~(BT_LE_SCAN_OPT_FILTER_DUPLICATE |
			       BT_LE_SCAN_OPT_FILTER_WHITELIST |
			       BT_LE_SCAN_OPT_CODED |
			       BT_LE_SCAN_OPT_NO_1M)) {
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

	if (param->type && !bt_id_scan_random_addr_check()) {
		return -EINVAL;
	}

	/* Return if active scan is already enabled */
	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN)) {
		return -EALREADY;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		err = bt_le_scan_set_enable(BT_HCI_LE_SCAN_DISABLE);
		if (err) {
			atomic_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN);
			return err;
		}
	}

	atomic_set_bit_to(bt_dev.flags, BT_DEV_SCAN_FILTER_DUP,
			  param->options & BT_LE_SCAN_OPT_FILTER_DUPLICATE);

#if defined(CONFIG_BT_WHITELIST)
	atomic_set_bit_to(bt_dev.flags, BT_DEV_SCAN_WL,
			  param->options & BT_LE_SCAN_OPT_FILTER_WHITELIST);
#endif /* defined(CONFIG_BT_WHITELIST) */

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		struct bt_hci_ext_scan_phy param_1m;
		struct bt_hci_ext_scan_phy param_coded;

		struct bt_hci_ext_scan_phy *phy_1m = NULL;
		struct bt_hci_ext_scan_phy *phy_coded = NULL;

		if (!(param->options & BT_LE_SCAN_OPT_NO_1M)) {
			param_1m.type = param->type;
			param_1m.interval = sys_cpu_to_le16(param->interval);
			param_1m.window = sys_cpu_to_le16(param->window);

			phy_1m = &param_1m;
		}

		if (param->options & BT_LE_SCAN_OPT_CODED) {
			uint16_t interval = param->interval_coded ?
				param->interval_coded :
				param->interval;
			uint16_t window = param->window_coded ?
				param->window_coded :
				param->window;

			param_coded.type = param->type;
			param_coded.interval = sys_cpu_to_le16(interval);
			param_coded.window = sys_cpu_to_le16(window);
			phy_coded = &param_coded;
		}

		err = start_le_scan_ext(phy_1m, phy_coded, param->timeout);
	} else {
		if (param->timeout) {
			atomic_clear_bit(bt_dev.flags, BT_DEV_EXPLICIT_SCAN);
			return -ENOTSUP;
		}

		err = start_le_scan_legacy(param->type, param->interval,
					   param->window);
	}

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

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_SCAN_LIMITED)) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);

#if defined(CONFIG_BT_SMP)
		bt_id_pending_keys_update();
#endif
	}

	return bt_le_scan_update(false);
}

void bt_le_scan_cb_register(struct bt_le_scan_cb *cb)
{
	sys_slist_append(&scan_cbs, &cb->node);
}

void bt_le_scan_cb_unregister(struct bt_le_scan_cb *cb)
{
	sys_slist_find_and_remove(&scan_cbs, &cb->node);
}

#if defined(CONFIG_BT_PER_ADV_SYNC)
uint8_t bt_le_per_adv_sync_get_index(struct bt_le_per_adv_sync *per_adv_sync)
{
	ptrdiff_t index = per_adv_sync - per_adv_sync_pool;

	__ASSERT(index >= 0 && ARRAY_SIZE(per_adv_sync_pool) > index,
		 "Invalid per_adv_sync pointer");
	return (uint8_t)index;
}

int bt_le_per_adv_sync_get_info(struct bt_le_per_adv_sync *per_adv_sync,
				struct bt_le_per_adv_sync_info *info)
{
	CHECKIF(per_adv_sync == NULL || info == NULL) {
		return -EINVAL;
	}

	bt_addr_le_copy(&info->addr, &per_adv_sync->addr);
	info->sid = per_adv_sync->sid;
	info->phy = per_adv_sync->phy;
	info->interval = per_adv_sync->interval;

	return 0;
}

struct bt_le_per_adv_sync *bt_le_per_adv_sync_lookup_addr(const bt_addr_le_t *adv_addr,
							  uint8_t sid)
{
	for (int i = 0; i < ARRAY_SIZE(per_adv_sync_pool); i++) {
		struct bt_le_per_adv_sync *sync = &per_adv_sync_pool[i];

		if (!atomic_test_bit(per_adv_sync_pool[i].flags,
				     BT_PER_ADV_SYNC_CREATED)) {
			continue;
		}

		if (!bt_addr_le_cmp(&sync->addr, adv_addr) && sync->sid == sid) {
			return sync;
		}
	}

	return NULL;
}

int bt_le_per_adv_sync_create(const struct bt_le_per_adv_sync_param *param,
			      struct bt_le_per_adv_sync **out_sync)
{
	struct bt_hci_cp_le_per_adv_create_sync *cp;
	struct net_buf *buf;
	struct bt_le_per_adv_sync *per_adv_sync;
	int err;

	if (!BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (get_pending_per_adv_sync()) {
		return -EBUSY;
	}

	if (param->sid > BT_GAP_SID_MAX ||
		   param->skip > BT_GAP_PER_ADV_MAX_SKIP ||
		   param->timeout > BT_GAP_PER_ADV_MAX_TIMEOUT ||
		   param->timeout < BT_GAP_PER_ADV_MIN_TIMEOUT) {
		return -EINVAL;
	}

	per_adv_sync = per_adv_sync_new();
	if (!per_adv_sync) {
		return -ENOMEM;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_PER_ADV_CREATE_SYNC, sizeof(*cp));
	if (!buf) {
		per_adv_sync_delete(per_adv_sync);
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));


	bt_addr_le_copy(&cp->addr, &param->addr);

	if (param->options & BT_LE_PER_ADV_SYNC_OPT_USE_PER_ADV_LIST) {
		cp->options |= BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_USE_LIST;
	}

	if (param->options & BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOA) {
		cp->cte_type |= BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_NO_AOA;
	}

	if (param->options & BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_1US) {
		cp->cte_type |=
			BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_NO_AOD_1US;
	}

	if (param->options & BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_2US) {
		cp->cte_type |=
			BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_NO_AOD_2US;
	}

	if (param->options & BT_LE_PER_ADV_SYNC_OPT_SYNC_ONLY_CONST_TONE_EXT) {
		cp->cte_type |= BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_ONLY_CTE;
	}

	if (param->options &
	    BT_LE_PER_ADV_SYNC_OPT_REPORTING_INITIALLY_DISABLED) {
		cp->options |=
			BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_REPORTS_DISABLED;

		atomic_set_bit(per_adv_sync->flags,
			       BT_PER_ADV_SYNC_RECV_DISABLED);
	}

	cp->sid = param->sid;
	cp->skip = sys_cpu_to_le16(param->skip);
	cp->sync_timeout = sys_cpu_to_le16(param->timeout);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_PER_ADV_CREATE_SYNC, buf, NULL);
	if (err) {
		per_adv_sync_delete(per_adv_sync);
		return err;
	}

	atomic_set_bit(per_adv_sync->flags, BT_PER_ADV_SYNC_SYNCING);

	/* Syncing requires that scan is enabled. If the caller doesn't enable
	 * scan first, we enable it here, and disable it once the sync has been
	 * established. We don't need to use any callbacks since we rely on
	 * the advertiser address in the sync params.
	 */
	if (!atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
		err = bt_le_scan_update(true);

		if (err) {
			bt_le_per_adv_sync_delete(per_adv_sync);
			return err;
		}
	}

	*out_sync = per_adv_sync;
	bt_addr_le_copy(&per_adv_sync->addr, &param->addr);
	per_adv_sync->sid = param->sid;

	return 0;
}

static int bt_le_per_adv_sync_create_cancel(
	struct bt_le_per_adv_sync *per_adv_sync)
{
	struct net_buf *buf;
	int err;

	if (get_pending_per_adv_sync() != per_adv_sync) {
		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_PER_ADV_CREATE_SYNC_CANCEL, 0);
	if (!buf) {
		return -ENOBUFS;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_PER_ADV_CREATE_SYNC_CANCEL, buf,
				   NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int bt_le_per_adv_sync_terminate(struct bt_le_per_adv_sync *per_adv_sync)
{
	int err;

	if (!atomic_test_bit(per_adv_sync->flags, BT_PER_ADV_SYNC_SYNCED)) {
		return -EINVAL;
	}

	err = per_adv_sync_terminate(per_adv_sync->handle);

	if (err) {
		return err;
	}

	return 0;
}

int bt_le_per_adv_sync_delete(struct bt_le_per_adv_sync *per_adv_sync)
{
	int err = 0;

	if (!BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (atomic_test_bit(per_adv_sync->flags, BT_PER_ADV_SYNC_SYNCED)) {
		err = bt_le_per_adv_sync_terminate(per_adv_sync);

		if (!err) {
			per_adv_sync_delete(per_adv_sync);
		}
	} else if (get_pending_per_adv_sync() == per_adv_sync) {
		err = bt_le_per_adv_sync_create_cancel(per_adv_sync);
		/* Delete of the per_adv_sync will be done in the event
		 * handler when cancelling.
		 */
	}

	return err;
}

void bt_le_per_adv_sync_cb_register(struct bt_le_per_adv_sync_cb *cb)
{
	sys_slist_append(&pa_sync_cbs, &cb->node);
}

static int bt_le_set_per_adv_recv_enable(
	struct bt_le_per_adv_sync *per_adv_sync, bool enable)
{
	struct bt_hci_cp_le_set_per_adv_recv_enable *cp;
	struct bt_le_per_adv_sync_cb *listener;
	struct bt_le_per_adv_sync_state_info info;
	struct net_buf *buf;
	struct bt_hci_cmd_state_set state;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (!BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (!atomic_test_bit(per_adv_sync->flags, BT_PER_ADV_SYNC_SYNCED)) {
		return -EINVAL;
	}

	if ((enable && !atomic_test_bit(per_adv_sync->flags,
					BT_PER_ADV_SYNC_RECV_DISABLED)) ||
	    (!enable && atomic_test_bit(per_adv_sync->flags,
					BT_PER_ADV_SYNC_RECV_DISABLED))) {
		return -EALREADY;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PER_ADV_RECV_ENABLE,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->handle = sys_cpu_to_le16(per_adv_sync->handle);
	cp->enable = enable ? 1 : 0;

	bt_hci_cmd_state_set_init(buf, &state, per_adv_sync->flags,
				  BT_PER_ADV_SYNC_RECV_DISABLED,
				  enable);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_PER_ADV_RECV_ENABLE,
				   buf, NULL);

	if (err) {
		return err;
	}

	info.recv_enabled = !atomic_test_bit(per_adv_sync->flags,
					     BT_PER_ADV_SYNC_RECV_DISABLED);

	SYS_SLIST_FOR_EACH_CONTAINER(&pa_sync_cbs, listener, node) {
		if (listener->state_changed) {
			listener->state_changed(per_adv_sync, &info);
		}
	}

	return 0;
}

int bt_le_per_adv_sync_recv_enable(struct bt_le_per_adv_sync *per_adv_sync)
{
	return bt_le_set_per_adv_recv_enable(per_adv_sync, true);
}

int bt_le_per_adv_sync_recv_disable(struct bt_le_per_adv_sync *per_adv_sync)
{
	return bt_le_set_per_adv_recv_enable(per_adv_sync, false);
}

#if defined(CONFIG_BT_CONN)
int bt_le_per_adv_sync_transfer(const struct bt_le_per_adv_sync *per_adv_sync,
				const struct bt_conn *conn,
				uint16_t service_data)
{
	struct bt_hci_cp_le_per_adv_sync_transfer *cp;
	struct net_buf *buf;


	if (!BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	} else if (!BT_FEAT_LE_PAST_SEND(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_PER_ADV_SYNC_TRANSFER,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->conn_handle = sys_cpu_to_le16(conn->handle);
	cp->sync_handle = sys_cpu_to_le16(per_adv_sync->handle);
	cp->service_data = sys_cpu_to_le16(service_data);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_PER_ADV_SYNC_TRANSFER, buf,
				    NULL);
}

static bool valid_past_param(
	const struct bt_le_per_adv_sync_transfer_param *param)
{
	if (param->skip > 0x01f3 ||
	    param->timeout < 0x000A ||
	    param->timeout > 0x4000) {
		return false;
	}

	return true;
}

static int past_param_set(const struct bt_conn *conn, uint8_t mode,
			  uint16_t skip, uint16_t timeout, uint8_t cte_type)
{
	struct bt_hci_cp_le_past_param *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_PAST_PARAM, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->conn_handle = sys_cpu_to_le16(conn->handle);
	cp->mode = mode;
	cp->skip = sys_cpu_to_le16(skip);
	cp->timeout = sys_cpu_to_le16(timeout);
	cp->cte_type = cte_type;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_PAST_PARAM, buf, NULL);
}

static int default_past_param_set(uint8_t mode, uint16_t skip, uint16_t timeout,
				  uint8_t cte_type)
{
	struct bt_hci_cp_le_default_past_param *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_DEFAULT_PAST_PARAM, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->mode = mode;
	cp->skip = sys_cpu_to_le16(skip);
	cp->timeout = sys_cpu_to_le16(timeout);
	cp->cte_type = cte_type;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_DEFAULT_PAST_PARAM, buf, NULL);
}

int bt_le_per_adv_sync_transfer_subscribe(
	const struct bt_conn *conn,
	const struct bt_le_per_adv_sync_transfer_param *param)
{
	uint8_t cte_type = 0;

	if (!BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	} else if (!BT_FEAT_LE_PAST_RECV(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (!valid_past_param(param)) {
		return -EINVAL;
	}

	if (param->options & BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOA) {
		cte_type |= BT_HCI_LE_PAST_CTE_TYPE_NO_AOA;
	}

	if (param->options & BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_1US) {
		cte_type |= BT_HCI_LE_PAST_CTE_TYPE_NO_AOD_1US;
	}

	if (param->options & BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_2US) {
		cte_type |= BT_HCI_LE_PAST_CTE_TYPE_NO_AOD_2US;
	}

	if (param->options & BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_ONLY_CTE) {
		cte_type |= BT_HCI_LE_PAST_CTE_TYPE_ONLY_CTE;
	}

	if (conn) {
		return past_param_set(conn, BT_HCI_LE_PAST_MODE_SYNC,
				      param->skip, param->timeout, cte_type);
	} else {
		return default_past_param_set(BT_HCI_LE_PAST_MODE_SYNC,
					      param->skip, param->timeout,
					      cte_type);
	}
}

int bt_le_per_adv_sync_transfer_unsubscribe(const struct bt_conn *conn)
{
	if (!BT_FEAT_LE_EXT_PER_ADV(bt_dev.le.features)) {
		return -ENOTSUP;
	} else if (!BT_FEAT_LE_PAST_RECV(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	if (conn) {
		return past_param_set(conn, BT_HCI_LE_PAST_MODE_NO_SYNC, 0,
				      0x0a, 0);
	} else {
		return default_past_param_set(BT_HCI_LE_PAST_MODE_NO_SYNC, 0,
					      0x0a, 0);
	}
}
#endif /* CONFIG_BT_CONN */

int bt_le_per_adv_list_add(const bt_addr_le_t *addr, uint8_t sid)
{
	struct bt_hci_cp_le_add_dev_to_per_adv_list *cp;
	struct net_buf *buf;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_ADD_DEV_TO_PER_ADV_LIST,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->addr, addr);
	cp->sid = sid;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_ADD_DEV_TO_PER_ADV_LIST, buf,
				   NULL);
	if (err) {
		BT_ERR("Failed to add device to periodic advertiser list");

		return err;
	}

	return 0;
}

int bt_le_per_adv_list_remove(const bt_addr_le_t *addr, uint8_t sid)
{
	struct bt_hci_cp_le_rem_dev_from_per_adv_list *cp;
	struct net_buf *buf;
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REM_DEV_FROM_PER_ADV_LIST,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->addr, addr);
	cp->sid = sid;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_REM_DEV_FROM_PER_ADV_LIST, buf,
				   NULL);
	if (err) {
		BT_ERR("Failed to remove device from periodic advertiser list");
		return err;
	}

	return 0;
}

int bt_le_per_adv_list_clear(void)
{
	int err;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_CLEAR_PER_ADV_LIST, NULL, NULL);
	if (err) {
		BT_ERR("Failed to clear periodic advertiser list");
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_BT_PER_ADV_SYNC) */
