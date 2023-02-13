/* btp_vcp.c - Bluetooth VCP Tester */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/testing.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/audio/aics.h>
#include "zephyr/bluetooth/audio/vocs.h"
#include "zephyr/sys/util.h"

#include <app_keys.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_vcp
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "btp/btp.h"

#define CONTROLLER_INDEX 0
#define BT_AICS_MAX_INPUT_DESCRIPTION_SIZE 16
#define BT_AICS_MAX_OUTPUT_DESCRIPTION_SIZE 16

struct bt_vcp_vol_rend_register_param vcp_register_param;
struct bt_vcp_included included;

static void set_register_params(uint8_t gain_mode);
uint8_t tester_init_vcp(void);

/* Volume Control Service */
static void vcs_supported_commands(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, VCS_READ_SUPPORTED_COMMANDS);
	net_buf_simple_add_u8(buf, VCS_SET_VOL);
	net_buf_simple_add_u8(buf, VCS_VOL_UP);
	net_buf_simple_add_u8(buf, VCS_VOL_DOWN);
	net_buf_simple_add_u8(buf, VCS_MUTE);
	net_buf_simple_add_u8(buf, VCS_UNMUTE);

	tester_send(BTP_SERVICE_ID_VCS, VCS_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, buf->data, buf->len);
}

static void set_volume(uint8_t *data)
{
	const struct vcs_set_vol_cmd *cmd = (void *)data;
	uint8_t volume;

	volume = cmd->volume;

	LOG_DBG("Set volume 0x%02x", volume);

	if (bt_vcp_vol_rend_set_vol(volume) != 0) {
		tester_rsp(BTP_SERVICE_ID_VCS, VCS_SET_VOL,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		return;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_SET_VOL, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void vol_up(void)
{
	LOG_DBG("Volume Up");

	if (bt_vcp_vol_rend_vol_up() != 0) {
		tester_rsp(BTP_SERVICE_ID_VCS, VCS_VOL_UP,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		return;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_VOL_UP, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void vol_down(void)
{
	LOG_DBG("Volume Down");

	if (bt_vcp_vol_rend_vol_down() != 0) {
		tester_rsp(BTP_SERVICE_ID_VCS, VCS_VOL_DOWN,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		return;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_VOL_DOWN, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void mute(void)
{
	LOG_DBG("Mute");

	if (bt_vcp_vol_rend_mute() != 0) {
		tester_rsp(BTP_SERVICE_ID_VCS, VCS_MUTE, CONTROLLER_INDEX,
			   BTP_STATUS_FAILED);
		return;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_MUTE, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void unmute(void)
{
	LOG_DBG("Unmute");

	if (bt_vcp_vol_rend_unmute() != 0) {
		tester_rsp(BTP_SERVICE_ID_VCS, VCS_UNMUTE, CONTROLLER_INDEX,
			   BTP_STATUS_FAILED);
		return;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_UNMUTE, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
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

void tester_handle_vcs(uint8_t opcode, uint8_t index, uint8_t *data,
		       uint16_t len)
{
	switch (opcode) {
	case VCS_READ_SUPPORTED_COMMANDS:
		vcs_supported_commands(data, len);
		break;
	case VCS_SET_VOL:
		set_volume(data);
		break;
	case VCS_VOL_UP:
		vol_up();
		break;
	case VCS_VOL_DOWN:
		vol_down();
		break;
	case VCS_MUTE:
		mute();
		break;
	case VCS_UNMUTE:
		unmute();
		break;
	default:
		tester_rsp(BTP_SERVICE_ID_VCS, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		break;
	}
}

/* Audio Input Control Service */
static void aics_supported_commands(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, AICS_READ_SUPPORTED_COMMANDS);
	net_buf_simple_add_u8(buf, AICS_SET_GAIN);
	net_buf_simple_add_u8(buf, AICS_MUTE);
	net_buf_simple_add_u8(buf, AICS_UNMUTE);
	net_buf_simple_add_u8(buf, AICS_MAN_GAIN);
	net_buf_simple_add_u8(buf, AICS_AUTO_GAIN);
	net_buf_simple_add_u8(buf, AICS_DESCRIPTION);

	tester_send(BTP_SERVICE_ID_AICS, AICS_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, buf->data, buf->len);
}

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

static struct bt_aics_cb aics_cb = {
	.state = aics_state_cb,
	.gain_setting = aics_gain_setting_cb,
	.type = aics_input_type_cb,
	.status = aics_status_cb,
	.description = aics_description_cb
};

void aics_set_gain(uint8_t *data)
{
	const struct aics_set_gain *cmd = (void *)data;

	const int8_t gain = cmd->gain;

	LOG_DBG("AICS set gain %d", gain);

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_gain_set(included.aics[0], gain) != 0) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_SET_GAIN,
				   CONTROLLER_INDEX, BTP_STATUS_FAILED);
			return;
		}
	}

	tester_rsp(BTP_SERVICE_ID_AICS, AICS_SET_GAIN, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

void aics_mute(void)
{
	LOG_DBG("AICS mute");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_mute(included.aics[i]) != 0) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_MUTE,
				   CONTROLLER_INDEX, BTP_STATUS_FAILED);
			return;
		}
	}

	tester_rsp(BTP_SERVICE_ID_AICS, AICS_MUTE, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

void aics_unmute(void)
{
	LOG_DBG("AICS unmute");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_unmute(included.aics[i]) != 0) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_UNMUTE,
				   CONTROLLER_INDEX, BTP_STATUS_FAILED);
			return;
		}
	}

	tester_rsp(BTP_SERVICE_ID_AICS, AICS_UNMUTE, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

void aics_man_gain(void)
{
	LOG_DBG("AICS manual gain set");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_manual_gain_set(included.aics[i]) != 0) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_MAN_GAIN,
				   CONTROLLER_INDEX, BTP_STATUS_FAILED);
			return;
		}
	}

	tester_rsp(BTP_SERVICE_ID_AICS, AICS_MAN_GAIN, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

void aics_auto_gain(void)
{
	LOG_DBG("AICS auto gain set");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_automatic_gain_set(included.aics[i]) != 0) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_AUTO_GAIN,
				   CONTROLLER_INDEX, BTP_STATUS_FAILED);
			return;
		}
	}

	tester_rsp(BTP_SERVICE_ID_AICS, AICS_AUTO_GAIN, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

/* TODO: Need new API function for this */
void aics_auto_gain_only(void)
{
	LOG_DBG("AICS auto gain only");

	tester_rsp(BTP_SERVICE_ID_AICS, AICS_AUTO_GAIN_ONLY, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

/* TODO: Need new API function for this */
void aics_auto_man_only(void)
{
	LOG_DBG("AICS manual gain only");

	tester_rsp(BTP_SERVICE_ID_AICS, AICS_MAN_GAIN_ONLY, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

/* TODO: Need new API function for this */
void aics_mute_disable(void)
{
	LOG_DBG("AICS mute disable");

	tester_rsp(BTP_SERVICE_ID_AICS, AICS_MUTE_DISABLE,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

void aics_desc(uint8_t *data)
{
	const struct aics_audio_desc *cmd = (void *) data;
	char description[BT_AICS_MAX_INPUT_DESCRIPTION_SIZE];

	LOG_DBG("AICS description");

	if (cmd->desc_len > BT_AICS_MAX_INPUT_DESCRIPTION_SIZE) {
		LOG_ERR("Too long input (%u chars required)",
			BT_AICS_MAX_INPUT_DESCRIPTION_SIZE);
		goto rsp;
	}

	memcpy(description, cmd->desc, cmd->desc_len);
	description[cmd->desc_len] = '\0';

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_description_set(included.aics[i], description) != 0) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_DESCRIPTION,
				   CONTROLLER_INDEX, BTP_STATUS_FAILED);
			return;
		}
	}
rsp:
	tester_rsp(BTP_SERVICE_ID_AICS, AICS_DESCRIPTION, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

void tester_handle_aics(uint8_t opcode, uint8_t index, uint8_t *data, uint16_t len)
{
	switch (opcode) {
	case AICS_READ_SUPPORTED_COMMANDS:
		aics_supported_commands(data, len);
		break;
	case AICS_SET_GAIN:
		aics_set_gain(data);
		break;
	case AICS_MUTE:
		aics_mute();
		break;
	case AICS_UNMUTE:
		aics_unmute();
		break;
	case AICS_MAN_GAIN:
		aics_man_gain();
		break;
	case AICS_AUTO_GAIN:
		aics_auto_gain();
		break;
	case AICS_MAN_GAIN_ONLY:
		aics_auto_gain_only();
		break;
	case AICS_AUTO_GAIN_ONLY:
		aics_auto_gain_only();
		break;
	case AICS_DESCRIPTION:
		aics_desc(data);
		break;
	case AICS_MUTE_DISABLE:
		aics_mute_disable();
		break;
	default:
		tester_rsp(BTP_SERVICE_ID_AICS, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		break;
	}
}

/* Volume Offset Control Service */
static void vocs_supported_commands(void)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, VOCS_READ_SUPPORTED_COMMANDS);
	net_buf_simple_add_u8(buf, VOCS_UPDATE_LOC);
	net_buf_simple_add_u8(buf, VOCS_UPDATE_DESC);

	tester_send(BTP_SERVICE_ID_VOCS, VOCS_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, buf->data, buf->len);
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

void vocs_audio_desc(uint8_t *data)
{
	struct vocs_audio_desc *cmd = (void *) data;
	char description[BT_AICS_MAX_OUTPUT_DESCRIPTION_SIZE];

	LOG_DBG("VOCS description");

	/* FIXME: Spec does not specify maximum desc length */
	if (cmd->desc_len >= BT_AICS_MAX_OUTPUT_DESCRIPTION_SIZE) {
		LOG_ERR("Too long input (%u chars required)", 16);
		goto rsp;
	}

	memcpy(description, cmd->desc, cmd->desc_len);
	description[cmd->desc_len] = '\0';

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT; i++) {
		if (bt_vocs_description_set(included.vocs[i], description) != 0) {
			tester_rsp(BTP_SERVICE_ID_VOCS, VOCS_UPDATE_DESC,
				   CONTROLLER_INDEX, BTP_STATUS_FAILED);
			return;
		}
	}
rsp:
	tester_rsp(BTP_SERVICE_ID_VOCS, VOCS_UPDATE_DESC, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

void vocs_audio_loc(uint8_t *data)
{
	const struct vocs_audio_loc *cmd = (void *) data;

	LOG_DBG("VOCS location");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT; i++) {
		if (bt_vocs_location_set(included.vocs[i], cmd->loc) != 0) {
			tester_rsp(BTP_SERVICE_ID_VOCS, VOCS_UPDATE_LOC,
				   CONTROLLER_INDEX, BTP_STATUS_FAILED);
			return;
		}
	}

	tester_rsp(BTP_SERVICE_ID_VOCS, VOCS_UPDATE_LOC, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

void tester_handle_vocs(uint8_t opcode, uint8_t index, uint8_t *data,
			uint16_t len)
{
	switch (opcode) {
	case VOCS_READ_SUPPORTED_COMMANDS:
		vocs_supported_commands();
		break;
	case VOCS_UPDATE_DESC:
		vocs_audio_desc(data);
		break;
	case VOCS_UPDATE_LOC:
		vocs_audio_loc(data);
		break;
	default:
		tester_rsp(BTP_SERVICE_ID_VOCS, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		break;
	}
}

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
		vcp_register_param.aics_param[i].cb = &aics_cb;
	}

	vcp_register_param.step = 1;
	vcp_register_param.mute = BT_VCP_STATE_UNMUTED;
	vcp_register_param.volume = 100;
	vcp_register_param.cb = &vcs_cb;
}

uint8_t tester_init_vcp(void)
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

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_vcp(void)
{
	return BTP_STATUS_SUCCESS;
}
