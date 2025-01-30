/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/util_macro.h>

static struct bt_tbs_client_cb *tbs_cbs;

int bt_tbs_client_register_cb(struct bt_tbs_client_cb *cbs)
{
	tbs_cbs = cbs;

	return 0;
}

int bt_tbs_client_discover(struct bt_conn *conn)
{
	if (tbs_cbs != NULL && tbs_cbs->discover != NULL) {
		uint8_t tbs_cnt = 0;

		IF_ENABLED(CONFIG_BT_TBS_CLIENT_TBS,
			   (tbs_cnt += CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES));

		tbs_cbs->discover(conn, 0, tbs_cnt, IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS));
	}

	return 0;
}
