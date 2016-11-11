/** @file
 *  @brief Service Discovery Protocol handling.
 */

/*
 * Copyright (c) 2016 Intel Corporation
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

#include <errno.h>
#include <misc/byteorder.h>

#include <bluetooth/log.h>
#include <bluetooth/sdp.h>

#include "l2cap_internal.h"
#include "sdp_internal.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_SDP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

#define SDP_PSM 0x0001

#define SDP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_sdp, chan.chan)

#define SDP_DATA_MTU 200

#define SDP_MTU (SDP_DATA_MTU + sizeof(struct bt_sdp_hdr))

#define SDP_SERVICE_HANDLE_BASE 0x10000

struct bt_sdp {
	struct bt_l2cap_br_chan chan;
	struct k_fifo           partial_resp_queue;
	/* TODO: Allow more than one pending request */
};

static struct bt_sdp_record *db;
static uint8_t num_services;

static struct bt_sdp bt_sdp_pool[CONFIG_BLUETOOTH_MAX_CONN];

/* Pool for outgoing SDP packets */
static struct k_fifo sdp_buf;
static NET_BUF_POOL(sdp_pool, CONFIG_BLUETOOTH_MAX_CONN,
		    BT_L2CAP_BUF_SIZE(SDP_MTU), &sdp_buf, NULL,
		    BT_BUF_USER_DATA_MIN);

/** @brief Callback for SDP connection
 *
 *  Gets called when an SDP connection is established
 *
 *  @param chan L2CAP channel
 *
 *  @return None
 */
static void bt_sdp_connected(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *ch = CONTAINER_OF(chan,
						   struct bt_l2cap_br_chan,
						   chan);

	struct bt_sdp *sdp = CONTAINER_OF(ch, struct bt_sdp, chan);

	BT_DBG("chan %p cid 0x%04x", ch, ch->tx.cid);

	k_fifo_init(&sdp->partial_resp_queue);

	ch->tx.mtu = SDP_MTU;
	ch->rx.mtu = SDP_MTU;
}

/** @brief Callback for SDP disconnection
 *
 *  Gets called when an SDP connection is terminated
 *
 *  @param chan L2CAP channel
 *
 *  @return None
 */
static void bt_sdp_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *ch = CONTAINER_OF(chan,
						   struct bt_l2cap_br_chan,
						   chan);

	struct bt_sdp *sdp = CONTAINER_OF(ch, struct bt_sdp, chan);

	BT_DBG("chan %p cid 0x%04x", ch, ch->tx.cid);

	memset(sdp, 0, sizeof(*sdp));
}

/** @brief Creates an SDP PDU
 *
 *  Creates an empty SDP PDU and returns the buffer
 *
 *  @param None
 *
 *  @return Pointer to the net_buf buffer
 */
struct net_buf *bt_sdp_create_pdu(void)
{
	struct net_buf *buf;

	buf = bt_l2cap_create_pdu(&sdp_buf, sizeof(struct bt_sdp_hdr));
	if (!buf) {
		BT_ERR("Failed to create PDU");
		return NULL;
	}

	return buf;
}

/** @brief Sends out an SDP PDU
 *
 *  Sends out an SDP PDU after adding the relevant header
 *
 *  @param chan L2CAP channel
 *  @param buf Buffer to be sent out
 *  @param op Opcode to be used in the packet header
 *  @param tid Transaction ID to be used in the packet header
 *
 *  @return None
 */
static void bt_sdp_send(struct bt_l2cap_chan *chan, struct net_buf *buf,
			uint8_t op, uint16_t tid)
{
	struct bt_sdp_hdr *hdr;
	uint16_t param_len = buf->len;

	hdr = net_buf_push(buf, sizeof(struct bt_sdp_hdr));
	hdr->op_code = op;
	hdr->tid = tid;
	hdr->param_len = sys_cpu_to_be16(param_len);

	bt_l2cap_chan_send(chan, buf);
}

/** @brief Sends an error response PDU
 *
 *  Creates and sends an error response PDU
 *
 *  @param chan L2CAP channel
 *  @param err Error code to be sent in the packet
 *  @param tid Transaction ID to be used in the packet header
 *
 *  @return None
 */
static void send_err_rsp(struct bt_l2cap_chan *chan, uint16_t err,
			 uint16_t tid)
{
	struct net_buf *buf;

	BT_DBG("tid %u, error %u", tid, err);

	buf = bt_sdp_create_pdu();

	net_buf_add_be16(buf, err);

	bt_sdp_send(chan, buf, BT_SDP_ERROR_RSP, tid);
}

static const struct {
	uint8_t  op_code;
	uint16_t  (*func)(struct bt_sdp *sdp, struct net_buf *buf,
			  uint16_t tid);
} handlers[] = {
};

/** @brief Callback for SDP data receive
 *
 *  Gets called when an SDP PDU is received. Calls the corresponding handler
 *  based on the op code of the PDU.
 *
 *  @param chan L2CAP channel
 *  @param buf Received PDU
 *
 *  @return None
 */
static void bt_sdp_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_l2cap_br_chan *ch = CONTAINER_OF(chan,
			struct bt_l2cap_br_chan, chan);
	struct bt_sdp *sdp = CONTAINER_OF(ch, struct bt_sdp, chan);
	struct bt_sdp_hdr *hdr = (struct bt_sdp_hdr *)buf->data;
	uint16_t err = BT_SDP_INVALID_SYNTAX;
	size_t i;

	BT_DBG("chan %p, ch %p, cid 0x%04x", chan, ch, ch->tx.cid);

	BT_ASSERT(sdp);

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small SDP PDU received");
		return;
	}

	BT_DBG("Received SDP code 0x%02x len %u", hdr->op_code, buf->len);

	net_buf_pull(buf, sizeof(*hdr));

	if (sys_cpu_to_be16(hdr->param_len) != buf->len) {
		err = BT_SDP_INVALID_PDU_SIZE;
	} else {
		for (i = 0; i < ARRAY_SIZE(handlers); i++) {
			if (hdr->op_code != handlers[i].op_code) {
				continue;
			}

			err = handlers[i].func(sdp, buf, hdr->tid);
			break;
		}
	}

	if (err) {
		BT_WARN("SDP error 0x%02x", err);
		send_err_rsp(chan, err, hdr->tid);
	}
}

/** @brief Callback for SDP connection accept
 *
 *  Gets called when an incoming SDP connection needs to be authorized.
 *  Registers the L2CAP callbacks and allocates an SDP context to the connection
 *
 *  @param conn BT connection object
 *  @param chan L2CAP channel structure (to be returned)
 *
 *  @return 0 for success, or relevant error code
 */
static int bt_sdp_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	static struct bt_l2cap_chan_ops ops = {
		.connected = bt_sdp_connected,
		.disconnected = bt_sdp_disconnected,
		.recv = bt_sdp_recv,
	};
	int i;

	BT_DBG("conn %p", conn);

	for (i = 0; i < ARRAY_SIZE(bt_sdp_pool); i++) {
		struct bt_sdp *sdp = &bt_sdp_pool[i];

		if (sdp->chan.chan.conn) {
			continue;
		}

		sdp->chan.chan.ops = &ops;
		sdp->chan.rx.mtu = SDP_MTU;

		*chan = &sdp->chan.chan;

		return 0;
	}

	BT_ERR("No available SDP context for conn %p", conn);

	return -ENOMEM;
}

void bt_sdp_init(void)
{
	static struct bt_l2cap_server server = {
		.psm = SDP_PSM,
		.accept = bt_sdp_accept,
	};
	int res;

	net_buf_pool_init(sdp_pool);

	res = bt_l2cap_br_server_register(&server);
	if (res) {
		BT_ERR("L2CAP server registration failed with error %d", res);
	}
}

int bt_sdp_register_service(struct bt_sdp_record *service)
{
	uint32_t handle = SDP_SERVICE_HANDLE_BASE;

	if (!service) {
		BT_ERR("No service record specified", service);
		return 0;
	}

	if (db) {
		handle = db->handle + 1;
	}

	service->next = db;
	service->index = num_services++;
	service->handle = handle;
	*((uint32_t *)(service->attrs[0].val.data)) = handle;
	db = service;

	BT_DBG("Service registered at %u", handle);

	return 0;
}
