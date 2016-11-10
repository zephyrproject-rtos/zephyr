/** @file
 * @brief Advance Audio Distribution Profile.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <misc/printk.h>
#include <bluetooth/log.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/a2dp.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "avdtp_internal.h"
#include "a2dp_internal.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_A2DP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define A2DP_NO_SPACE (-1)

struct bt_a2dp {
	struct bt_avdtp session;
};

/* Connections */
static struct bt_a2dp connection[CONFIG_BLUETOOTH_MAX_CONN];

/* Callback for action confirmation */
static struct bt_avdtp_cfm_cb cb_cfm = {
	/*TODO*/
};

/* Callback for incoming requests */
static struct bt_avdtp_ind_cb cb_ind = {
	/*TODO*/
};

/* The above callback structures need to be packed and passed to AVDTP */
static struct bt_avdtp_event_cb avdtp_cb = {
	.ind = &cb_ind,
	.cfm = &cb_cfm
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
	int i, err;
	int8_t free;

	free = A2DP_NO_SPACE;

	if (!conn) {
		BT_ERR("Invalid Input (err: %d)", -EINVAL);
		return NULL;
	}

	/* Find a space */
	for (i = 0; i < CONFIG_BLUETOOTH_MAX_CONN; i++) {
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

	err = bt_avdtp_connect(conn, &connection[free].session);
	if (err < 0) {
		/* If error occurs, undo the saving and return the error */
		memset(&connection[free], 0, sizeof(struct bt_a2dp));
		BT_DBG("AVDTP Connect failed");
		return NULL;
	}

	BT_DBG("Connect request sent");
	return &connection[free];
}
