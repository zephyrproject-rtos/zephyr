/** @file
 *  @brief Bluetooth Volume Control Profile (VCP) Volume Renderer role.
 *
 *  Copyright (c) 2020 Bose Corporation
 *  Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *  Copyright (c) 2022 Codecoup
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

static struct bt_vcp_included vcp_included;

static void vcs_state_cb(struct bt_conn *conn, int err, uint8_t volume, uint8_t mute)
{
	if (err) {
		printk("VCS state get failed (%d)\n", err);
	} else {
		printk("VCS volume %u, mute %u\n", volume, mute);
	}
}

static void vcs_flags_cb(struct bt_conn *conn, int err, uint8_t flags)
{
	if (err) {
		printk("VCS flags get failed (%d)\n", err);
	} else {
		printk("VCS flags 0x%02X\n", flags);
	}
}

static void aics_state_cb(struct bt_aics *inst, int err, int8_t gain, uint8_t mute, uint8_t mode)
{
	if (err) {
		printk("AICS state get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p state gain %d, mute %u, mode %u\n",
		       inst, gain, mute, mode);
	}
}

static void aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units, int8_t minimum,
				 int8_t maximum)
{
	if (err) {
		printk("AICS gain settings get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p gain settings units %u, min %d, max %d\n",
		       inst, units, minimum, maximum);
	}
}

static void aics_input_type_cb(struct bt_aics *inst, int err, uint8_t input_type)
{
	if (err) {
		printk("AICS input type get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p input type %u\n", inst, input_type);
	}
}

static void aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	if (err) {
		printk("AICS status get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p status %s\n", inst, active ? "active" : "inactive");
	}

}
static void aics_description_cb(struct bt_aics *inst, int err, char *description)
{
	if (err) {
		printk("AICS description get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p description %s\n", inst, description);
	}
}
static void vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	if (err) {
		printk("VOCS state get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("VOCS inst %p offset %d\n", inst, offset);
	}
}

static void vocs_location_cb(struct bt_vocs *inst, int err, uint32_t location)
{
	if (err) {
		printk("VOCS location get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("VOCS inst %p location %u\n", inst, location);
	}
}

static void vocs_description_cb(struct bt_vocs *inst, int err, char *description)
{
	if (err) {
		printk("VOCS description get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("VOCS inst %p description %s\n", inst, description);
	}
}

static struct bt_vcp_vol_rend_cb vcp_cbs = {
	.state = vcs_state_cb,
	.flags = vcs_flags_cb,
};

static struct bt_aics_cb aics_cbs = {
	.state = aics_state_cb,
	.gain_setting = aics_gain_setting_cb,
	.type = aics_input_type_cb,
	.status = aics_status_cb,
	.description = aics_description_cb
};

static struct bt_vocs_cb vocs_cbs = {
	.state = vocs_state_cb,
	.location = vocs_location_cb,
	.description = vocs_description_cb
};

int vcp_vol_renderer_init(void)
{
	int err;
	struct bt_vcp_vol_rend_register_param vcp_register_param;
	char input_desc[CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT][16];
	char output_desc[CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT][16];

	memset(&vcp_register_param, 0, sizeof(vcp_register_param));

	for (int i = 0; i < ARRAY_SIZE(vcp_register_param.vocs_param); i++) {
		vcp_register_param.vocs_param[i].location_writable = true;
		vcp_register_param.vocs_param[i].desc_writable = true;
		snprintf(output_desc[i], sizeof(output_desc[i]), "Output %d", i + 1);
		vcp_register_param.vocs_param[i].output_desc = output_desc[i];
		vcp_register_param.vocs_param[i].cb = &vocs_cbs;
	}

	for (int i = 0; i < ARRAY_SIZE(vcp_register_param.aics_param); i++) {
		vcp_register_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]), "Input %d", i + 1);
		vcp_register_param.aics_param[i].description = input_desc[i];
		vcp_register_param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
		vcp_register_param.aics_param[i].status = true;
		vcp_register_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		vcp_register_param.aics_param[i].units = 1;
		vcp_register_param.aics_param[i].min_gain = -100;
		vcp_register_param.aics_param[i].max_gain = 100;
		vcp_register_param.aics_param[i].cb = &aics_cbs;
	}

	vcp_register_param.step = 1;
	vcp_register_param.mute = BT_VCP_STATE_UNMUTED;
	vcp_register_param.volume = 100;
	vcp_register_param.cb = &vcp_cbs;

	err = bt_vcp_vol_rend_register(&vcp_register_param);
	if (err) {
		return err;
	}

	err = bt_vcp_vol_rend_included_get(&vcp_included);
	if (err != 0) {
		return err;
	}

	return 0;
}
