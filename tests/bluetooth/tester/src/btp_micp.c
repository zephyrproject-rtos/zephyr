/* btp_micp.c - Bluetooth MICP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/audio/aics.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <../../subsys/bluetooth/audio/micp_internal.h>
#include <../../subsys/bluetooth/audio/aics_internal.h>

#include "bap_endpoint.h"
#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_micp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

static struct bt_micp_mic_ctlr *mic_ctlr;

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static struct bt_micp_included micp_included;
struct chrc_handles {
	uint16_t mute_handle;
	uint16_t state_handle;
	uint16_t gain_handle;
	uint16_t type_handle;
	uint16_t status_handle;
	uint16_t control_handle;
	uint16_t desc_handle;
};
struct chrc_handles micp_handles;
extern struct btp_aics_instance aics_client_instance;
extern struct bt_aics_cb aics_client_cb;
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

static void btp_send_micp_found_ev(struct bt_conn *conn, const struct chrc_handles *micp_handles)
{
	struct btp_micp_discovered_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.mute_handle = sys_cpu_to_le16(micp_handles->mute_handle);
	ev.state_handle = sys_cpu_to_le16(micp_handles->state_handle);
	ev.gain_handle = sys_cpu_to_le16(micp_handles->gain_handle);
	ev.type_handle = sys_cpu_to_le16(micp_handles->type_handle);
	ev.status_handle = sys_cpu_to_le16(micp_handles->status_handle);
	ev.control_handle = sys_cpu_to_le16(micp_handles->control_handle);
	ev.desc_handle = sys_cpu_to_le16(micp_handles->desc_handle);

	tester_event(BTP_SERVICE_ID_MICP, BTP_MICP_DISCOVERED_EV, &ev, sizeof(ev));
}

static void btp_send_micp_mute_state_ev(struct bt_conn *conn, uint8_t mute)
{
	struct btp_micp_mute_state_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.mute = mute;

	tester_event(BTP_SERVICE_ID_MICP, BTP_MICP_MUTE_STATE_EV, &ev, sizeof(ev));
}

static void micp_mic_ctlr_mute_cb(struct bt_micp_mic_ctlr *mic_ctlr, int err, uint8_t mute)
{
	struct bt_conn *conn;

	bt_micp_mic_ctlr_conn_get(mic_ctlr, &conn);
	btp_send_micp_mute_state_ev(conn, mute);

	LOG_DBG("MICP Mute cb (%d)", err);
}

static void micp_mic_ctlr_mute_written_cb(struct bt_micp_mic_ctlr *mic_ctlr, int err)
{
	struct bt_conn *conn;
	uint8_t mute_state = bt_micp_mic_ctlr_mute_get(mic_ctlr);

	bt_micp_mic_ctlr_conn_get(mic_ctlr, &conn);
	btp_send_micp_mute_state_ev(conn, mute_state);

	LOG_DBG("MICP Mute Written cb (%d))", err);
}

static void micp_mic_ctlr_unmute_written_cb(struct bt_micp_mic_ctlr *mic_ctlr, int err)
{
	struct bt_conn *conn;
	uint8_t mute_state = bt_micp_mic_ctlr_mute_get(mic_ctlr);

	bt_micp_mic_ctlr_conn_get(mic_ctlr, &conn);
	btp_send_micp_mute_state_ev(conn, mute_state);

	LOG_DBG("MICP Mute Unwritten cb (%d))", err);
}

static void micp_mic_ctlr_discover_cb(struct bt_micp_mic_ctlr *mic_ctlr, int err,
				      uint8_t aics_count)
{
	struct bt_conn *conn;

	if (err) {
		LOG_DBG("Discovery failed (%d)", err);
		return;
	}

	LOG_DBG("Discovery done with %u AICS",
		aics_count);

	bt_micp_mic_ctlr_conn_get(mic_ctlr, &conn);

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	if (bt_micp_mic_ctlr_included_get(mic_ctlr, &micp_included) != 0) {
		LOG_DBG("Could not get included services");
		memset(&micp_handles, 0, sizeof(micp_handles));
	} else {
		aics_client_instance.aics_cnt = micp_included.aics_cnt;
		aics_client_instance.aics = micp_included.aics;
		bt_aics_client_cb_register(aics_client_instance.aics[0], &aics_client_cb);

		micp_handles.state_handle = micp_included.aics[0]->cli.state_handle;
		micp_handles.gain_handle = micp_included.aics[0]->cli.gain_handle;
		micp_handles.type_handle = micp_included.aics[0]->cli.type_handle;
		micp_handles.status_handle = micp_included.aics[0]->cli.status_handle;
		micp_handles.control_handle = micp_included.aics[0]->cli.control_handle;
		micp_handles.desc_handle = micp_included.aics[0]->cli.desc_handle;
	}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

	micp_handles.mute_handle = mic_ctlr->mute_handle;
	btp_send_micp_found_ev(conn, &micp_handles);
}

static struct bt_micp_mic_ctlr_cb micp_cbs = {
	.discover = micp_mic_ctlr_discover_cb,
	.mute = micp_mic_ctlr_mute_cb,
	.mute_written = micp_mic_ctlr_mute_written_cb,
	.unmute_written = micp_mic_ctlr_unmute_written_cb,
};

static uint8_t micp_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	struct btp_micp_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_MICP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_MICP_CTLR_DISCOVER);
	tester_set_bit(rp->data, BTP_MICP_CTLR_MUTE_READ);
	tester_set_bit(rp->data, BTP_MICP_CTLR_MUTE);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t micp_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_micp_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_micp_mic_ctlr_discover(conn, &mic_ctlr);
	if (err) {
		LOG_DBG("Fail: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t micp_mute_read(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("Read mute");

	err = bt_micp_mic_ctlr_mute_get(mic_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t micp_mute(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("MICP Mute");

	err = bt_micp_mic_ctlr_mute(mic_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler micp_handlers[] = {
	{
		.opcode = BTP_MICP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = micp_supported_commands,
	},
	{
		.opcode = BTP_MICP_CTLR_DISCOVER,
		.expect_len = sizeof(struct btp_micp_discover_cmd),
		.func = micp_discover,
	},
	{
		.opcode = BTP_MICP_CTLR_MUTE_READ,
		.expect_len = sizeof(struct btp_micp_mute_read_cmd),
		.func = micp_mute_read,
	},
	{
		.opcode = BTP_MICP_CTLR_MUTE,
		.expect_len = sizeof(struct btp_micp_mute_cmd),
		.func = micp_mute,
	}
};

uint8_t tester_init_micp(void)
{
	int err;

	err = bt_micp_mic_ctlr_cb_register(&micp_cbs);

	if (err) {
		LOG_DBG("Failed to register callbacks: %d", err);
		return BTP_STATUS_FAILED;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_MICP, micp_handlers,
					 ARRAY_SIZE(micp_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_micp(void)
{
	(void)bt_micp_mic_ctlr_cb_register(NULL);
	return BTP_STATUS_SUCCESS;
}
