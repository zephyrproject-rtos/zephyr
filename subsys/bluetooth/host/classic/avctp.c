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

NET_BUF_POOL_FIXED_DEFINE(avctp_pool, CONFIG_BT_MAX_CONN*3, CONFIG_BT_AVCTP_DATA_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

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

static int send_single_avctp(struct bt_avctp *avctp, uint8_t tid, bt_avctp_cr_t cr, uint8_t ipid,
			     struct net_buf *buf)
{
	struct bt_avctp_header *avctp_hdr;

	if ((avctp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (net_buf_headroom(buf) < BT_AVCTP_HDR_SIZE_SINGLE) {
		LOG_ERR("Not enough headroom in buffer for AVCTP SINGLE header");
		return -ENOMEM;
	}

	avctp_hdr = net_buf_push(buf, BT_AVCTP_HDR_SIZE_SINGLE);
	memset(avctp_hdr, 0, BT_AVCTP_HDR_SIZE_SINGLE);
	avctp_hdr->pid = sys_cpu_to_be16(avctp->pid);
	bt_avctp_set_header(avctp_hdr, cr, BT_AVCTP_PKT_TYPE_SINGLE, ipid, tid);

	return bt_l2cap_chan_send(&avctp->br_chan.chan, buf);
}

static int send_fragmented_avctp(struct bt_avctp *avctp, uint8_t tid,
				 bt_avctp_pkt_type_t pkt_type, bt_avctp_cr_t cr, uint8_t ipid,
				 const uint8_t *data, uint16_t data_len, uint16_t num_packet)
{
	struct net_buf *buf;
	struct bt_avctp_header *avctp_hdr;
	int err;

	if ((avctp == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	buf = bt_avctp_create_pdu(&avctp_pool);
	if (buf == NULL) {
		LOG_ERR("No buf");
		return -ENOMEM;
	}

	switch (pkt_type) {
	case BT_AVCTP_PKT_TYPE_START:
		if (net_buf_headroom(buf) < BT_AVCTP_HDR_SIZE_START) {
			LOG_ERR("Not enough headroom in buffer for AVCTP START header");
			net_buf_unref(buf);
			return -ENOMEM;
		}
		avctp_hdr = net_buf_push(buf, BT_AVCTP_HDR_SIZE_START);
		memset(avctp_hdr, 0, BT_AVCTP_HDR_SIZE_START);
		avctp_hdr->fragmented.pid = sys_cpu_to_be16(avctp->pid);
		avctp_hdr->fragmented.number_packet = num_packet;
		break;

	case BT_AVCTP_PKT_TYPE_CONTINUE:
	case BT_AVCTP_PKT_TYPE_END:
		if (net_buf_headroom(buf) < BT_AVCTP_HDR_SIZE_CONTINUE_END) {
			LOG_ERR("Not enough headroom in buffer for AVCTP CONTINUE/END header");
			net_buf_unref(buf);
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
	err = bt_l2cap_chan_send(&avctp->br_chan.chan, buf);
	if (err < 0) {
		LOG_ERR("Failed to send l2cap chan (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
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

static int bt_avctp_send_common(struct bt_avctp *session, struct net_buf *buf,
				bt_avctp_cr_t cr, uint8_t tid,
				uint8_t pkt_type, uint8_t ipid)
{
	uint16_t mtu = session->br_chan.tx.mtu;
	uint16_t sent_len = 0U;
	int err;

	avctp_lock(session);
	/* SINGLE packet */
	if (pkt_type == BT_AVCTP_PKT_TYPE_SINGLE || buf->len <= (mtu - BT_AVCTP_HDR_SIZE_SINGLE)) {
		err = send_single_avctp(session, tid, cr, ipid, buf);
		if (err < 0) {
			LOG_ERR("Failed to send SINGLE packet, len=%u", buf->len);
		}
		avctp_unlock(session);
		return err;
	}

	/* Multi-packet fragmented send */
	uint16_t num_packet = get_avctp_packet_num(buf->len, mtu);

	while (sent_len < buf->len) {
		uint16_t remaining = buf->len - sent_len;
		uint16_t chunk_size;
		uint8_t cur_type;

		if (sent_len == 0U) {
			cur_type = BT_AVCTP_PKT_TYPE_START;
			chunk_size = MIN(remaining, mtu - BT_AVCTP_HDR_SIZE_START);
		} else {
			bool is_last = (remaining <= (mtu - BT_AVCTP_HDR_SIZE_CONTINUE_END));

			cur_type = is_last ? BT_AVCTP_PKT_TYPE_END : BT_AVCTP_PKT_TYPE_CONTINUE;
			chunk_size = MIN(remaining, mtu - BT_AVCTP_HDR_SIZE_CONTINUE_END);
		}

		err = send_fragmented_avctp(session, tid, cur_type, cr, ipid,
					   buf->data + sent_len, chunk_size, num_packet);
		if (err < 0) {
			LOG_ERR("Failed to send fragment at offset %u", sent_len);
			avctp_unlock(session);
			return err;
		}

		sent_len += chunk_size;
	}

	avctp_unlock(session);
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
	struct bt_avctp_frag_rx *rx = NULL, *next;
	struct net_buf *reassembly_buf = NULL;
	uint16_t pid;

	if (pkt_type == BT_AVCTP_PKT_TYPE_START) {
		hdr = net_buf_pull_mem(buf, BT_AVCTP_HDR_SIZE_START);

		reassembly_buf = net_buf_alloc(&avctp_pool, K_NO_WAIT);
		if (reassembly_buf == NULL) {
			LOG_ERR("Failed to allocate reassembly buffer");
			return -ENOMEM;
		}

		net_buf_reserve(reassembly_buf, sizeof(*rx));
		if (net_buf_headroom(reassembly_buf) < sizeof(*rx)) {
			LOG_ERR("Not enough headroom in buffer for avctp frag rx");
			net_buf_unref(reassembly_buf);
			return -ENOMEM;
		}

		rx = net_buf_push(reassembly_buf, sizeof(*rx));
		rx->reassembly_buf = reassembly_buf;
		rx->tid = tid;
		rx->cr = cr;
		rx->pid = hdr->fragmented.pid;
		rx->number_packet = hdr->fragmented.number_packet;

		if (net_buf_tailroom(rx->reassembly_buf) < buf->len) {
			LOG_WRN("Not enough tailroom: required");
			goto failed;
		}

		net_buf_add_mem(rx->reassembly_buf, buf->data, buf->len);
		rx->number_packet--;

		sys_slist_append(&avctp->rx_pending, &rx->node);
		return 0;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&avctp->rx_pending, rx, next, node) {
		if (rx->tid == tid && rx->cr == cr) {
			hdr = net_buf_pull_mem(buf, BT_AVCTP_HDR_SIZE_CONTINUE_END);

			if (net_buf_tailroom(rx->reassembly_buf) < buf->len) {
				LOG_WRN("Not enough tailroom for continuation");
				goto failed;
			}

			net_buf_add_mem(rx->reassembly_buf, buf->data, buf->len);
			rx->number_packet--;

			if (pkt_type == BT_AVCTP_PKT_TYPE_END) {
				if (rx->number_packet > 0) {
					LOG_WRN("Unexpected packets: %d", rx->number_packet);
					goto failed;
				}

				pid = rx->pid;
				if (rx->reassembly_buf->len < sizeof(*rx)) {
					LOG_WRN("invalid reassembly_buf");
					goto failed;
				}
				net_buf_pull_mem(rx->reassembly_buf,  sizeof(*rx));
				dispatch_avctp_packet(avctp, rx->reassembly_buf, hdr, pid);
				net_buf_unref(rx->reassembly_buf);
				sys_slist_find_and_remove(&avctp->rx_pending, &rx->node);
				return 0;
			}
			return 0;
		}
	}
	LOG_WRN("No matching rx found for tid=%u, cr=%u", tid, cr);

failed:
	if (rx->reassembly_buf) {
		net_buf_unref(rx->reassembly_buf);
	}
	sys_slist_find_and_remove(&avctp->rx_pending, &rx->node);
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

	if (pkt_type != BT_AVCTP_PKT_TYPE_SINGLE) {
		return avctp_recv_fragmented(session, buf);
	}

	hdr = net_buf_pull_mem(buf, BT_AVCTP_HDR_SIZE_SINGLE);
	pid = hdr->pid;
	return dispatch_avctp_packet(session, buf, hdr, pid);
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
	sys_slist_init(&session->rx_pending);
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
	}

	return buf;
}

int bt_avctp_send_single(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr,
			 uint8_t tid)
{
	if ((session == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (buf->len > (session->br_chan.tx.mtu - BT_AVCTP_HDR_SIZE_SINGLE)) {
		LOG_ERR("Buffer size %u exceeds MTU %u", buf->len,
			session->br_chan.tx.mtu - BT_AVCTP_HDR_SIZE_SINGLE);
		return -EMSGSIZE;
	}
	return bt_avctp_send_common(session, buf, cr, tid, BT_AVCTP_PKT_TYPE_SINGLE,
				    BT_AVCTP_IPID_NONE);
}

int bt_avctp_send(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr, uint8_t tid)
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
	sys_slist_init(&session->rx_pending);
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
	return 0;
}
