/* btp_hap.c - Bluetooth HAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/services/ias.h>

#include "../bluetooth/audio/has_internal.h"
#include "btp/btp.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bttester_hap, CONFIG_BTTESTER_LOG_LEVEL);

static uint8_t read_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	struct btp_hap_read_supported_commands_rp *rp = rsp;

	tester_set_bit(rp->data, BTP_HAP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_HAP_HA_INIT);
	tester_set_bit(rp->data, BTP_HAP_HAUC_INIT);
	tester_set_bit(rp->data, BTP_HAP_IAC_INIT);
	tester_set_bit(rp->data, BTP_HAP_IAC_DISCOVER);
	tester_set_bit(rp->data, BTP_HAP_IAC_SET_ALERT);
	tester_set_bit(rp->data, BTP_HAP_HAUC_DISCOVER);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t ha_init(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_hap_ha_init_cmd *cp = cmd;
	struct bt_has_features_param params;
	const uint16_t opts = sys_le16_to_cpu(cp->opts);
	const bool presets_sync = (opts & BTP_HAP_HA_OPT_PRESETS_SYNC) > 0;
	const bool presets_independent = (opts & BTP_HAP_HA_OPT_PRESETS_INDEPENDENT) > 0;
	const bool presets_writable = (opts & BTP_HAP_HA_OPT_PRESETS_WRITABLE) > 0;
	const bool presets_dynamic = (opts & BTP_HAP_HA_OPT_PRESETS_DYNAMIC) > 0;
	int err;

	if (!IS_ENABLED(CONFIG_BT_HAS_PRESET_SUPPORT) &&
	    (presets_sync || presets_independent || presets_writable || presets_dynamic)) {
		return BTP_STATUS_VAL(-ENOTSUP);
	}

	/* Only dynamic presets are supported */
	if (!presets_dynamic) {
		return BTP_STATUS_VAL(-ENOTSUP);
	}

	/* Preset name writable support mismatch */
	if (presets_writable != IS_ENABLED(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)) {
		return BTP_STATUS_VAL(-ENOTSUP);
	}

	params.type = cp->type;
	params.preset_sync_support = presets_sync;
	params.independent_presets = presets_independent;

	if (cp->type == BT_HAS_HEARING_AID_TYPE_BANDED) {
		err = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT |
							      BT_AUDIO_LOCATION_FRONT_RIGHT);
	} else {
		err = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
	}

	if (err != 0) {
		return BTP_STATUS_VAL(err);
	}

	err = bt_has_register(&params);
	if (err != 0) {
		return BTP_STATUS_VAL(err);
	}

	return BTP_STATUS_SUCCESS;
}

static void has_client_discover_cb(struct bt_conn *conn, int err, struct bt_has *has,
				   enum bt_has_hearing_aid_type type, enum bt_has_capabilities caps)
{
	struct btp_hap_hauc_discovery_complete_ev ev = { 0 };

	LOG_DBG("conn %p err %d", (void *)conn, err);

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));
	ev.status = BTP_STATUS_VAL(err);

	if (err != 0 && err != BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		LOG_DBG("Client discovery failed: %d", err);
	} else {
		struct bt_has_client *inst = CONTAINER_OF(has, struct bt_has_client, has);

		ev.has_hearing_aid_features_handle = inst->features_subscription.value_handle;
		ev.has_control_point_handle = inst->control_point_subscription.value_handle;
		ev.has_active_preset_index_handle = inst->active_index_subscription.value_handle;
	}

	tester_event(BTP_SERVICE_ID_HAP, BT_HAP_EV_HAUC_DISCOVERY_COMPLETE, &ev, sizeof(ev));
}

static void has_client_preset_switch_cb(struct bt_has *has, int err, uint8_t index)
{

}

static const struct bt_has_client_cb has_client_cb = {
	.discover = has_client_discover_cb,
	.preset_switch = has_client_preset_switch_cb,
};

static uint8_t hauc_init(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	err = bt_has_client_cb_register(&has_client_cb);
	if (err != 0) {
		LOG_DBG("Failed to register client callbacks: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static void ias_client_discover_cb(struct bt_conn *conn, int err)
{
	struct btp_hap_iac_discovery_complete_ev ev;
	struct bt_conn_info info;

	bt_conn_get_info(conn, &info);

	bt_addr_le_copy(&ev.address, info.le.dst);
	if (err < 0) {
		ev.status = BT_ATT_ERR_UNLIKELY;
	} else {
		ev.status = (uint8_t)err;
	}

	tester_event(BTP_SERVICE_ID_HAP, BT_HAP_EV_IAC_DISCOVERY_COMPLETE, &ev, sizeof(ev));
}

static const struct bt_ias_client_cb ias_client_cb = {
	.discover = ias_client_discover_cb,
};

static uint8_t iac_init(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	err = bt_ias_client_cb_register(&ias_client_cb);
	if (err != 0) {
		return BTP_STATUS_VAL(err);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t iac_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_hap_iac_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_ias_discover(conn);

	bt_conn_unref(conn);

	return BTP_STATUS_VAL(err);
}

static uint8_t iac_set_alert(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_hap_iac_set_alert_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_ias_client_alert_write(conn, (enum bt_ias_alert_lvl)cp->alert);

	bt_conn_unref(conn);

	return BTP_STATUS_VAL(err);
}

static uint8_t hauc_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_hap_hauc_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_has_client_discover(conn);
	if (err != 0) {
		LOG_DBG("Failed to discover remote HAS: %d", err);
	}

	bt_conn_unref(conn);

	return BTP_STATUS_VAL(err);
}

static const struct btp_handler hap_handlers[] = {
	{
		.opcode = BTP_HAP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = read_supported_commands,
	},
	{
		.opcode = BTP_HAP_HA_INIT,
		.expect_len = sizeof(struct btp_hap_ha_init_cmd),
		.func = ha_init,
	},
	{
		.opcode = BTP_HAP_HAUC_INIT,
		.expect_len = 0,
		.func = hauc_init,
	},
	{
		.opcode = BTP_HAP_IAC_INIT,
		.expect_len = 0,
		.func = iac_init,
	},
	{
		.opcode = BTP_HAP_IAC_DISCOVER,
		.expect_len = sizeof(struct btp_hap_iac_discover_cmd),
		.func = iac_discover,
	},
	{
		.opcode = BTP_HAP_IAC_SET_ALERT,
		.expect_len = sizeof(struct btp_hap_iac_set_alert_cmd),
		.func = iac_set_alert,
	},
	{
		.opcode = BTP_HAP_HAUC_DISCOVER,
		.expect_len = sizeof(struct btp_hap_hauc_discover_cmd),
		.func = hauc_discover,
	},
};

uint8_t tester_init_hap(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_HAP, hap_handlers,
					 ARRAY_SIZE(hap_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_hap(void)
{
	return BTP_STATUS_SUCCESS;
}
