/* vcp.c - Bluetooth VCP Tester */

/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/bluetooth/testing.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/audio/aics.h>
#include "zephyr/bluetooth/audio/vocs.h"
#include <app_keys.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_vcp
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "btp/btp.h"

#define CONTROLLER_INDEX 0

struct bt_vcp_vol_rend_register_param vcp_register_param;
struct bt_vcp_included included;
static bool g_aics_active = 1;

static void set_register_params(uint8_t gain_mode);

/* Volume Control Service */
static void vcs_supported_commands(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, VCS_READ_SUPPORTED_COMMANDS);
	net_buf_simple_add_u8(buf, VCS_INIT);
	net_buf_simple_add_u8(buf, VCS_SET_VOL);
	net_buf_simple_add_u8(buf, VCS_VOL_UP);
	net_buf_simple_add_u8(buf, VCS_VOL_DOWN);
	net_buf_simple_add_u8(buf, VCS_MUTE);
	net_buf_simple_add_u8(buf, VCS_UNMUTE);

	tester_send(BTP_SERVICE_ID_VCS, VCS_READ_SUPPORTED_COMMANDS,
			    CONTROLLER_INDEX, buf->data, buf->len);
}

static void set_volume(uint8_t *data, uint16_t len)
{
	const struct vcs_set_vol_cmd *cmd = (void *) data;
	uint8_t status = BTP_STATUS_SUCCESS;
	uint32_t volume;
	int err;

	volume = sys_le32_to_cpu(cmd->volume);
	
	LOG_DBG("volume 0x%04x", volume);

	err = bt_vcp_vol_rend_set_vol(volume);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_SET_VOL, CONTROLLER_INDEX, status);
}

static void vol_up()
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	LOG_DBG("Volume Up");

	err = bt_vcp_vol_rend_vol_up();
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_VOL_UP, CONTROLLER_INDEX, status);
}

static void vol_down()
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	LOG_DBG("Volume Down");

	err = bt_vcp_vol_rend_vol_down();
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_VOL_DOWN, CONTROLLER_INDEX, status);
}

static void mute()
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	err = bt_vcp_vol_rend_mute();
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_MUTE, CONTROLLER_INDEX, status);
}

static void unmute()
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	err = bt_vcp_vol_rend_unmute();
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_VCS, VCS_UNMUTE, CONTROLLER_INDEX, status);
}

static void vcs_state_cb(int err, uint8_t volume, uint8_t mute)
{
	if (err) {
		LOG_DBG("VCP state cb err (%d)", err);
		return;
	}
}

static void vcs_flags_cb(int err, uint8_t flags)
{
	if (err) {
		LOG_DBG("VCP flags cb err (%d)", err);
		return;
	}
}

static struct bt_vcp_vol_rend_cb vcs_cb = {
	.state = vcs_state_cb,
	.flags = vcs_flags_cb,
};

static void vcp_init()
{
	struct bt_vcp_vol_rend_register_param vcp_register_param;
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	/* Default values */
	vcp_register_param.step = 1;
	vcp_register_param.mute = BT_VCP_STATE_UNMUTED;
	vcp_register_param.volume = 100;
	vcp_register_param.cb = &vcs_cb;

	LOG_DBG("");

	err = bt_vcp_vol_rend_register(&vcp_register_param);
	if (err) {
		status = BTP_STATUS_FAILED;

		goto rsp;
	}
rsp:
	tester_rsp(BTP_SERVICE_ID_VCS, VCS_INIT, CONTROLLER_INDEX,
		   status);
}

void tester_handle_vcs(uint8_t opcode, uint8_t index, uint8_t *data, uint16_t len)
{
	switch (opcode) {
		case VCS_READ_SUPPORTED_COMMANDS:
			vcs_supported_commands(data, len);
			break;
		case VCS_INIT:
			vcp_init();
			break;
		case VCS_SET_VOL:
			set_volume(data, len);
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
	net_buf_simple_add_u8(buf, AICS_SET_GAIN);
	net_buf_simple_add_u8(buf, AICS_MUTE);
	net_buf_simple_add_u8(buf, AICS_UNMUTE);
	net_buf_simple_add_u8(buf, AICS_MAN_GAIN);
	net_buf_simple_add_u8(buf, AICS_AUTO_GAIN);

	tester_send(BTP_SERVICE_ID_AICS, AICS_READ_SUPPORTED_COMMANDS,
				CONTROLLER_INDEX, buf->data, buf->len);
}

static void aics_state_cb(struct bt_aics *inst, int err, int8_t gain,
			  uint8_t mute, uint8_t mode)
{
	LOG_DBG("AICS state callback");
}

static void aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units,
				 int8_t minimum, int8_t maximum)
{
	LOG_DBG("AICS gain setting callback");
}

static void aics_input_type_cb(struct bt_aics *inst, int err,
				   uint8_t input_type)
{
	LOG_DBG("AICS input type callback");
}

static void aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	LOG_DBG("AICS status callback");
}

static void aics_description_cb(struct bt_aics *inst, int err,
				char *description)
{
	LOG_DBG("AICS description callback");
}

static struct bt_aics_cb aics_cb = {
	.state = aics_state_cb,
	.gain_setting = aics_gain_setting_cb,
	.type = aics_input_type_cb,
	.status = aics_status_cb,
	.description = aics_description_cb
};

void aics_set_gain(uint8_t *data, uint8_t len)
{
	const struct aics_set_gain *cmd = (void *) data;
	uint32_t gain;
	int err;

	gain = sys_le32_to_cpu(cmd->gain);
	
	LOG_DBG("AICS set gain 0x%04x", gain);

	for (int i = 0; i < (int)CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		err = bt_aics_gain_set(included.aics[0], gain);
		if (err) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_SET_GAIN,
					   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		}
	}
}

void aics_mute()
{
	int err;
	
	LOG_DBG("AICS mute");

	for (int i = 0; i < (int)CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		err = bt_aics_mute(included.aics[i]);
		if (err) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_MUTE,
					   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		}
	}
}

void aics_unmute()
{
	int err;

	LOG_DBG("AICS unmute");

	for (int i = 0; i < (int)CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		err = bt_aics_unmute(included.aics[i]);
		if (err) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_UNMUTE,
					   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		}
	}
}

void aics_man_gain()
{
	int err;
	
	LOG_DBG("AICS manual gain set");

	for (int i = 0; i < (int)CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		err = bt_aics_manual_gain_set(included.aics[i]);
		if (err) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_MAN_GAIN,
					   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		}
	}
}

void aics_auto_gain()
{
	int err;
	
	LOG_DBG("AICS auto gain set");
	for (int i = 0; i < (int)CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		err = bt_aics_automatic_gain_set(included.aics[i]);
		if (err) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_AUTO_GAIN,
					   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		}
	}
}

/* TODO: Re-register with proper gain option */
void aics_auto_gain_only()
{

}

/* TODO: Re-register with proper gain option */
void aics_auto_man_only()
{

}

void aics_desc(const char *description, uint8_t len)
{
	int err;

	LOG_DBG("AICS description");
	for (int i = 0; i < (int)CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		err = bt_aics_description_set(included.aics[i], description);
		if (err) {
			tester_rsp(BTP_SERVICE_ID_AICS, AICS_DESCRIPTION,
					   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		}
	}
}

void tester_handle_aics(uint8_t opcode, uint8_t index, uint8_t *data, uint16_t len)
{
	switch (opcode) {
		case AICS_READ_SUPPORTED_COMMANDS:
			aics_supported_commands(data, len);
			break;
		case AICS_SET_GAIN:
			aics_set_gain(data, len);
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
			aics_desc(data, len);
			break;
		default:
			tester_rsp(BTP_SERVICE_ID_AICS, opcode, index,
					   BTP_STATUS_UNKNOWN_CMD);
			break;
	}
}

/* Volume Offset Control Service */
static void vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	LOG_DBG("VOCS state callback");
}

static void vocs_location_cb(struct bt_vocs *inst, int err, uint32_t location)
{
	LOG_DBG("VOCS location callback");
}

static void vocs_description_cb(struct bt_vocs *inst, int err,
								char *description)
{
	LOG_DBG("VOCS desctripion callback");
}

static struct bt_vocs_cb vocs_cb = {
	.state = vocs_state_cb,
	.location = vocs_location_cb,
	.description = vocs_description_cb
};

void vocs_audio_desc(const char *description, uint8_t len)
{
	int err;

	LOG_DBG("VOCS description");
	for (int i = 0; i < (int)CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		err = bt_vocs_description_set(included.vocs[i], description);
		if (err) {
			tester_rsp(BTP_SERVICE_ID_VOCS, VOCS_AUDIO_OUT_DESC_UPDATE,
					   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		}
	}
}

void vocs_audio_loc(uint8_t *data, uint8_t len)
{
	int err;

	LOG_DBG("VOCS location");
	for (int i = 0; i < (int)CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		err = bt_vocs_location_set(included.vocs[i], (int)data);
		if (err) {
			tester_rsp(BTP_SERVICE_ID_VOCS, VOCS_UPDATE_AUDIO_LOC,
					   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		}
	}
}

void tester_handle_vocs(uint8_t opcode, uint8_t index, uint8_t *data, uint16_t len)
{
	switch (opcode) {
		case VOCS_AUDIO_OUT_DESC_UPDATE:
			vocs_audio_desc(data, len);
			break;
		case VOCS_UPDATE_AUDIO_LOC:
			vocs_audio_loc(data, len);
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
	char input_desc[CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT][16];
	char output_desc[CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT][16];

	memset(&vcp_register_param, 0, sizeof(vcp_register_param));

	for (int i = 0; i < ARRAY_SIZE(vcp_register_param.vocs_param); i++) {
		vcp_register_param.vocs_param[i].location_writable = true;
		vcp_register_param.vocs_param[i].desc_writable = true;
		vcp_register_param.vocs_param[i].output_desc = output_desc[i];
		vcp_register_param.vocs_param[i].cb = &vocs_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(vcp_register_param.aics_param); i++) {
		vcp_register_param.aics_param[i].desc_writable = true;
		vcp_register_param.aics_param[i].description = input_desc[i];
		vcp_register_param.aics_param[i].type = BT_AICS_INPUT_TYPE_DIGITAL;
		vcp_register_param.aics_param[i].status = g_aics_active;
		vcp_register_param.aics_param[i].gain_mode = gain_mode;
		vcp_register_param.aics_param[i].units = 1;
		vcp_register_param.aics_param[i].min_gain = 0;
		vcp_register_param.aics_param[i].max_gain = 100;
		vcp_register_param.aics_param[i].cb = &aics_cb;
	}

	/* Default values */
	vcp_register_param.step = 1;
	vcp_register_param.mute = BT_VCP_STATE_UNMUTED;
	vcp_register_param.volume = 100;
}

uint8_t tester_init_vcp(void)
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	set_register_params(BT_AICS_MODE_MANUAL);

	err = bt_vcp_vol_rend_register(&vcp_register_param);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	err = bt_vcp_vol_rend_included_get(&included);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	return status;
}

uint8_t tester_unregister_vcp(void)
{
	return BTP_STATUS_SUCCESS;
}
