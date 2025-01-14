/* csip.c - CAP Commander specific VCP mocks */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

static struct bt_vcp_vol_ctlr_cb *vcp_cb;

static struct bt_vcp_vol_ctlr {
	struct bt_conn *conn;
	struct bt_vocs *vocs[CONFIG_BT_VCP_VOL_CTLR_MAX_VOCS_INST];
	struct bt_aics *aics[CONFIG_BT_VCP_VOL_CTLR_MAX_AICS_INST];
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
	if (vcp_cb != NULL && vcp_cb->vol_set != NULL) {
		vcp_cb->vol_set(vol_ctlr, 0);
	}

	return 0;
}

int bt_vcp_vol_ctlr_mute(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	if (vcp_cb != NULL && vcp_cb->mute != NULL) {
		vcp_cb->mute(vol_ctlr, 0);
	}

	return 0;
}

int bt_vcp_vol_ctlr_unmute(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	if (vcp_cb != NULL && vcp_cb->unmute != NULL) {
		vcp_cb->unmute(vol_ctlr, 0);
	}

	return 0;
}

int bt_vcp_vol_ctlr_discover(struct bt_conn *conn, struct bt_vcp_vol_ctlr **vol_ctlr)
{
	for (size_t i = 0; i < ARRAY_SIZE(vol_ctlrs); i++) {
		if (vol_ctlrs[i].conn == NULL) {
			for (size_t j = 0U; j < ARRAY_SIZE(vol_ctlrs[i].vocs); j++) {
				const int err = bt_vocs_discover(conn, vol_ctlrs[i].vocs[j], NULL);

				if (err != 0) {
					return err;
				}
			}

			vol_ctlrs[i].conn = conn;
			*vol_ctlr = &vol_ctlrs[i];

			return 0;
		}
	}

	return -ENOMEM;
}

int bt_vcp_vol_ctlr_cb_register(struct bt_vcp_vol_ctlr_cb *cb)
{
	vcp_cb = cb;

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR_VOCS)) {
		for (size_t i = 0U; i < ARRAY_SIZE(vol_ctlrs); i++) {
			for (size_t j = 0U; j < ARRAY_SIZE(vol_ctlrs[i].vocs); j++) {
				bt_vocs_client_cb_register(vol_ctlrs[i].vocs[j], &cb->vocs_cb);
			}
		}
	}

	return 0;
}

int bt_vcp_vol_ctlr_included_get(struct bt_vcp_vol_ctlr *vol_ctlr, struct bt_vcp_included *included)
{
	included->vocs_cnt = ARRAY_SIZE(vol_ctlr->vocs);
	included->vocs = vol_ctlr->vocs;

	included->aics_cnt = ARRAY_SIZE(vol_ctlr->aics);
	included->aics = vol_ctlr->aics;

	return 0;
}

void mock_bt_vcp_init(void)
{
	if (IS_ENABLED(CONFIG_BT_VCP_VOL_CTLR_VOCS)) {
		for (size_t i = 0U; i < ARRAY_SIZE(vol_ctlrs); i++) {
			for (size_t j = 0U; j < ARRAY_SIZE(vol_ctlrs[i].vocs); j++) {
				vol_ctlrs[i].vocs[j] = bt_vocs_client_free_instance_get();

				__ASSERT(vol_ctlrs[i].vocs[j],
					 "Could not allocate VOCS client instance");
			}
		}
	}
}

void mock_bt_vcp_cleanup(void)
{
	memset(vol_ctlrs, 0, sizeof(vol_ctlrs));
}
