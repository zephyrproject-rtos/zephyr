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
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#define BT_AICS_MAX_INPUT_DESCRIPTION_SIZE 16
#define BT_AICS_MAX_OUTPUT_DESCRIPTION_SIZE 16

static struct bt_vcp_vol_rend_register_param vcp_register_param;
static struct bt_vcp_included included;

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

/* Audio Input Control Service */
static uint8_t aics_supported_commands(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	struct btp_aics_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_AICS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_AICS_SET_GAIN);
	tester_set_bit(rp->data, BTP_AICS_MUTE);
	tester_set_bit(rp->data, BTP_AICS_UNMUTE);
	tester_set_bit(rp->data, BTP_AICS_MUTE_DISABLE);
	tester_set_bit(rp->data, BTP_AICS_MAN_GAIN);
	tester_set_bit(rp->data, BTP_AICS_AUTO_GAIN);
	tester_set_bit(rp->data, BTP_AICS_MAN_GAIN_ONLY);
	tester_set_bit(rp->data, BTP_AICS_AUTO_GAIN_ONLY);

	/* octet 1 */
	tester_set_bit(rp->data, BTP_AICS_DESCRIPTION);

	*rsp_len = sizeof(*rp) + 2;

	return BTP_STATUS_SUCCESS;
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

static uint8_t aics_set_gain(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_set_gain_cmd *cp = cmd;

	LOG_DBG("AICS set gain %d", cp->gain);

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_gain_set(included.aics[0], cp->gain) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_mute(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("AICS mute");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_mute(included.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_mute_disable(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("AICS mute disable");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_disable_mute(included.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_unmute(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("AICS unmute");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_unmute(included.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_man_gain(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("AICS manual gain set");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_manual_gain_set(included.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_auto_gain(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("AICS auto gain set");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_automatic_gain_set(included.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_auto_gain_only(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("AICS auto gain only set");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_gain_set_auto_only(included.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_man_gain_only(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("AICS manual gain only set");

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_gain_set_manual_only(included.aics[i]) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t aics_desc(const void *cmd, uint16_t cmd_len,
			 void *rsp, uint16_t *rsp_len)
{
	const struct btp_aics_audio_desc_cmd *cp = cmd;
	char description[BT_AICS_MAX_INPUT_DESCRIPTION_SIZE];

	LOG_DBG("AICS description");

	if (cmd_len < sizeof(*cp) ||
	    cmd_len != sizeof(*cp) + cp->desc_len) {
		return BTP_STATUS_FAILED;
	}

	if (cp->desc_len >= sizeof(description)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(description, cp->desc, cp->desc_len);
	description[cp->desc_len] = '\0';

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT; i++) {
		if (bt_aics_description_set(included.aics[i], description) != 0) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler aics_handlers[] = {
	{
		.opcode = BTP_AICS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = aics_supported_commands,
	},
	{
		.opcode = BTP_AICS_SET_GAIN,
		.expect_len = sizeof(struct btp_aics_set_gain_cmd),
		.func = aics_set_gain,
	},
	{
		.opcode = BTP_AICS_MUTE,
		.expect_len = 0,
		.func = aics_mute,
	},
	{
		.opcode = BTP_AICS_UNMUTE,
		.expect_len = 0,
		.func = aics_unmute,
	},
	{
		.opcode = BTP_AICS_MUTE_DISABLE,
		.expect_len = 0,
		.func = aics_mute_disable,
	},
	{
		.opcode = BTP_AICS_MAN_GAIN,
		.expect_len = 0,
		.func = aics_man_gain,
	},
	{
		.opcode = BTP_AICS_AUTO_GAIN,
		.expect_len = 0,
		.func = aics_auto_gain,
	},
	{
		.opcode = BTP_AICS_AUTO_GAIN_ONLY,
		.expect_len = 0,
		.func = aics_auto_gain_only,
	},
	{
		.opcode = BTP_AICS_MAN_GAIN_ONLY,
		.expect_len = 0,
		.func = aics_man_gain_only,
	},
	{
		.opcode = BTP_AICS_DESCRIPTION,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = aics_desc,
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

static uint8_t vocs_audio_loc(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_vocs_audio_loc_cmd *cp = cmd;
	uint32_t loc = sys_le32_to_cpu(cp->loc);

	for (int i = 0; i < CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT; i++) {
		if (bt_vocs_location_set(included.vocs[i], loc) != 0) {
			return BTP_STATUS_FAILED;
		}
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
		vcp_register_param.aics_param[i].cb = &aics_cb;
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

	tester_register_command_handlers(BTP_SERVICE_ID_VCS, vcs_handlers,
					 ARRAY_SIZE(vcs_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_vcs(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_init_aics(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_AICS, aics_handlers,
					 ARRAY_SIZE(aics_handlers));

	return tester_init_vcs();
}

uint8_t tester_unregister_aics(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_init_vocs(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_VOCS, vocs_handlers,
					 ARRAY_SIZE(vocs_handlers));

	return tester_init_vcs();
}

uint8_t tester_unregister_vocs(void)
{
	return BTP_STATUS_SUCCESS;
}
