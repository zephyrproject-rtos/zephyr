/** @file
 * @brief Advance Audio Distribution Profile.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/avdtp.h>
#include <bluetooth/a2dp.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_A2DP)
#define LOG_MODULE_NAME bt_a2dp
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"

#define A2DP_NO_SPACE (-1)

struct bt_a2dp {
	struct bt_avdtp session;
};

/* Connections */
static struct bt_a2dp connection[CONFIG_BT_MAX_CONN];

void a2d_reset(struct bt_a2dp *a2dp_conn)
{
	(void)memset(a2dp_conn, 0, sizeof(struct bt_a2dp));
}

struct bt_a2dp *get_new_connection(struct bt_conn *conn)
{
	int8_t i, free;

	free = A2DP_NO_SPACE;

	if (!conn) {
		BT_ERR("Invalid Input (err: %d)", -EINVAL);
		return NULL;
	}

	/* Find a space */
	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (connection[i].session.br_chan.chan.conn == conn) {
			BT_DBG("Conn already exists");
			return NULL;
		}

		if (!connection[i].session.br_chan.chan.conn &&
		    free == A2DP_NO_SPACE) {
			free = i;
		}
	}

	if (free == A2DP_NO_SPACE) {
		BT_DBG("More connection cannot be supported");
		return NULL;
	}

	/* Clean the memory area before returning */
	a2d_reset(&connection[free]);

	return &connection[free];
}

int a2dp_accept(struct bt_conn *conn, struct bt_avdtp **session)
{
	struct bt_a2dp *a2dp_conn;

	a2dp_conn = get_new_connection(conn);
	if (!a2dp_conn) {
		return -ENOMEM;
	}

	*session = &(a2dp_conn->session);
	BT_DBG("session: %p", &(a2dp_conn->session));

	return 0;
}

/* Callback for incoming requests */
static struct bt_avdtp_ind_cb cb_ind = {
	/*TODO*/
};

/* The above callback structures need to be packed and passed to AVDTP */
static struct bt_avdtp_event_cb avdtp_cb = {
	.ind = &cb_ind,
	.accept = a2dp_accept
};

int bt_a2dp_init(void)
{
	int err;

	/* Register event handlers with AVDTP */
	err = bt_avdtp_register(&avdtp_cb);
	if (err < 0) {
		BT_ERR("A2DP registration failed");
		return err;
	}

	BT_DBG("A2DP Initialized successfully.");
	return 0;
}

struct bt_a2dp *bt_a2dp_connect(struct bt_conn *conn)
{
	struct bt_a2dp *a2dp_conn;
	int err;

	a2dp_conn = get_new_connection(conn);
	if (!a2dp_conn) {
		BT_ERR("Cannot allocate memory");
		return NULL;
	}

	err = bt_avdtp_connect(conn, &(a2dp_conn->session));
	if (err < 0) {
		/* If error occurs, undo the saving and return the error */
		a2d_reset(a2dp_conn);
		BT_DBG("AVDTP Connect failed");
		return NULL;
	}

	BT_DBG("Connect request sent");
	return a2dp_conn;
}

int bt_a2dp_register_endpoint(struct bt_a2dp_endpoint *endpoint,
			      uint8_t media_type, uint8_t role)
{
	int err;

	BT_ASSERT(endpoint);

	err = bt_avdtp_register_sep(media_type, role, &(endpoint->info));
	if (err < 0) {
		return err;
	}

	/* TODO: Register SDP record */

	return 0;
}
