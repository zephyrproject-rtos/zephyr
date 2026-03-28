/* csip.c - CAP Commander specific MICP mocks */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

static struct bt_micp_mic_ctlr_cb *micp_cb;

static struct bt_micp_mic_ctlr {
	struct bt_conn *conn;
	struct bt_aics *aics[CONFIG_BT_MICP_MIC_CTLR_MAX_AICS_INST];
} mic_ctlrs[CONFIG_BT_MAX_CONN];

struct bt_micp_mic_ctlr *bt_micp_mic_ctlr_get_by_conn(const struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(mic_ctlrs); i++) {
		if (mic_ctlrs[i].conn == conn) {
			return &mic_ctlrs[i];
		}
	}

	return NULL;
}

int bt_micp_mic_ctlr_conn_get(const struct bt_micp_mic_ctlr *mic_ctlr, struct bt_conn **conn)
{
	*conn = mic_ctlr->conn;

	return 0;
}

int bt_micp_mic_ctlr_mute(struct bt_micp_mic_ctlr *mic_ctlr)
{
	if (micp_cb != NULL && micp_cb->mute_written != NULL) {
		micp_cb->mute_written(mic_ctlr, 0);
	}

	return 0;
}

int bt_micp_mic_ctlr_unmute(struct bt_micp_mic_ctlr *mic_ctlr)
{
	if (micp_cb != NULL && micp_cb->unmute_written != NULL) {
		micp_cb->unmute_written(mic_ctlr, 0);
	}

	return 0;
}

int bt_micp_mic_ctlr_discover(struct bt_conn *conn, struct bt_micp_mic_ctlr **mic_ctlr)
{
	for (size_t i = 0; i < ARRAY_SIZE(mic_ctlrs); i++) {
		if (mic_ctlrs[i].conn == NULL) {
			for (size_t j = 0U; j < ARRAY_SIZE(mic_ctlrs[i].aics); j++) {
				const int err = bt_aics_discover(conn, mic_ctlrs[i].aics[j], NULL);

				if (err != 0) {
					return err;
				}
			}

			mic_ctlrs[i].conn = conn;
			*mic_ctlr = &mic_ctlrs[i];

			return 0;
		}
	}

	return -ENOMEM;
}

int bt_micp_mic_ctlr_cb_register(struct bt_micp_mic_ctlr_cb *cb)
{
	micp_cb = cb;

	if (IS_ENABLED(CONFIG_BT_MICP_MIC_CTLR_AICS)) {
		for (size_t i = 0U; i < ARRAY_SIZE(mic_ctlrs); i++) {
			for (size_t j = 0U; j < ARRAY_SIZE(mic_ctlrs[i].aics); j++) {
				bt_aics_client_cb_register(mic_ctlrs[i].aics[j], &cb->aics_cb);
			}
		}
	}

	return 0;
}

int bt_micp_mic_ctlr_included_get(struct bt_micp_mic_ctlr *mic_ctlr,
				  struct bt_micp_included *included)
{
	included->aics_cnt = ARRAY_SIZE(mic_ctlr->aics);
	included->aics = mic_ctlr->aics;

	return 0;
}

void mock_bt_micp_init(void)
{
	if (IS_ENABLED(CONFIG_BT_MICP_MIC_CTLR_AICS)) {
		for (size_t i = 0U; i < ARRAY_SIZE(mic_ctlrs); i++) {
			for (size_t j = 0U; j < ARRAY_SIZE(mic_ctlrs[i].aics); j++) {
				mic_ctlrs[i].aics[j] = bt_aics_client_free_instance_get();

				__ASSERT(mic_ctlrs[i].aics[j],
					 "Could not allocate AICS client instance");
			}
		}
	}
}

void mock_bt_micp_cleanup(void)
{
	memset(mic_ctlrs, 0, sizeof(mic_ctlrs));
}
