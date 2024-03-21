/* btp_vcp.c - Bluetooth VCP Tester */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/testing.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/sys/util.h>
#include "btp/btp.h"

#include <../../subsys/bluetooth/audio/micp_internal.h>
#include <../../subsys/bluetooth/audio/aics_internal.h>
#include <../../subsys/bluetooth/audio/vcp_internal.h>
#include <../../subsys/bluetooth/audio/vocs_internal.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME bttester_vcp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#define BT_AICS_MAX_INPUT_DESCRIPTION_SIZE 16
#define BT_AICS_MAX_OUTPUT_DESCRIPTION_SIZE 16

static struct bt_vcp_vol_rend_register_param vcp_register_param;
static struct bt_vcp_vol_ctlr *vol_ctlr;
static struct bt_vcp_included included;
extern struct btp_aics_instance aics_server_instance;
extern struct btp_aics_instance aics_client_instance;
extern struct bt_aics_cb aics_client_cb;

struct service_handles {
	struct {
		uint16_t ctrl_pt;
		uint16_t flags;
		uint16_t state;
	} vcp_handles;

	struct {
		uint16_t state;
		uint16_t location;
		uint16_t control;
		uint16_t desc;
	} vocs_handles;

	struct {
		uint16_t mute;
		uint16_t state;
		uint16_t gain;
		uint16_t type;
		uint16_t status;
		uint16_t control;
		uint16_t desc;
	} aics_handles;
};

struct service_handles chrc_handles;

/* Volume Control Service */
static uint8_t vcs_supported_commands(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_vcs_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_VCS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_VCS_SET_VOL);
	tester_set_bit(rp->data, BTP_VCS_VOL_UP);
	tester_set_bit(rp->data, BTP_VCS_VOL_DOWN);
	tester_set_bit(rp->data, BTP_VCS_MUTE);
	tester_set_bit(rp->data, BTP_VCS_UNMUTE);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_volume(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_vcs_set_vol_cmd *cp = cmd;

	LOG_DBG("Set volume 0x%02x", cp->volume);

	if (bt_vcp_vol_rend_set_vol(cp->volume) != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vol_up(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("Volume Up");

	if (bt_vcp_vol_rend_vol_up() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vol_down(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("Volume Down");

	if (bt_vcp_vol_rend_vol_down() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t mute(const void *cmd, uint16_t cmd_len,
		    void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("Mute");

	if (bt_vcp_vol_rend_mute() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t unmute(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("Unmute");

	if (bt_vcp_vol_rend_unmute() != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static void vcs_state_cb(int err, uint8_t volume, uint8_t mute)
{
	LOG_DBG("VCP state cb err (%d)", err);
}

static void vcs_flags_cb(int err, uint8_t flags)
{
	LOG_DBG("VCP flags cb err (%d)", err);
}

static struct bt_vcp_vol_rend_cb vcs_cb = {
	.state = vcs_state_cb,
	.flags = vcs_flags_cb,
};

static const struct btp_handler vcs_handlers[] = {
	{
		.opcode = BTP_VCS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = vcs_supported_commands,
	},
	{
		.opcode = BTP_VCS_SET_VOL,
		.expect_len = sizeof(struct btp_vcs_set_vol_cmd),
		.func = set_volume,
	},
	{
		.opcode = BTP_VCS_VOL_UP,
		.expect_len = 0,
		.func = vol_up,
	},
	{
		.opcode = BTP_VCS_VOL_DOWN,
		.expect_len = 0,
		.func = vol_down,
	},
	{
		.opcode = BTP_VCS_MUTE,
		.expect_len = 0,
		.func = mute,
	},
	{
		.opcode = BTP_VCS_UNMUTE,
		.expect_len = 0,
		.func = unmute,
	},
};

/* Volume Offset Control Service */
static uint8_t vocs_supported_commands(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	struct btp_vocs_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_VOCS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_VOCS_UPDATE_LOC);
	tester_set_bit(rp->data, BTP_VOCS_UPDATE_DESC);
	tester_set_bit(rp->data, BTP_VOCS_STATE_GET);
	tester_set_bit(rp->data, BTP_VOCS_LOCATION_GET);
	tester_set_bit(rp->data, BTP_VOCS_OFFSET_STATE_SET);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static void vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	LOG_DBG("VOCS state callback err (%d)", err);
}

static void vocs_location_cb(struct bt_vocs *inst, int err, uint32_t location)
{
	LOG_DBG("VOCS location callback err (%d)", err);
}

static void vocs_description_cb(struct bt_vocs *inst, int err,
				char *description)
{
	LOG_DBG("VOCS desctripion callback (%d)", err);
}

static struct bt_vocs_cb vocs_cb = {
	.state = vocs_state_cb,
	.location = vocs_location_cb,
	.description = vocs_description_cb
};

static void btp_send_vocs_state_ev(struct bt_conn *conn, uint8_t att_status, int16_t offset)
{
	struct btp_vocs_offset_state_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.offset = sys_cpu_to_le16(offset);

	tester_event(BTP_SERVICE_ID_VOCS, BTP_VOCS_OFFSET_STATE_EV, &ev, sizeof(ev));
}

static void btp_send_vocs_location_ev(struct bt_conn *conn, uint8_t att_status, uint32_t location)
{
	struct btp_vocs_audio_location_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.location = sys_cpu_to_le32(location);

	tester_event(BTP_SERVICE_ID_VOCS, BTP_VOCS_AUDIO_LOCATION_EV, &ev, sizeof(ev));
}

static void btp_send_vocs_procedure_ev(struct bt_conn *conn, uint8_t att_status, uint8_t opcode)
{
	struct btp_vocs_procedure_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.opcode = opcode;

	tester_event(BTP_SERVICE_ID_VOCS, BTP_VOCS_PROCEDURE_EV, &ev, sizeof(ev));
}

static void vcp_vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	struct bt_conn *conn;

	bt_vocs_client_conn_get(inst, &conn);
	btp_send_vocs_state_ev(conn, err, offset);

	LOG_DBG("VOCS Offset State callback");
}

static void vcp_vocs_location_cb(struct bt_vocs *inst, int err, uint32_t location)
{
	struct bt_conn *conn;

	bt_vocs_client_conn_get(inst, &conn);
	btp_send_vocs_location_ev(conn, err, location);

	LOG_DBG("VOCS location callback err (%d)", err);
}

static void vcp_vocs_description_cb(struct bt_vocs *inst, int err,
				char *description)
{
	LOG_DBG("VOCS desctripion callback (%d)", err);
}

static void vcp_vocs_set_offset_cb(struct bt_vocs *inst, int err)
{
	struct bt_conn *conn;

	bt_vocs_client_conn_get(inst, &conn);
	btp_send_vocs_procedure_ev(conn, err, BTP_VOCS_OFFSET_STATE_SET);

	LOG_DBG("VOCS Set Offset callback (%d)", err);
}

static struct bt_vocs_cb vocs_cl_cb = {
	.state = vcp_vocs_state_cb,
	.location = vcp_vocs_location_cb,
	.description = vcp_vocs_description_cb,
#if defined(CONFIG_BT_VOCS_CLIENT)
	.set_offset = vcp_vocs_set_offset_cb
#endif /* CONFIG_BT_VOCS_CLIENT */
};

static uint8_t vocs_audio_desc(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_vocs_audio_desc_cmd *cp = cmd;
	char description[BT_AICS_MAX_OUTPUT_DESCRIPTION_SIZE];

	if (cmd_len < sizeof(*cp) ||
	    cmd_len != sizeof(*cp) + cp->desc_len) {
		return BTP_STATUS_FAILED;
	}

	if (cp->desc_len >= sizeof(description)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(description, cp->desc, cp->desc_len);
	description[cp->desc_len] = '\0';

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT; i++) {
		if (bt_vocs_description_set(included.vocs[i], description) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vocs_audio_loc(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_vocs_audio_loc_cmd *cp = cmd;
	uint32_t loc = sys_le32_to_cpu(cp->loc);

	for (uint8_t i = 0; i < included.vocs_cnt; i++) {
		if (bt_vocs_location_set(included.vocs[i], loc) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vocs_state_get(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("Volume Offset Control Service offset state get");

	err = bt_vocs_state_get(included.vocs[0]);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vocs_state_set(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_vocs_offset_set_cmd *cp = cmd;
	int16_t offset = sys_le16_to_cpu(cp->offset);
	int err;

	LOG_DBG("VCP CTLR Set absolute volume %d", offset);

	err = bt_vocs_state_set(included.vocs[0], cp->offset);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vocs_audio_location_get(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	int err;

	LOG_DBG("Volume Offset Control Service Audio Location get");

	err = bt_vocs_location_get(included.vocs[0]);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler vocs_handlers[] = {
	{
		.opcode = BTP_VOCS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = vocs_supported_commands,
	},
	{
		.opcode = BTP_VOCS_UPDATE_DESC,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = vocs_audio_desc,
	},
	{
		.opcode = BTP_VOCS_UPDATE_LOC,
		.expect_len = sizeof(struct btp_vocs_audio_loc_cmd),
		.func = vocs_audio_loc,
	},
	{
		.opcode = BTP_VOCS_STATE_GET,
		.expect_len = sizeof(struct btp_vocs_state_get_cmd),
		.func = vocs_state_get,
	},
	{
		.opcode = BTP_VOCS_LOCATION_GET,
		.expect_len = sizeof(struct btp_vocs_location_get_cmd),
		.func = vocs_audio_location_get,
	},
	{
		.opcode = BTP_VOCS_OFFSET_STATE_SET,
		.expect_len = sizeof(struct btp_vocs_offset_set_cmd),
		.func = vocs_state_set,
	},
};

/* AICS Callbacks */
static void aics_state_cb(struct bt_aics *inst, int err, int8_t gain,
			  uint8_t mute, uint8_t mode)
{
	LOG_DBG("AICS state callback (%d)", err);
}

static void aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units,
				 int8_t minimum, int8_t maximum)
{
	LOG_DBG("AICS gain setting callback (%d)", err);
}

static void aics_input_type_cb(struct bt_aics *inst, int err,
			       uint8_t input_type)
{
	LOG_DBG("AICS input type callback (%d)", err);
}

static void aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	LOG_DBG("AICS status callback (%d)", err);
}

static void aics_description_cb(struct bt_aics *inst, int err,
				char *description)
{
	LOG_DBG("AICS description callback (%d)", err);
}

struct bt_aics_cb aics_server_cb = {
	.state = aics_state_cb,
	.gain_setting = aics_gain_setting_cb,
	.type = aics_input_type_cb,
	.status = aics_status_cb,
	.description = aics_description_cb,
};

/* General profile handling */
static void set_register_params(uint8_t gain_mode)
{
	char input_desc[CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT]
		       [BT_AICS_MAX_INPUT_DESCRIPTION_SIZE];
	char output_desc[CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT]
			[BT_AICS_MAX_OUTPUT_DESCRIPTION_SIZE];

	memset(&vcp_register_param, 0, sizeof(vcp_register_param));

	for (size_t i = 0; i < ARRAY_SIZE(vcp_register_param.vocs_param); i++) {
		vcp_register_param.vocs_param[i].location_writable = true;
		vcp_register_param.vocs_param[i].desc_writable = true;
		snprintf(output_desc[i], sizeof(output_desc[i]),
			 "Output %zu", i + 1);
		vcp_register_param.vocs_param[i].output_desc = output_desc[i];
		vcp_register_param.vocs_param[i].cb = &vocs_cb;
	}

	for (size_t i = 0; i < ARRAY_SIZE(vcp_register_param.aics_param); i++) {
		vcp_register_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %zu", i + 1);
		vcp_register_param.aics_param[i].description = input_desc[i];
		vcp_register_param.aics_param[i].type = BT_AICS_INPUT_TYPE_DIGITAL;
		vcp_register_param.aics_param[i].status = 1;
		vcp_register_param.aics_param[i].gain_mode = gain_mode;
		vcp_register_param.aics_param[i].units = 1;
		vcp_register_param.aics_param[i].min_gain = 0;
		vcp_register_param.aics_param[i].max_gain = 100;
		vcp_register_param.aics_param[i].cb = &aics_server_cb;
	}

	vcp_register_param.step = 1;
	vcp_register_param.mute = BT_VCP_STATE_UNMUTED;
	vcp_register_param.volume = 100;
	vcp_register_param.cb = &vcs_cb;
}

uint8_t tester_init_vcs(void)
{
	int err;

	set_register_params(BT_AICS_MODE_MANUAL);

	err = bt_vcp_vol_rend_register(&vcp_register_param);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	err = bt_vcp_vol_rend_included_get(&included);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	aics_server_instance.aics_cnt = included.aics_cnt;
	aics_server_instance.aics = included.aics;

	tester_register_command_handlers(BTP_SERVICE_ID_VCS, vcs_handlers,
					 ARRAY_SIZE(vcs_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_vcs(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_init_vocs(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_VOCS, vocs_handlers,
					 ARRAY_SIZE(vocs_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_vocs(void)
{
	return BTP_STATUS_SUCCESS;
}

/* Volume Control Profile */
static void btp_send_vcp_found_ev(struct bt_conn *conn, uint8_t att_status,
				  const struct service_handles *chrc_handles)
{
	struct btp_vcp_discovered_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.vcs_handles.control_handle = sys_cpu_to_le16(chrc_handles->vcp_handles.ctrl_pt);
	ev.vcs_handles.flag_handle = sys_cpu_to_le16(chrc_handles->vcp_handles.flags);
	ev.vcs_handles.state_handle = sys_cpu_to_le16(chrc_handles->vcp_handles.state);
	ev.vocs_handles.state_handle = sys_cpu_to_le16(chrc_handles->vocs_handles.state);
	ev.vocs_handles.location_handle = sys_cpu_to_le16(chrc_handles->vocs_handles.location);
	ev.vocs_handles.control_handle = sys_cpu_to_le16(chrc_handles->vocs_handles.control);
	ev.vocs_handles.desc_handle = sys_cpu_to_le16(chrc_handles->vocs_handles.desc);
	ev.aics_handles.state_handle = sys_cpu_to_le16(chrc_handles->aics_handles.state);
	ev.aics_handles.gain_handle = sys_cpu_to_le16(chrc_handles->aics_handles.gain);
	ev.aics_handles.type_handle = sys_cpu_to_le16(chrc_handles->aics_handles.type);
	ev.aics_handles.status_handle = sys_cpu_to_le16(chrc_handles->aics_handles.status);
	ev.aics_handles.control_handle = sys_cpu_to_le16(chrc_handles->aics_handles.control);
	ev.aics_handles.desc_handle = sys_cpu_to_le16(chrc_handles->aics_handles.desc);

	tester_event(BTP_SERVICE_ID_VCP, BTP_VCP_DISCOVERED_EV, &ev, sizeof(ev));
}

static void btp_send_vcp_state_ev(struct bt_conn *conn, uint8_t att_status, uint8_t volume,
				  uint8_t mute)
{
	struct btp_vcp_state_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.volume = volume;
	ev.mute = mute;

	tester_event(BTP_SERVICE_ID_VCP, BTP_VCP_STATE_EV, &ev, sizeof(ev));
}

static void btp_send_vcp_volume_flags_ev(struct bt_conn *conn, uint8_t att_status, uint8_t flags)
{
	struct btp_vcp_volume_flags_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.flags = flags;

	tester_event(BTP_SERVICE_ID_VCP, BTP_VCP_FLAGS_EV, &ev, sizeof(ev));
}

static void btp_send_vcp_procedure_ev(struct bt_conn *conn, uint8_t att_status, uint8_t opcode)
{
	struct btp_vcp_procedure_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.att_status = att_status;
	ev.opcode = opcode;

	tester_event(BTP_SERVICE_ID_VCP, BTP_VCP_PROCEDURE_EV, &ev, sizeof(ev));
}

static uint8_t vcp_supported_commands(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_vcp_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_VCP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_DISCOVER);
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_STATE_READ);
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_FLAGS_READ);
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_VOL_DOWN);
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_VOL_UP);
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_UNMUTE_VOL_DOWN);

	/* octet 1 */
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_UNMUTE_VOL_UP);
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_SET_VOL);
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_UNMUTE);
	tester_set_bit(rp->data, BTP_VCP_VOL_CTLR_MUTE);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static void vcp_vol_ctlr_discover_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err, uint8_t vocs_count,
				     uint8_t aics_count)
{
	struct bt_conn *conn;

	if (err) {
		LOG_DBG("Discovery failed (%d)", err);
		return;
	}

	LOG_DBG("Discovery done with %u VOCS, %u AICS",
		vocs_count, aics_count);

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);

	if (bt_vcp_vol_ctlr_included_get(vol_ctlr, &included) != 0) {
		LOG_DBG("Could not get included services");
		memset(&chrc_handles.vocs_handles, 0, sizeof(chrc_handles.vocs_handles));
		memset(&chrc_handles.aics_handles, 0, sizeof(chrc_handles.aics_handles));
	} else {
		aics_client_instance.aics_cnt = included.aics_cnt;
		aics_client_instance.aics = included.aics;
		bt_vocs_client_cb_register(vol_ctlr->vocs[0], &vocs_cl_cb);
		bt_aics_client_cb_register(vol_ctlr->aics[0], &aics_client_cb);

		struct bt_vocs_client *vocs_cli =
			CONTAINER_OF(vol_ctlr->vocs[0], struct bt_vocs_client, vocs);
		struct bt_aics_client *aics_cli = &vol_ctlr->aics[0]->cli;

		chrc_handles.vocs_handles.state = vocs_cli->state_handle;
		chrc_handles.vocs_handles.location = vocs_cli->location_handle;
		chrc_handles.vocs_handles.control = vocs_cli->control_handle;
		chrc_handles.vocs_handles.desc = vocs_cli->desc_handle;
		chrc_handles.aics_handles.state = aics_cli->state_handle;
		chrc_handles.aics_handles.gain = aics_cli->gain_handle;
		chrc_handles.aics_handles.type = aics_cli->type_handle;
		chrc_handles.aics_handles.status = aics_cli->status_handle;
		chrc_handles.aics_handles.control = aics_cli->control_handle;
		chrc_handles.aics_handles.desc = aics_cli->desc_handle;
	}

	chrc_handles.vcp_handles.ctrl_pt = vol_ctlr->control_handle;
	chrc_handles.vcp_handles.flags = vol_ctlr->flag_handle;
	chrc_handles.vcp_handles.state = vol_ctlr->state_handle;
	btp_send_vcp_found_ev(conn, err, &chrc_handles);
}

static void vcp_vol_ctlr_state_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err, uint8_t volume,
				  uint8_t mute)
{
	struct bt_conn *conn;

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	btp_send_vcp_state_ev(conn, err, volume, mute);

	LOG_DBG("VCP Volume CTLR State callback");
}

static void vcp_vol_ctlr_flags_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err, uint8_t flags)
{
	struct bt_conn *conn;

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	btp_send_vcp_volume_flags_ev(conn, err, flags);

	LOG_DBG("VCP CTLR Volume Flags callback");
}

static void vcp_vol_ctlr_vol_down_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_conn *conn;

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	btp_send_vcp_procedure_ev(conn, err, BTP_VCP_VOL_CTLR_VOL_DOWN);

	LOG_DBG("VCP CTLR Volume down callback");
}

static void vcp_vol_ctlr_vol_up_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_conn *conn;

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	btp_send_vcp_procedure_ev(conn, err, BTP_VCP_VOL_CTLR_VOL_UP);

	LOG_DBG("VCP CTLR Volume down callback");
}

static void vcp_vol_ctlr_unmute_vol_down_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_conn *conn;

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	btp_send_vcp_procedure_ev(conn, err, BTP_VCP_VOL_CTLR_UNMUTE_VOL_DOWN);

	LOG_DBG("VCP CTLR Volume down and unmute callback");
}

static void vcp_vol_ctlr_unmute_vol_up_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_conn *conn;

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	btp_send_vcp_procedure_ev(conn, err, BTP_VCP_VOL_CTLR_UNMUTE_VOL_UP);

	LOG_DBG("VCP CTLR Volume down and unmute callback");
}

static void vcp_vol_ctlr_set_vol_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_conn *conn;

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	btp_send_vcp_procedure_ev(conn, err, BTP_VCP_VOL_CTLR_SET_VOL);

	LOG_DBG("VCP CTLR Set absolute volume callback");
}

static void vcp_vol_ctlr_unmute_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_conn *conn;

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	btp_send_vcp_procedure_ev(conn, err, BTP_VCP_VOL_CTLR_UNMUTE);

	LOG_DBG("VCP CTLR Volume down and unmute callback");
}

static void vcp_vol_ctlr_mute_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_conn *conn;

	bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	btp_send_vcp_procedure_ev(conn, err, BTP_VCP_VOL_CTLR_MUTE);

	LOG_DBG("VCP CTLR Set absolute volume callback");
}

static struct bt_vcp_vol_ctlr_cb vcp_cbs = {
	.discover = vcp_vol_ctlr_discover_cb,
	.state = vcp_vol_ctlr_state_cb,
	.flags = vcp_vol_ctlr_flags_cb,
	.vol_down = vcp_vol_ctlr_vol_down_cb,
	.vol_up = vcp_vol_ctlr_vol_up_cb,
	.mute = vcp_vol_ctlr_mute_cb,
	.unmute = vcp_vol_ctlr_unmute_cb,
	.vol_down_unmute = vcp_vol_ctlr_unmute_vol_down_cb,
	.vol_up_unmute = vcp_vol_ctlr_unmute_vol_up_cb,
	.vol_set = vcp_vol_ctlr_set_vol_cb
};

static uint8_t vcp_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_vcp_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_vcp_vol_ctlr_discover(conn, &vol_ctlr);
	if (err) {
		LOG_DBG("Fail: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vcp_state_read(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("VCP State read");

	err = bt_vcp_vol_ctlr_read_state(vol_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vcp_volume_flags_read(const void *cmd, uint16_t cmd_len, void *rsp,
				     uint16_t *rsp_len)
{
	int err;

	LOG_DBG("VCP Volume Flags read");

	err = bt_vcp_vol_ctlr_read_flags(vol_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vcp_ctlr_vol_down(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("VCP CTLR Volume down");

	err = bt_vcp_vol_ctlr_vol_down(vol_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vcp_ctlr_vol_up(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("VCP CTLR Volume up");

	err = bt_vcp_vol_ctlr_vol_up(vol_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vcp_ctlr_unmute_vol_down(const void *cmd, uint16_t cmd_len, void *rsp,
					uint16_t *rsp_len)
{
	int err;

	LOG_DBG("VCP CTLR Unmute, vol down");

	err = bt_vcp_vol_ctlr_unmute_vol_down(vol_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vcp_ctlr_unmute_vol_up(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	int err;

	LOG_DBG("VCP CTLR Unmute, Volume up");

	err = bt_vcp_vol_ctlr_unmute_vol_up(vol_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vcp_ctlr_set_vol(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_vcp_ctlr_set_vol_cmd *cp = cmd;
	int err;

	LOG_DBG("VCP CTLR Set absolute volume %d", cp->volume);

	err = bt_vcp_vol_ctlr_set_vol(vol_ctlr, cp->volume);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vcp_ctlr_unmute(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("VCP CTLR Unmute");

	err = bt_vcp_vol_ctlr_unmute(vol_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t vcp_ctlr_mute(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("VCP CTLR Mute");

	err = bt_vcp_vol_ctlr_mute(vol_ctlr);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler vcp_handlers[] = {
	{
		.opcode = BTP_VCP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = vcp_supported_commands,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_DISCOVER,
		.expect_len = sizeof(struct btp_vcp_discover_cmd),
		.func = vcp_discover,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_STATE_READ,
		.expect_len = sizeof(struct btp_vcp_state_read_cmd),
		.func = vcp_state_read,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_FLAGS_READ,
		.expect_len = sizeof(struct btp_vcp_flags_read_cmd),
		.func = vcp_volume_flags_read,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_VOL_DOWN,
		.expect_len = sizeof(struct btp_vcp_ctlr_vol_down_cmd),
		.func = vcp_ctlr_vol_down,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_VOL_UP,
		.expect_len = sizeof(struct btp_vcp_ctlr_vol_up_cmd),
		.func = vcp_ctlr_vol_up,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_UNMUTE_VOL_DOWN,
		.expect_len = sizeof(struct btp_vcp_ctlr_unmute_vol_down_cmd),
		.func = vcp_ctlr_unmute_vol_down,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_UNMUTE_VOL_UP,
		.expect_len = sizeof(struct btp_vcp_ctlr_unmute_vol_up_cmd),
		.func = vcp_ctlr_unmute_vol_up,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_SET_VOL,
		.expect_len = sizeof(struct btp_vcp_ctlr_set_vol_cmd),
		.func = vcp_ctlr_set_vol,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_UNMUTE,
		.expect_len = sizeof(struct btp_vcp_ctlr_unmute_cmd),
		.func = vcp_ctlr_unmute,
	},
	{
		.opcode = BTP_VCP_VOL_CTLR_MUTE,
		.expect_len = sizeof(struct btp_vcp_ctlr_mute_cmd),
		.func = vcp_ctlr_mute,
	},
};

uint8_t tester_init_vcp(void)
{
	int err;

	err = bt_vcp_vol_ctlr_cb_register(&vcp_cbs);

	if (err) {
		LOG_DBG("Failed to register callbacks: %d", err);
		return BTP_STATUS_FAILED;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_VCP, vcp_handlers,
					 ARRAY_SIZE(vcp_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_vcp(void)
{
	int err;

	err = bt_vcp_vol_ctlr_cb_unregister(&vcp_cbs);
	if (err != 0) {
		LOG_DBG("Failed to unregister callbacks: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
