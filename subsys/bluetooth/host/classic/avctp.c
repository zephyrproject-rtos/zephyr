/** @file
 *  @brief Audio Video Control Transport Protocol
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <strings.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "avctp_internal.h"
#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"

#define LOG_LEVEL CONFIG_BT_AVCTP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_avctp);

/** @brief AVCTP Header Sizes for different packet types */
#define BT_AVCTP_HDR_SIZE_SINGLE        (sizeof(struct bt_avctp_header_common) + sizeof(uint16_t))
#define BT_AVCTP_HDR_SIZE_START         (BT_AVCTP_HDR_SIZE_SINGLE + sizeof(uint8_t))
#define BT_AVCTP_HDR_SIZE_CONTINUE_END  (sizeof(struct bt_avctp_header_common))

#define AVCTP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_avctp, br_chan.chan)
/* L2CAP Server list */
static sys_slist_t avctp_l2cap_server = SYS_SLIST_STATIC_INIT(&avctp_l2cap_server);
static struct k_sem avctp_server_lock;

NET_BUF_POOL_FIXED_DEFINE(avctp_rx_pool, CONFIG_BT_AVCTP_RX_DATA_BUF_CNT ,
			  CONFIG_BT_AVCTP_RX_DATA_BUF_SIZE, CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

NET_BUF_POOL_FIXED_DEFINE(avctp_tx_pool, CONFIG_BT_AVCTP_TX_DATA_BUF_CNT,
			  CONFIG_BT_AVCTP_TX_DATA_BUF_SIZE, CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_avctp_tx avctp_tx[CONFIG_BT_AVCTP_TX_DATA_BUF_CNT*2];
static K_FIFO_DEFINE(avctp_tx_free);

static struct bt_avctp_frag_rx avctp_rx[CONFIG_BT_AVCTP_RX_DATA_BUF_CNT*2];
static K_FIFO_DEFINE(avctp_rx_free);

static void avctp_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_avctp *session;

	if (!chan) {
		LOG_ERR("Invalid AVCTP chan");
		return;
	}

	session = AVCTP_CHAN(chan);
	LOG_DBG("chan %p session %p", chan, session);

	if (session->ops && session->ops->connected) {
		session->ops->connected(session);
	}
}

static void avctp_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_avctp *session;

	if (!chan) {
		LOG_ERR("Invalid AVCTP chan");
		return;
	}

	session = AVCTP_CHAN(chan);
	LOG_DBG("chan %p session %p", chan, session);
	session->br_chan.chan.conn = NULL;

	if (session->ops && session->ops->disconnected) {
		session->ops->disconnected(session);
	}
}

static void avctp_l2cap_encrypt_changed(struct bt_l2cap_chan *chan, uint8_t status)
{
	LOG_DBG("");
}

static void avctp_lock(struct bt_avctp *avctp)
{
	k_sem_take(&avctp->lock, K_FOREVER);
}

static void avctp_unlock(struct bt_avctp *avctp)
{
	k_sem_give(&avctp->lock);
}

static void *bt_avctp_alloc(struct k_fifo *fifo, size_t size, const char *type_name)
{
	if (k_current_get() == &k_sys_work_q.thread) {
		return k_fifo_get(fifo, K_NO_WAIT);
	}

	if (IS_ENABLED(CONFIG_BT_AVRCP_LOG_LEVEL)) {
		void *ctx = k_fifo_get(fifo, K_NO_WAIT);
		if (ctx) {
			return ctx;
		}
		LOG_WRN("Unable to get an immediate free %s", type_name);
	}

	return k_fifo_get(fifo, K_FOREVER);
}

static struct bt_avctp_frag_rx *bt_avctp_frag_rx_alloc(void)
{
	return bt_avctp_alloc(&avctp_rx_free, sizeof(struct bt_avctp_frag_rx), "bt_avctp_frag_rx");
}

static struct bt_avctp_tx *bt_avctp_tx_alloc(void)
{
	return bt_avctp_alloc(&avctp_tx_free, sizeof(struct bt_avctp_tx), "bt_avctp_tx");
}

static void bt_avctp_free(struct k_fifo *fifo, void *ctx, size_t size, const char *type_name)
{
	LOG_DBG("Free %s buffer %p", type_name, ctx);
	(void)memset(ctx, 0, size);
	k_fifo_put(fifo, ctx);
}

static void bt_avctp_frag_rx_free(struct bt_avctp_frag_rx *rx)
{
	bt_avctp_free(&avctp_rx_free, rx, sizeof(*rx), "bt_avctp_frag_rx");
}

static void bt_avctp_tx_free(struct bt_avctp_tx *tx)
{
	bt_avctp_free(&avctp_tx_free, tx, sizeof(*tx), "bt_avctp_tx");
}

static int bt_avctp_send_common(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr,
			        uint8_t tid, uint8_t pkt_type, uint8_t ipid)
{
	struct bt_avctp_tx *tx;

	if ((session == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	tx = bt_avctp_tx_alloc();
	if (tx == NULL) {
		LOG_ERR("No tx buffers!");
		return -ENOMEM;
	}

	tx->pkt_type = pkt_type;
	tx->cr =cr;
	tx->tid = tid;
	tx->buf = buf;
	tx->ipid = ipid;
	tx->total_len = buf->len;
	tx->sent_len = 0U;

	avctp_lock(session);
	sys_slist_append(&session->tx_pending, &tx->node);
	avctp_unlock(session);

	k_work_reschedule(&session->tx_work, K_NO_WAIT);

	return 0;
}

static int dispatch_avctp_packet(struct bt_avctp *session, struct net_buf *buf,
			         struct bt_avctp_header *hdr, uint16_t pid)
{
	struct net_buf *rsp;
	int err;
	bt_avctp_cr_t cr = BT_AVCTP_HDR_GET_CR(&hdr->common);
	uint8_t tid = BT_AVCTP_HDR_GET_TRANSACTION_LABLE(&hdr->common);

	LOG_DBG("AVCTP msg received, cr:0x%X, tid:0x%X, pid: 0x%04X",
		cr, tid, sys_be16_to_cpu(pid));

	if (sys_be16_to_cpu(pid) == session->pid) {
		return session->ops->recv(session, buf, cr, tid);
	}

	LOG_ERR("unsupported AVCTP PID received: 0x%04x", sys_be16_to_cpu(pid));

	if (cr == BT_AVCTP_CMD) {
		rsp = bt_avctp_create_pdu(NULL);
		if (rsp == NULL) {
			__ASSERT(0, "Failed to create AVCTP response PDU");
			return -ENOMEM;
		}

		err = bt_avctp_send_common(session, rsp, BT_AVCTP_RESPONSE, tid,
					   BT_AVCTP_PKT_TYPE_SINGLE, BT_AVCTP_IPID_INVALID);
		if (err < 0) {
			net_buf_unref(rsp);
			LOG_ERR("AVCTP send fail, err = %d", err);
			bt_avctp_disconnect(session);
			return err;
		}
	}

	return 0; /* No need to report to the upper layer */
}

static int avctp_recv_fragmented(struct bt_avctp *avctp, struct net_buf *buf)
{
	struct bt_avctp_header *hdr = (void *)buf->data;
	bt_avctp_pkt_type_t pkt_type = BT_AVCTP_HDR_GET_PACKET_TYPE(&hdr->common);
	uint8_t tid = BT_AVCTP_HDR_GET_TRANSACTION_LABLE(&hdr->common);
	bt_avctp_cr_t cr = BT_AVCTP_HDR_GET_CR(&hdr->common);
	struct bt_avctp_frag_rx *rx = NULL;
	struct net_buf *reassembly_buf = NULL;
	sys_snode_t *node;
	uint16_t pid;

	if (pkt_type == BT_AVCTP_PKT_TYPE_START) {
		hdr = net_buf_pull_mem(buf, BT_AVCTP_HDR_SIZE_START);
		rx = bt_avctp_frag_rx_alloc();
		if (rx == NULL) {
			LOG_ERR("No rx buffers!");
			net_buf_unref(buf);
			return -ENOMEM;
		}

		rx->tid = tid;
		rx->cr = cr;
		rx->pid = hdr->fragmented.pid;
		rx->number_packet = hdr->fragmented.number_packet;

		rx->reassembly_buf = net_buf_alloc(&avctp_rx_pool, K_NO_WAIT);
		if (rx->reassembly_buf == NULL) {
			LOG_ERR("Failed to allocate reassembly buffer");
			return -ENOMEM;
		}

		if (net_buf_tailroom(rx->reassembly_buf) < buf->len) {
			LOG_WRN("Not enough tailroom: required");
			goto failed;
		}

		net_buf_add_mem(rx->reassembly_buf, buf->data, buf->len);
		sys_slist_append(&avctp->rx_pending, &rx->node);
		return 0;
	}

	SYS_SLIST_FOR_EACH_NODE(&avctp->rx_pending, node) {
		hdr = net_buf_pull_mem(buf, BT_AVCTP_HDR_SIZE_CONTINUE_END);
		rx = CONTAINER_OF(node, struct bt_avctp_frag_rx, node);

		if ((rx->tid == tid) && (rx->cr == cr)) {
			if (net_buf_tailroom(rx->reassembly_buf) < buf->len) {
				LOG_WRN("Not enough tailroom: required");
				goto failed;
			}
			net_buf_add_mem(rx->reassembly_buf, buf->data, buf->len);
			rx->number_packet--;

			if (pkt_type == BT_AVCTP_PKT_TYPE_END) {
				if (rx->number_packet > 0) {
					LOG_WRN("Unexpected packets number: %d", rx->number_packet);
				}

				reassembly_buf = rx->reassembly_buf;
				pid = rx->pid;

				sys_slist_find_and_remove(&avctp->rx_pending, &rx->node);
				bt_avctp_frag_rx_free(rx);
				dispatch_avctp_packet(avctp, reassembly_buf, hdr, pid);
				net_buf_unref(reassembly_buf);
				return 0;
			}
		}
	}
	return  0;

failed:
	if (rx->reassembly_buf) {
		net_buf_unref(rx->reassembly_buf);
	}
	sys_slist_find_and_remove(&avctp->rx_pending, &rx->node);
	bt_avctp_frag_rx_free(rx);
	return -EINVAL;
}

static int avctp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_avctp *session = AVCTP_CHAN(chan);
	struct bt_avctp_header *hdr = (void *)buf->data;
	bt_avctp_pkt_type_t pkt_type;
	uint16_t pid;

	if (buf->len < sizeof(hdr->common)) {
		LOG_ERR("invalid AVCTP header received");
		return -EINVAL;
	}

	pkt_type = BT_AVCTP_HDR_GET_PACKET_TYPE(&hdr->common);

	if(pkt_type == BT_AVCTP_PKT_TYPE_SINGLE) {
		hdr = net_buf_pull_mem(buf, BT_AVCTP_HDR_SIZE_SINGLE);
		pid = hdr->pid;
	} else {
		return avctp_recv_fragmented(session, buf);

	}
	return dispatch_avctp_packet(session, buf, hdr, pid);
}

void bt_avctp_set_header(struct bt_avctp_header *avctp_hdr, bt_avctp_cr_t cr,
			 bt_avctp_pkt_type_t pkt_type, bt_avctp_ipid_t ipid,
			 uint8_t tid)
{
	LOG_DBG("");

	BT_AVCTP_HDR_SET_TRANSACTION_LABLE(&avctp_hdr->common, tid);
	BT_AVCTP_HDR_SET_PACKET_TYPE(&avctp_hdr->common, pkt_type);
	BT_AVCTP_HDR_SET_CR(&avctp_hdr->common, cr);
	BT_AVCTP_HDR_SET_IPID(&avctp_hdr->common, ipid);
}

static int send_fragmented_avctp(struct bt_avctp *avctp, uint8_t tid,
				 bt_avctp_pkt_type_t pkt_type, bt_avctp_cr_t cr, uint8_t ipid,
				 const uint8_t *data, uint16_t data_len, uint16_t num_packet)
{
	struct net_buf *buf;
	struct bt_avctp_header *avctp_hdr;

	if ((avctp == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	buf = bt_avctp_create_pdu(&avctp_tx_pool);
	if (buf == NULL) {
		LOG_ERR("No buf");
		return -ENOMEM;
	}

	switch (pkt_type) {
	case BT_AVCTP_PKT_TYPE_SINGLE:
		if (net_buf_headroom(buf) < BT_AVCTP_HDR_SIZE_SINGLE) {
			LOG_ERR("Not enough headroom in buffer for AVCTP SINGLE header");
			return -ENOMEM;
		}
		avctp_hdr = net_buf_push(buf, BT_AVCTP_HDR_SIZE_SINGLE);
		memset(avctp_hdr, 0, BT_AVCTP_HDR_SIZE_SINGLE);
		avctp_hdr->pid = sys_be16_to_cpu(avctp->pid);
		break;

	case BT_AVCTP_PKT_TYPE_START:
		if (net_buf_headroom(buf) < BT_AVCTP_HDR_SIZE_START) {
			LOG_ERR("Not enough headroom in buffer for AVCTP START header");
			return -ENOMEM;
		}
		avctp_hdr = net_buf_push(buf, BT_AVCTP_HDR_SIZE_START);
		memset(avctp_hdr, 0, BT_AVCTP_HDR_SIZE_START);
		avctp_hdr->fragmented.pid = sys_be16_to_cpu(avctp->pid);
		avctp_hdr->fragmented.number_packet = num_packet;
		break;

	case BT_AVCTP_PKT_TYPE_CONTINUE:
	case BT_AVCTP_PKT_TYPE_END:
		if (net_buf_headroom(buf) < BT_AVCTP_HDR_SIZE_CONTINUE_END) {
			LOG_ERR("Not enough headroom in buffer for AVCTP CONTINUE/END header");
			return -ENOMEM;
		}
		avctp_hdr = net_buf_push(buf, BT_AVCTP_HDR_SIZE_CONTINUE_END);
		memset(avctp_hdr, 0, BT_AVCTP_HDR_SIZE_CONTINUE_END);
		break;

	default:
		LOG_ERR("Invalid packet type: %d", pkt_type);
		net_buf_unref(buf);
		return -EINVAL;
	}

	bt_avctp_set_header(avctp_hdr, cr, pkt_type, ipid, tid);

	if (data_len > 0U) {
		if (net_buf_tailroom(buf) < data_len) {
			LOG_WRN("Not enough tailroom for AVCTP payload");
			net_buf_unref(buf);
			return -ENOMEM;
		}
		net_buf_add_mem(buf, data, data_len);
	}
	return bt_l2cap_chan_send(&avctp->br_chan.chan, buf);
}

static inline uint16_t get_avctp_packet_num(uint16_t total_bytes, uint16_t l2cap_mtu)
{
	uint16_t first_payload = l2cap_mtu - BT_AVCTP_HDR_SIZE_START;
	uint16_t fragment_payload = l2cap_mtu - BT_AVCTP_HDR_SIZE_CONTINUE_END;
	uint16_t remaining;
	uint16_t additional_packet_num;

	if (total_bytes <= first_payload) {
		return 1;
	}

	remaining = total_bytes - first_payload;
	additional_packet_num = (remaining + fragment_payload - 1) / fragment_payload;

	return 1 + additional_packet_num;
}

static void bt_avctp_tx_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_avctp *avctp = CONTAINER_OF(dwork, struct bt_avctp, tx_work);
	bt_avctp_pkt_type_t pkt_type = BT_AVCTP_PKT_TYPE_START;
	sys_snode_t *node;
	struct bt_avctp_tx *tx;
	uint16_t max_payload_size;
	uint16_t chunk_size = 0U;
	uint16_t sent_len = 0U;
	uint16_t num_packet = 0U;
	int err;

	avctp_lock(avctp);

	node = sys_slist_peek_head(&avctp->tx_pending);
	if (node == NULL) {
		LOG_WRN("No pending tx");
		avctp_unlock(avctp);
		return;
	}

	tx = CONTAINER_OF(node, struct bt_avctp_tx, node);

	/* Calculate maximum payload size per fragment SINGLE packet */
	max_payload_size = avctp->br_chan.tx.mtu - BT_AVCTP_HDR_SIZE_SINGLE;

	/* for SINGLE case*/
	if (tx->pkt_type == BT_AVCTP_PKT_TYPE_SINGLE ||
	   (tx->sent_len == 0 && tx->total_len <= max_payload_size)) {
		err = send_fragmented_avctp(avctp, tx->tid, BT_AVCTP_PKT_TYPE_SINGLE,
					    tx->cr, tx->ipid, tx->buf->data, tx->buf->len, 0);
		if (err < 0) {
			LOG_ERR(" Failed to send Single packet at offset %u", sent_len);
		}
		goto done;
	}

	/* Multi-packet fragmented send */
	if (tx->sent_len == 0U) {
		/* first packet */
		num_packet = get_avctp_packet_num(tx->total_len, avctp->br_chan.tx.mtu);
		max_payload_size = avctp->br_chan.tx.mtu - BT_AVCTP_HDR_SIZE_START;
	} else { /* continue or end packet */
		bool is_last = (tx->sent_len + max_payload_size >= tx->total_len);
		pkt_type = is_last ? BT_AVCTP_PKT_TYPE_END : BT_AVCTP_PKT_TYPE_CONTINUE;
		max_payload_size = avctp->br_chan.tx.mtu - BT_AVCTP_HDR_SIZE_CONTINUE_END;
	}

	chunk_size = MIN(max_payload_size, tx->total_len - tx->sent_len);

	err = send_fragmented_avctp(avctp, tx->tid, pkt_type, tx->cr, tx->ipid,
				    tx->buf->data + tx->sent_len, chunk_size, num_packet);
	if (err < 0) {
		LOG_ERR("Failed to send fragment at offset %u", tx->sent_len);
		goto done;
	}

	tx->sent_len += chunk_size;

	if (tx->sent_len < tx->total_len) {
		avctp_unlock(avctp);
		k_work_reschedule(&avctp->tx_work, K_NO_WAIT);
		return;
	}

	LOG_DBG("Multi-packet transmission complete: total_len=%u", tx->total_len);

done:
	sys_slist_find_and_remove(&avctp->tx_pending, &tx->node);
	net_buf_unref(tx->buf);
	bt_avctp_tx_free(tx);
	avctp_unlock(avctp);
	/* restart the tx work */
	k_work_reschedule(&avctp->tx_work, K_NO_WAIT);
}

static const struct bt_l2cap_chan_ops ops = {
	.connected = avctp_l2cap_connected,
	.disconnected = avctp_l2cap_disconnected,
	.encrypt_change = avctp_l2cap_encrypt_changed,
	.recv = avctp_l2cap_recv,
};

int bt_avctp_connect(struct bt_conn *conn, uint16_t psm, struct bt_avctp *session)
{
	if (!session) {
		return -EINVAL;
	}

	session->br_chan.chan.ops = &ops;
	k_work_init_delayable(&session->tx_work, bt_avctp_tx_work);
	sys_slist_init(&session->tx_pending);

	k_sem_init(&session->lock, 1, 1);
	return bt_l2cap_chan_connect(conn, &session->br_chan.chan, psm);
}

int bt_avctp_disconnect(struct bt_avctp *session)
{
	if (!session) {
		return -EINVAL;
	}

	LOG_DBG("session %p", session);

	return bt_l2cap_chan_disconnect(&session->br_chan.chan);
}

struct net_buf *bt_avctp_create_pdu(struct net_buf_pool *pool)
{
	struct net_buf *buf;

	LOG_DBG("");

	/* reserved avctp_header */
	buf = bt_l2cap_create_pdu(pool, sizeof(struct bt_avctp_header));
	if (buf == NULL) {
		LOG_ERR("No buff available");
		return buf;
	}

	return buf;
}

int bt_avctp_send_single(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr,
			 uint8_t tid)
{
	size_t hdr_size;

	hdr_size = sizeof(struct bt_avctp_header_common) + sizeof(uint16_t);
	if ((session == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (buf->len > (session->br_chan.tx.mtu - hdr_size)) {
		LOG_ERR("Buffer size %u exceeds MTU %u", buf->len,
			session->br_chan.tx.mtu - hdr_size);
		return -EMSGSIZE;
	}
	return bt_avctp_send_common(session, buf, cr, tid, BT_AVCTP_PKT_TYPE_SINGLE,
				    BT_AVCTP_IPID_NONE);
}

int bt_avctp_send(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr,
			 uint8_t tid)
{
	if ((session == NULL) || (buf == NULL)) {
		return -EINVAL;
	}
	return bt_avctp_send_common(session, buf, cr, tid, -1, BT_AVCTP_IPID_NONE);
}

static int avctp_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			      struct bt_l2cap_chan **chan)
{
	struct bt_avctp *session = NULL;
	struct bt_avctp_server *avctp_server;
	int err;

	LOG_DBG("conn %p", conn);

	avctp_server = CONTAINER_OF(server, struct bt_avctp_server, l2cap);

	k_sem_take(&avctp_server_lock, K_FOREVER);

	if (!sys_slist_find(&avctp_l2cap_server, &avctp_server->node, NULL)) {
		LOG_WRN("Invalid l2cap server");
		k_sem_give(&avctp_server_lock);
		return -EINVAL;
	}

	k_sem_give(&avctp_server_lock);

	/* Get the AVCTP session from upper layer */
	err = avctp_server->accept(conn, &session);
	if (err < 0) {
		LOG_ERR("Incoming connection rejected");
		return err;
	}

	session->br_chan.chan.ops = &ops;
	k_work_init_delayable(&session->tx_work, bt_avctp_tx_work);
	sys_slist_init(&session->tx_pending);

	k_sem_init(&session->lock, 1, 1);

	*chan = &session->br_chan.chan;
	return 0;
}

int bt_avctp_server_register(struct bt_avctp_server *server)
{
	int err;
	LOG_DBG("");

	if ((server == NULL) || (server->accept == NULL)) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	k_sem_take(&avctp_server_lock, K_FOREVER);

	if (sys_slist_find(&avctp_l2cap_server, &server->node, NULL)) {
		LOG_WRN("L2CAP server has been registered");
		k_sem_give(&avctp_server_lock);
		return -EEXIST;
	}

	server->l2cap.accept = avctp_l2cap_accept;
	err = bt_l2cap_br_server_register(&server->l2cap);
	if (err < 0) {
		LOG_ERR("AVCTP L2CAP registration failed %d", err);
		k_sem_give(&avctp_server_lock);
		return err;
	}

	LOG_DBG("Register L2CAP server %p", server);
	sys_slist_append(&avctp_l2cap_server, &server->node);

	k_sem_give(&avctp_server_lock);

	return err;
}

int bt_avctp_init(void)
{
	LOG_DBG("Initializing AVCTP");
	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&avctp_server_lock, 1, 1);

	k_fifo_init(&avctp_tx_free);

	for (int index = 0; index < ARRAY_SIZE(avctp_tx); index++) {
		k_fifo_put(&avctp_tx_free, &avctp_tx[index]);
	}

	k_fifo_init(&avctp_rx_free);

	for (int index = 0; index < ARRAY_SIZE(avctp_rx); index++) {
		k_fifo_put(&avctp_rx_free, &avctp_rx[index]);
	}
	return 0;
}
