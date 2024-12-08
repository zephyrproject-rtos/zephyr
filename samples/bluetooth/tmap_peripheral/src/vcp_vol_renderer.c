/** @file
 *  @brief Bluetooth Volume Control Profile (VCP) Volume Renderer role.
 *
 *  Copyright (c) 2020 Bose Corporation
 *  Copyright (c) 2020-2024 Nordic Semiconductor ASA
 *  Copyright (c) 2022 Codecoup
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

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

static struct bt_vcp_vol_rend_cb vcp_cbs = {
	.state = vcs_state_cb,
	.flags = vcs_flags_cb,
};

int vcp_vol_renderer_init(void)
{
	int err;
	struct bt_vcp_vol_rend_register_param vcp_register_param;

	memset(&vcp_register_param, 0, sizeof(vcp_register_param));

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
