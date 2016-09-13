/*
 * Audio Video Distribution Protocol
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
 *
*/

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <misc/nano_work.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "avdtp_internal.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_AVDTP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

/* TODO add config file*/
#define CONFIG_BLUETOOTH_AVDTP_CONN CONFIG_BLUETOOTH_MAX_CONN

/* Pool for outgoing BR/EDR signaling packets, min MTU is 48 */
static struct nano_fifo avdtp_sig;
static NET_BUF_POOL(avdtp_sig_pool, CONFIG_BLUETOOTH_AVDTP_CONN,
		    BT_AVDTP_BUF_SIZE(BT_AVDTP_MIN_MTU),
		    &avdtp_sig, NULL, BT_BUF_USER_DATA_MIN);

static struct bt_avdtp bt_avdtp_pool[CONFIG_BLUETOOTH_AVDTP_CONN];

static struct bt_avdtp_event_cb *event_cb;

#define AVDTP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_avdtp, br_chan.chan)

/* L2CAP Interface callbacks */
void bt_avdtp_l2cap_connected(struct bt_l2cap_chan *chan)
{
	BT_DBG("");
}

void bt_avdtp_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	BT_DBG("");
}

void bt_avdtp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	BT_DBG("");
}

int bt_avdtp_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static struct bt_l2cap_chan_ops ops = {
		.connected = bt_avdtp_l2cap_connected,
		.disconnected = bt_avdtp_l2cap_disconnected,
		.recv = bt_avdtp_l2cap_recv,
	};

	BT_DBG("conn %p, handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_avdtp_pool); i++) {
		struct bt_avdtp *avdtp = &bt_avdtp_pool[i];

		if (avdtp->br_chan.chan.conn) {
			continue;
		}

		avdtp->br_chan.chan.ops = &ops;
		avdtp->br_chan.rx.mtu = BT_AVDTP_MAX_MTU;
		*chan = &avdtp->br_chan.chan;
		return 0;
	}

	return -ENOMEM;
}

/* Application will register its callback */
int bt_avdtp_register(struct bt_avdtp_event_cb *cb)
{
	BT_DBG("");

	if (event_cb) {
		return -EALREADY;
	}

	event_cb = cb;

	return 0;
}

/* init function */
int bt_avdtp_init(void)
{
	int err;
	static struct bt_l2cap_server avdtp_l2cap = {
		.psm = BT_L2CAP_PSM_AVDTP,
		.accept = bt_avdtp_l2cap_accept,
	};

	BT_DBG("");

	/* Memory Initialization */
	net_buf_pool_init(avdtp_sig_pool);

	/* Register AVDTP PSM with L2CAP */
	err = bt_l2cap_br_server_register(&avdtp_l2cap);
	if (err < 0) {
		BT_ERR("AVDTP L2CAP Registration failed %d", err);
	}

	return err;
}
