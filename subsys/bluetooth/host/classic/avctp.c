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
#define BT_AVCTP_HDR_SIZE_SINGLE        (sizeof(struct bt_avctp_header_single))
#define BT_AVCTP_HDR_SIZE_START         (sizeof(struct bt_avctp_header_start))
#define BT_AVCTP_HDR_SIZE_CONTINUE_END  (sizeof(struct bt_avctp_header_continue_end))

#define AVCTP_TX_RETRY_DELAY (100)

#define AVCTP_CHAN(_ch) CONTAINER_OF(_ch, struct bt_avctp, br_chan.chan)
/* L2CAP Server list */
static sys_slist_t avctp_l2cap_server = SYS_SLIST_STATIC_INIT(&avctp_l2cap_server);
static struct k_sem avctp_lock;

/* tx list for packets */
static sys_slist_t avctp_tx_list = SYS_SLIST_STATIC_INIT(&avctp_tx_list);
static struct k_work_delayable avctp_tx_work;

struct avctp_buf_user_data {
	struct bt_avctp *session;
	uint16_t sent_len;
	uint16_t pid;
	uint8_t tid;
	uint8_t num_packet;
	bt_avctp_cr_t cr;
	uint8_t ipid;
} __packed;

static void avctp_tx_raise(int msec)
{
	if (sys_slist_is_empty(&avctp_tx_list)) {
		LOG_DBG("TX list is empty, no work to submit");
		return;
	}
	LOG_DBG("kick TX");
	k_work_schedule(&avctp_tx_work, K_MSEC(msec));
}

static void bt_avctp_clear_tx(struct bt_avctp *session)
{
	struct net_buf *buf, *next;
	struct avctp_buf_user_data *user_data;
	const sys_snode_t *head = sys_slist_peek_head(&avctp_tx_list);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&avctp_tx_list, buf, next, node) {
		user_data = net_buf_user_data(buf);

		if (user_data->session != session) {
			continue;
		}

		bool is_head = (&buf->node == head);

		sys_slist_find_and_remove(&avctp_tx_list, &buf->node);
		net_buf_unref(buf);

		if (is_head) {
			avctp_tx_raise(0);
			head = sys_slist_peek_head(&avctp_tx_list);
		}
	}
}

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

	if (session->reassembly_buf != NULL) {
		net_buf_unref(session->reassembly_buf);
		session->reassembly_buf = NULL;
	}

	k_sem_take(&avctp_lock, K_FOREVER);
	bt_avctp_clear_tx(session);
	k_sem_give(&avctp_lock);

	if (session->ops && session->ops->disconnected) {
		session->ops->disconnected(session);
	}
}

static void avctp_l2cap_encrypt_changed(struct bt_l2cap_chan *chan, uint8_t status)
{
	LOG_DBG("");
}

static void avctp_tx_cb(struct bt_conn *conn, void *user_data, int err)
{
	if (err < 0) {
		LOG_WRN("AVCTP TX completed with err=%d", err);
	}
	avctp_tx_raise(0);
}

static void avctp_tx_remove(struct net_buf *buf)
{
	k_sem_take(&avctp_lock, K_FOREVER);
	sys_slist_find_and_remove(&avctp_tx_list, &buf->node);
	k_sem_give(&avctp_lock);
}

static int send_single_avctp(struct bt_avctp *avctp, uint8_t tid, bt_avctp_cr_t cr, uint8_t ipid,
			     uint16_t pid, struct net_buf *buf)
{
	struct bt_avctp_header_single *avctp_hdr;
	int err;

	if ((avctp == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	if (net_buf_headroom(buf) < BT_AVCTP_HDR_SIZE_SINGLE) {
		LOG_ERR("Not enough headroom in buffer for AVCTP SINGLE header");
		return -ENOMEM;
	}

	avctp_hdr = net_buf_push(buf, BT_AVCTP_HDR_SIZE_SINGLE);
	memset(avctp_hdr, 0, BT_AVCTP_HDR_SIZE_SINGLE);
	avctp_hdr->pid = sys_cpu_to_be16(pid);
	BT_AVCTP_HDR_SET(&avctp_hdr->common, ipid, cr, BT_AVCTP_PKT_TYPE_SINGLE, tid);

	avctp_tx_remove(buf);
	err = bt_l2cap_br_chan_send_cb(&avctp->br_chan.chan, buf, avctp_tx_cb, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send l2cap chan (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

static int send_fragmented_avctp(struct bt_avctp *avctp, struct avctp_buf_user_data *user_data,
				 bt_avctp_pkt_type_t pkt_type, uint8_t *data, uint16_t data_len)
{
	struct net_buf *buf;
	struct bt_avctp_header_start *avctp_hdr_start;
	struct bt_avctp_header_continue_end *avctp_hdr_continue_end;
	int err;

	if ((avctp == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	switch (pkt_type) {
	case BT_AVCTP_PKT_TYPE_START:
		buf = bt_l2cap_create_pdu(avctp->tx_pool, BT_AVCTP_HDR_SIZE_START);
		if (buf == NULL) {
			LOG_ERR("No buf for AVCTP START");
			return -ENOBUFS;
		}
		avctp_hdr_start = net_buf_push(buf, BT_AVCTP_HDR_SIZE_START);
		memset(avctp_hdr_start, 0, BT_AVCTP_HDR_SIZE_START);
		avctp_hdr_start->pid = sys_cpu_to_be16(avctp->pid);
		avctp_hdr_start->number_packet = user_data->num_packet;
		BT_AVCTP_HDR_SET(&avctp_hdr_start->common, user_data->ipid, user_data->cr, pkt_type,
				 user_data->tid);
		break;

	case BT_AVCTP_PKT_TYPE_CONTINUE:
	case BT_AVCTP_PKT_TYPE_END:
		buf = bt_l2cap_create_pdu(avctp->tx_pool, BT_AVCTP_HDR_SIZE_CONTINUE_END);
		if (buf == NULL) {
			LOG_ERR("No buf for AVCTP CONTINUE/END");
			return -ENOBUFS;
		}
		avctp_hdr_continue_end = net_buf_push(buf, BT_AVCTP_HDR_SIZE_CONTINUE_END);
		memset(avctp_hdr_continue_end, 0, BT_AVCTP_HDR_SIZE_CONTINUE_END);
		BT_AVCTP_HDR_SET(&avctp_hdr_continue_end->common, user_data->ipid, user_data->cr,
				 pkt_type, user_data->tid);
		break;

	default:
		LOG_ERR("Invalid packet type: %d", pkt_type);
		return -EINVAL;
	}

	if (data_len > 0U) {
		if (net_buf_tailroom(buf) < data_len) {
			LOG_WRN("Not enough tailroom for AVCTP payload (len: %d)", data_len);
			net_buf_unref(buf);
			return -ENOMEM;
		}
		net_buf_add_mem(buf, data, data_len);
	}

	err = bt_l2cap_br_chan_send_cb(&avctp->br_chan.chan, buf, avctp_tx_cb, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send l2cap chan (err: %d)", err);
		net_buf_unref(buf);
	}
	return err;
}

static inline uint8_t get_avctp_packet_num(uint16_t total_bytes, uint16_t max_payload_size)
{
	uint16_t first_payload = max_payload_size - BT_AVCTP_HDR_SIZE_START;
	uint16_t fragment_payload = max_payload_size - BT_AVCTP_HDR_SIZE_CONTINUE_END;
	uint16_t remaining;
	uint8_t additional_packet_num;

	if (total_bytes <= max_payload_size - BT_AVCTP_HDR_SIZE_SINGLE) {
		return 1;
	}

	remaining = total_bytes - first_payload;
	additional_packet_num = (remaining + fragment_payload - 1) / fragment_payload;

	return 1 + additional_packet_num;
}

static inline bool avctp_is_retryable_err(int err)
{
	return (err == -ENOBUFS);
}

static void avctp_tx_processor(struct k_work *item)
{
	struct bt_avctp *session;
	struct avctp_buf_user_data *user_data;
	const sys_snode_t *node;
	struct net_buf *buf;
	int err = 0;
	uint16_t chunk_size;
	uint8_t pkt_type;
	uint16_t max_payload_size;

	k_sem_take(&avctp_lock, K_FOREVER);
	node = sys_slist_peek_head(&avctp_tx_list);
	if (node == NULL) {
		k_sem_give(&avctp_lock);
		LOG_DBG("No pending tx");
		return;
	}
	k_sem_give(&avctp_lock);

	buf = CONTAINER_OF(node, struct net_buf, node);

	__ASSERT_NO_MSG(buf->user_data_size >= sizeof(*user_data));
	user_data = net_buf_user_data(buf);

	session = user_data->session;
	__ASSERT_NO_MSG(session != NULL);
	max_payload_size = MIN(session->br_chan.tx.mtu, session->max_tx_payload_size);

	/* The SINGLE packet buf can be sent directly */
	if (user_data->num_packet == 1) {
		err = send_single_avctp(session, user_data->tid, user_data->cr, user_data->ipid,
					user_data->pid, buf);
		if (err < 0) {
			LOG_ERR("Failed to send SINGLE packet, len=%u", buf->len);
			avctp_tx_raise(0);
		}
		return;
	}

	/* Multi-packet fragmented send */
	if (user_data->sent_len == 0U) {
		pkt_type = BT_AVCTP_PKT_TYPE_START;
		chunk_size = max_payload_size - BT_AVCTP_HDR_SIZE_START;
	} else {
		bool is_last = (buf->len <= (max_payload_size - BT_AVCTP_HDR_SIZE_CONTINUE_END));

		pkt_type = is_last ? BT_AVCTP_PKT_TYPE_END : BT_AVCTP_PKT_TYPE_CONTINUE;
		chunk_size = MIN(buf->len, max_payload_size - BT_AVCTP_HDR_SIZE_CONTINUE_END);
	}

	err = send_fragmented_avctp(session, user_data, pkt_type, buf->data, chunk_size);
	if (err < 0) {
		if (avctp_is_retryable_err(err)) {
			LOG_WRN("Retry %p on %p", buf, session);
			avctp_tx_raise(AVCTP_TX_RETRY_DELAY);
			return;
		}
		LOG_ERR("Failed to send fragment at offset %u", user_data->sent_len);
		goto failed;
	}

	if (net_buf_tailroom(buf) < chunk_size) {
		LOG_WRN("Not enough tailroom for AVCTP payload (len: %d)", chunk_size);
		goto failed;
	}
	net_buf_pull_mem(buf, chunk_size);

	user_data->sent_len += chunk_size;
	if (buf->len == 0) {
		avctp_tx_remove(buf);
		net_buf_unref(buf);
	}
	return;

failed:
	avctp_tx_remove(buf);
	net_buf_unref(buf);
	avctp_tx_raise(0);
}

static int bt_avctp_send_common(struct bt_avctp *session, struct net_buf *buf,
				bt_avctp_cr_t cr, uint8_t tid, uint16_t pid,
				uint8_t num_packet, uint8_t ipid)
{
	struct avctp_buf_user_data *user_data;

	__ASSERT_NO_MSG(buf->user_data_size >= sizeof(*user_data));

	user_data = net_buf_user_data(buf);
	user_data->session = session;
	user_data->sent_len = 0;
	user_data->cr = cr;
	user_data->pid = pid;
	user_data->ipid = ipid;
	user_data->num_packet = num_packet;
	user_data->tid = tid;

	k_sem_take(&avctp_lock, K_FOREVER);
	sys_slist_append(&avctp_tx_list, &buf->node);
	k_sem_give(&avctp_lock);

	avctp_tx_raise(0);
	return 0;
}

static int dispatch_avctp_packet(struct bt_avctp *session, struct net_buf *buf,
				 struct bt_avctp_header_common *hdr_common, uint16_t pid)
{
	struct net_buf *rsp;
	int err;
	bt_avctp_cr_t cr = BT_AVCTP_HDR_GET_CR(hdr_common);
	uint8_t tid = BT_AVCTP_HDR_GET_TRANSACTION_LABLE(hdr_common);

	LOG_DBG("AVCTP msg received, cr:0x%X, tid:0x%X, pid: 0x%04X", cr, tid, pid);

	if (pid == session->pid) {
		return session->ops->recv(session, buf, cr, tid);
	}

	LOG_ERR("unsupported AVCTP PID received: 0x%04x", pid);

	if (cr == BT_AVCTP_CMD) {
		rsp = bt_avctp_create_pdu(NULL);
		if (rsp == NULL) {
			__ASSERT(0, "Failed to create AVCTP response PDU");
			return -ENOMEM;
		}

		err = bt_avctp_send_common(session, rsp, BT_AVCTP_RESPONSE, tid, pid, 1,
					   BT_AVCTP_IPID_INVALID);
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
	struct bt_avctp_header_common *hdr_common = (void *)buf->data;
	struct bt_avctp_header_start *hdr_reassembly;
	bt_avctp_pkt_type_t pkt_type = BT_AVCTP_HDR_GET_PACKET_TYPE(hdr_common);
	uint8_t tid = BT_AVCTP_HDR_GET_TRANSACTION_LABLE(hdr_common);
	bt_avctp_cr_t cr = BT_AVCTP_HDR_GET_CR(hdr_common);

	if (pkt_type == BT_AVCTP_PKT_TYPE_START) {
		if (buf->len < BT_AVCTP_HDR_SIZE_START) {
			LOG_WRN("AVCTP HDR too short (len=%u, tid=%u, cr=%u)", buf->len, tid, cr);
			goto failed;
		}

		if (avctp->reassembly_buf != NULL) {
			LOG_WRN("Interleaving fragments not allowed (tid=%u, cr=%u)", tid, cr);
			net_buf_unref(avctp->reassembly_buf);
			avctp->reassembly_buf = NULL;
		}

		if (avctp->rx_pool == NULL) {
			LOG_WRN("Missing RX pool for AVCTP reassembly (tid=%u, cr=%u)", tid, cr);
			goto failed;
		}

		avctp->reassembly_buf = net_buf_alloc(avctp->rx_pool, K_FOREVER);
		if (avctp->reassembly_buf == NULL) {
			LOG_ERR("Failed to allocate reassembly buffer");
			return -ENOMEM;
		}

		__ASSERT_NO_MSG(avctp->reassembly_buf->user_data_size >= sizeof(*hdr_reassembly));
		hdr_reassembly = net_buf_user_data(avctp->reassembly_buf);
		memcpy(hdr_reassembly, net_buf_pull_mem(buf, sizeof(*hdr_reassembly)),
		       sizeof(*hdr_reassembly));

		if (net_buf_tailroom(avctp->reassembly_buf) < buf->len) {
			LOG_WRN("Not enough tailroom for start fragment");
			goto failed;
		}
		net_buf_add_mem(avctp->reassembly_buf, buf->data, buf->len);

		if (hdr_reassembly->number_packet == 0) {
			LOG_WRN("Invalid start number_packet (%u) — expected > 0 (tid=%u, cr=%u)",
				hdr_reassembly->number_packet, tid, cr);
			goto failed;
		}
		hdr_reassembly->number_packet--;
		return 0;
	}

	if (avctp->reassembly_buf == NULL) {
		LOG_WRN("Continuation/end received without Start (tid=%u, cr=%u, pkt_type=%u)",
			tid, cr, (uint8_t)pkt_type);
		goto failed;
	}

	__ASSERT_NO_MSG(avctp->reassembly_buf->user_data_size >= sizeof(*hdr_reassembly));
	hdr_reassembly = net_buf_user_data(avctp->reassembly_buf);

	if ((BT_AVCTP_HDR_GET_TRANSACTION_LABLE(&hdr_reassembly->common) == tid) &&
	    (BT_AVCTP_HDR_GET_CR(&hdr_reassembly->common) == cr)) {
		if (buf->len < BT_AVCTP_HDR_SIZE_CONTINUE_END) {
			LOG_WRN("invalid AVCTP continuation/end header received");
			goto failed;
		}
		hdr_common = net_buf_pull_mem(buf, BT_AVCTP_HDR_SIZE_CONTINUE_END);

		if (net_buf_tailroom(avctp->reassembly_buf) < buf->len) {
			LOG_WRN("Not enough tailroom for continuation/end");
			goto failed;
		}

		net_buf_add_mem(avctp->reassembly_buf, buf->data, buf->len);

		if (hdr_reassembly->number_packet == 0) {
			LOG_WRN("unexpected number_packet underflow (value=%u, tid=%u, cr=%u)",
				hdr_reassembly->number_packet, tid, cr);
			goto failed;
		}
		hdr_reassembly->number_packet--;

		if (pkt_type == BT_AVCTP_PKT_TYPE_END) {
			if (hdr_reassembly->number_packet > 0) {
				LOG_WRN("Fragments remaining on END (remaining=%u, tid=%u, cr=%u)",
					hdr_reassembly->number_packet, tid, cr);
				goto failed;
			}

			dispatch_avctp_packet(avctp, avctp->reassembly_buf, hdr_common,
					      sys_be16_to_cpu(hdr_reassembly->pid));
			net_buf_unref(avctp->reassembly_buf);
			avctp->reassembly_buf = NULL;
			return 0;
		}
		return 0;
	}

	LOG_WRN("No matching START packet found for tid=%u, cr=%u", tid, cr);

failed:
	if (avctp->reassembly_buf != NULL) {
		net_buf_unref(avctp->reassembly_buf);
		avctp->reassembly_buf = NULL;
	}
	return 0; /* Need keep L2CAP up */
}

static int avctp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_avctp *session = AVCTP_CHAN(chan);
	struct bt_avctp_header_common *hdr = (void *)buf->data;
	struct bt_avctp_header_single *hdr_single;
	bt_avctp_pkt_type_t pkt_type;
	int err;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("invalid AVCTP header received");
		return -EINVAL;
	}

	pkt_type = BT_AVCTP_HDR_GET_PACKET_TYPE(hdr);

	if (pkt_type != BT_AVCTP_PKT_TYPE_SINGLE) {
		err = avctp_recv_fragmented(session, buf);
		if (err < 0) {
			LOG_ERR("AVCTP recv fragmented packet fail, err = %d", err);
		}
		return err;
	}

	if (session->reassembly_buf != NULL) {
		LOG_WRN("AVCTP: aborting in-progress reassembly due to SINGLE pkt");
		net_buf_unref(session->reassembly_buf);
		session->reassembly_buf = NULL;
	}

	if (buf->len < BT_AVCTP_HDR_SIZE_SINGLE) {
		LOG_ERR("invalid AVCTP single header received");
		return -EINVAL;
	}
	hdr_single = net_buf_pull_mem(buf, BT_AVCTP_HDR_SIZE_SINGLE);
	return dispatch_avctp_packet(session, buf, &hdr_single->common,
				     sys_be16_to_cpu(hdr_single->pid));
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
	session->reassembly_buf = NULL;
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
	buf = bt_l2cap_create_pdu(pool, sizeof(struct bt_avctp_header_start));
	if (buf == NULL) {
		LOG_ERR("No buff available");
	}

	return buf;
}

int bt_avctp_send(struct bt_avctp *session, struct net_buf *buf, bt_avctp_cr_t cr, uint8_t tid)
{
	uint8_t num_packet;
	uint16_t max_payload_size;

	if ((session == NULL) || (buf == NULL)) {
		return -EINVAL;
	}
	if ((session->tx_pool != NULL) && (session->max_tx_payload_size == 0)) {
		LOG_ERR("Invalid max_tx_payload_size %u", session->max_tx_payload_size);
		return -EINVAL;
	}

	if (session->tx_pool == NULL) {
		if (buf->len > (session->br_chan.tx.mtu - BT_AVCTP_HDR_SIZE_SINGLE)) {
			LOG_ERR("Buffer size %u exceeds MTU %u", buf->len,
				session->br_chan.tx.mtu - BT_AVCTP_HDR_SIZE_SINGLE);
			return -EMSGSIZE;
		}
		num_packet = 1;
	} else {
		max_payload_size = MIN(session->br_chan.tx.mtu, session->max_tx_payload_size);
		num_packet = get_avctp_packet_num(buf->len, max_payload_size);
	}
	return bt_avctp_send_common(session, buf, cr, tid, session->pid, num_packet,
				    BT_AVCTP_IPID_NONE);
}

static int avctp_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			      struct bt_l2cap_chan **chan)
{
	struct bt_avctp *session = NULL;
	struct bt_avctp_server *avctp_server;
	int err;

	LOG_DBG("conn %p", conn);

	avctp_server = CONTAINER_OF(server, struct bt_avctp_server, l2cap);

	k_sem_take(&avctp_lock, K_FOREVER);

	if (!sys_slist_find(&avctp_l2cap_server, &avctp_server->node, NULL)) {
		LOG_WRN("Invalid l2cap server");
		k_sem_give(&avctp_lock);
		return -EINVAL;
	}

	k_sem_give(&avctp_lock);

	/* Get the AVCTP session from upper layer */
	err = avctp_server->accept(conn, &session);
	if (err < 0) {
		LOG_ERR("Incoming connection rejected");
		return err;
	}

	session->br_chan.chan.ops = &ops;
	session->reassembly_buf = NULL;

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

	k_sem_take(&avctp_lock, K_FOREVER);

	if (sys_slist_find(&avctp_l2cap_server, &server->node, NULL)) {
		LOG_WRN("L2CAP server has been registered");
		k_sem_give(&avctp_lock);
		return -EEXIST;
	}

	server->l2cap.accept = avctp_l2cap_accept;
	err = bt_l2cap_br_server_register(&server->l2cap);
	if (err < 0) {
		LOG_ERR("AVCTP L2CAP registration failed %d", err);
		k_sem_give(&avctp_lock);
		return err;
	}

	LOG_DBG("Register L2CAP server %p", server);
	sys_slist_append(&avctp_l2cap_server, &server->node);

	k_sem_give(&avctp_lock);

	return err;
}

int bt_avctp_init(void)
{
	LOG_DBG("Initializing AVCTP");
	/* Locking semaphore initialized to 1 (unlocked) */
	k_sem_init(&avctp_lock, 1, 1);
	k_work_init_delayable(&avctp_tx_work, avctp_tx_processor);

	return 0;
}
