/* csip.c - CAP Commander specific VCP mocks */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/vcp.h>

static struct bt_vcp_vol_ctlr_cb *vcp_cb;

static struct bt_vcp_vol_ctlr {
	struct bt_conn *conn;
} vol_ctlrs[CONFIG_BT_MAX_CONN];

struct bt_vcp_vol_ctlr *bt_vcp_vol_ctlr_get_by_conn(const struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(vol_ctlrs); i++) {
		if (vol_ctlrs[i].conn == conn) {
			return &vol_ctlrs[i];
		}
	}

	return NULL;
}

int bt_vcp_vol_ctlr_conn_get(const struct bt_vcp_vol_ctlr *vol_ctlr, struct bt_conn **conn)
{
	*conn = vol_ctlr->conn;

	return 0;
}

int bt_vcp_vol_ctlr_set_vol(struct bt_vcp_vol_ctlr *vol_ctlr, uint8_t volume)
{
	if (vcp_cb->vol_set != NULL) {
		vcp_cb->vol_set(vol_ctlr, 0);
	}
}

int bt_vcp_vol_ctlr_cb_register(struct bt_vcp_vol_ctlr_cb *cb)
{
	vcp_cb = cb;
}
