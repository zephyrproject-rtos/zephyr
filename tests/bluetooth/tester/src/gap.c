/* gap.c - Bluetooth GAP Tester */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/atomic.h>
#include <zephyr/types.h>
#include <string.h>

#include <toolchain.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>

#include <sys/byteorder.h>
#include <net/buf.h>

#include <logging/log.h>
#define LOG_MODULE_NAME bttester_gap
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "bttester.h"

#define CONTROLLER_INDEX 0
#define CONTROLLER_NAME "btp_tester"

#define BT_LE_AD_DISCOV_MASK (BT_LE_AD_LIMITED | BT_LE_AD_GENERAL)
#define ADV_BUF_LEN (sizeof(struct gap_device_found_ev) + 2 * 31)

static atomic_t current_settings;
struct bt_conn_auth_cb cb;
static uint8_t oob_legacy_tk[16] = { 0 };

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
static struct bt_le_oob oob_sc_local = { 0 };
static struct bt_le_oob oob_sc_remote = { 0 };
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

static void le_connected(struct bt_conn *conn, uint8_t err)
{
	struct gap_device_connected_ev ev;
	struct bt_conn_info info;

	if (err) {
		return;
	}

	bt_conn_get_info(conn, &info);

	memcpy(ev.address, info.le.dst->a.val, sizeof(ev.address));
	ev.address_type = info.le.dst->type;
	ev.interval = sys_cpu_to_le16(info.le.interval);
	ev.latency = sys_cpu_to_le16(info.le.latency);
	ev.timeout = sys_cpu_to_le16(info.le.timeout);

	tester_send(BTP_SERVICE_ID_GAP, GAP_EV_DEVICE_CONNECTED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void le_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct gap_device_disconnected_ev ev;
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);

	memcpy(ev.address, addr->a.val, sizeof(ev.address));
	ev.address_type = addr->type;

	tester_send(BTP_SERVICE_ID_GAP, GAP_EV_DEVICE_DISCONNECTED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void le_identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
				 const bt_addr_le_t *identity)
{
	struct gap_identity_resolved_ev ev;

	ev.address_type = rpa->type;
	memcpy(ev.address, rpa->a.val, sizeof(ev.address));

	ev.identity_address_type = identity->type;
	memcpy(ev.identity_address, identity->a.val,
	       sizeof(ev.identity_address));

	tester_send(BTP_SERVICE_ID_GAP, GAP_EV_IDENTITY_RESOLVED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	struct gap_conn_param_update_ev ev;
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);

	memcpy(ev.address, addr->a.val, sizeof(ev.address));
	ev.address_type = addr->type;
	ev.interval = sys_cpu_to_le16(interval);
	ev.latency = sys_cpu_to_le16(latency);
	ev.timeout = sys_cpu_to_le16(timeout);

	tester_send(BTP_SERVICE_ID_GAP, GAP_EV_CONN_PARAM_UPDATE,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static struct bt_conn_cb conn_callbacks = {
	.connected = le_connected,
	.disconnected = le_disconnected,
	.identity_resolved = le_identity_resolved,
	.le_param_updated = le_param_updated,
};

static void supported_commands(uint8_t *data, uint16_t len)
{
	uint8_t cmds[4];
	struct gap_read_supported_commands_rp *rp = (void *) &cmds;

	(void)memset(cmds, 0, sizeof(cmds));

	tester_set_bit(cmds, GAP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(cmds, GAP_READ_CONTROLLER_INDEX_LIST);
	tester_set_bit(cmds, GAP_READ_CONTROLLER_INFO);
	tester_set_bit(cmds, GAP_SET_CONNECTABLE);
	tester_set_bit(cmds, GAP_SET_DISCOVERABLE);
	tester_set_bit(cmds, GAP_SET_BONDABLE);
	tester_set_bit(cmds, GAP_START_ADVERTISING);
	tester_set_bit(cmds, GAP_STOP_ADVERTISING);
	tester_set_bit(cmds, GAP_START_DISCOVERY);
	tester_set_bit(cmds, GAP_STOP_DISCOVERY);
	tester_set_bit(cmds, GAP_CONNECT);
	tester_set_bit(cmds, GAP_DISCONNECT);
	tester_set_bit(cmds, GAP_SET_IO_CAP);
	tester_set_bit(cmds, GAP_PAIR);
	tester_set_bit(cmds, GAP_PASSKEY_ENTRY);
	tester_set_bit(cmds, GAP_PASSKEY_CONFIRM);
	tester_set_bit(cmds, GAP_CONN_PARAM_UPDATE);
	tester_set_bit(cmds, GAP_SET_MITM);
	tester_set_bit(cmds, GAP_OOB_LEGACY_SET_DATA);
#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	tester_set_bit(cmds, GAP_OOB_SC_GET_LOCAL_DATA);
	tester_set_bit(cmds, GAP_OOB_SC_SET_REMOTE_DATA);
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

	tester_send(BTP_SERVICE_ID_GAP, GAP_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, (uint8_t *) rp, sizeof(cmds));
}

static void controller_index_list(uint8_t *data,  uint16_t len)
{
	struct gap_read_controller_index_list_rp *rp;
	uint8_t buf[sizeof(*rp) + 1];

	rp = (void *) buf;

	rp->num = 1U;
	rp->index[0] = CONTROLLER_INDEX;

	tester_send(BTP_SERVICE_ID_GAP, GAP_READ_CONTROLLER_INDEX_LIST,
		    BTP_INDEX_NONE, (uint8_t *) rp, sizeof(buf));
}

static void controller_info(uint8_t *data, uint16_t len)
{
	struct gap_read_controller_info_rp rp;
	uint32_t supported_settings;

	(void)memset(&rp, 0, sizeof(rp));

	struct bt_le_oob oob_local = { 0 };

	bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);
	memcpy(rp.address, &oob_local.addr.a, sizeof(bt_addr_t));

	/*
	 * Re-use the oob data read here in get_oob_sc_local_data()
	 */
#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	oob_sc_local = oob_local;
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

	/*
	 * If privacy is used, the device uses random type address, otherwise
	 * static random or public type address is used.
	 */
#if !defined(CONFIG_BT_PRIVACY)
	if (oob_local.addr.type == BT_ADDR_LE_RANDOM) {
		atomic_set_bit(&current_settings, GAP_SETTINGS_STATIC_ADDRESS);
	}
#endif /* CONFIG_BT_PRIVACY */

	supported_settings = BIT(GAP_SETTINGS_POWERED);
	supported_settings |= BIT(GAP_SETTINGS_CONNECTABLE);
	supported_settings |= BIT(GAP_SETTINGS_BONDABLE);
	supported_settings |= BIT(GAP_SETTINGS_LE);
	supported_settings |= BIT(GAP_SETTINGS_ADVERTISING);

	rp.supported_settings = sys_cpu_to_le32(supported_settings);
	rp.current_settings = sys_cpu_to_le32(current_settings);

	memcpy(rp.name, CONTROLLER_NAME, sizeof(CONTROLLER_NAME));

	tester_send(BTP_SERVICE_ID_GAP, GAP_READ_CONTROLLER_INFO,
		    CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));
}

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
static const char *oob_config_str(int oob_config)
{
	switch (oob_config) {
	case BT_CONN_OOB_LOCAL_ONLY:
		return "Local";
	case BT_CONN_OOB_REMOTE_ONLY:
		return "Remote";
	case BT_CONN_OOB_BOTH_PEERS:
		return "Local and Remote";
	case BT_CONN_OOB_NO_DATA:
	default:
		return "no";
	}
}
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

static void oob_data_request(struct bt_conn *conn,
			     struct bt_conn_oob_info *oob_info)
{
	struct bt_conn_info info;
	int err = bt_conn_get_info(conn, &info);

	if (err) {
		return;
	}

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info.le.dst, addr, sizeof(addr));

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	if (oob_info->type == BT_CONN_OOB_LE_SC) {
		LOG_DBG("Set %s OOB SC data for %s, ",
			oob_config_str(oob_info->lesc.oob_config),
			log_strdup(addr));

		struct bt_le_oob_sc_data *oobd_local =
			oob_info->lesc.oob_config != BT_CONN_OOB_REMOTE_ONLY ?
				      &oob_sc_local.le_sc_data :
				      NULL;

		struct bt_le_oob_sc_data *oobd_remote =
			oob_info->lesc.oob_config != BT_CONN_OOB_LOCAL_ONLY ?
				      &oob_sc_remote.le_sc_data :
				      NULL;

		if (oobd_remote) {
			/* Assume that oob_sc_remote
			 * corresponds to the currently connected peer
			 */
			bt_addr_le_copy(&oob_sc_remote.addr, info.le.remote);
		}

		if (oobd_local &&
		    bt_addr_le_cmp(info.le.local, &oob_sc_local.addr)) {
			bt_addr_le_to_str(info.le.local, addr, sizeof(addr));
			LOG_DBG("No OOB data available for local %s",
				log_strdup(addr));
			bt_conn_auth_cancel(conn);
			return;
		}

		err = bt_le_oob_set_sc_data(conn, oobd_local, oobd_remote);
		if (err) {
			LOG_DBG("bt_le_oob_set_sc_data failed with: %d", err);
		}

		return;
	}
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

	LOG_DBG("Legacy OOB TK requested from remote %s", log_strdup(addr));

	err = bt_le_oob_set_legacy_tk(conn, oob_legacy_tk);
	if (err < 0) {
		LOG_ERR("Failed to set OOB Temp Key: %d", err);
	}
}

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
static void get_oob_sc_local_data(void)
{
	cb.oob_data_request = oob_data_request;
	struct gap_oob_sc_get_local_data_rp rp = { 0 };

	memcpy(&rp.conf[0], &oob_sc_local.le_sc_data.c[0], sizeof(rp.conf));
	memcpy(&rp.rand[0], &oob_sc_local.le_sc_data.r[0], sizeof(rp.rand));
	tester_send(BTP_SERVICE_ID_GAP, GAP_OOB_SC_GET_LOCAL_DATA,
		    CONTROLLER_INDEX, (uint8_t *)&rp, sizeof(rp));
}

static void set_oob_sc_remote_data(const uint8_t *data, uint16_t len)
{
	cb.oob_data_request = oob_data_request;
	bt_set_oob_data_flag(true);

	const struct gap_oob_sc_set_remote_data_cmd *cmd = (void *)data;

	/* Note that the .addr field
	 * will be set by the oob_data_request callback
	 */
	memcpy(&oob_sc_remote.le_sc_data.r[0], &cmd->rand[0],
	       sizeof(oob_sc_remote.le_sc_data.r));
	memcpy(&oob_sc_remote.le_sc_data.c[0], &cmd->conf[0],
	       sizeof(oob_sc_remote.le_sc_data.c));

	tester_rsp(BTP_SERVICE_ID_GAP, GAP_OOB_SC_SET_REMOTE_DATA,
		   CONTROLLER_INDEX, BTP_STATUS_SUCCESS);
}
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

static void set_connectable(uint8_t *data, uint16_t len)
{
	const struct gap_set_connectable_cmd *cmd = (void *) data;
	struct gap_set_connectable_rp rp;

	if (cmd->connectable) {
		atomic_set_bit(&current_settings, GAP_SETTINGS_CONNECTABLE);
	} else {
		atomic_clear_bit(&current_settings, GAP_SETTINGS_CONNECTABLE);
	}

	rp.current_settings = sys_cpu_to_le32(current_settings);

	tester_send(BTP_SERVICE_ID_GAP, GAP_SET_CONNECTABLE, CONTROLLER_INDEX,
		    (uint8_t *) &rp, sizeof(rp));
}

static uint8_t ad_flags = BT_LE_AD_NO_BREDR;
static struct bt_data ad[10] = {
	BT_DATA(BT_DATA_FLAGS, &ad_flags, sizeof(ad_flags)),
};
static struct bt_data sd[10];

static void set_discoverable(uint8_t *data, uint16_t len)
{
	const struct gap_set_discoverable_cmd *cmd = (void *) data;
	struct gap_set_discoverable_rp rp;

	switch (cmd->discoverable) {
	case GAP_NON_DISCOVERABLE:
		ad_flags &= ~(BT_LE_AD_GENERAL | BT_LE_AD_LIMITED);
		atomic_clear_bit(&current_settings, GAP_SETTINGS_DISCOVERABLE);
		break;
	case GAP_GENERAL_DISCOVERABLE:
		ad_flags &= ~BT_LE_AD_LIMITED;
		ad_flags |= BT_LE_AD_GENERAL;
		atomic_set_bit(&current_settings, GAP_SETTINGS_DISCOVERABLE);
		break;
	case GAP_LIMITED_DISCOVERABLE:
		ad_flags &= ~BT_LE_AD_GENERAL;
		ad_flags |= BT_LE_AD_LIMITED;
		atomic_set_bit(&current_settings, GAP_SETTINGS_DISCOVERABLE);
		break;
	default:
		LOG_WRN("unknown mode: 0x%x", cmd->discoverable);
		tester_rsp(BTP_SERVICE_ID_GAP, GAP_SET_DISCOVERABLE,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		return;
	}

	rp.current_settings = sys_cpu_to_le32(current_settings);

	tester_send(BTP_SERVICE_ID_GAP, GAP_SET_DISCOVERABLE, CONTROLLER_INDEX,
		    (uint8_t *) &rp, sizeof(rp));
}

static void set_bondable(uint8_t *data, uint16_t len)
{
	const struct gap_set_bondable_cmd *cmd = (void *) data;
	struct gap_set_bondable_rp rp;

	LOG_DBG("cmd->bondable: %d", cmd->bondable);

	if (cmd->bondable) {
		atomic_set_bit(&current_settings, GAP_SETTINGS_BONDABLE);
	} else {
		atomic_clear_bit(&current_settings, GAP_SETTINGS_BONDABLE);
	}

	bt_set_bondable(cmd->bondable);

	rp.current_settings = sys_cpu_to_le32(current_settings);

	tester_send(BTP_SERVICE_ID_GAP, GAP_SET_BONDABLE, CONTROLLER_INDEX,
		    (uint8_t *) &rp, sizeof(rp));
}

static void start_advertising(const uint8_t *data, uint16_t len)
{
	const struct gap_start_advertising_cmd *cmd = (void *) data;
	struct gap_start_advertising_rp rp;
	uint8_t adv_len, sd_len;
	bool adv_conn;
	int i;

	for (i = 0, adv_len = 1U; i < cmd->adv_data_len; adv_len++) {
		if (adv_len >= ARRAY_SIZE(ad)) {
			LOG_ERR("ad[] Out of memory");
			goto fail;
		}

		ad[adv_len].type = cmd->adv_sr_data[i++];
		ad[adv_len].data_len = cmd->adv_sr_data[i++];
		ad[adv_len].data = &cmd->adv_sr_data[i];
		i += ad[adv_len].data_len;
	}

	for (sd_len = 0U; i < cmd->adv_data_len+cmd->scan_rsp_len; sd_len++) {
		if (sd_len >= ARRAY_SIZE(sd)) {
			LOG_ERR("sd[] Out of memory");
			goto fail;
		}

		sd[sd_len].type = cmd->adv_sr_data[i++];
		sd[sd_len].data_len = cmd->adv_sr_data[i++];
		sd[sd_len].data = &cmd->adv_sr_data[i];
		i += sd[sd_len].data_len;
	}

	adv_conn = atomic_test_bit(&current_settings, GAP_SETTINGS_CONNECTABLE);

	/* BTP API don't allow to set empty scan response data. */
	if (bt_le_adv_start(adv_conn ? BT_LE_ADV_CONN : BT_LE_ADV_NCONN,
			    ad, adv_len, sd_len ? sd : NULL, sd_len) < 0) {
		LOG_ERR("Failed to start advertising");
		goto fail;
	}

	atomic_set_bit(&current_settings, GAP_SETTINGS_ADVERTISING);
	rp.current_settings = sys_cpu_to_le32(current_settings);

	tester_send(BTP_SERVICE_ID_GAP, GAP_START_ADVERTISING, CONTROLLER_INDEX,
		    (uint8_t *) &rp, sizeof(rp));
	return;
fail:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_START_ADVERTISING, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void stop_advertising(const uint8_t *data, uint16_t len)
{
	struct gap_stop_advertising_rp rp;
	int err;

	err = bt_le_adv_stop();
	if (err < 0) {
		tester_rsp(BTP_SERVICE_ID_GAP, GAP_STOP_ADVERTISING,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		LOG_ERR("Failed to stop advertising: %d", err);
		return;
	}

	atomic_clear_bit(&current_settings, GAP_SETTINGS_ADVERTISING);
	rp.current_settings = sys_cpu_to_le32(current_settings);

	tester_send(BTP_SERVICE_ID_GAP, GAP_STOP_ADVERTISING, CONTROLLER_INDEX,
		    (uint8_t *) &rp, sizeof(rp));
}

static uint8_t get_ad_flags(struct net_buf_simple *ad)
{
	uint8_t len, i;

	/* Parse advertisement to get flags */
	for (i = 0U; i < ad->len; i += len - 1) {
		len = ad->data[i++];
		if (!len) {
			break;
		}

		/* Check if field length is correct */
		if (len > (ad->len - i) || (ad->len - i) < 1) {
			break;
		}

		switch (ad->data[i++]) {
		case BT_DATA_FLAGS:
			return ad->data[i];
		default:
			break;
		}
	}

	return 0;
}

static uint8_t discovery_flags;
static struct net_buf_simple *adv_buf = NET_BUF_SIMPLE(ADV_BUF_LEN);

static void store_adv(const bt_addr_le_t *addr, int8_t rssi,
		      struct net_buf_simple *ad)
{
	struct gap_device_found_ev *ev;

	/* cleanup */
	net_buf_simple_init(adv_buf, 0);

	ev = net_buf_simple_add(adv_buf, sizeof(*ev));

	memcpy(ev->address, addr->a.val, sizeof(ev->address));
	ev->address_type = addr->type;
	ev->rssi = rssi;
	ev->flags = GAP_DEVICE_FOUND_FLAG_AD | GAP_DEVICE_FOUND_FLAG_RSSI;
	ev->eir_data_len = ad->len;
	memcpy(net_buf_simple_add(adv_buf, ad->len), ad->data, ad->len);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t evtype,
			 struct net_buf_simple *ad)
{
	/* if General/Limited Discovery - parse Advertising data to get flags */
	if (!(discovery_flags & GAP_DISCOVERY_FLAG_LE_OBSERVE) &&
	    (evtype != BT_GAP_ADV_TYPE_SCAN_RSP)) {
		uint8_t flags = get_ad_flags(ad);

		/* ignore non-discoverable devices */
		if (!(flags & BT_LE_AD_DISCOV_MASK)) {
			LOG_DBG("Non discoverable, skipping");
			return;
		}

		/* if Limited Discovery - ignore general discoverable devices */
		if ((discovery_flags & GAP_DISCOVERY_FLAG_LIMITED) &&
		    !(flags & BT_LE_AD_LIMITED)) {
			LOG_DBG("General discoverable, skipping");
			return;
		}
	}

	/* attach Scan Response data */
	if (evtype == BT_GAP_ADV_TYPE_SCAN_RSP) {
		struct gap_device_found_ev *ev;
		bt_addr_le_t a;

		/* skip if there is no pending advertisement */
		if (!adv_buf->len) {
			LOG_INF("No pending advertisement, skipping");
			return;
		}

		ev = (void *) adv_buf->data;
		a.type = ev->address_type;
		memcpy(a.a.val, ev->address, sizeof(a.a.val));

		/*
		 * in general, the Scan Response comes right after the
		 * Advertisement, but if not if send stored event and ignore
		 * this one
		 */
		if (bt_addr_le_cmp(addr, &a)) {
			LOG_INF("Address does not match, skipping");
			goto done;
		}

		ev->eir_data_len += ad->len;
		ev->flags |= GAP_DEVICE_FOUND_FLAG_SD;

		memcpy(net_buf_simple_add(adv_buf, ad->len), ad->data, ad->len);

		goto done;
	}

	/*
	 * if there is another pending advertisement, send it and store the
	 * current one
	 */
	if (adv_buf->len) {
		tester_send(BTP_SERVICE_ID_GAP, GAP_EV_DEVICE_FOUND,
			    CONTROLLER_INDEX, adv_buf->data, adv_buf->len);
		net_buf_simple_reset(adv_buf);
	}

	store_adv(addr, rssi, ad);

	/* if Active Scan and scannable event - wait for Scan Response */
	if ((discovery_flags & GAP_DISCOVERY_FLAG_LE_ACTIVE_SCAN) &&
	    (evtype == BT_GAP_ADV_TYPE_ADV_IND ||
	     evtype == BT_GAP_ADV_TYPE_ADV_SCAN_IND)) {
		LOG_DBG("Waiting for scan response");
		return;
	}
done:
	tester_send(BTP_SERVICE_ID_GAP, GAP_EV_DEVICE_FOUND,
		    CONTROLLER_INDEX, adv_buf->data, adv_buf->len);
	net_buf_simple_reset(adv_buf);
}

static void start_discovery(const uint8_t *data, uint16_t len)
{
	const struct gap_start_discovery_cmd *cmd = (void *) data;
	uint8_t status;

	/* only LE scan is supported */
	if (cmd->flags & GAP_DISCOVERY_FLAG_BREDR) {
		status = BTP_STATUS_FAILED;
		LOG_WRN("BR/EDR not supported");
		goto reply;
	}

	if (bt_le_scan_start(cmd->flags & GAP_DISCOVERY_FLAG_LE_ACTIVE_SCAN ?
			     BT_LE_SCAN_ACTIVE : BT_LE_SCAN_PASSIVE,
			     device_found) < 0) {
		status = BTP_STATUS_FAILED;
		LOG_ERR("Failed to start scanning");
		goto reply;
	}

	net_buf_simple_init(adv_buf, 0);
	discovery_flags = cmd->flags;

	status = BTP_STATUS_SUCCESS;
reply:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_START_DISCOVERY, CONTROLLER_INDEX,
		   status);
}

static void stop_discovery(const uint8_t *data, uint16_t len)
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	err = bt_le_scan_stop();
	if (err < 0) {
		LOG_ERR("Failed to stop scanning: %d", err);
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_GAP, GAP_STOP_DISCOVERY, CONTROLLER_INDEX,
		   status);
}

static void connect(const uint8_t *data, uint16_t len)
{
	struct bt_conn *conn;
	uint8_t status;
	int err;

	err = bt_conn_le_create((bt_addr_le_t *) data, BT_CONN_LE_CREATE_CONN,
				 BT_LE_CONN_PARAM_DEFAULT, &conn);
	if (err) {
		LOG_ERR("Failed to create connection (%d)", err);
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	bt_conn_unref(conn);
	status = BTP_STATUS_SUCCESS;

rsp:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_CONNECT, CONTROLLER_INDEX, status);
}

static void disconnect(const uint8_t *data, uint16_t len)
{
	struct bt_conn *conn;
	uint8_t status;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		status = BTP_STATUS_FAILED;
		LOG_ERR("Unknown connection");
		goto rsp;
	}

	if (bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN)) {
		LOG_ERR("Failed to disconnect");
		status = BTP_STATUS_FAILED;
	} else {
		status = BTP_STATUS_SUCCESS;
	}

	bt_conn_unref(conn);

rsp:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_DISCONNECT, CONTROLLER_INDEX,
		   status);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	struct gap_passkey_display_ev ev;
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);

	memcpy(ev.address, addr->a.val, sizeof(ev.address));
	ev.address_type = addr->type;
	ev.passkey = sys_cpu_to_le32(passkey);

	tester_send(BTP_SERVICE_ID_GAP, GAP_EV_PASSKEY_DISPLAY,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void auth_passkey_entry(struct bt_conn *conn)
{
	struct gap_passkey_entry_req_ev ev;
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);

	memcpy(ev.address, addr->a.val, sizeof(ev.address));
	ev.address_type = addr->type;

	tester_send(BTP_SERVICE_ID_GAP, GAP_EV_PASSKEY_ENTRY_REQ,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	struct gap_passkey_confirm_req_ev ev;
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);

	memcpy(ev.address, addr->a.val, sizeof(ev.address));
	ev.address_type = addr->type;
	ev.passkey = sys_cpu_to_le32(passkey);

	tester_send(BTP_SERVICE_ID_GAP, GAP_EV_PASSKEY_CONFIRM_REQ,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void auth_cancel(struct bt_conn *conn)
{
	/* TODO */
}

static void set_io_cap(const uint8_t *data, uint16_t len)
{
	const struct gap_set_io_cap_cmd *cmd = (void *) data;
	uint8_t status;

	/* Reset io cap requirements */
	(void)memset(&cb, 0, sizeof(cb));
	bt_conn_auth_cb_register(NULL);

	LOG_DBG("io_cap: %d", cmd->io_cap);

	switch (cmd->io_cap) {
	case GAP_IO_CAP_DISPLAY_ONLY:
		cb.cancel = auth_cancel;
		cb.passkey_display = auth_passkey_display;
		break;
	case GAP_IO_CAP_KEYBOARD_DISPLAY:
		cb.cancel = auth_cancel;
		cb.passkey_display = auth_passkey_display;
		cb.passkey_entry = auth_passkey_entry;
		cb.passkey_confirm = auth_passkey_confirm;
		break;
	case GAP_IO_CAP_NO_INPUT_OUTPUT:
		cb.cancel = auth_cancel;
		break;
	case GAP_IO_CAP_KEYBOARD_ONLY:
		cb.cancel = auth_cancel;
		cb.passkey_entry = auth_passkey_entry;
		break;
	case GAP_IO_CAP_DISPLAY_YESNO:
	default:
		LOG_WRN("Unhandled io_cap: 0x%x", cmd->io_cap);
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	if (bt_conn_auth_cb_register(&cb)) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	status = BTP_STATUS_SUCCESS;

rsp:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_SET_IO_CAP, CONTROLLER_INDEX,
		   status);
}

static void pair(const uint8_t *data, uint16_t len)
{
	struct bt_conn *conn;
	uint8_t status;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		LOG_ERR("Unknown connection");
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err < 0) {
		LOG_ERR("Failed to set security: %d", err);
		status = BTP_STATUS_FAILED;
		bt_conn_unref(conn);
		goto rsp;
	}

	bt_conn_unref(conn);
	status = BTP_STATUS_SUCCESS;

rsp:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_PAIR, CONTROLLER_INDEX, status);
}

static void unpair(const uint8_t *data, uint16_t len)
{
	struct gap_unpair_cmd *cmd = (void *) data;
	struct bt_conn *conn;
	bt_addr_le_t addr;
	uint8_t status;
	int err;

	addr.type = cmd->address_type;
	memcpy(addr.a.val, cmd->address, sizeof(addr.a.val));

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &addr);
	if (!conn) {
		LOG_ERR("Unknown connection");
		goto keys;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	bt_conn_unref(conn);

	if (err < 0) {
		LOG_ERR("Failed to disconnect: %d", err);
		status = BTP_STATUS_FAILED;
		goto rsp;
	}
keys:
	err = bt_unpair(BT_ID_DEFAULT, &addr);

	status = err < 0 ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
rsp:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_UNPAIR, CONTROLLER_INDEX, status);
}

static void passkey_entry(const uint8_t *data, uint16_t len)
{
	const struct gap_passkey_entry_cmd *cmd = (void *) data;
	struct bt_conn *conn;
	uint8_t status;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		LOG_ERR("Unknown connection");
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	err = bt_conn_auth_passkey_entry(conn, sys_le32_to_cpu(cmd->passkey));
	if (err < 0) {
		LOG_ERR("Failed to enter passkey: %d", err);
	}

	bt_conn_unref(conn);
	status = err < 0 ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;

rsp:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_PASSKEY_ENTRY, CONTROLLER_INDEX,
		   status);
}

static void passkey_confirm(const uint8_t *data, uint16_t len)
{
	const struct gap_passkey_confirm_cmd *cmd = (void *) data;
	struct bt_conn *conn;
	uint8_t status;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		LOG_ERR("Unknown connection");
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	if (cmd->match) {
		err = bt_conn_auth_passkey_confirm(conn);
		if (err < 0) {
			LOG_ERR("Failed to confirm passkey: %d", err);
		}
	} else {
		err = bt_conn_auth_cancel(conn);
		if (err < 0) {
			LOG_ERR("Failed to cancel auth: %d", err);
		}
	}

	bt_conn_unref(conn);
	status = err < 0 ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;

rsp:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_PASSKEY_CONFIRM, CONTROLLER_INDEX,
		   status);
}


static void conn_param_update(const uint8_t *data, uint16_t len)
{
	const struct gap_conn_param_update_cmd *cmd = (void *) data;
	struct bt_le_conn_param param = {
		.interval_min = sys_le16_to_cpu(cmd->interval_min),
		.interval_max = sys_le16_to_cpu(cmd->interval_max),
		.latency = sys_le16_to_cpu(cmd->latency),
		.timeout = sys_le16_to_cpu(cmd->timeout),
	};
	struct bt_conn *conn;
	uint8_t status = BTP_STATUS_FAILED;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		LOG_ERR("Unknown connection");
		goto rsp;
	}

	err = bt_conn_le_param_update(conn, &param);
	if (err < 0) {
		LOG_ERR("Failed to update params: %d", err);
	}

	bt_conn_unref(conn);
	status = err < 0 ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;

rsp:
	tester_rsp(BTP_SERVICE_ID_GAP, GAP_CONN_PARAM_UPDATE, CONTROLLER_INDEX,
		   status);
}

static void set_mitm(const uint8_t *data, uint16_t len)
{
	LOG_WRN("Use CONFIG_BT_SMP_ENFORCE_MITM instead");

	tester_rsp(BTP_SERVICE_ID_GAP, GAP_SET_MITM, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void set_oob_legacy_data(const uint8_t *data, uint16_t len)
{
	const struct gap_oob_legacy_set_data_cmd *cmd = (void *) data;

	memcpy(oob_legacy_tk, cmd->oob_data, 16);

	bt_set_oob_data_flag(true);
	cb.oob_data_request = oob_data_request;

	tester_rsp(BTP_SERVICE_ID_GAP, GAP_OOB_LEGACY_SET_DATA,
		   CONTROLLER_INDEX, BTP_STATUS_SUCCESS);
}

void tester_handle_gap(uint8_t opcode, uint8_t index, uint8_t *data,
		       uint16_t len)
{
	LOG_DBG("opcode: 0x%02x", opcode);
	switch (opcode) {
	case GAP_READ_SUPPORTED_COMMANDS:
	case GAP_READ_CONTROLLER_INDEX_LIST:
		if (index != BTP_INDEX_NONE){
			tester_rsp(BTP_SERVICE_ID_GAP, opcode, index,
				   BTP_STATUS_FAILED);
			LOG_WRN("index != BTP_INDEX_NONE: opcode: 0x%x "
				"index: 0x%x", opcode, index);
			return;
		}
		break;
	default:
		if (index != CONTROLLER_INDEX){
			tester_rsp(BTP_SERVICE_ID_GAP, opcode, index,
				   BTP_STATUS_FAILED);
			LOG_WRN("index != CONTROLLER_INDEX: opcode: 0x%x "
				 "index: 0x%x", opcode, index);
			return;
		}
		break;
	}

	switch (opcode) {
	case GAP_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		return;
	case GAP_READ_CONTROLLER_INDEX_LIST:
		controller_index_list(data, len);
		return;
	case GAP_READ_CONTROLLER_INFO:
		controller_info(data, len);
		return;
	case GAP_SET_CONNECTABLE:
		set_connectable(data, len);
		return;
	case GAP_SET_DISCOVERABLE:
		set_discoverable(data, len);
		return;
	case GAP_SET_BONDABLE:
		set_bondable(data, len);
		return;
	case GAP_START_ADVERTISING:
		start_advertising(data, len);
		return;
	case GAP_STOP_ADVERTISING:
		stop_advertising(data, len);
		return;
	case GAP_START_DISCOVERY:
		start_discovery(data, len);
		return;
	case GAP_STOP_DISCOVERY:
		stop_discovery(data, len);
		return;
	case GAP_CONNECT:
		connect(data, len);
		return;
	case GAP_DISCONNECT:
		disconnect(data, len);
		return;
	case GAP_SET_IO_CAP:
		set_io_cap(data, len);
		return;
	case GAP_PAIR:
		pair(data, len);
		return;
	case GAP_UNPAIR:
		unpair(data, len);
		return;
	case GAP_PASSKEY_ENTRY:
		passkey_entry(data, len);
		return;
	case GAP_PASSKEY_CONFIRM:
		passkey_confirm(data, len);
		return;
	case GAP_CONN_PARAM_UPDATE:
		conn_param_update(data, len);
		return;
	case GAP_SET_MITM:
		set_mitm(data, len);
		return;
	case GAP_OOB_LEGACY_SET_DATA:
		set_oob_legacy_data(data, len);
		return;
#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	case GAP_OOB_SC_GET_LOCAL_DATA:
		get_oob_sc_local_data();
		return;
	case GAP_OOB_SC_SET_REMOTE_DATA:
		set_oob_sc_remote_data(data, len);
		return;
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */
	default:
		LOG_WRN("Unknown opcode: 0x%x", opcode);
		tester_rsp(BTP_SERVICE_ID_GAP, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

static void tester_init_gap_cb(int err)
{
	if (err) {
		tester_rsp(BTP_SERVICE_ID_CORE, CORE_REGISTER_SERVICE,
			   BTP_INDEX_NONE, BTP_STATUS_FAILED);
		LOG_WRN("Error: %d", err);
		return;
	}

	atomic_clear(&current_settings);
	atomic_set_bit(&current_settings, GAP_SETTINGS_POWERED);
	atomic_set_bit(&current_settings, GAP_SETTINGS_CONNECTABLE);
	atomic_set_bit(&current_settings, GAP_SETTINGS_BONDABLE);
	atomic_set_bit(&current_settings, GAP_SETTINGS_LE);
#if defined(CONFIG_BT_PRIVACY)
	atomic_set_bit(&current_settings, GAP_SETTINGS_PRIVACY);
#endif /* CONFIG_BT_PRIVACY */

	bt_conn_cb_register(&conn_callbacks);

	tester_rsp(BTP_SERVICE_ID_CORE, CORE_REGISTER_SERVICE, BTP_INDEX_NONE,
		   BTP_STATUS_SUCCESS);
}

uint8_t tester_init_gap(void)
{
	int err;

	err = bt_enable(tester_init_gap_cb);
	if (err < 0) {
		LOG_ERR("Unable to enable Bluetooth: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_gap(void)
{
	return BTP_STATUS_SUCCESS;
}
