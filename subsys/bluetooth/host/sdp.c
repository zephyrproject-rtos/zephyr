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
NET_BUF_POOL_DEFINE(sdp_pool, CONFIG_BLUETOOTH_MAX_CONN,
		    BT_L2CAP_BUF_SIZE(SDP_MTU), BT_BUF_USER_DATA_MIN, NULL);

#define SDP_CLIENT_CHAN(_ch) CONTAINER_OF(_ch, struct bt_sdp_client, chan.chan)

#define SDP_CLIENT_MTU 64

struct bt_sdp_client {
	struct bt_l2cap_br_chan              chan;
	/* list of waiting to be resolved UUID params */
	sys_slist_t                          reqs;
	/* required SDP transaction ID */
	uint16_t                             tid;
	/* UUID params holder being now resolved */
	const struct bt_sdp_discover_params *param;
};

static struct bt_sdp_client bt_sdp_client_pool[CONFIG_BLUETOOTH_MAX_CONN];

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

	buf = bt_l2cap_create_pdu(&sdp_pool, K_FOREVER);
	/* NULL is not a possible return due to K_FOREVER */
	net_buf_reserve(buf, sizeof(struct bt_sdp_hdr));

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

	res = bt_l2cap_br_server_register(&server);
	if (res) {
		BT_ERR("L2CAP server registration failed with error %d", res);
	}
}

int bt_sdp_register_service(struct bt_sdp_record *service)
{
	uint32_t handle = SDP_SERVICE_HANDLE_BASE;

	if (!service) {
		BT_ERR("No service record specified");
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

#define GET_PARAM(__node) \
	CONTAINER_OF(__node, struct bt_sdp_discover_params, _node)

/* ServiceSearchAttribute PDU, ref to BT Core 4.2, Vol 3, part B, 4.7.1 */
static int sdp_client_ssa_search(struct bt_sdp_client *session)
{
	const struct bt_sdp_discover_params *param;
	struct bt_sdp_hdr *hdr;
	struct net_buf *buf;

	/*
	 * Select proper user params, if session->param is invalid it means
	 * getting new UUID from top of to be resolved params list. Otherwise
	 * the context is in a middle of partial SDP PDU responses and cached
	 * value from context can be used.
	 */
	if (!session->param) {
		param = GET_PARAM(sys_slist_peek_head(&session->reqs));
	} else {
		param = session->param;
	}

	if (!param) {
		BT_WARN("No UUIDs to be resolved on remote");
		return -EINVAL;
	}

	buf = bt_l2cap_create_pdu(&sdp_pool, 0);
	if (!buf) {
		BT_ERR("No bufs for PDU");
		return -ENOMEM;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));

	hdr->op_code = BT_SDP_SVC_SEARCH_ATTR_REQ;
	/* BT_SDP_SEQ8 means length of sequence is on additional next byte */
	net_buf_add_u8(buf, BT_SDP_SEQ8);

	switch (param->uuid->type) {
	case BT_UUID_TYPE_16:
		/* Seq length */
		net_buf_add_u8(buf, 0x03);
		/* Seq type */
		net_buf_add_u8(buf, BT_SDP_UUID16);
		/* Seq value */
		net_buf_add_be16(buf, BT_UUID_16(param->uuid)->val);
		break;
	case BT_UUID_TYPE_32:
		net_buf_add_u8(buf, 0x05);
		net_buf_add_u8(buf, BT_SDP_UUID32);
		net_buf_add_be32(buf, BT_UUID_32(param->uuid)->val);
		break;
	case BT_UUID_TYPE_128:
		net_buf_add_u8(buf, 0x11);
		net_buf_add_u8(buf, BT_SDP_UUID128);
		net_buf_add_mem(buf, BT_UUID_128(param->uuid)->val,
				ARRAY_SIZE(BT_UUID_128(param->uuid)->val));
		break;
	default:
		BT_ERR("Unknown UUID type %u", param->uuid->type);
		return -EINVAL;
	}

	/* Set attribute max bytes count to be returned from server */
	net_buf_add_be16(buf, BT_SDP_MAX_ATTR_LEN);
	/*
	 * Sequence definition where data is sequence of elements and where
	 * additional next byte points the size of elements within
	 */
	net_buf_add_u8(buf, BT_SDP_SEQ8);
	net_buf_add_u8(buf, 0x05);
	/* Data element definition for two following 16bits range elements */
	net_buf_add_u8(buf, BT_SDP_UINT32);
	/* Get all attributes. It enables filter out wanted only attributes */
	net_buf_add_be16(buf, 0x0000);
	net_buf_add_be16(buf, 0xffff);

	/* Initial continuation state octet */
	net_buf_add_u8(buf, 0x00);

	/* set overall PDU length */
	hdr->param_len = sys_cpu_to_be16(buf->len - sizeof(*hdr));

	/* Update context param to the one being resolving now */
	session->param = param;
	session->tid++;
	hdr->tid = sys_cpu_to_be16(session->tid);

	return bt_l2cap_chan_send(&session->chan.chan, buf);
}

static void sdp_client_receive(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_sdp_client *session = SDP_CLIENT_CHAN(chan);
	struct bt_sdp_hdr *hdr = (void *)buf->data;
	uint16_t len, tid;

	ARG_UNUSED(session);

	BT_DBG("session %p buf %p", session, buf);

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small SDP PDU");
		return;
	}

	if (hdr->op_code == BT_SDP_ERROR_RSP) {
		BT_INFO("Error SDP PDU response");
		return;
	}

	len = sys_be16_to_cpu(hdr->param_len);
	tid = sys_be16_to_cpu(hdr->tid);
	net_buf_pull(buf, sizeof(*hdr));

	BT_DBG("SDP PDU tid %u len %u", tid, len);

	if (buf->len != len) {
		BT_ERR("SDP PDU length mismatch (%u != %u)", buf->len, len);
		return;
	}
}

static int sdp_client_chan_connect(struct bt_sdp_client *session)
{
	return bt_l2cap_br_chan_connect(session->chan.chan.conn,
					&session->chan.chan, SDP_PSM);
}

static void sdp_client_connected(struct bt_l2cap_chan *chan)
{
	struct bt_sdp_client *session = SDP_CLIENT_CHAN(chan);

	BT_DBG("session %p chan %p connected", session, chan);

	sdp_client_ssa_search(session);
}

static void sdp_client_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_sdp_client *session = SDP_CLIENT_CHAN(chan);

	BT_DBG("session %p chan %p disconnected", session, chan);

	/*
	 * Reset session excluding L2CAP channel member. Let's the channel
	 * resets autonomous.
	 */
	memset(&session->reqs, 0, sizeof(*session) - sizeof(session->chan));
}

static struct bt_l2cap_chan_ops sdp_client_chan_ops = {
		.connected = sdp_client_connected,
		.disconnected = sdp_client_disconnected,
		.recv = sdp_client_receive,
};

static struct bt_sdp_client *sdp_client_new_session(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_sdp_client_pool); i++) {
		struct bt_sdp_client *session = &bt_sdp_client_pool[i];
		int err;

		if (session->chan.chan.conn) {
			continue;
		}

		sys_slist_init(&session->reqs);

		session->chan.chan.ops = &sdp_client_chan_ops;
		session->chan.chan.conn = conn;
		session->chan.rx.mtu = SDP_CLIENT_MTU;

		err = sdp_client_chan_connect(session);
		if (err) {
			memset(session, 0, sizeof(*session));
			BT_ERR("Cannot connect %d", err);
			return NULL;
		}

		return session;
	}

	BT_ERR("No available SDP client context");

	return NULL;
}

static struct bt_sdp_client *sdp_client_get_session(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_sdp_client_pool); i++) {
		if (bt_sdp_client_pool[i].chan.chan.conn == conn) {
			return &bt_sdp_client_pool[i];
		}
	}

	/*
	 * Try to allocate session context since not found in pool and attempt
	 * connect to remote SDP endpoint.
	 */
	return sdp_client_new_session(conn);
}

int bt_sdp_discover(struct bt_conn *conn,
		    const struct bt_sdp_discover_params *params)
{
	struct bt_sdp_client *session;

	if (!params || !params->uuid || !params->func || !params->pool) {
		BT_WARN("Invalid user params");
		return -EINVAL;
	}

	session = sdp_client_get_session(conn);
	if (!session) {
		return -ENOMEM;
	}

	sys_slist_append(&session->reqs, (sys_snode_t *)&params->_node);

	return 0;
}
