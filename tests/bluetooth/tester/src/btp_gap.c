/* gap.c - Bluetooth GAP Tester */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>

#include <hci_core.h>
#include <iso_internal.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_gap
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);
#define CONTROLLER_NAME "btp_tester"

#define BT_LE_AD_DISCOV_MASK (BT_LE_AD_LIMITED | BT_LE_AD_GENERAL)
#if defined(CONFIG_BT_EXT_ADV)
#define ADV_BUF_LEN (sizeof(struct btp_gap_device_found_ev) + 2 * CONFIG_BT_EXT_SCAN_BUF_SIZE)
#else
#define ADV_BUF_LEN (sizeof(struct btp_gap_device_found_ev) + 2 * 31)
#endif

#if defined(CONFIG_BT_CLASSIC)
#define BR_ADV_BUF_LEN (sizeof(struct btp_gap_device_found_ev) + BT_BR_EIR_SIZE_MAX)
#endif /* CONFIG_BT_CLASSIC */

static atomic_t current_settings;
struct bt_conn_auth_cb cb;
static uint8_t oob_legacy_tk[16] = { 0 };

static bool filter_list_in_use;

static struct bt_le_oob oob_sc_local = { 0 };
static struct bt_le_oob oob_sc_remote = { 0 };

/* connection parameters for rejection test */
#define REJECT_INTERVAL_MIN 0x0C80
#define REJECT_INTERVAL_MAX 0x0C80
#define REJECT_LATENCY 0x0000
#define REJECT_SUPERVISION_TIMEOUT 0x0C80

static struct {
	bt_addr_le_t addr;
	bool supported;
} cars[CONFIG_BT_MAX_PAIRED];

static uint8_t read_car_cb(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_read_params *params, const void *data,
			  uint16_t length)
{
	struct bt_conn_info info;
	bool supported = false;

	if (!err && data && length == 1) {
		const uint8_t *tmp = data;

		/* only 0 or 1 are valid values */
		if (tmp[0] == 1) {
			supported = true;
		}
	}

	bt_conn_get_info(conn, &info);

	for (int i = 0; i < CONFIG_BT_MAX_PAIRED; i++) {
		if (bt_addr_le_eq(info.le.dst, &cars[i].addr)) {
			cars[i].supported = supported;
			break;
		}
	}

	return BT_GATT_ITER_STOP;
}

static struct bt_gatt_read_params read_car_params = {
	.func = read_car_cb,
	.by_uuid.uuid = BT_UUID_CENTRAL_ADDR_RES,
	.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
	.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
};

static void le_connected(struct bt_conn *conn, uint8_t err)
{
	struct btp_gap_device_connected_ev ev;
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;

	(void)memset(addr_str, 0, sizeof(addr_str));

	if (bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		(void)bt_addr_to_str(bt_conn_get_dst_br(conn), addr_str, sizeof(addr_str));
	} else {
		LOG_WRN("Unsupported transport");
		return;
	}

	LOG_DBG("%s: 0x%02x", addr_str, err);

	if (err) {
		return;
	}

	bt_conn_get_info(conn, &info);

	(void)memset(&ev, 0, sizeof(ev));

	if (bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		bt_addr_le_copy(&ev.address, info.le.dst);
		ev.interval = sys_cpu_to_le16(info.le.interval);
		ev.latency = sys_cpu_to_le16(info.le.latency);
		ev.timeout = sys_cpu_to_le16(info.le.timeout);
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&ev.address.a, info.br.dst);
	}

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_CONNECTED, &ev, sizeof(ev));

	if (info.role == BT_CONN_ROLE_PERIPHERAL) {
		struct btp_gap_new_settings_ev sev;

		atomic_clear_bit(&current_settings, BTP_GAP_SETTINGS_ADVERTISING);
		sev.current_settings = sys_cpu_to_le32(current_settings);
		tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_NEW_SETTINGS, &sev, sizeof(sev));
	}
}

static void le_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct btp_gap_device_disconnected_ev ev;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		const bt_addr_le_t *addr;

		addr = bt_conn_get_dst(conn);
		(void)bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		bt_addr_le_copy(&ev.address, addr);
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		const bt_addr_t *br_addr;

		br_addr = bt_conn_get_dst_br(conn);
		(void)bt_addr_to_str(br_addr, addr_str, sizeof(addr_str));
		ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&ev.address.a, br_addr);
	} else {
		LOG_WRN("Unsupported transport");
		return;
	}

	LOG_DBG("%s: 0x%02x", addr_str, reason);

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_DISCONNECTED, &ev, sizeof(ev));
}

static void le_identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
				 const bt_addr_le_t *identity)
{
	struct btp_gap_identity_resolved_ev ev;

	bt_addr_le_copy(&ev.address, rpa);
	bt_addr_le_copy(&ev.identity_address, identity);

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_IDENTITY_RESOLVED, &ev, sizeof(ev));
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	struct btp_gap_conn_param_update_ev ev;
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);

	if (!bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		return;
	}

	bt_addr_le_copy(&ev.address, addr);
	ev.interval = sys_cpu_to_le16(interval);
	ev.latency = sys_cpu_to_le16(latency);
	ev.timeout = sys_cpu_to_le16(timeout);

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_CONN_PARAM_UPDATE, &ev, sizeof(ev));
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	/* reject update if all parameters match reject pattern */
	if ((param->interval_min == REJECT_INTERVAL_MIN) &&
			(param->interval_max == REJECT_INTERVAL_MAX) &&
			(param->latency == REJECT_LATENCY) &&
			(param->timeout == REJECT_SUPERVISION_TIMEOUT)) {
		return false;
	}

	return true;
}

static void le_security_changed(struct bt_conn *conn, bt_security_t level,
				enum bt_security_err err)
{
	struct btp_gap_sec_level_changed_ev sec_ev;
	struct btp_gap_bond_lost_ev bond_ev;
	struct btp_gap_encryption_change_ev enc_ev;
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) != 0) {
		LOG_WRN("Failed to get connection info of %p", conn);
		return;
	}

	if (info.type == BT_CONN_TYPE_LE) {
		bt_addr_le_copy(&sec_ev.address, info.le.dst);
		bt_addr_le_copy(&bond_ev.address, info.le.dst);
		bt_addr_le_copy(&enc_ev.address, info.le.dst);
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && (info.type == BT_CONN_TYPE_BR)) {
		sec_ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&sec_ev.address.a, info.br.dst);
		bond_ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&bond_ev.address.a, info.br.dst);
		enc_ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&enc_ev.address.a, info.br.dst);
	} else {
		LOG_WRN("Unsupported transport");
		return;
	}

	enc_ev.enabled = (err == BT_SECURITY_ERR_SUCCESS) ? true : false;
	enc_ev.key_size = info.security.enc_key_size;
	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_ENCRYPTION_CHANGE, &enc_ev, sizeof(enc_ev));

	switch (err) {
	case BT_SECURITY_ERR_SUCCESS:
		/* enum matches BTP values */
		sec_ev.sec_level = level;

		tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_SEC_LEVEL_CHANGED,
			     &sec_ev, sizeof(sec_ev));
		break;
	case BT_SECURITY_ERR_PIN_OR_KEY_MISSING:
		/* for central role this means that peer have no LTK when we
		 * started encryption procedure
		 *
		 * This means bond is lost and we restart pairing to re-bond
		 */
		if (info.role == BT_CONN_ROLE_CENTRAL) {
			LOG_DBG("Bond lost");

			tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BOND_LOST,
				     &bond_ev, sizeof(bond_ev));

			(void)bt_conn_set_security(conn, BT_SECURITY_L2 | BT_SECURITY_FORCE_PAIR);
		}
		break;
	default:
		break;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = le_connected,
	.disconnected = le_disconnected,
	.identity_resolved = le_identity_resolved,
	.le_param_updated = le_param_updated,
	.le_param_req = le_param_req,
	.security_changed = le_security_changed,
};

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_gap_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_GAP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t controller_index_list(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	struct btp_gap_read_controller_index_list_rp *rp = rsp;

	rp->num = 1U;
	rp->index[0] = BTP_INDEX;

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t controller_info(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	struct btp_gap_read_controller_info_rp *rp = rsp;
	uint32_t supported_settings;
	struct bt_le_oob oob_local = { 0 };

	bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);

	bt_addr_copy(&rp->address, &oob_local.addr.a);

	/*
	 * Re-use the oob data read here in get_oob_sc_local_data()
	 */
	if (!IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		oob_sc_local = oob_local;
	}

	/*
	 * If privacy is used, the device uses random type address, otherwise
	 * static random or public type address is used.
	 */
	if (!IS_ENABLED(CONFIG_BT_PRIVACY)) {
		if (oob_local.addr.type == BT_ADDR_LE_RANDOM) {
			atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_STATIC_ADDRESS);
		}
	}

	supported_settings = BIT(BTP_GAP_SETTINGS_POWERED);
	supported_settings |= BIT(BTP_GAP_SETTINGS_CONNECTABLE);
	supported_settings |= BIT(BTP_GAP_SETTINGS_BONDABLE);
	supported_settings |= BIT(BTP_GAP_SETTINGS_LE);
	supported_settings |= BIT(BTP_GAP_SETTINGS_ADVERTISING);
	supported_settings |= BIT(BTP_GAP_SETTINGS_EXTENDED_ADVERTISING);
	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		supported_settings |= BIT(BTP_GAP_SETTINGS_BREDR);
		supported_settings |= BIT(BTP_GAP_SETTINGS_SSP);
	}

	rp->supported_settings = sys_cpu_to_le32(supported_settings);
	rp->current_settings = sys_cpu_to_le32(current_settings);

	memcpy(rp->name, CONTROLLER_NAME, sizeof(CONTROLLER_NAME));

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static const char *oob_config_str(int oob_config)
{
	if (!IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		return "no";
	}

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

	switch (oob_info->type) {
	case BT_CONN_OOB_LE_SC:
	{
		if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
			LOG_ERR("OOB LE SC not supported");
			break;
		}

		LOG_DBG("Set %s OOB SC data for %s, ",
			oob_config_str(oob_info->lesc.oob_config),
			addr);

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
		    !bt_addr_le_eq(info.le.local, &oob_sc_local.addr)) {
			bt_addr_le_to_str(info.le.local, addr, sizeof(addr));
			LOG_DBG("No OOB data available for local %s",
				addr);
			bt_conn_auth_cancel(conn);
			return;
		}

		err = bt_le_oob_set_sc_data(conn, oobd_local, oobd_remote);
		if (err) {
			LOG_DBG("bt_le_oob_set_sc_data failed with: %d", err);
		}

		break;
	}

	case BT_CONN_OOB_LE_LEGACY:
		if (IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY)) {
			LOG_ERR("OOB LE Legacy not supported");
			break;
		}

		LOG_DBG("Legacy OOB TK requested from remote %s", addr);

		err = bt_le_oob_set_legacy_tk(conn, oob_legacy_tk);
		if (err < 0) {
			LOG_ERR("Failed to set OOB TK: %d", err);
		}

		break;
	default:
		LOG_ERR("Unhandled OOB type %d", oob_info->type);
		break;
	}
}

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
static uint8_t get_oob_sc_local_data(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	struct btp_gap_oob_sc_get_local_data_rp *rp = rsp;

	cb.oob_data_request = oob_data_request;

	memcpy(rp->conf, &oob_sc_local.le_sc_data.c[0], sizeof(rp->conf));
	memcpy(rp->rand, &oob_sc_local.le_sc_data.r[0], sizeof(rp->rand));

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t set_oob_sc_remote_data(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_oob_sc_set_remote_data_cmd *cp = cmd;

	cb.oob_data_request = oob_data_request;
	bt_le_oob_set_sc_flag(true);

	/* Note that the .addr field
	 * will be set by the oob_data_request callback
	 */
	memcpy(&oob_sc_remote.le_sc_data.r[0], cp->rand,
	       sizeof(oob_sc_remote.le_sc_data.r));
	memcpy(&oob_sc_remote.le_sc_data.c[0], cp->conf,
	       sizeof(oob_sc_remote.le_sc_data.c));

	return BTP_STATUS_SUCCESS;
}
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

static uint8_t set_powered(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_set_powered_cmd *cp = cmd;
	struct btp_gap_set_powered_rp *rp = rsp;
	int err;

	if (cp->powered) {
		err = bt_enable(NULL);
		if (err < 0) {
			LOG_ERR("Unable to enable Bluetooth: %d", err);
			return BTP_STATUS_FAILED;
		}
		bt_conn_cb_register(&conn_callbacks);
		atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_POWERED);
	} else {
		err = bt_disable();
		if (err < 0) {
			LOG_ERR("Unable to disable Bluetooth: %d", err);
			return BTP_STATUS_FAILED;
		}
		bt_conn_cb_unregister(&conn_callbacks);
		atomic_clear_bit(&current_settings, BTP_GAP_SETTINGS_POWERED);
	}
	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_connectable(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_set_connectable_cmd *cp = cmd;
	struct btp_gap_set_connectable_rp *rp = rsp;

	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		int err;

		err = bt_br_set_connectable(cp->connectable ? true : false);
		if ((err < 0) && (err != -EALREADY)) {
			return BTP_STATUS_FAILED;
		}
	}

	if (cp->connectable) {
		atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_CONNECTABLE);
	} else {
		atomic_clear_bit(&current_settings, BTP_GAP_SETTINGS_CONNECTABLE);
	}

	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t ad_flags = BT_LE_AD_NO_BREDR;
static struct bt_data ad[10] = {
	BT_DATA(BT_DATA_FLAGS, &ad_flags, sizeof(ad_flags)),
};
static struct bt_data sd[10];

#if CONFIG_BT_EXT_ADV
static struct bt_le_ext_adv *ext_adv_sets[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
#else
static struct bt_le_ext_adv *ext_adv_sets[1];
#endif

struct bt_le_ext_adv *tester_gap_ext_adv_get(uint8_t ext_adv_idx)
{
	if (!IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		return NULL;
	}

	if (ext_adv_idx >= ARRAY_SIZE(ext_adv_sets)) {
		LOG_ERR("Invalid ext_adv_id: %d", ext_adv_idx);
		return NULL;
	}

	return ext_adv_sets[ext_adv_idx];
}

static int tester_gap_ext_adv_idx_free_get(void)
{
	if (!IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		return -ENOTSUP;
	}

	for (int i = 0; i < ARRAY_SIZE(ext_adv_sets); i++) {
		if (ext_adv_sets[i] == NULL) {
			return i;
		}
	}

	return -ENOMEM;
}

int tester_gap_start_ext_adv(struct bt_le_ext_adv *ext_adv)
{
	if (!IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		return -ENOTSUP;
	}

	int err;

	if (ext_adv == NULL) {
		LOG_ERR("Invalid ext_adv");
		return -EINVAL;
	}

	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		LOG_ERR("Failed to start advertising");
		return -EINVAL;
	}

	atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_ADVERTISING);

	return 0;
}

int tester_gap_stop_ext_adv(struct bt_le_ext_adv *ext_adv)
{
	if (!IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		return -ENOTSUP;
	}

	int err;

	if (ext_adv == NULL) {
		LOG_ERR("Invalid ext_adv");
		return -EINVAL;
	}

	err = bt_le_ext_adv_stop(ext_adv);
	if (err != 0) {
		LOG_ERR("Failed to stop advertising");

		return -EINVAL;
	}

	atomic_clear_bit(&current_settings, BTP_GAP_SETTINGS_ADVERTISING);

	return 0;
}

static uint8_t set_discoverable(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_set_discoverable_cmd *cp = cmd;
	struct btp_gap_set_discoverable_rp *rp = rsp;

	switch (cp->discoverable) {
	case BTP_GAP_NON_DISCOVERABLE:
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			int err;

			err = bt_br_set_discoverable(false, false);
			if ((err < 0) && (err != -EALREADY)) {
				return BTP_STATUS_FAILED;
			}
		}

		ad_flags &= ~(BT_LE_AD_GENERAL | BT_LE_AD_LIMITED);
		atomic_clear_bit(&current_settings, BTP_GAP_SETTINGS_DISCOVERABLE);
		break;
	case BTP_GAP_GENERAL_DISCOVERABLE:
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			int err;

			err = bt_br_set_connectable(true);
			if (err == -EALREADY) {
				err = bt_br_set_discoverable(false, false);
				if ((err < 0) && (err != -EALREADY)) {
					return BTP_STATUS_FAILED;
				}
			} else if (err < 0) {
				return BTP_STATUS_FAILED;
			}

			err = bt_br_set_discoverable(true, false);
			if (err < 0) {
				return BTP_STATUS_FAILED;
			}
			ad_flags &= ~BT_LE_AD_NO_BREDR;
		}

		ad_flags &= ~BT_LE_AD_LIMITED;
		ad_flags |= BT_LE_AD_GENERAL;
		atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_DISCOVERABLE);
		break;
	case BTP_GAP_LIMITED_DISCOVERABLE:
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			int err;

			err = bt_br_set_connectable(true);
			if (err == -EALREADY) {
				err = bt_br_set_discoverable(false, false);
				if ((err < 0) && (err != -EALREADY)) {
					return BTP_STATUS_FAILED;
				}
			} else if (err < 0) {
				return BTP_STATUS_FAILED;
			}

			err = bt_br_set_discoverable(true, true);
			if (err < 0) {
				return BTP_STATUS_FAILED;
			}
			ad_flags &= ~BT_LE_AD_NO_BREDR;
		}

		ad_flags &= ~BT_LE_AD_GENERAL;
		ad_flags |= BT_LE_AD_LIMITED;
		atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_DISCOVERABLE);
		break;
	default:
		return BTP_STATUS_FAILED;
	}

	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t set_bondable(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_set_bondable_cmd *cp = cmd;
	struct btp_gap_set_bondable_rp *rp = rsp;

	LOG_DBG("bondable: %d", cp->bondable);

	if (cp->bondable) {
		atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_BONDABLE);
	} else {
		atomic_clear_bit(&current_settings, BTP_GAP_SETTINGS_BONDABLE);
	}

	bt_set_bondable(cp->bondable);

	rp->current_settings = sys_cpu_to_le32(current_settings);
	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

int tester_gap_create_adv_instance(struct bt_le_adv_param *param,
				   uint8_t own_addr_type, const struct bt_data *ad, size_t ad_len,
				   const struct bt_data *sd, size_t sd_len, uint32_t *settings,
				   struct bt_le_ext_adv **ext_adv)
{
	int err = 0;

	if (settings != NULL) {
		atomic_set(&current_settings, *settings);
	}

	if (atomic_test_bit(&current_settings, BTP_GAP_SETTINGS_CONNECTABLE)) {
		param->options |= BT_LE_ADV_OPT_CONN;

		if (filter_list_in_use) {
			param->options |= BT_LE_ADV_OPT_FILTER_CONN;
		}
	}

	if (filter_list_in_use) {
		param->options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
	}

	switch (own_addr_type) {
	case BTP_GAP_ADDR_TYPE_IDENTITY:
		param->options |= BT_LE_ADV_OPT_USE_IDENTITY;
		break;
	case BTP_GAP_ADDR_TYPE_RESOLVABLE_PRIVATE:
		if (!IS_ENABLED(CONFIG_BT_PRIVACY)) {
			return -EINVAL;
		}

		/* RPA usage is controlled via privacy settings */
		if (!atomic_test_bit(&current_settings, BTP_GAP_SETTINGS_PRIVACY)) {
			return -EINVAL;
		}
		break;
	case BTP_GAP_ADDR_TYPE_NON_RESOLVABLE_PRIVATE:
		if (!IS_ENABLED(CONFIG_BT_PRIVACY)) {
			return -EINVAL;
		}

		/* NRPA is used only for non-connectable advertising */
		if (atomic_test_bit(&current_settings, BTP_GAP_SETTINGS_CONNECTABLE)) {
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) && atomic_test_bit(&current_settings,
	    BTP_GAP_SETTINGS_EXTENDED_ADVERTISING)) {

		param->options |= BT_LE_ADV_OPT_EXT_ADV;
		if (*ext_adv) {
			err = bt_le_ext_adv_stop(*ext_adv);
			if (err != 0) {
				return err;
			}

			err = bt_le_ext_adv_delete(*ext_adv);
			if (err != 0) {
				return err;
			}

			*ext_adv = NULL;
		}

		int index = tester_gap_ext_adv_idx_free_get();

		if (index < 0) {
			LOG_ERR("No free ext_adv index");
			return -ENOMEM;
		}

		/* Set the set_id to be matching the index of the ext_adv set */
		param->sid = index;

		err = bt_le_ext_adv_create(param, NULL, &ext_adv_sets[index]);
		if (err != 0) {
			return BTP_STATUS_FAILED;
		}

		err = bt_le_ext_adv_set_data(ext_adv_sets[index], ad, ad_len, sd_len ?
					     sd : NULL, sd_len);

		*ext_adv = ext_adv_sets[index];
	}

	return err;
}

static uint8_t start_advertising(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_start_advertising_cmd *cp = cmd;
	struct btp_gap_start_advertising_rp *rp = rsp;
	struct bt_le_adv_param param =
		BT_LE_ADV_PARAM_INIT(0, BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);
	uint8_t own_addr_type;
	uint32_t duration;
	uint8_t adv_len;
	uint8_t sd_len;
	int err;
	int i;

	/* This command is very unfortunate since after variable data there is
	 * additional 5 bytes (4 bytes for duration, 1 byte for own address
	 * type.
	 */
	if ((cmd_len < sizeof(*cp)) ||
	    (cmd_len != sizeof(*cp) + cp->adv_data_len + cp->scan_rsp_len +
			    sizeof(duration) + sizeof(own_addr_type))) {
		return BTP_STATUS_FAILED;
	}

	/* currently ignored */
	duration = sys_get_le32(cp->adv_sr_data + cp->adv_data_len + cp->scan_rsp_len);
	(void)duration;
	own_addr_type = cp->adv_sr_data[cp->adv_data_len + cp->scan_rsp_len + sizeof(duration)];

	for (i = 0, adv_len = 1U; i < cp->adv_data_len; adv_len++) {
		if (adv_len >= ARRAY_SIZE(ad)) {
			LOG_ERR("ad[] Out of memory");
			return BTP_STATUS_FAILED;
		}

		ad[adv_len].type = cp->adv_sr_data[i++];
		ad[adv_len].data_len = cp->adv_sr_data[i++];
		ad[adv_len].data = &cp->adv_sr_data[i];
		i += ad[adv_len].data_len;
	}

	for (sd_len = 0U; i < cp->adv_data_len + cp->scan_rsp_len; sd_len++) {
		if (sd_len >= ARRAY_SIZE(sd)) {
			LOG_ERR("sd[] Out of memory");
			return BTP_STATUS_FAILED;
		}

		sd[sd_len].type = cp->adv_sr_data[i++];
		sd[sd_len].data_len = cp->adv_sr_data[i++];
		sd[sd_len].data = &cp->adv_sr_data[i];
		i += sd[sd_len].data_len;
	}

	struct bt_le_ext_adv *ext_adv = NULL;

	err = tester_gap_create_adv_instance(&param, own_addr_type, ad, adv_len, sd,
					     sd_len, NULL, &ext_adv);
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    atomic_test_bit(&current_settings, BTP_GAP_SETTINGS_EXTENDED_ADVERTISING)) {
		err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	} else {
		err = bt_le_adv_start(&param, ad, adv_len, sd_len ? sd : NULL, sd_len);
	}

	/* BTP API don't allow to set empty scan response data. */
	if (err < 0) {
		LOG_ERR("Failed to start advertising");

		return BTP_STATUS_FAILED;
	}

	atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_ADVERTISING);
	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t start_directed_advertising(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_start_directed_adv_cmd *cp = cmd;
	struct btp_gap_start_directed_adv_rp *rp = rsp;
	struct bt_le_adv_param adv_param;
	uint16_t options = sys_le16_to_cpu(cp->options);

	adv_param = *BT_LE_ADV_CONN_DIR(&cp->address);

	if (!(options & BTP_GAP_START_DIRECTED_ADV_HD)) {
		adv_param.options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;
		adv_param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
		adv_param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY) && (options & BTP_GAP_START_DIRECTED_ADV_PEER_RPA)) {
		/* check if peer supports Central Address Resolution */
		for (int i = 0; i < CONFIG_BT_MAX_PAIRED; i++) {
			if (bt_addr_le_eq(&cp->address, &cars[i].addr)) {
				if (cars[i].supported) {
					adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;
				}
			}
		}
	}

	if (bt_le_adv_start(&adv_param, NULL, 0, NULL, 0) < 0) {
		LOG_ERR("Failed to start advertising");
		return BTP_STATUS_FAILED;
	}

	atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_ADVERTISING);
	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t stop_advertising(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	struct btp_gap_stop_advertising_rp *rp = rsp;
	int err;

	err = bt_le_adv_stop();
	if (err < 0) {
		tester_rsp(BTP_SERVICE_ID_GAP, BTP_GAP_STOP_ADVERTISING, BTP_STATUS_FAILED);
		LOG_ERR("Failed to stop advertising: %d", err);
		return BTP_STATUS_FAILED;
	}

	atomic_clear_bit(&current_settings, BTP_GAP_SETTINGS_ADVERTISING);
	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t get_ad_flags(struct net_buf_simple *buf_ad)
{
	uint8_t len, i;

	/* Parse advertisement to get flags */
	for (i = 0U; i < buf_ad->len; i += len - 1) {
		len = buf_ad->data[i++];
		if (!len) {
			break;
		}

		/* Check if field length is correct */
		if (len > (buf_ad->len - i) || (buf_ad->len - i) < 1) {
			break;
		}

		switch (buf_ad->data[i++]) {
		case BT_DATA_FLAGS:
			return buf_ad->data[i];
		default:
			break;
		}
	}

	return 0;
}

static uint8_t discovery_flags;
static struct net_buf_simple *adv_buf = NET_BUF_SIMPLE(ADV_BUF_LEN);

static void store_adv(const bt_addr_le_t *addr, int8_t rssi,
		      struct net_buf_simple *buf_ad)
{
	struct btp_gap_device_found_ev *ev;

	/* cleanup */
	net_buf_simple_init(adv_buf, 0);

	ev = net_buf_simple_add(adv_buf, sizeof(*ev));

	bt_addr_le_copy(&ev->address, addr);
	ev->rssi = rssi;
	ev->flags = BTP_GAP_DEVICE_FOUND_FLAG_AD | BTP_GAP_DEVICE_FOUND_FLAG_RSSI;
	ev->eir_data_len = buf_ad->len;
	memcpy(net_buf_simple_add(adv_buf, buf_ad->len), buf_ad->data, buf_ad->len);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t evtype,
			 struct net_buf_simple *buf_ad)
{
	/* if General/Limited Discovery - parse Advertising data to get flags */
	if (!(discovery_flags & BTP_GAP_DISCOVERY_FLAG_LE_OBSERVE) &&
	    (evtype != BT_GAP_ADV_TYPE_SCAN_RSP)) {
		uint8_t flags = get_ad_flags(buf_ad);

		/* ignore non-discoverable devices */
		if (!(flags & BT_LE_AD_DISCOV_MASK)) {
			LOG_DBG("Non discoverable, skipping");
			return;
		}

		/* if Limited Discovery - ignore general discoverable devices */
		if ((discovery_flags & BTP_GAP_DISCOVERY_FLAG_LIMITED) &&
		    !(flags & BT_LE_AD_LIMITED)) {
			LOG_DBG("General discoverable, skipping");
			return;
		}
	}

	/* attach Scan Response data */
	if (evtype == BT_GAP_ADV_TYPE_SCAN_RSP) {
		struct btp_gap_device_found_ev *ev;
		bt_addr_le_t a;

		/* skip if there is no pending advertisement */
		if (!adv_buf->len) {
			LOG_INF("No pending advertisement, skipping");
			return;
		}

		ev = (void *) adv_buf->data;

		bt_addr_le_copy(&a, &ev->address);

		/*
		 * in general, the Scan Response comes right after the
		 * Advertisement, but if not if send stored event and ignore
		 * this one
		 */
		if (!bt_addr_le_eq(addr, &a)) {
			LOG_INF("Address does not match, skipping");
			goto done;
		}

		ev->eir_data_len += buf_ad->len;
		ev->flags |= BTP_GAP_DEVICE_FOUND_FLAG_SD;

		memcpy(net_buf_simple_add(adv_buf, buf_ad->len), buf_ad->data, buf_ad->len);

		goto done;
	}

	/*
	 * if there is another pending advertisement, send it and store the
	 * current one
	 */
	if (adv_buf->len) {
		tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_FOUND,
			    adv_buf->data, adv_buf->len);
		net_buf_simple_reset(adv_buf);
	}

	store_adv(addr, rssi, buf_ad);

	/* if Active Scan and scannable event - wait for Scan Response */
	if ((discovery_flags & BTP_GAP_DISCOVERY_FLAG_LE_ACTIVE_SCAN) &&
	    (evtype == BT_GAP_ADV_TYPE_ADV_IND ||
	     evtype == BT_GAP_ADV_TYPE_ADV_SCAN_IND)) {
		LOG_DBG("Waiting for scan response");
		return;
	}
done:
	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_FOUND,
		    adv_buf->data, adv_buf->len);
	net_buf_simple_reset(adv_buf);
}

#if defined(CONFIG_BT_CLASSIC)
static struct net_buf_simple *br_adv_buf = NET_BUF_SIMPLE(BR_ADV_BUF_LEN);

static void br_discover_recv(const struct bt_br_discovery_result *result)
{
	struct btp_gap_device_found_ev *ev;

	/* cleanup */
	net_buf_simple_init(br_adv_buf, 0);

	ev = net_buf_simple_add(br_adv_buf, sizeof(*ev));

	ev = (void *) br_adv_buf->data;
	ev->address.type = BTP_BR_ADDRESS_TYPE;
	bt_addr_copy(&ev->address.a, &result->addr);

	ev->eir_data_len = 0;
	ev->rssi = result->rssi;
	ev->flags = BTP_GAP_DEVICE_FOUND_FLAG_AD | BTP_GAP_DEVICE_FOUND_FLAG_RSSI;
	for (size_t i = 0U; i < sizeof(result->eir); i = ev->eir_data_len) {
		if (result->eir[i] != 0) {
			/* Append EIR length and length field itself */
			ev->eir_data_len += result->eir[i] + 1;
		} else {
			/* invalid length found, jump out*/
			break;
		}
	}
	(void)memcpy(net_buf_simple_add(br_adv_buf, ev->eir_data_len), result->eir,
		     ev->eir_data_len);

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_FOUND, br_adv_buf->data,
		     br_adv_buf->len);
	net_buf_simple_reset(br_adv_buf);
}

static struct bt_br_discovery_cb br_discover = {
	.recv = br_discover_recv,
};

static struct bt_br_discovery_param br_discover_param;

#define BR_DISCOVER_RESULT_COUNT 10
static struct bt_br_discovery_result br_discover_result[BR_DISCOVER_RESULT_COUNT];

static uint8_t br_start_discovery(const struct btp_gap_start_discovery_cmd *cp)
{
	static bool registered;
	int err;

	if (!registered) {
		registered = true;
		bt_br_discovery_cb_register(&br_discover);
	}

	br_discover_param.length = BR_DISCOVER_RESULT_COUNT;
	if (cp->flags & BTP_GAP_DISCOVERY_FLAG_LIMITED) {
		br_discover_param.limited = true;
	} else {
		br_discover_param.limited = false;
	}

	err = bt_br_discovery_start(&br_discover_param, br_discover_result,
				    ARRAY_SIZE(br_discover_result));
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#else
static uint8_t br_start_discovery(const struct btp_gap_start_discovery_cmd *cp)
{
	LOG_WRN("BR/EDR not supported");
	return BTP_STATUS_FAILED;
}
#endif /* CONFIG_BT_CLASSIC */

static uint8_t start_discovery(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_start_discovery_cmd *cp = cmd;

	if (cp->flags & BTP_GAP_DISCOVERY_FLAG_BREDR) {
		/* Start BR discovery*/
		return br_start_discovery(cp);
	}

	/* Start LE scanning */
	if (bt_le_scan_start(cp->flags & BTP_GAP_DISCOVERY_FLAG_LE_ACTIVE_SCAN ?
			     BT_LE_SCAN_ACTIVE : BT_LE_SCAN_PASSIVE,
			     device_found) < 0) {
		LOG_ERR("Failed to start scanning");
		return BTP_STATUS_FAILED;
	}

	net_buf_simple_init(adv_buf, 0);
	discovery_flags = cp->flags;

	return BTP_STATUS_SUCCESS;
}

static uint8_t stop_discovery(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
		err = bt_br_discovery_stop();
		if ((err < 0) && (err != -EALREADY)) {
			LOG_ERR("Failed to stop discovery: %d", err);
			return BTP_STATUS_FAILED;
		}
	}

	err = bt_le_scan_stop();
	if (err < 0) {
		LOG_ERR("Failed to stop scanning: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_CLASSIC)
static uint8_t br_connect(const struct btp_gap_connect_cmd *cp)
{
	struct bt_conn *conn;

	conn = bt_conn_create_br(&cp->address.a, BT_BR_CONN_PARAM_DEFAULT);
	if (conn == NULL) {
		LOG_ERR("Failed to create connection");
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);

	return BTP_STATUS_SUCCESS;
}
#else
static uint8_t br_connect(const struct btp_gap_connect_cmd *cp)
{
	return BTP_STATUS_FAILED;
}
#endif /* CONFIG_BT_CLASSIC */

static uint8_t connect(const void *cmd, uint16_t cmd_len,
		       void *rsp, uint16_t *rsp_len)
{
	struct bt_le_conn_param conn_param;

	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST) || IS_ENABLED(CONFIG_BT_BAP_BROADCAST_ASSISTANT)) {
		conn_param = *BT_BAP_CONN_PARAM_RELAXED;
	} else {
		conn_param = *BT_LE_CONN_PARAM_DEFAULT;
	}

	const struct btp_gap_connect_cmd *cp = cmd;
	int err;

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		return br_connect(cp);
	}

	if (!bt_addr_le_eq(&cp->address, BT_ADDR_LE_ANY)) {
		struct bt_conn *conn = NULL;

		err = bt_conn_le_create(&cp->address, BT_CONN_LE_CREATE_CONN, &conn_param, &conn);
		if (err) {
			LOG_ERR("Failed to create connection (%d)", err);
			return BTP_STATUS_FAILED;
		}

		bt_conn_unref(conn);
	} else {
		err = bt_conn_le_create_auto(BT_CONN_LE_CREATE_CONN, &conn_param);
		if (err) {
			LOG_ERR("Failed to create auto connection (%d)", err);
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t disconnect(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_disconnect_cmd *cp = cmd;
	struct bt_conn *conn;
	uint8_t status;

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			conn = bt_conn_lookup_addr_br(&cp->address.a);
		} else {
			return BTP_STATUS_FAILED;
		}
	} else {
		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	}

	if (conn == NULL) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	if (bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN)) {
		LOG_ERR("Failed to disconnect");
		status = BTP_STATUS_FAILED;
	} else {
		status = BTP_STATUS_SUCCESS;
	}

	bt_conn_unref(conn);

	return status;
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	struct btp_gap_passkey_display_ev ev;

	if (bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		const bt_addr_le_t *addr;

		addr = bt_conn_get_dst(conn);
		bt_addr_le_copy(&ev.address, addr);
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		const bt_addr_t *br_addr;

		br_addr = bt_conn_get_dst_br(conn);
		ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&ev.address.a, br_addr);
	} else {
		LOG_WRN("Unsupported transport");
		return;
	}

	ev.passkey = sys_cpu_to_le32(passkey);

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PASSKEY_DISPLAY, &ev, sizeof(ev));
}

static void auth_passkey_entry(struct bt_conn *conn)
{
	struct btp_gap_passkey_entry_req_ev ev;

	if (bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		const bt_addr_le_t *addr;

		addr = bt_conn_get_dst(conn);
		bt_addr_le_copy(&ev.address, addr);
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		const bt_addr_t *br_addr;

		br_addr = bt_conn_get_dst_br(conn);
		ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&ev.address.a, br_addr);
	} else {
		LOG_WRN("Unsupported transport");
		return;
	}

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PASSKEY_ENTRY_REQ, &ev, sizeof(ev));
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	struct btp_gap_passkey_confirm_req_ev ev;

	if (bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		const bt_addr_le_t *addr;

		addr = bt_conn_get_dst(conn);
		bt_addr_le_copy(&ev.address, addr);
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		const bt_addr_t *br_addr;

		br_addr = bt_conn_get_dst_br(conn);
		ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&ev.address.a, br_addr);
	} else {
		LOG_WRN("Unsupported transport");
		return;
	}

	ev.passkey = sys_cpu_to_le32(passkey);

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PASSKEY_CONFIRM_REQ, &ev, sizeof(ev));
}

static void auth_cancel(struct bt_conn *conn)
{
	/* TODO */
}

enum bt_security_err auth_pairing_accept(struct bt_conn *conn,
					 const struct bt_conn_pairing_feat *const feat)
{
	struct btp_gap_bond_lost_ev ev;

	if (bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		const bt_addr_le_t *addr;

		addr = bt_conn_get_dst(conn);
		if (!bt_le_bond_exists(BT_ID_DEFAULT, addr)) {
			return BT_SECURITY_ERR_SUCCESS;
		}

		bt_addr_le_copy(&ev.address, addr);
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		const bt_addr_t *br_addr;

		br_addr = bt_conn_get_dst_br(conn);
		if (!bt_br_bond_exists(br_addr)) {
			return BT_SECURITY_ERR_SUCCESS;
		}

		ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&ev.address.a, br_addr);
	} else {
		LOG_WRN("Unsupported transport");
		return BTP_STATUS_FAILED;
	}

	/* If a peer is already bonded and tries to pair again then it means that
	 * the it has lost its bond information.
	 */
	LOG_DBG("Bond lost");

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BOND_LOST, &ev, sizeof(ev));

	return BT_SECURITY_ERR_SUCCESS;
}

void auth_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	struct btp_gap_bond_pairing_failed_ev ev;

	if (bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		const bt_addr_le_t *addr;

		addr = bt_conn_get_dst(conn);
		bt_addr_le_copy(&ev.address, addr);
	} else if (IS_ENABLED(CONFIG_BT_CLASSIC) && bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		const bt_addr_t *br_addr;

		br_addr = bt_conn_get_dst_br(conn);
		ev.address.type = BTP_BR_ADDRESS_TYPE;
		bt_addr_copy(&ev.address.a, br_addr);
	} else {
		LOG_WRN("Unsupported transport");
		return;
	}

	ev.reason = reason;

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PAIRING_FAILED, &ev, sizeof(ev));
}

static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
	if (IS_ENABLED(CONFIG_BT_PRIVACY) && bonded) {
		/* Read peer's Central Address Resolution if bonded */
		bt_gatt_read(conn, &read_car_params);
	}
}

static struct bt_conn_auth_info_cb auth_info_cb = {
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
};

#if defined(CONFIG_BT_CLASSIC)
static void auth_pincode_entry(struct bt_conn *conn, bool highsec)
{
	const char *pin = "0000";
	const char *pin16 = "0000000000000000";

	if (!bt_conn_is_type(conn, BT_CONN_TYPE_BR)) {
		return;
	}

	if (highsec) {
		bt_conn_auth_pincode_entry(conn, pin16);
	} else {
		bt_conn_auth_pincode_entry(conn, pin);
	}
}
#endif /* CONFIG_BT_CLASSIC */

static uint8_t set_io_cap(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_set_io_cap_cmd *cp = cmd;

	/* Reset io cap requirements */
	(void)memset(&cb, 0, sizeof(cb));
	bt_conn_auth_cb_register(NULL);

	LOG_DBG("io_cap: %d", cp->io_cap);

	switch (cp->io_cap) {
	case BTP_GAP_IO_CAP_DISPLAY_ONLY:
		cb.cancel = auth_cancel;
		cb.passkey_display = auth_passkey_display;
		break;
	case BTP_GAP_IO_CAP_KEYBOARD_DISPLAY:
		cb.cancel = auth_cancel;
		cb.passkey_display = auth_passkey_display;
		cb.passkey_entry = auth_passkey_entry;
		cb.passkey_confirm = auth_passkey_confirm;
		break;
	case BTP_GAP_IO_CAP_NO_INPUT_OUTPUT:
		cb.cancel = auth_cancel;
		break;
	case BTP_GAP_IO_CAP_KEYBOARD_ONLY:
		cb.cancel = auth_cancel;
		cb.passkey_entry = auth_passkey_entry;
		break;
	case BTP_GAP_IO_CAP_DISPLAY_YESNO:
		cb.cancel = auth_cancel;
		cb.passkey_display = auth_passkey_display;
		cb.passkey_confirm = auth_passkey_confirm;
		break;
	default:
		LOG_WRN("Unhandled io_cap: 0x%x", cp->io_cap);
		return BTP_STATUS_FAILED;
	}

	cb.pairing_accept = auth_pairing_accept;

#if defined(CONFIG_BT_CLASSIC)
	cb.pincode_entry = auth_pincode_entry;
#endif /* CONFIG_BT_CLASSIC */

	if (bt_conn_auth_cb_register(&cb)) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t pair(const void *cmd, uint16_t cmd_len,
		    void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_pair_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			conn = bt_conn_lookup_addr_br(&cp->address.a);
		} else {
			return BTP_STATUS_FAILED;
		}
	} else {
		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	}

	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	/*
	 * If the error code is `-EBUSY`, it means the pairing/enryption is ongoing.
	 * Just ignore the error.
	 */
	if ((err < 0) && (err != -EBUSY)) {
		LOG_ERR("Failed to set security: %d", err);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;
}

static uint8_t pair_v2(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_pair_v2_cmd *cp = cmd;
	struct bt_conn *conn;
	bt_security_t level;
	int err;

	/* TODO: Only supports mode 4 */
	if ((cp->mode != BTP_GAP_PAIR_V2_MODE_4) && (cp->mode != BTP_GAP_PAIR_V2_MODE_ANY)) {
		LOG_WRN("Unsupport mode %d", cp->mode);
		return BTP_STATUS_FAILED;
	}

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			conn = bt_conn_lookup_addr_br(&cp->address.a);
		} else {
			return BTP_STATUS_FAILED;
		}
	} else {
		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	}

	if (conn == NULL) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	level = (bt_security_t)cp->level;
	if (cp->flags & BTP_GAP_PAIR_V2_FLAG_FORCE_PAIR) {
		level = (bt_security_t)((uint8_t)level | BT_SECURITY_FORCE_PAIR);
	}

	err = bt_conn_set_security(conn, level);
	if (err < 0) {
		LOG_ERR("Failed to set security: %d", err);
		bt_conn_unref(conn);
		return BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);
	return BTP_STATUS_SUCCESS;
}

static uint8_t unpair(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_unpair_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			conn = bt_conn_lookup_addr_br(&cp->address.a);
		} else {
			return BTP_STATUS_FAILED;
		}
	} else {
		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	}

	if (!conn) {
		LOG_INF("Unknown connection");
		goto keys;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	bt_conn_unref(conn);

	if (err < 0) {
		LOG_ERR("Failed to disconnect: %d", err);
		return BTP_STATUS_FAILED;
	}
keys:
	err = bt_unpair(BT_ID_DEFAULT, &cp->address);
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t passkey_entry(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_passkey_entry_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			conn = bt_conn_lookup_addr_br(&cp->address.a);
		} else {
			return BTP_STATUS_FAILED;
		}
	} else {
		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	}

	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_conn_auth_passkey_entry(conn, sys_le32_to_cpu(cp->passkey));
	bt_conn_unref(conn);

	if (err < 0) {
		LOG_ERR("Failed to enter passkey: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t passkey_confirm(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_passkey_confirm_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			conn = bt_conn_lookup_addr_br(&cp->address.a);
		} else {
			return BTP_STATUS_FAILED;
		}
	} else {
		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	}

	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	if (cp->match) {
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

	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t conn_param_update(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_conn_param_update_cmd *cp = cmd;
	struct bt_le_conn_param param = {
		.interval_min = sys_le16_to_cpu(cp->interval_min),
		.interval_max = sys_le16_to_cpu(cp->interval_max),
		.latency = sys_le16_to_cpu(cp->latency),
		.timeout = sys_le16_to_cpu(cp->timeout),
	};
	struct bt_conn *conn;
	int err;

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			conn = bt_conn_lookup_addr_br(&cp->address.a);
		} else {
			return BTP_STATUS_FAILED;
		}
	} else {
		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	}

	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_conn_le_param_update(conn, &param);
	bt_conn_unref(conn);

	if (err < 0) {
		LOG_ERR("Failed to update params: %d", err);
		return BTP_STATUS_FAILED;
	}
	return BTP_STATUS_SUCCESS;
}

static uint8_t set_mitm(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
	/* TODO verify if can be done in runtime */
	LOG_WRN("Use CONFIG_BT_SMP_ENFORCE_MITM instead");

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_oob_legacy_data(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_oob_legacy_set_data_cmd *cp = cmd;

	memcpy(oob_legacy_tk, cp->oob_data, 16);

	bt_le_oob_set_legacy_flag(true);
	cb.oob_data_request = oob_data_request;

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_filter_list(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_set_filter_list *cp = cmd;
	int err;

	if ((cmd_len < sizeof(*cp)) ||
	    (cmd_len != sizeof(*cp) + (cp->cnt * sizeof(cp->addr[0])))) {
		return BTP_STATUS_FAILED;
	}

	(void)bt_le_filter_accept_list_clear();

	for (int i = 0; i < cp->cnt; i++) {
		err = bt_le_filter_accept_list_add(&cp->addr[i]);
		if (err < 0) {
			return BTP_STATUS_FAILED;
		}
	}

	filter_list_in_use = cp->cnt != 0;

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_EXT_ADV)
static uint8_t set_extended_advertising(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	const struct btp_gap_set_extended_advertising_cmd *cp = cmd;
	struct btp_gap_set_extended_advertising_rp *rp = rsp;

	LOG_DBG("ext adv settings: %u", cp->settings);

	if (cp->settings != 0) {
		atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_EXTENDED_ADVERTISING);
	} else {
		atomic_clear_bit(&current_settings, BTP_GAP_SETTINGS_EXTENDED_ADVERTISING);
	}

	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}
#endif /* defined(CONFIG_BT_EXT_ADV) */

static struct bt_le_per_adv_sync *pa_sync;

struct bt_le_per_adv_sync *tester_gap_padv_get(void)
{
	if (!IS_ENABLED(CONFIG_BT_PER_ADV)) {
		return NULL;
	}

	return pa_sync;
}

static void pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
			      struct bt_le_per_adv_sync_synced_info *info)
{
	LOG_DBG("");

	/* sync transfer received */
	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER) && info->conn != NULL) {
		struct btp_gap_ev_periodic_transfer_received_ev ev;
		struct bt_conn_info conn_info;

		if (pa_sync != NULL) {
			return;
		}

		pa_sync = sync;

		bt_conn_get_info(info->conn, &conn_info);

		bt_addr_le_copy(&ev.adv_address, &sync->addr);
		ev.sync_handle = sys_cpu_to_le16(sync->handle);
		ev.status = 0;
		bt_addr_le_copy(&ev.peer_address, conn_info.le.dst);

		tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PERIODIC_TRANSFER_RECEIVED,
			     &ev, sizeof(ev));
		return;
	}

	/* locally initiated */
	if (sync == pa_sync) {
		struct btp_gap_ev_periodic_sync_established_ev ev;

		bt_addr_le_copy(&ev.address, info->addr);
		ev.sync_handle = sys_cpu_to_le16(sync->handle);
		ev.status = 0;

		tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PERIODIC_SYNC_ESTABLISHED,
			     &ev, sizeof(ev));
	}
}

static void pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				  const struct bt_le_per_adv_sync_term_info *info)
{
	LOG_DBG("");

	if (sync == pa_sync) {
		struct btp_gap_ev_periodic_sync_lost_ev ev;

		LOG_DBG("PA sync lost with reason %u", info->reason);
		pa_sync = NULL;

		ev.sync_handle = sys_cpu_to_le16(sync->handle);
		ev.reason = info->reason;

		tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PERIODIC_SYNC_LOST,
			     &ev, sizeof(ev));
	}
}

static struct net_buf_simple *periodic_adv_buf =
	NET_BUF_SIMPLE(sizeof(struct btp_gap_ev_periodic_report_ev) + UINT8_MAX);

static void pa_sync_recv_cb(struct bt_le_per_adv_sync *sync,
			    const struct bt_le_per_adv_sync_recv_info *info,
			    struct net_buf_simple *buf)
{
	struct btp_gap_ev_periodic_report_ev *ev;

	LOG_DBG("");

	/* cleanup */
	net_buf_simple_init(periodic_adv_buf, 0);

	ev = net_buf_simple_add(periodic_adv_buf, sizeof(*ev));

	ev->sync_handle = sys_cpu_to_le16(sync->handle);
	ev->rssi = info->rssi;
	ev->cte_type = info->cte_type;

	/* TODO BTP has fragmented data (as in HCI) */
	if (buf->len > UINT8_MAX) {
		ev->data_len = UINT8_MAX;
	} else {
		ev->data_len = buf->len;
	}

	memcpy(net_buf_simple_add(periodic_adv_buf, ev->data_len), buf->data, ev->data_len);

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PERIODIC_REPORT,
		     ev, sizeof(*ev) + ev->data_len);
}

static struct bt_le_per_adv_sync_cb pa_sync_cb = {
	.synced = pa_sync_synced_cb,
	.term = pa_sync_terminated_cb,
	.recv = pa_sync_recv_cb,
};

#if defined(CONFIG_BT_PER_ADV)
static struct bt_data padv[10];

int tester_gap_padv_configure(struct bt_le_ext_adv *ext_adv,
			      const struct bt_le_per_adv_param *param)
{
	int err;
	struct bt_le_adv_param ext_adv_param =
		BT_LE_ADV_PARAM_INIT(0, param->interval_min, param->interval_max, NULL);

	if (ext_adv == NULL) {
		current_settings = BIT(BTP_GAP_SETTINGS_DISCOVERABLE) |
				   BIT(BTP_GAP_SETTINGS_EXTENDED_ADVERTISING);
		err = tester_gap_create_adv_instance(&ext_adv_param,
						     BTP_GAP_ADDR_TYPE_IDENTITY, ad, 1, NULL, 0,
						     NULL, &ext_adv);
		if (err != 0) {
			return -EINVAL;
		}
	}

	/* Set periodic advertising parameters and the required
	 * bit in AD Flags of extended advertising.
	 */
	err = bt_le_per_adv_set_param(ext_adv, param);
	if (err != 0) {
		LOG_DBG("Failed to set periodic advertising parameters (err %d)\n", err);
	}

	return err;
}

static uint8_t padv_configure(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	int err;
	uint32_t options = BT_LE_PER_ADV_OPT_NONE;
	const struct btp_gap_padv_configure_cmd *cp = cmd;
	struct btp_gap_padv_configure_rp *rp = rsp;

	if (cp->flags & BTP_GAP_PADV_INCLUDE_TX_POWER) {
		options |= BT_LE_PER_ADV_OPT_USE_TX_POWER;
	}

	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get(0);

	err = tester_gap_padv_configure(ext_adv,
					BT_LE_PER_ADV_PARAM(sys_le16_to_cpu(cp->interval_min),
					sys_le16_to_cpu(cp->interval_max), options));
	if (err) {
		return BTP_STATUS_FAILED;
	}

	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

int tester_gap_padv_start(struct bt_le_ext_adv *ext_adv)
{
	int err;

	if (ext_adv == NULL) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&current_settings, BTP_GAP_SETTINGS_ADVERTISING)) {
		err = tester_gap_start_ext_adv(ext_adv);
		if (err != 0) {
			return -EINVAL;
		}
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(ext_adv);
	if (err != 0) {
		LOG_DBG("Failed to start periodic advertising data: %d", err);
	}

	return err;
}

static uint8_t padv_start(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	int err;
	struct btp_gap_padv_start_rp *rp = rsp;

	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get(0);

	err = tester_gap_padv_start(ext_adv);

	if (err) {
		return BTP_STATUS_FAILED;
	}

	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

int tester_gap_padv_stop(struct bt_le_ext_adv *ext_adv)
{
	int err;

	if (ext_adv == NULL) {
		/* Ext adv not yet created */
		return -ESRCH;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_stop(ext_adv);
	if (err != 0) {
		LOG_DBG("Failed to stop periodic advertising data: %d", err);
	}

	return err;
}

static uint8_t padv_stop(const void *cmd, uint16_t cmd_len,
			 void *rsp, uint16_t *rsp_len)
{
	int err;
	struct btp_gap_padv_stop_rp *rp = rsp;
	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get(0);

	err = tester_gap_padv_stop(ext_adv);

	if (err) {
		return BTP_STATUS_FAILED;
	}

	rp->current_settings = sys_cpu_to_le32(current_settings);

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

int tester_gap_padv_set_data(struct bt_le_ext_adv *ext_adv, struct bt_data *per_ad, uint8_t ad_len)
{
	int err;

	if (ext_adv == NULL) {
		return -EINVAL;
	}

	/* Set Periodic Advertising data */
	err = bt_le_per_adv_set_data(ext_adv, per_ad, ad_len);
	if (err != 0) {
		LOG_DBG("Failed to set periodic advertising data: %d", err);
	}

	return err;
}

static uint8_t padv_set_data(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_gap_padv_set_data_cmd *cp = cmd;

	uint8_t padv_len = 0U;

	for (uint8_t i = 0; i < cp->data_len; padv_len++) {
		if (padv_len >= ARRAY_SIZE(padv)) {
			LOG_ERR("padv[] Out of memory");
			return BTP_STATUS_FAILED;
		}

		padv[padv_len].data_len = cp->data[i++] - 1;
		padv[padv_len].type = cp->data[i++];
		padv[padv_len].data = &cp->data[i];
		i += padv[padv_len].data_len;
	}

	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get(0);

	err = tester_gap_padv_set_data(ext_adv, padv, padv_len);

	return BTP_STATUS_VAL(err);
}
#endif /* defined(CONFIG_BT_PER_ADV) */

int tester_gap_padv_create_sync(struct bt_le_per_adv_sync_param *create_params)
{
	if (!IS_ENABLED(CONFIG_BT_PER_ADV_SYNC)) {
		return -ENOTSUP;
	}

	int err;

	if (pa_sync != NULL) {
		return -EBUSY;
	}

	err = bt_le_per_adv_sync_create(create_params, &pa_sync);

	if (err != 0) {
		LOG_DBG("Unable to sync to PA: %d", err);
	}

	return err;
}

int tester_gap_padv_stop_sync(void)
{
	if (!IS_ENABLED(CONFIG_BT_PER_ADV_SYNC)) {
		return -ENOTSUP;
	}

	int err;

	if (pa_sync == NULL) {
		return -EALREADY;
	}

	err = bt_le_per_adv_sync_delete(pa_sync);
	if (err != 0) {
		LOG_DBG("Unable to stop sync to PA: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_PER_ADV)
static uint8_t padv_create_sync(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_gap_padv_create_sync_cmd *cp = cmd;
	struct bt_le_per_adv_sync_param create_params = {0};

	bt_addr_le_copy(&create_params.addr, &cp->address);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_NONE;
	create_params.sid = cp->advertiser_sid;
	create_params.skip = sys_le16_to_cpu(cp->skip);
	create_params.timeout = sys_le16_to_cpu(cp->sync_timeout);

	if (cp->flags & BTP_GAP_PADV_CREATE_SYNC_FLAG_REPORTS_DISABLED) {
		create_params.options |= BT_LE_PER_ADV_SYNC_OPT_REPORTING_INITIALLY_DISABLED;
	}

	if (cp->flags & BTP_GAP_PADV_CREATE_SYNC_FLAG_FILTER_DUPLICATES) {
		create_params.options |= BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	}

	err = tester_gap_padv_create_sync(&create_params);

	return BTP_STATUS_VAL(err);
}

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
static uint8_t padv_sync_transfer_set_info(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_padv_sync_transfer_set_info_cmd *cp = cmd;
	struct bt_le_ext_adv *ext_adv = tester_gap_ext_adv_get(0);
	struct bt_conn *conn;
	int err;

	if (ext_adv == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_le_per_adv_set_info_transfer(ext_adv, conn,
					      sys_le16_to_cpu(cp->service_data));

	bt_conn_unref(conn);

	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t padv_padv_sync_transfer_start(const void *cmd, uint16_t cmd_len,
					     void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_padv_sync_transfer_start_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	if (pa_sync == NULL) {
		return BTP_STATUS_FAILED;
	}

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	err = bt_le_per_adv_sync_transfer(pa_sync, conn,
					  sys_le16_to_cpu(cp->service_data));

	bt_conn_unref(conn);

	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER) */

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)
static uint8_t padv_padv_sync_transfer_recv(const void *cmd, uint16_t cmd_len,
					    void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_padv_sync_transfer_recv_cmd *cp = cmd;
	struct bt_le_per_adv_sync_transfer_param param = {0};
	struct bt_conn *conn;
	int err;

	if (pa_sync != NULL) {
		return BTP_STATUS_FAILED;
	}

	if (cp->address.type == BTP_BR_ADDRESS_TYPE) {
		return BTP_STATUS_FAILED;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	param.skip = sys_le16_to_cpu(cp->skip);
	param.timeout = sys_le16_to_cpu(cp->sync_timeout);

	if ((cp->flags & BTP_GAP_PADV_SYNC_TRANSFER_RECV_FLAG_REPORTS_DISABLED) != 0) {
		param.options |= BT_LE_PER_ADV_SYNC_TRANSFER_OPT_REPORTING_INITIALLY_DISABLED;
	}

	err = bt_le_per_adv_sync_transfer_subscribe(conn, &param);

	bt_conn_unref(conn);

	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER) */
#endif /* defined(CONFIG_BT_PER_ADV) */

#if defined(CONFIG_BT_RPA_TIMEOUT_DYNAMIC)
static uint8_t set_rpa_timeout(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_gap_set_rpa_timeout_cmd *cp = cmd;
	int err;

	err = bt_le_set_rpa_timeout(sys_le16_to_cpu(cp->rpa_timeout));

	if (err != 0) {
		LOG_DBG("Failed to set RPA timeout: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* defined(CONFIG_BT_RPA_TIMEOUT_DYNAMIC) */

static struct bt_iso_big *iso_sync_receiver_big;

static void iso_sync_receiver_big_started_cb(struct bt_iso_big *big)
{
	__maybe_unused struct btp_gap_big_sync_established_ev ev;
	struct bt_iso_info info;
	struct bt_iso_chan *bis;
	bool found = false;

	if (big != iso_sync_receiver_big) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&big->bis_channels, bis, node) {
		int err = bt_iso_chan_get_info(bis, &info);

		if (err != 0) {
			LOG_DBG("Failed to get ISO chan info: %d", err);
			continue;
		}

		if (info.type != BT_ISO_CHAN_TYPE_SYNC_RECEIVER) {
			continue;
		}

		found = true;
		break;
	}

	__ASSERT(found, "Failed to find the valid ISO info");

	if (!found) {
		LOG_ERR("Failed to find the valid ISO info");
		return;
	}

#if defined(CONFIG_BT_ISO_SYNC_RECEIVER)
	bt_addr_le_copy(&ev.address, &pa_sync->addr);
	ev.latency = sys_cpu_to_le32(info.sync_receiver.latency);
	ev.nse = info.max_subevent;
	ev.bn = info.sync_receiver.bn;
	ev.pto = sys_cpu_to_le32(info.sync_receiver.pto);
	ev.irc = info.sync_receiver.irc;
	ev.max_pdu = sys_cpu_to_le16(info.sync_receiver.max_pdu);
	ev.iso_interval = sys_cpu_to_le16(info.iso_interval);

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BIG_SYNC_ESTABLISHED, &ev, sizeof(ev));
#endif /* CONFIG_BT_ISO_SYNC_RECEIVER */
}

static void iso_sync_receiver_big_stopped_cb(struct bt_iso_big *big, uint8_t reason)
{
	struct btp_gap_big_sync_lost_ev ev;

	if (big != iso_sync_receiver_big) {
		return;
	}

	bt_addr_le_copy(&ev.address, &pa_sync->addr);
	ev.reason = reason;

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BIG_SYNC_LOST, &ev, sizeof(ev));

	iso_sync_receiver_big = NULL;
}

static struct bt_iso_big_cb iso_sync_receiver_big_cb = {
	.started = iso_sync_receiver_big_started_cb,
	.stopped = iso_sync_receiver_big_stopped_cb,
};

#if defined(CONFIG_BT_ISO_SYNC_RECEIVER)
#define BIS_ISO_CHAN_COUNT 1

#define BIS_STREAM_DATA_LEN (CONFIG_BT_ISO_RX_MTU + \
			     sizeof(struct btp_gap_bis_stream_received_ev))

BUILD_ASSERT(BIS_STREAM_DATA_LEN < BTP_DATA_MAX_SIZE, "BIS stream data length exceeds BTP MTU");

static struct net_buf_simple *iso_sync_receiver_buf = NET_BUF_SIMPLE(BIS_STREAM_DATA_LEN);

static void iso_sync_receiver_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
				   struct net_buf *buf)
{
	struct btp_gap_bis_stream_received_ev *ev;

	if (pa_sync == NULL) {
		return;
	}

	/* cleanup */
	net_buf_simple_init(iso_sync_receiver_buf, 0);

	ev = net_buf_simple_add(iso_sync_receiver_buf, sizeof(*ev));

	__ASSERT(buf->len <= net_buf_simple_tailroom(iso_sync_receiver_buf), "No more tailroom");

	bt_addr_le_copy(&ev->address, &pa_sync->addr);
	ev->bis_id = 0;
	ev->data_len = buf->len;
	ev->flags = info->flags;
	ev->ts = sys_cpu_to_le32(info->ts);
	ev->seq_num = sys_cpu_to_le16(info->seq_num);
	net_buf_simple_add_mem(iso_sync_receiver_buf, buf->data, ev->data_len);

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BIS_STREAM_RECEIVED,
		     iso_sync_receiver_buf->data, iso_sync_receiver_buf->len);
}

static void iso_sync_receiver_connected(struct bt_iso_chan *chan)
{
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};
	int err;
	struct btp_gap_bis_data_path_setup_ev ev;

	if (pa_sync == NULL) {
		return;
	}

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_CTLR_TO_HOST, &hci_path);
	if (err != 0) {
		LOG_ERR("Failed to setup ISO RX data path: %d", err);
		return;
	}

	bt_addr_le_copy(&ev.address, &pa_sync->addr);
	ev.bis_id = 0;

	tester_event(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BIS_DATA_PATH_SETUP, &ev, sizeof(ev));
}

static void iso_sync_receiver_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	int err;

	err = bt_iso_remove_data_path(chan, BT_HCI_DATAPATH_DIR_CTLR_TO_HOST);
	if (err != 0) {
		LOG_ERR("Failed to remove ISO RX data path: %d", err);
	}
}

static struct bt_iso_chan_ops iso_sync_receiver_ops = {
	.recv = iso_sync_receiver_recv,
	.connected = iso_sync_receiver_connected,
	.disconnected = iso_sync_receiver_disconnected,
};

static struct bt_iso_chan_qos bis_iso_qos;

static struct bt_iso_chan bis_iso_chan = {
	.qos = &bis_iso_qos,
};

#define DEFAULT_IO_QOS {.sdu = 40u, .phy = BT_GAP_LE_PHY_2M, .rtn = 2u,}

static struct bt_iso_chan_io_qos iso_sync_receiver_qos = DEFAULT_IO_QOS;

static struct bt_iso_chan *bis_channels[BIS_ISO_CHAN_COUNT] = { &bis_iso_chan };

static uint8_t big_create_sync(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;
	const struct btp_gap_big_create_sync_cmd *cp = cmd;
	struct bt_iso_big_sync_param param;

	if ((cmd_len < sizeof(*cp)) ||
	    (cmd_len != (sizeof(*cp) + sizeof(param.bcode) * cp->encryption))) {
		LOG_ERR("Invalid cmd len");
		return BTP_STATUS_FAILED;
	}

	if (!(cp->encryption == BTP_GAP_BIG_CREATE_SYNC_ENC_DISABLE ||
	      cp->encryption == BTP_GAP_BIG_CREATE_SYNC_ENC_ENABLE)) {
		LOG_ERR("Invalid encryption %u", cp->encryption);
		return BTP_STATUS_FAILED;
	}

	if ((pa_sync == NULL) || !bt_addr_le_eq(&cp->address, &pa_sync->addr) ||
	    (cp->sid != pa_sync->sid)) {
		LOG_ERR("Invalid PA sync or SID");
		return BTP_STATUS_FAILED;
	}

	if (cp->num_bis > ARRAY_SIZE(bis_channels)) {
		LOG_ERR("BIS num exceeds %u > %u", cp->num_bis, ARRAY_SIZE(bis_channels));
		return BTP_STATUS_FAILED;
	}

	bis_iso_qos.tx = NULL;
	bis_iso_qos.rx = &iso_sync_receiver_qos;

	bis_iso_chan.ops = &iso_sync_receiver_ops;

	param.bis_channels = bis_channels;
	param.num_bis = cp->num_bis;
	param.bis_bitfield = sys_le32_to_cpu(cp->bis_bitfield);
	param.mse = sys_le16_to_cpu(cp->mse);
	param.sync_timeout = sys_le16_to_cpu(cp->sync_timeout);
	param.encryption = cp->encryption == BTP_GAP_BIG_CREATE_SYNC_ENC_ENABLE;
	if (param.encryption) {
		memcpy(param.bcode, cp->broadcast_code, sizeof(param.bcode));
	}

	err = bt_iso_big_sync(pa_sync, &param, &iso_sync_receiver_big);
	if (err != 0) {
		LOG_ERR("Unable to sync to BIG (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("BIG syncing");

	return BTP_STATUS_SUCCESS;
}
#endif /* defined(CONFIG_BT_ISO_SYNC_RECEIVER) */

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_GAP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_GAP_READ_CONTROLLER_INDEX_LIST,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = controller_index_list,
	},
	{
		.opcode = BTP_GAP_READ_CONTROLLER_INFO,
		.expect_len = 0,
		.func = controller_info,
	},
	{
		.opcode = BTP_GAP_SET_POWERED,
		.expect_len = sizeof(struct btp_gap_set_powered_cmd),
		.func = set_powered,
	},
	{
		.opcode = BTP_GAP_SET_CONNECTABLE,
		.expect_len = sizeof(struct btp_gap_set_connectable_cmd),
		.func = set_connectable,
	},
	{
		.opcode = BTP_GAP_SET_DISCOVERABLE,
		.expect_len = sizeof(struct btp_gap_set_discoverable_cmd),
		.func = set_discoverable,
	},
	{
		.opcode = BTP_GAP_SET_BONDABLE,
		.expect_len = sizeof(struct btp_gap_set_bondable_cmd),
		.func = set_bondable,
	},
	{
		.opcode = BTP_GAP_START_ADVERTISING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = start_advertising,
	},
	{
		.opcode = BTP_GAP_START_DIRECTED_ADV,
		.expect_len = sizeof(struct btp_gap_start_directed_adv_cmd),
		.func = start_directed_advertising,
	},
	{
		.opcode = BTP_GAP_STOP_ADVERTISING,
		.expect_len = 0,
		.func = stop_advertising,
	},
	{
		.opcode = BTP_GAP_START_DISCOVERY,
		.expect_len = sizeof(struct btp_gap_start_discovery_cmd),
		.func = start_discovery,
	},
	{
		.opcode = BTP_GAP_STOP_DISCOVERY,
		.expect_len = 0,
		.func = stop_discovery,
	},
	{
		.opcode = BTP_GAP_CONNECT,
		.expect_len = sizeof(struct btp_gap_connect_cmd),
		.func = connect,
	},
	{
		.opcode = BTP_GAP_DISCONNECT,
		.expect_len = sizeof(struct btp_gap_disconnect_cmd),
		.func = disconnect,
	},
	{
		.opcode = BTP_GAP_SET_IO_CAP,
		.expect_len = sizeof(struct btp_gap_set_io_cap_cmd),
		.func = set_io_cap,
	},
	{
		.opcode = BTP_GAP_PAIR,
		.expect_len = sizeof(struct btp_gap_pair_cmd),
		.func = pair,
	},
	{
		.opcode = BTP_GAP_PAIR_V2,
		.expect_len = sizeof(struct btp_gap_pair_v2_cmd),
		.func = pair_v2,
	},
	{
		.opcode = BTP_GAP_UNPAIR,
		.expect_len = sizeof(struct btp_gap_unpair_cmd),
		.func = unpair,
	},
	{
		.opcode = BTP_GAP_PASSKEY_ENTRY,
		.expect_len = sizeof(struct btp_gap_passkey_entry_cmd),
		.func = passkey_entry,
	},
	{
		.opcode = BTP_GAP_PASSKEY_CONFIRM,
		.expect_len = sizeof(struct btp_gap_passkey_confirm_cmd),
		.func = passkey_confirm,
	},
	{
		.opcode = BTP_GAP_CONN_PARAM_UPDATE,
		.expect_len = sizeof(struct btp_gap_conn_param_update_cmd),
		.func = conn_param_update,
	},
	{
		.opcode = BTP_GAP_SET_MITM,
		.expect_len = sizeof(struct btp_gap_set_mitm),
		.func = set_mitm,
	},
	{
		.opcode = BTP_GAP_OOB_LEGACY_SET_DATA,
		.expect_len = sizeof(struct btp_gap_oob_legacy_set_data_cmd),
		.func = set_oob_legacy_data,
	},
#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	{
		.opcode = BTP_GAP_OOB_SC_GET_LOCAL_DATA,
		.expect_len = 0,
		.func = get_oob_sc_local_data,
	},
	{
		.opcode = BTP_GAP_OOB_SC_SET_REMOTE_DATA,
		.expect_len = sizeof(struct btp_gap_oob_sc_set_remote_data_cmd),
		.func = set_oob_sc_remote_data,
	},
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */
	{
		.opcode = BTP_GAP_SET_FILTER_LIST,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = set_filter_list,
	},
#if defined(CONFIG_BT_EXT_ADV)
	{
		.opcode = BTP_GAP_SET_EXTENDED_ADVERTISING,
		.expect_len = sizeof(struct btp_gap_set_extended_advertising_cmd),
		.func = set_extended_advertising,
	},
#if defined(CONFIG_BT_PER_ADV)
	{
		.opcode = BTP_GAP_PADV_CONFIGURE,
		.expect_len = sizeof(struct btp_gap_padv_configure_cmd),
		.func = padv_configure,
	},
	{
		.opcode = BTP_GAP_PADV_START,
		.expect_len = sizeof(struct btp_gap_padv_start_cmd),
		.func = padv_start,
	},
	{
		.opcode = BTP_GAP_PADV_STOP,
		.expect_len = sizeof(struct btp_gap_padv_stop_cmd),
		.func = padv_stop,
	},
	{
		.opcode = BTP_GAP_PADV_SET_DATA,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = padv_set_data,
	},
	{
		.opcode = BTP_GAP_PADV_CREATE_SYNC,
		.expect_len = sizeof(struct btp_gap_padv_create_sync_cmd),
		.func = padv_create_sync,
	},
#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)
	{
		.opcode = BTP_GAP_PADV_SYNC_TRANSFER_SET_INFO,
		.expect_len = sizeof(struct btp_gap_padv_sync_transfer_set_info_cmd),
		.func = padv_sync_transfer_set_info,
	},
	{
		.opcode = BTP_GAP_PADV_SYNC_TRANSFER_START,
		.expect_len = sizeof(struct btp_gap_padv_sync_transfer_start_cmd),
		.func = padv_padv_sync_transfer_start,
	},
#endif /* defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER) */
#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)
	{
		.opcode = BTP_GAP_PADV_SYNC_TRANSFER_RECV,
		.expect_len = sizeof(struct btp_gap_padv_sync_transfer_recv_cmd),
		.func = padv_padv_sync_transfer_recv,
	},
#endif /* defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER) */
#endif /* defined(CONFIG_BT_PER_ADV) */
#endif /* defined(CONFIG_BT_EXT_ADV) */
#if defined(CONFIG_BT_RPA_TIMEOUT_DYNAMIC)
	{
		.opcode = BTP_GAP_SET_RPA_TIMEOUT,
		.expect_len = sizeof(struct btp_gap_set_rpa_timeout_cmd),
		.func = set_rpa_timeout,
	},
#endif /* defined(CONFIG_BT_RPA_TIMEOUT_DYNAMIC) */
#if defined(CONFIG_BT_ISO_SYNC_RECEIVER)
	{
		.opcode = BTP_GAP_BIG_CREATE_SYNC,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = big_create_sync,
	},
#endif /* defined(CONFIG_BT_ISO_SYNC_RECEIVER) */
};

uint8_t tester_init_gap(void)
{
	int err;

	(void)memset(&cb, 0, sizeof(cb));
	bt_conn_auth_cb_register(NULL);
	cb.pairing_accept = auth_pairing_accept;
	if (bt_conn_auth_cb_register(&cb)) {
		return BTP_STATUS_FAILED;
	}

	err = bt_enable(NULL);
	if (err < 0) {
		LOG_ERR("Unable to enable Bluetooth: %d", err);
		return BTP_STATUS_FAILED;
	}

	atomic_clear(&current_settings);
	atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_POWERED);
	atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_CONNECTABLE);
	atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_BONDABLE);
	atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_LE);
	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		atomic_set_bit(&current_settings, BTP_GAP_SETTINGS_PRIVACY);
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_info_cb_register(&auth_info_cb);

	if (IS_ENABLED(CONFIG_BT_PER_ADV)) {
		bt_le_per_adv_sync_cb_register(&pa_sync_cb);
	}

	if (IS_ENABLED(CONFIG_BT_ISO_SYNC_RECEIVER)) {
		bt_iso_big_register_cb(&iso_sync_receiver_big_cb);
	}

	tester_register_command_handlers(BTP_SERVICE_ID_GAP, handlers,
					 ARRAY_SIZE(handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_gap(void)
{
	return BTP_STATUS_SUCCESS;
}
