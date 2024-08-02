/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/bluetooth/audio/bap.h"

static struct bt_bap_broadcast_assistant_cb *broadcast_assistant_cbs;

int bt_bap_broadcast_assistant_register_cb(struct bt_bap_broadcast_assistant_cb *cb)
{
	broadcast_assistant_cbs = cb;

	return 0;
}

int bt_bap_broadcast_assistant_add_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_add_src_param *param)
{
	/* Note that proper parameter checking is done in the caller */
	zassert_not_null(conn, "conn is NULL");
	zassert_not_null(param, "param is NULL");

	if (broadcast_assistant_cbs->add_src != NULL) {
		broadcast_assistant_cbs->add_src(conn, 0);
	}

	return 0;
}
