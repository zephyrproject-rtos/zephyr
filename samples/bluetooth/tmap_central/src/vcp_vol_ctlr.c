/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/vcp.h>

static struct bt_vcp_vol_ctlr *vcp_vol_ctlr;

static void vcs_discover_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err,
			    uint8_t vocs_count, uint8_t aics_count)
{
	if (err != 0) {
		printk("VCP: Service could not be discovered (%d)\n", err);
		return;
	}
}

static void vcs_write_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	if (err != 0) {
		printk("VCP: Write failed (%d)\n", err);
		return;
	}
}

static void vcs_state_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err,
			 uint8_t volume, uint8_t mute)
{
	if (err != 0) {
		printk("VCP: state cb err (%d)", err);
		return;
	}

	printk("VCS volume %u, mute %u\n", volume, mute);
}

static void vcs_flags_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err,
			 uint8_t flags)
{
	if (err != 0) {
		printk("VCP: flags cb err (%d)", err);
		return;
	}

	printk("VCS flags 0x%02X\n", flags);
}

static struct bt_vcp_vol_ctlr_cb vcp_cbs = {
	.discover = vcs_discover_cb,
	.vol_down = vcs_write_cb,
	.vol_up = vcs_write_cb,
	.mute = vcs_write_cb,
	.unmute = vcs_write_cb,
	.vol_down_unmute = vcs_write_cb,
	.vol_up_unmute = vcs_write_cb,
	.vol_set = vcs_write_cb,
	.state = vcs_state_cb,
	.flags = vcs_flags_cb,
};

static int process_profile_connection(struct bt_conn *conn)
{
	int err = 0;

	err = bt_vcp_vol_ctlr_discover(conn, &vcp_vol_ctlr);

	if (err != 0) {
		printk("bt_vcp_vol_ctlr_discover (err %d)\n", err);
	}

	return err;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %d)\n", err);
		return;
	}

	if (process_profile_connection(conn) != 0) {
		printk("Profile connection failed");
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
};

int vcp_vol_ctlr_init(void)
{
	int err;

	err = bt_vcp_vol_ctlr_cb_register(&vcp_cbs);
	if (err != 0) {
		printk("CB register failed (err %d)\n", err);
	}

	return err;
}

int vcp_vol_ctlr_mute(void)
{
	int err;

	if (vcp_vol_ctlr != NULL) {
		err = bt_vcp_vol_ctlr_mute(vcp_vol_ctlr);
	} else {
		err = -EINVAL;
	}

	return err;
}

int vcp_vol_ctlr_unmute(void)
{
	int err;

	if (vcp_vol_ctlr != NULL) {
		err = bt_vcp_vol_ctlr_unmute(vcp_vol_ctlr);
	} else {
		err = -EINVAL;
	}

	return err;
}

int vcp_vol_ctlr_set_vol(uint8_t volume)
{
	int err;

	if (vcp_vol_ctlr != NULL) {
		err = bt_vcp_vol_ctlr_set_vol(vcp_vol_ctlr, volume);
	} else {
		err = -EINVAL;
	}

	return err;
}
