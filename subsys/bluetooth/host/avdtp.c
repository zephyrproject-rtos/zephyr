/*
 * Audio Video Distribution Protocol
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_DEBUG_AVDTP)
#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/avdtp.h>

#include "l2cap_internal.h"
#include "avdtp_internal.h"

/* TODO add config file*/
#define CONFIG_BLUETOOTH_AVDTP_CONN CONFIG_BLUETOOTH_MAX_CONN

/* Pool for outgoing BR/EDR signaling packets, min MTU is 48 */
/*
NET_BUF_POOL_DEFINE(avdtp_sig_pool, CONFIG_BLUETOOTH_AVDTP_CONN,
		    BT_AVDTP_BUF_SIZE(BT_AVDTP_MIN_MTU),
		    BT_BUF_USER_DATA_MIN, NULL);
*/

typedef int (*bt_avdtp_func_t)(struct bt_avdtp *session,
			       struct bt_avdtp_req *req);

static struct bt_avdtp_event_cb *event_cb;

static struct bt_avdtp_seid_lsep *lseps;

struct bt_avdtp_req {
	uint8_t signal_id;
	uint8_t transaction_id;
	bt_avdtp_func_t func;
	struct k_delayed_work timeout_work;
};

#define AVDTP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_avdtp, br_chan.chan)

#define AVDTP_KWORK(_work) CONTAINER_OF(_work, struct bt_avdtp_req,\
					timeout_work)

#define AVDTP_TIMEOUT K_SECONDS(6)

/* Timeout handler */
static void avdtp_timeout(struct k_work *work)
{
	BT_DBG("Failed Signal_id = %d", (AVDTP_KWORK(work))->signal_id);

	/* Gracefully Disconnect the Signalling and streaming L2cap chann*/

}

/* L2CAP Interface callbacks */
void bt_avdtp_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session;

	if (!chan) {
		BT_ERR("Invalid AVDTP chan");
		return;
	}

	session = AVDTP_CHAN(chan);
	BT_DBG("chan %p session %p", chan, session);
	/* Init the timer */
	k_delayed_work_init(&session->req->timeout_work, avdtp_timeout);

}

void bt_avdtp_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_avdtp *session = AVDTP_CHAN(chan);

	BT_DBG("chan %p session %p", chan, session);
	session->br_chan.chan.conn = NULL;
	/* Clear the Pending req if set*/
}

void bt_avdtp_l2cap_encrypt_changed(struct bt_l2cap_chan *chan, uint8_t status)
{
	BT_DBG("");
}

void bt_avdtp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	BT_DBG("");
}

/*A2DP Layer interface */
int bt_avdtp_connect(struct bt_conn *conn, struct bt_avdtp *session)
{
	static struct bt_l2cap_chan_ops ops = {
		.connected = bt_avdtp_l2cap_connected,
		.disconnected = bt_avdtp_l2cap_disconnected,
		.encrypt_change = bt_avdtp_l2cap_encrypt_changed,
		.recv = bt_avdtp_l2cap_recv
	};

	if (!session) {
		return -EINVAL;
	}

	session->br_chan.chan.ops = &ops;
	session->br_chan.chan.required_sec_level = BT_SECURITY_MEDIUM;

	return bt_l2cap_chan_connect(conn, &session->br_chan.chan,
				     BT_L2CAP_PSM_AVDTP);
}

int bt_avdtp_disconnect(struct bt_avdtp *session)
{
	if (!session) {
		return -EINVAL;
	}

	BT_DBG("session %p", session);

	return bt_l2cap_chan_disconnect(&session->br_chan.chan);
}

int bt_avdtp_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	struct bt_avdtp *session = NULL;
	int result;
	static struct bt_l2cap_chan_ops ops = {
		.connected = bt_avdtp_l2cap_connected,
		.disconnected = bt_avdtp_l2cap_disconnected,
		.recv = bt_avdtp_l2cap_recv,
	};

	BT_DBG("conn %p", conn);
	/* Get the AVDTP session from upper layer */
	result = event_cb->accept(conn, &session);
	if (result < 0) {
		return result;
	}
	session->br_chan.chan.ops = &ops;
	session->br_chan.rx.mtu = BT_AVDTP_MAX_MTU;
	*chan = &session->br_chan.chan;
	return 0;
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

int bt_avdtp_register_sep(uint8_t media_type, uint8_t role,
			  struct bt_avdtp_seid_lsep *lsep)
{
	BT_DBG("");

	static uint8_t bt_avdtp_seid = BT_AVDTP_MIN_SEID;

	if (!lsep) {
		return -EIO;
	}

	if (bt_avdtp_seid == BT_AVDTP_MAX_SEID) {
		return -EIO;
	}

	lsep->sep.id = bt_avdtp_seid++;
	lsep->sep.inuse = 0;
	lsep->sep.media_type = media_type;
	lsep->sep.tsep = role;

	lsep->next = lseps;
	lseps = lsep;

	return 0;
}

/* init function */
int bt_avdtp_init(void)
{
	int err;
	static struct bt_l2cap_server avdtp_l2cap = {
		.psm = BT_L2CAP_PSM_AVDTP,
		.sec_level = BT_SECURITY_MEDIUM,
		.accept = bt_avdtp_l2cap_accept,
	};

	BT_DBG("");

	/* Register AVDTP PSM with L2CAP */
	err = bt_l2cap_br_server_register(&avdtp_l2cap);
	if (err < 0) {
		BT_ERR("AVDTP L2CAP Registration failed %d", err);
	}

	return err;
}

/* AVDTP Discover Request */
int bt_avdtp_discover(struct bt_avdtp *session,
		      struct bt_avdtp_discover_params *param)
{
	BT_DBG("");
	if (!param || !session) {
		BT_DBG("Error: Callback/Session not valid");
	}
	return 0;
}
