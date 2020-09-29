/* att.c - Attribute protocol handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <drivers/bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_ATT)
#define LOG_MODULE_NAME bt_att
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "smp.h"
#include "att_internal.h"
#include "gatt_internal.h"

#define ATT_CHAN(_ch) CONTAINER_OF(_ch, struct bt_att_chan, chan.chan)
#define ATT_REQ(_node) CONTAINER_OF(_node, struct bt_att_req, node)

#define ATT_CMD_MASK				0x40

#if defined(CONFIG_BT_EATT)
#define ATT_CHAN_MAX				(CONFIG_BT_EATT_MAX + 1)
#else
#define ATT_CHAN_MAX				1
#endif /* CONFIG_BT_EATT */

typedef enum __packed {
		ATT_COMMAND,
		ATT_REQUEST,
		ATT_RESPONSE,
		ATT_NOTIFICATION,
		ATT_CONFIRMATION,
		ATT_INDICATION,
		ATT_UNKNOWN,
} att_type_t;

static att_type_t att_op_get_type(uint8_t op);

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
struct bt_attr_data {
	uint16_t handle;
	uint16_t offset;
};

/* Pool for incoming ATT packets */
NET_BUF_POOL_DEFINE(prep_pool, CONFIG_BT_ATT_PREPARE_COUNT, BT_ATT_MTU,
		    sizeof(struct bt_attr_data), NULL);
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */

K_MEM_SLAB_DEFINE(req_slab, sizeof(struct bt_att_req),
		  CONFIG_BT_ATT_TX_MAX, 16);

enum {
	ATT_PENDING_RSP,
	ATT_PENDING_CFM,
	ATT_DISCONNECTED,
	ATT_ENHANCED,
	ATT_PENDING_SENT,

	/* Total number of flags - must be at the end of the enum */
	ATT_NUM_FLAGS,
};

/* ATT channel specific data */
struct bt_att_chan {
	/* Connection this channel is associated with */
	struct bt_att		*att;
	struct bt_l2cap_le_chan	chan;
	ATOMIC_DEFINE(flags, ATT_NUM_FLAGS);
	struct bt_att_req	*req;
	struct k_fifo		tx_queue;
	struct k_delayed_work	timeout_work;
	struct k_sem            tx_sem;
	void (*sent)(struct bt_att_chan *chan);
	sys_snode_t		node;
};

/* ATT connection specific data */
struct bt_att {
	struct bt_conn		*conn;
	/* Shared request queue */
	sys_slist_t		reqs;
	struct k_fifo		tx_queue;
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
	struct k_fifo		prep_queue;
#endif
	/* Contains bt_att_chan instance(s) */
	sys_slist_t		chans;
};

K_MEM_SLAB_DEFINE(att_slab, sizeof(struct bt_att),
		  CONFIG_BT_MAX_CONN, 16);
K_MEM_SLAB_DEFINE(chan_slab, sizeof(struct bt_att_chan),
		  CONFIG_BT_MAX_CONN * ATT_CHAN_MAX, 16);
static struct bt_att_req cancel;

static void att_req_destroy(struct bt_att_req *req)
{
	BT_DBG("req %p", req);

	if (req->buf) {
		net_buf_unref(req->buf);
	}

	if (req->destroy) {
		req->destroy(req);
	}

	bt_att_req_free(req);
}

typedef void (*bt_att_chan_sent_t)(struct bt_att_chan *chan);

static bt_att_chan_sent_t chan_cb(struct net_buf *buf);
static bt_conn_tx_cb_t att_cb(bt_att_chan_sent_t cb);

void att_sent(struct bt_conn *conn, void *user_data)
{
	struct bt_l2cap_chan *chan = user_data;

	BT_DBG("conn %p chan %p", conn, chan);

	if (chan->ops->sent) {
		chan->ops->sent(chan);
	}
}

static int chan_send(struct bt_att_chan *chan, struct net_buf *buf,
		     bt_att_chan_sent_t cb)
{
	struct bt_att_hdr *hdr;

	hdr = (void *)buf->data;

	BT_DBG("code 0x%02x", hdr->code);

	if (IS_ENABLED(CONFIG_BT_EATT) &&
	    atomic_test_bit(chan->flags, ATT_ENHANCED)) {
		int err;

		/* Check if sent is pending already, if it does it cannot be
		 * modified so the operation will need to be queued.
		 */
		if (atomic_test_and_set_bit(chan->flags, ATT_PENDING_SENT)) {
			return -EAGAIN;
		}

		chan->sent = cb ? cb : chan_cb(buf);

		if (hdr->code == BT_ATT_OP_SIGNED_WRITE_CMD) {
			return -ENOTSUP;
		}

		/* Check if the channel is ready to send in case of a request */
		if (att_op_get_type(hdr->code) == ATT_REQUEST &&
		    !atomic_test_bit(chan->chan.chan.status,
				     BT_L2CAP_STATUS_OUT)) {
			return -EAGAIN;
		}

		/* bt_l2cap_chan_send does actually return the number of bytes
		 * that could be sent immediatelly.
		 */
		err = bt_l2cap_chan_send(&chan->chan.chan, buf);
		if (err < 0) {
			return err;
		}

		return 0;
	}

	if (hdr->code == BT_ATT_OP_SIGNED_WRITE_CMD) {
		int err;

		err = bt_smp_sign(chan->att->conn, buf);
		if (err) {
			BT_ERR("Error signing data");
			net_buf_unref(buf);
			return err;
		}
	}

	chan->sent = cb ? cb : chan_cb(buf);

	return bt_l2cap_send_cb(chan->att->conn, BT_L2CAP_CID_ATT, buf,
				att_cb(chan->sent), &chan->chan.chan);
}

static int process_queue(struct bt_att_chan *chan, struct k_fifo *queue)
{
	struct net_buf *buf;
	int err;

	buf = net_buf_get(queue, K_NO_WAIT);
	if (buf) {
		err = chan_send(chan, buf, NULL);
		if (err) {
			/* Push it back if it could not be send */
			k_queue_prepend(&queue->_queue, buf);
			return err;
		}

		return 0;
	}

	return -ENOENT;
}

/* Send requests without taking tx_sem */
static int chan_req_send(struct bt_att_chan *chan, struct bt_att_req *req)
{
	int err;

	if (chan->chan.tx.mtu < net_buf_frags_len(req->buf)) {
		return -EMSGSIZE;
	}

	BT_DBG("chan %p req %p len %zu", chan, req,
	       net_buf_frags_len(req->buf));

	chan->req = req;

	/* Save request state so it can be resent */
	net_buf_simple_save(&req->buf->b, &req->state);

	/* Keep a reference for resending in case of an error */
	err = chan_send(chan, net_buf_ref(req->buf), NULL);
	if (err) {
		net_buf_unref(req->buf);
		chan->req = NULL;
	}

	return err;
}

static void bt_att_sent(struct bt_l2cap_chan *ch)
{
	struct bt_att_chan *chan = ATT_CHAN(ch);
	struct bt_att *att = chan->att;
	int err;

	BT_DBG("chan %p", chan);

	if (chan->sent) {
		chan->sent(chan);
	}

	atomic_clear_bit(chan->flags, ATT_PENDING_SENT);

	/* Process pending requests first since they require a response they
	 * can only be processed one at time while if other queues were
	 * processed before they may always contain a buffer starving the
	 * request queue.
	 */
	if (!chan->req && !sys_slist_is_empty(&att->reqs)) {
		sys_snode_t *node = sys_slist_get(&att->reqs);

		if (chan_req_send(chan, ATT_REQ(node)) >= 0) {
			return;
		}

		/* Prepend back to the list as it could not be sent */
		sys_slist_prepend(&att->reqs, node);
	}

	/* Process channel queue */
	err = process_queue(chan, &chan->tx_queue);
	if (!err) {
		return;
	}

	/* Process global queue */
	err = process_queue(chan, &att->tx_queue);
	if (!err) {
		return;
	}

	k_sem_give(&chan->tx_sem);
}

static void chan_cfm_sent(struct bt_att_chan *chan)
{
	BT_DBG("chan %p", chan);

	if (IS_ENABLED(CONFIG_BT_ATT_ENFORCE_FLOW)) {
		atomic_clear_bit(chan->flags, ATT_PENDING_CFM);
	}
}

static void chan_rsp_sent(struct bt_att_chan *chan)
{
	BT_DBG("chan %p", chan);

	if (IS_ENABLED(CONFIG_BT_ATT_ENFORCE_FLOW)) {
		atomic_clear_bit(chan->flags, ATT_PENDING_RSP);
	}
}

static void chan_req_sent(struct bt_att_chan *chan)
{
	BT_DBG("chan %p chan->req %p", chan, chan->req);

	/* Start timeout work */
	if (chan->req) {
		k_delayed_work_submit(&chan->timeout_work, BT_ATT_TIMEOUT);
	}
}

static bt_att_chan_sent_t chan_cb(struct net_buf *buf)
{
	switch (att_op_get_type(buf->data[0])) {
	case ATT_RESPONSE:
		return chan_rsp_sent;
	case ATT_CONFIRMATION:
		return chan_cfm_sent;
	case ATT_REQUEST:
	case ATT_INDICATION:
		return chan_req_sent;
	default:
		return NULL;
	}
}

static void att_cfm_sent(struct bt_conn *conn, void *user_data)
{
	struct bt_l2cap_chan *ch = user_data;
	struct bt_att_chan *chan = ATT_CHAN(ch);

	BT_DBG("conn %p chan %p", conn, chan);

	chan->sent = chan_cfm_sent;

	att_sent(conn, user_data);
}

static void att_rsp_sent(struct bt_conn *conn, void *user_data)
{
	struct bt_l2cap_chan *ch = user_data;
	struct bt_att_chan *chan = ATT_CHAN(ch);

	BT_DBG("conn %p chan %p", conn, chan);

	chan->sent = chan_rsp_sent;

	att_sent(conn, user_data);
}

static void att_req_sent(struct bt_conn *conn, void *user_data)
{
	struct bt_l2cap_chan *ch = user_data;
	struct bt_att_chan *chan = ATT_CHAN(ch);

	BT_DBG("conn %p chan %p", conn, chan);

	chan->sent = chan_req_sent;

	att_sent(conn, user_data);
}

static bt_conn_tx_cb_t att_cb(bt_att_chan_sent_t cb)
{
	if (cb == chan_rsp_sent) {
		return att_rsp_sent;
	} else if (cb == chan_cfm_sent) {
		return att_cfm_sent;
	} else if (cb == chan_req_sent) {
		return att_req_sent;
	} else {
		return att_sent;
	}
}

struct net_buf *bt_att_chan_create_pdu(struct bt_att_chan *chan, uint8_t op,
				       size_t len)
{
	struct bt_att_hdr *hdr;
	struct net_buf *buf;

	if (len + sizeof(op) > chan->chan.tx.mtu) {
		BT_WARN("ATT MTU exceeded, max %u, wanted %zu",
			chan->chan.tx.mtu, len + sizeof(op));
		return NULL;
	}

	switch (att_op_get_type(op)) {
	case ATT_RESPONSE:
	case ATT_CONFIRMATION:
		/* Use a timeout only when responding/confirming */
		buf = bt_l2cap_create_pdu_timeout(NULL, 0, BT_ATT_TIMEOUT);
		break;
	default:
		buf = bt_l2cap_create_pdu(NULL, 0);
	}

	if (!buf) {
		BT_ERR("Unable to allocate buffer for op 0x%02x", op);
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}

static inline bool att_chan_is_connected(struct bt_att_chan *chan)
{
	return (chan->att->conn->state != BT_CONN_CONNECTED ||
		!atomic_test_bit(chan->flags, ATT_DISCONNECTED));
}

static int bt_att_chan_send(struct bt_att_chan *chan, struct net_buf *buf,
			    bt_att_chan_sent_t cb)
{
	struct bt_att_hdr *hdr;

	hdr = (void *)buf->data;

	BT_DBG("chan %p flags %u code 0x%02x", chan, atomic_get(chan->flags),
	       hdr->code);

	/* Don't use tx_sem if caller has set it own callback */
	if (!cb) {
		if (k_sem_take(&chan->tx_sem, K_NO_WAIT) < 0) {
			return -EAGAIN;
		}
	}

	return chan_send(chan, buf, cb);
}

static void bt_att_chan_send_rsp(struct bt_att_chan *chan, struct net_buf *buf,
				 bt_att_chan_sent_t cb)
{
	int err;

	err = bt_att_chan_send(chan, buf, cb);
	if (err) {
		/* Responses need to be sent back using the same channel */
		net_buf_put(&chan->tx_queue, buf);
	}
}

static void send_err_rsp(struct bt_att_chan *chan, uint8_t req, uint16_t handle,
			 uint8_t err)
{
	struct bt_att_error_rsp *rsp;
	struct net_buf *buf;

	/* Ignore opcode 0x00 */
	if (!req) {
		return;
	}

	buf = bt_att_chan_create_pdu(chan, BT_ATT_OP_ERROR_RSP, sizeof(*rsp));
	if (!buf) {
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->request = req;
	rsp->handle = sys_cpu_to_le16(handle);
	rsp->error = err;

	bt_att_chan_send_rsp(chan, buf, chan_rsp_sent);
}

static uint8_t att_mtu_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_conn *conn = chan->att->conn;
	struct bt_att_exchange_mtu_req *req;
	struct bt_att_exchange_mtu_rsp *rsp;
	struct net_buf *pdu;
	uint16_t mtu_client, mtu_server;

	/* Exchange MTU sub-procedure shall only be supported on the
	 * LE Fixed Channel Unenhanced ATT bearer.
	 */
	if (atomic_test_bit(chan->flags, ATT_ENHANCED)) {
		return BT_ATT_ERR_NOT_SUPPORTED;
	}

	req = (void *)buf->data;

	mtu_client = sys_le16_to_cpu(req->mtu);

	BT_DBG("Client MTU %u", mtu_client);

	/* Check if MTU is valid */
	if (mtu_client < BT_ATT_DEFAULT_LE_MTU) {
		return BT_ATT_ERR_INVALID_PDU;
	}

	pdu = bt_att_create_pdu(conn, BT_ATT_OP_MTU_RSP, sizeof(*rsp));
	if (!pdu) {
		return BT_ATT_ERR_UNLIKELY;
	}

	mtu_server = BT_ATT_MTU;

	BT_DBG("Server MTU %u", mtu_server);

	rsp = net_buf_add(pdu, sizeof(*rsp));
	rsp->mtu = sys_cpu_to_le16(mtu_server);

	bt_att_chan_send_rsp(chan, pdu, chan_rsp_sent);

	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part F] page 484:
	 *
	 * A device's Exchange MTU Request shall contain the same MTU as the
	 * device's Exchange MTU Response (i.e. the MTU shall be symmetric).
	 */
	chan->chan.rx.mtu = MIN(mtu_client, mtu_server);
	chan->chan.tx.mtu = chan->chan.rx.mtu;

	BT_DBG("Negotiated MTU %u", chan->chan.rx.mtu);
	return 0;
}

static int bt_att_chan_req_send(struct bt_att_chan *chan,
				struct bt_att_req *req)
{
	int err;

	__ASSERT_NO_MSG(chan);
	__ASSERT_NO_MSG(req);
	__ASSERT_NO_MSG(req->func);
	__ASSERT_NO_MSG(!chan->req);

	BT_DBG("req %p", req);

	if (k_sem_take(&chan->tx_sem, K_NO_WAIT) < 0) {
		return -EAGAIN;
	}

	err = chan_req_send(chan, req);
	if (err < 0) {
		k_sem_give(&chan->tx_sem);
	}

	return err;
}

static void att_process(struct bt_att *att)
{
	sys_snode_t *node;
	struct bt_att_chan *chan, *tmp;

	/* Pull next request from the list */
	node = sys_slist_get(&att->reqs);
	if (!node) {
		return;
	}

	BT_DBG("req %p", ATT_REQ(node));

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		/* If there is nothing pending use the channel */
		if (!chan->req) {
			if (bt_att_chan_req_send(chan, ATT_REQ(node)) >= 0) {
				return;
			}
		}
	}

	/* Prepend back to the list as it could not be sent */
	sys_slist_prepend(&att->reqs, node);
}

static uint8_t att_handle_rsp(struct bt_att_chan *chan, void *pdu, uint16_t len,
			   uint8_t err)
{
	bt_att_func_t func = NULL;
	void *params;

	BT_DBG("chan %p err 0x%02x len %u: %s", chan, err, len,
	       bt_hex(pdu, len));

	/* Cancel timeout if ongoing */
	k_delayed_work_cancel(&chan->timeout_work);

	if (!chan->req) {
		BT_WARN("No pending ATT request");
		goto process;
	}

	/* Check if request has been cancelled */
	if (chan->req == &cancel) {
		chan->req = NULL;
		goto process;
	}

	/* Release original buffer */
	if (chan->req->buf) {
		net_buf_unref(chan->req->buf);
		chan->req->buf = NULL;
	}

	/* Reset func so it can be reused by the callback */
	func = chan->req->func;
	chan->req->func = NULL;
	params = chan->req->user_data;

	/* free allocated request so its memory can be reused */
	att_req_destroy(chan->req);
	chan->req = NULL;

process:
	/* Process pending requests */
	att_process(chan->att);
	if (func) {
		func(chan->att->conn, err, pdu, len, params);
	}

	return 0;
}

#if defined(CONFIG_BT_GATT_CLIENT)
static uint8_t att_mtu_rsp(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_exchange_mtu_rsp *rsp;
	uint16_t mtu;

	rsp = (void *)buf->data;

	mtu = sys_le16_to_cpu(rsp->mtu);

	BT_DBG("Server MTU %u", mtu);

	/* Check if MTU is valid */
	if (mtu < BT_ATT_DEFAULT_LE_MTU) {
		return att_handle_rsp(chan, NULL, 0, BT_ATT_ERR_INVALID_PDU);
	}

	chan->chan.rx.mtu = MIN(mtu, BT_ATT_MTU);

	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part F] page 484:
	 *
	 * A device's Exchange MTU Request shall contain the same MTU as the
	 * device's Exchange MTU Response (i.e. the MTU shall be symmetric).
	 */
	chan->chan.tx.mtu = chan->chan.rx.mtu;

	BT_DBG("Negotiated MTU %u", chan->chan.rx.mtu);

	return att_handle_rsp(chan, rsp, buf->len, 0);
}
#endif /* CONFIG_BT_GATT_CLIENT */

static bool range_is_valid(uint16_t start, uint16_t end, uint16_t *err)
{
	/* Handle 0 is invalid */
	if (!start || !end) {
		if (err) {
			*err = 0U;
		}
		return false;
	}

	/* Check if range is valid */
	if (start > end) {
		if (err) {
			*err = start;
		}
		return false;
	}

	return true;
}

struct find_info_data {
	struct bt_att_chan *chan;
	struct net_buf *buf;
	struct bt_att_find_info_rsp *rsp;
	union {
		struct bt_att_info_16 *info16;
		struct bt_att_info_128 *info128;
	};
};

static uint8_t find_info_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct find_info_data *data = user_data;
	struct bt_att_chan *chan = data->chan;

	BT_DBG("handle 0x%04x", attr->handle);

	/* Initialize rsp at first entry */
	if (!data->rsp) {
		data->rsp = net_buf_add(data->buf, sizeof(*data->rsp));
		data->rsp->format = (attr->uuid->type == BT_UUID_TYPE_16) ?
				    BT_ATT_INFO_16 : BT_ATT_INFO_128;
	}

	switch (data->rsp->format) {
	case BT_ATT_INFO_16:
		if (attr->uuid->type != BT_UUID_TYPE_16) {
			return BT_GATT_ITER_STOP;
		}

		/* Fast forward to next item position */
		data->info16 = net_buf_add(data->buf, sizeof(*data->info16));
		data->info16->handle = sys_cpu_to_le16(attr->handle);
		data->info16->uuid = sys_cpu_to_le16(BT_UUID_16(attr->uuid)->val);

		if (chan->chan.tx.mtu - data->buf->len >
		    sizeof(*data->info16)) {
			return BT_GATT_ITER_CONTINUE;
		}

		break;
	case BT_ATT_INFO_128:
		if (attr->uuid->type != BT_UUID_TYPE_128) {
			return BT_GATT_ITER_STOP;
		}

		/* Fast forward to next item position */
		data->info128 = net_buf_add(data->buf, sizeof(*data->info128));
		data->info128->handle = sys_cpu_to_le16(attr->handle);
		memcpy(data->info128->uuid, BT_UUID_128(attr->uuid)->val,
		       sizeof(data->info128->uuid));

		if (chan->chan.tx.mtu - data->buf->len >
		    sizeof(*data->info128)) {
			return BT_GATT_ITER_CONTINUE;
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t att_find_info_rsp(struct bt_att_chan *chan, uint16_t start_handle,
			      uint16_t end_handle)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct find_info_data data;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_INFO_RSP, 0);
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.chan = chan;
	bt_gatt_foreach_attr(start_handle, end_handle, find_info_cb, &data);

	if (!data.rsp) {
		net_buf_unref(data.buf);
		/* Respond since handle is set */
		send_err_rsp(chan, BT_ATT_OP_FIND_INFO_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return 0;
	}

	bt_att_chan_send_rsp(chan, data.buf, chan_rsp_sent);

	return 0;
}

static uint8_t att_find_info_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_find_info_req *req;
	uint16_t start_handle, end_handle, err_handle;

	req = (void *)buf->data;

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);

	BT_DBG("start_handle 0x%04x end_handle 0x%04x", start_handle,
	       end_handle);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(chan, BT_ATT_OP_FIND_INFO_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	return att_find_info_rsp(chan, start_handle, end_handle);
}

struct find_type_data {
	struct bt_att_chan *chan;
	struct net_buf *buf;
	struct bt_att_handle_group *group;
	const void *value;
	uint8_t value_len;
	uint8_t err;
};

static uint8_t find_type_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct find_type_data *data = user_data;
	struct bt_att_chan *chan = data->chan;
	struct bt_conn *conn = chan->chan.chan.conn;
	int read;
	uint8_t uuid[16];
	struct net_buf *frag;
	size_t len;

	/* Skip secondary services */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
		goto skip;
	}

	/* Update group end_handle if not a primary service */
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY)) {
		if (data->group &&
		    attr->handle > sys_le16_to_cpu(data->group->end_handle)) {
			data->group->end_handle = sys_cpu_to_le16(attr->handle);
		}
		return BT_GATT_ITER_CONTINUE;
	}

	BT_DBG("handle 0x%04x", attr->handle);

	/* stop if there is no space left */
	if (chan->chan.tx.mtu - net_buf_frags_len(data->buf) <
	    sizeof(*data->group)) {
		return BT_GATT_ITER_STOP;
	}

	frag = net_buf_frag_last(data->buf);

	len = MIN(chan->chan.tx.mtu - net_buf_frags_len(data->buf),
		  net_buf_tailroom(frag));
	if (!len) {
		frag = net_buf_alloc(net_buf_pool_get(data->buf->pool_id),
				     K_NO_WAIT);
		/* If not buffer can be allocated immediately stop */
		if (!frag) {
			return BT_GATT_ITER_STOP;
		}

		net_buf_frag_add(data->buf, frag);
	}

	/* Read attribute value and store in the buffer */
	read = attr->read(conn, attr, uuid, sizeof(uuid), 0);
	if (read < 0) {
		/*
		 * Since we don't know if it is the service with requested UUID,
		 * we cannot respond with an error to this request.
		 */
		goto skip;
	}

	/* Check if data matches */
	if (read != data->value_len) {
		/* Use bt_uuid_cmp() to compare UUIDs of different form. */
		struct bt_uuid_128 ref_uuid;
		struct bt_uuid_128 recvd_uuid;

		if (!bt_uuid_create(&recvd_uuid.uuid, data->value, data->value_len)) {
			BT_WARN("Unable to create UUID: size %u", data->value_len);
			goto skip;
		}
		if (!bt_uuid_create(&ref_uuid.uuid, uuid, read)) {
			BT_WARN("Unable to create UUID: size %d", read);
			goto skip;
		}
		if (bt_uuid_cmp(&recvd_uuid.uuid, &ref_uuid.uuid)) {
			goto skip;
		}
	} else if (memcmp(data->value, uuid, read)) {
		goto skip;
	}

	/* If service has been found, error should be cleared */
	data->err = 0x00;

	/* Fast forward to next item position */
	data->group = net_buf_add(frag, sizeof(*data->group));
	data->group->start_handle = sys_cpu_to_le16(attr->handle);
	data->group->end_handle = sys_cpu_to_le16(attr->handle);

	/* continue to find the end_handle */
	return BT_GATT_ITER_CONTINUE;

skip:
	data->group = NULL;
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_find_type_rsp(struct bt_att_chan *chan, uint16_t start_handle,
			      uint16_t end_handle, const void *value,
			      uint8_t value_len)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct find_type_data data;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_TYPE_RSP, 0);
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.chan = chan;
	data.group = NULL;
	data.value = value;
	data.value_len = value_len;

	/* Pre-set error in case no service will be found */
	data.err = BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;

	bt_gatt_foreach_attr(start_handle, end_handle, find_type_cb, &data);

	/* If error has not been cleared, no service has been found */
	if (data.err) {
		net_buf_unref(data.buf);
		/* Respond since handle is set */
		send_err_rsp(chan, BT_ATT_OP_FIND_TYPE_REQ, start_handle,
			     data.err);
		return 0;
	}

	bt_att_chan_send_rsp(chan, data.buf, chan_rsp_sent);

	return 0;
}

static uint8_t att_find_type_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_find_type_req *req;
	uint16_t start_handle, end_handle, err_handle, type;
	uint8_t *value;

	req = net_buf_pull_mem(buf, sizeof(*req));

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);
	type = sys_le16_to_cpu(req->type);
	value = buf->data;

	BT_DBG("start_handle 0x%04x end_handle 0x%04x type %u", start_handle,
	       end_handle, type);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(chan, BT_ATT_OP_FIND_TYPE_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	/* The Attribute Protocol Find By Type Value Request shall be used with
	 * the Attribute Type parameter set to the UUID for "Primary Service"
	 * and the Attribute Value set to the 16-bit Bluetooth UUID or 128-bit
	 * UUID for the specific primary service.
	 */
	if (bt_uuid_cmp(BT_UUID_DECLARE_16(type), BT_UUID_GATT_PRIMARY)) {
		send_err_rsp(chan, BT_ATT_OP_FIND_TYPE_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return 0;
	}

	return att_find_type_rsp(chan, start_handle, end_handle, value,
				 buf->len);
}

static uint8_t err_to_att(int err)
{
	BT_DBG("%d", err);

	if (err < 0 && err >= -0xff) {
		return -err;
	}

	return BT_ATT_ERR_UNLIKELY;
}

struct read_type_data {
	struct bt_att_chan *chan;
	struct bt_uuid *uuid;
	struct net_buf *buf;
	struct bt_att_read_type_rsp *rsp;
	struct bt_att_data *item;
	uint8_t err;
};

typedef bool (*attr_read_cb)(struct net_buf *buf, ssize_t read,
			     void *user_data);

static bool attr_read_type_cb(struct net_buf *frag, ssize_t read,
			      void *user_data)
{
	struct read_type_data *data = user_data;

	if (!data->rsp->len) {
		/* Set len to be the first item found */
		data->rsp->len = read + sizeof(*data->item);
	} else if (data->rsp->len != read + sizeof(*data->item)) {
		/* All items should have the same size */
		frag->len -= sizeof(*data->item);
		data->item = NULL;
		return false;
	}

	return true;
}

static ssize_t att_chan_read(struct bt_att_chan *chan,
			     const struct bt_gatt_attr *attr,
			     struct net_buf *buf, uint16_t offset,
			     attr_read_cb cb, void *user_data)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	ssize_t read;
	struct net_buf *frag;
	size_t len, total = 0;

	if (chan->chan.tx.mtu <= net_buf_frags_len(buf)) {
		return 0;
	}

	frag = net_buf_frag_last(buf);

	/* Create necessary fragments if MTU is bigger than what a buffer can
	 * hold.
	 */
	do {
		len = MIN(chan->chan.tx.mtu - net_buf_frags_len(buf),
			  net_buf_tailroom(frag));
		if (!len) {
			frag = net_buf_alloc(net_buf_pool_get(buf->pool_id),
					     K_NO_WAIT);
			/* If not buffer can be allocated immediately return */
			if (!frag) {
				return total;
			}

			net_buf_frag_add(buf, frag);

			len = MIN(chan->chan.tx.mtu - net_buf_frags_len(buf),
				  net_buf_tailroom(frag));
		}

		read = attr->read(conn, attr, frag->data + frag->len, len,
				  offset);
		if (read < 0) {
			if (total) {
				return total;
			}

			return read;
		}

		if (cb && !cb(frag, read, user_data)) {
			break;
		}

		net_buf_add(frag, read);
		total += read;
		offset += read;
	} while (chan->chan.tx.mtu > net_buf_frags_len(buf) && read == len);

	return total;
}

static uint8_t read_type_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_type_data *data = user_data;
	struct bt_att_chan *chan = data->chan;
	struct bt_conn *conn = chan->chan.chan.conn;
	ssize_t read;

	/* Skip if doesn't match */
	if (bt_uuid_cmp(attr->uuid, data->uuid)) {
		return BT_GATT_ITER_CONTINUE;
	}

	BT_DBG("handle 0x%04x", attr->handle);

	/*
	 * If an attribute in the set of requested attributes would cause an
	 * Error Response then this attribute cannot be included in a
	 * Read By Type Response and the attributes before this attribute
	 * shall be returned
	 *
	 * If the first attribute in the set of requested attributes would
	 * cause an Error Response then no other attributes in the requested
	 * attributes can be considered.
	 */
	data->err = bt_gatt_check_perm(conn, attr, BT_GATT_PERM_READ_MASK);
	if (data->err) {
		if (data->rsp->len) {
			data->err = 0x00;
		}
		return BT_GATT_ITER_STOP;
	}

	/*
	 * If any attribute is founded in handle range it means that error
	 * should be changed from pre-set: attr not found error to no error.
	 */
	data->err = 0x00;

	/* Fast foward to next item position */
	data->item = net_buf_add(net_buf_frag_last(data->buf),
				 sizeof(*data->item));
	data->item->handle = sys_cpu_to_le16(attr->handle);

	read = att_chan_read(chan, attr, data->buf, 0, attr_read_type_cb, data);
	if (read < 0) {
		data->err = err_to_att(read);
		return BT_GATT_ITER_STOP;
	}

	if (!data->item) {
		return BT_GATT_ITER_STOP;
	}

	/* continue only if there are still space for more items */
	return chan->chan.tx.mtu - net_buf_frags_len(data->buf) >
	       data->rsp->len ? BT_GATT_ITER_CONTINUE : BT_GATT_ITER_STOP;
}

static uint8_t att_read_type_rsp(struct bt_att_chan *chan, struct bt_uuid *uuid,
			      uint16_t start_handle, uint16_t end_handle)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct read_type_data data;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_TYPE_RSP,
				     sizeof(*data.rsp));
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.chan = chan;
	data.uuid = uuid;
	data.rsp = net_buf_add(data.buf, sizeof(*data.rsp));
	data.rsp->len = 0U;

	/* Pre-set error if no attr will be found in handle */
	data.err = BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;

	bt_gatt_foreach_attr(start_handle, end_handle, read_type_cb, &data);

	if (data.err) {
		net_buf_unref(data.buf);
		/* Response here since handle is set */
		send_err_rsp(chan, BT_ATT_OP_READ_TYPE_REQ, start_handle,
			     data.err);
		return 0;
	}

	bt_att_chan_send_rsp(chan, data.buf, chan_rsp_sent);

	return 0;
}

static uint8_t att_read_type_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_type_req *req;
	uint16_t start_handle, end_handle, err_handle;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;
	uint8_t uuid_len = buf->len - sizeof(*req);

	/* Type can only be UUID16 or UUID128 */
	if (uuid_len != 2 && uuid_len != 16) {
		return BT_ATT_ERR_INVALID_PDU;
	}

	req = net_buf_pull_mem(buf, sizeof(*req));

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);
	if (!bt_uuid_create(&u.uuid, req->uuid, uuid_len)) {
		return BT_ATT_ERR_UNLIKELY;
	}

	BT_DBG("start_handle 0x%04x end_handle 0x%04x type %s",
	       start_handle, end_handle, bt_uuid_str(&u.uuid));

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(chan, BT_ATT_OP_READ_TYPE_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	return att_read_type_rsp(chan, &u.uuid, start_handle, end_handle);
}

struct read_data {
	struct bt_att_chan *chan;
	uint16_t offset;
	struct net_buf *buf;
	struct bt_att_read_rsp *rsp;
	uint8_t err;
};

static uint8_t read_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_data *data = user_data;
	struct bt_att_chan *chan = data->chan;
	struct bt_conn *conn = chan->chan.chan.conn;
	int ret;

	BT_DBG("handle 0x%04x", attr->handle);

	data->rsp = net_buf_add(data->buf, sizeof(*data->rsp));

	/*
	 * If any attribute is founded in handle range it means that error
	 * should be changed from pre-set: invalid handle error to no error.
	 */
	data->err = 0x00;

	/* Check attribute permissions */
	data->err = bt_gatt_check_perm(conn, attr, BT_GATT_PERM_READ_MASK);
	if (data->err) {
		return BT_GATT_ITER_STOP;
	}

	/* Read attribute value and store in the buffer */
	ret = att_chan_read(chan, attr, data->buf, data->offset, NULL, NULL);
	if (ret < 0) {
		data->err = err_to_att(ret);
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_read_rsp(struct bt_att_chan *chan, uint8_t op, uint8_t rsp,
			 uint16_t handle, uint16_t offset)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct read_data data;

	if (!bt_gatt_change_aware(conn, true)) {
		return BT_ATT_ERR_DB_OUT_OF_SYNC;
	}

	if (!handle) {
		return BT_ATT_ERR_INVALID_HANDLE;
	}

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, rsp, 0);
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.chan = chan;
	data.offset = offset;

	/* Pre-set error if no attr will be found in handle */
	data.err = BT_ATT_ERR_INVALID_HANDLE;

	bt_gatt_foreach_attr(handle, handle, read_cb, &data);

	/* In case of error discard data and respond with an error */
	if (data.err) {
		net_buf_unref(data.buf);
		/* Respond here since handle is set */
		send_err_rsp(chan, op, handle, data.err);
		return 0;
	}

	bt_att_chan_send_rsp(chan, data.buf, chan_rsp_sent);

	return 0;
}

static uint8_t att_read_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_req *req;
	uint16_t handle;

	req = (void *)buf->data;

	handle = sys_le16_to_cpu(req->handle);

	BT_DBG("handle 0x%04x", handle);

	return att_read_rsp(chan, BT_ATT_OP_READ_REQ, BT_ATT_OP_READ_RSP,
			    handle, 0);
}

static uint8_t att_read_blob_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_blob_req *req;
	uint16_t handle, offset;

	req = (void *)buf->data;

	handle = sys_le16_to_cpu(req->handle);
	offset = sys_le16_to_cpu(req->offset);

	BT_DBG("handle 0x%04x offset %u", handle, offset);

	return att_read_rsp(chan, BT_ATT_OP_READ_BLOB_REQ,
			    BT_ATT_OP_READ_BLOB_RSP, handle, offset);
}

#if defined(CONFIG_BT_GATT_READ_MULTIPLE)
static uint8_t att_read_mult_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct read_data data;
	uint16_t handle;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_MULT_RSP, 0);
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.chan = chan;

	while (buf->len >= sizeof(uint16_t)) {
		handle = net_buf_pull_le16(buf);

		BT_DBG("handle 0x%04x ", handle);

		/* An Error Response shall be sent by the server in response to
		 * the Read Multiple Request [....] if a read operation is not
		 * permitted on any of the Characteristic Values.
		 *
		 * If handle is not valid then return invalid handle error.
		 * If handle is found error will be cleared by read_cb.
		 */
		data.err = BT_ATT_ERR_INVALID_HANDLE;

		bt_gatt_foreach_attr(handle, handle, read_cb, &data);

		/* Stop reading in case of error */
		if (data.err) {
			net_buf_unref(data.buf);
			/* Respond here since handle is set */
			send_err_rsp(chan, BT_ATT_OP_READ_MULT_REQ, handle,
				     data.err);
			return 0;
		}
	}

	bt_att_chan_send_rsp(chan, data.buf, chan_rsp_sent);

	return 0;
}

#if defined(CONFIG_BT_EATT)
static uint8_t read_vl_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_data *data = user_data;
	struct bt_att_chan *chan = data->chan;
	struct bt_conn *conn = chan->chan.chan.conn;
	struct bt_att_read_mult_vl_rsp *rsp;
	int read;

	BT_DBG("handle 0x%04x", attr->handle);

	data->rsp = net_buf_add(data->buf, sizeof(*data->rsp));

	/*
	 * If any attribute is founded in handle range it means that error
	 * should be changed from pre-set: invalid handle error to no error.
	 */
	data->err = 0x00;

	/* Check attribute permissions */
	data->err = bt_gatt_check_perm(conn, attr, BT_GATT_PERM_READ_MASK);
	if (data->err) {
		return BT_GATT_ITER_STOP;
	}

	/* The Length Value Tuple List may be truncated within the first two
	 * octets of a tuple due to the size limits of the current ATT_MTU.
	 */
	if (chan->chan.tx.mtu - data->buf->len < 2) {
		return BT_GATT_ITER_STOP;
	}

	rsp = net_buf_add(data->buf, sizeof(*rsp));

	read = att_chan_read(chan, attr, data->buf, data->offset, NULL, NULL);
	if (read < 0) {
		data->err = err_to_att(read);
		return BT_GATT_ITER_STOP;
	}

	rsp->len = read;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_read_mult_vl_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct read_data data;
	uint16_t handle;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_MULT_VL_RSP, 0);
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.chan = chan;

	while (buf->len >= sizeof(uint16_t)) {
		handle = net_buf_pull_le16(buf);

		BT_DBG("handle 0x%04x ", handle);

		/* If handle is not valid then return invalid handle error.
		 * If handle is found error will be cleared by read_cb.
		 */
		data.err = BT_ATT_ERR_INVALID_HANDLE;

		bt_gatt_foreach_attr(handle, handle, read_vl_cb, &data);

		/* Stop reading in case of error */
		if (data.err) {
			net_buf_unref(data.buf);
			/* Respond here since handle is set */
			send_err_rsp(chan, BT_ATT_OP_READ_MULT_VL_REQ, handle,
				     data.err);
			return 0;
		}
	}

	bt_att_chan_send_rsp(chan, data.buf, chan_rsp_sent);

	return 0;
}
#endif /* CONFIG_BT_EATT */
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */

struct read_group_data {
	struct bt_att_chan *chan;
	struct bt_uuid *uuid;
	struct net_buf *buf;
	struct bt_att_read_group_rsp *rsp;
	struct bt_att_group_data *group;
};

static bool attr_read_group_cb(struct net_buf *frag, ssize_t read,
			       void *user_data)
{
	struct read_group_data *data = user_data;

	if (!data->rsp->len) {
		/* Set len to be the first group found */
		data->rsp->len = read + sizeof(*data->group);
	} else if (data->rsp->len != read + sizeof(*data->group)) {
		/* All groups entries should have the same size */
		data->buf->len -= sizeof(*data->group);
		data->group = NULL;
		return false;
	}

	return true;
}

static uint8_t read_group_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_group_data *data = user_data;
	struct bt_att_chan *chan = data->chan;
	int read;

	/* Update group end_handle if attribute is not a service */
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) &&
	    bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
		if (data->group &&
		    attr->handle > sys_le16_to_cpu(data->group->end_handle)) {
			data->group->end_handle = sys_cpu_to_le16(attr->handle);
		}
		return BT_GATT_ITER_CONTINUE;
	}

	/* If Group Type don't match skip */
	if (bt_uuid_cmp(attr->uuid, data->uuid)) {
		data->group = NULL;
		return BT_GATT_ITER_CONTINUE;
	}

	BT_DBG("handle 0x%04x", attr->handle);

	/* Stop if there is no space left */
	if (data->rsp->len &&
	    chan->chan.tx.mtu - data->buf->len < data->rsp->len) {
		return BT_GATT_ITER_STOP;
	}

	/* Fast forward to next group position */
	data->group = net_buf_add(data->buf, sizeof(*data->group));

	/* Initialize group handle range */
	data->group->start_handle = sys_cpu_to_le16(attr->handle);
	data->group->end_handle = sys_cpu_to_le16(attr->handle);

	/* Read attribute value and store in the buffer */
	read = att_chan_read(chan, attr, data->buf, 0, attr_read_group_cb,
			     data);
	if (read < 0) {
		/* TODO: Handle read errors */
		return BT_GATT_ITER_STOP;
	}

	if (!data->group) {
		return BT_GATT_ITER_STOP;
	}

	/* continue only if there are still space for more items */
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_read_group_rsp(struct bt_att_chan *chan, struct bt_uuid *uuid,
			       uint16_t start_handle, uint16_t end_handle)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct read_group_data data;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_GROUP_RSP,
				     sizeof(*data.rsp));
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.chan = chan;
	data.uuid = uuid;
	data.rsp = net_buf_add(data.buf, sizeof(*data.rsp));
	data.rsp->len = 0U;
	data.group = NULL;

	bt_gatt_foreach_attr(start_handle, end_handle, read_group_cb, &data);

	if (!data.rsp->len) {
		net_buf_unref(data.buf);
		/* Respond here since handle is set */
		send_err_rsp(chan, BT_ATT_OP_READ_GROUP_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return 0;
	}

	bt_att_chan_send_rsp(chan, data.buf, chan_rsp_sent);

	return 0;
}

static uint8_t att_read_group_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_read_group_req *req;
	uint16_t start_handle, end_handle, err_handle;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;
	uint8_t uuid_len = buf->len - sizeof(*req);

	/* Type can only be UUID16 or UUID128 */
	if (uuid_len != 2 && uuid_len != 16) {
		return BT_ATT_ERR_INVALID_PDU;
	}

	req = net_buf_pull_mem(buf, sizeof(*req));

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);

	if (!bt_uuid_create(&u.uuid, req->uuid, uuid_len)) {
		return BT_ATT_ERR_UNLIKELY;
	}

	BT_DBG("start_handle 0x%04x end_handle 0x%04x type %s",
	       start_handle, end_handle, bt_uuid_str(&u.uuid));

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(chan, BT_ATT_OP_READ_GROUP_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	/* Core v4.2, Vol 3, sec 2.5.3 Attribute Grouping:
	 * Not all of the grouping attributes can be used in the ATT
	 * Read By Group Type Request. The "Primary Service" and "Secondary
	 * Service" grouping types may be used in the Read By Group Type
	 * Request. The "Characteristic" grouping type shall not be used in
	 * the ATT Read By Group Type Request.
	 */
	if (bt_uuid_cmp(&u.uuid, BT_UUID_GATT_PRIMARY) &&
	    bt_uuid_cmp(&u.uuid, BT_UUID_GATT_SECONDARY)) {
		send_err_rsp(chan, BT_ATT_OP_READ_GROUP_REQ, start_handle,
			     BT_ATT_ERR_UNSUPPORTED_GROUP_TYPE);
		return 0;
	}

	return att_read_group_rsp(chan, &u.uuid, start_handle, end_handle);
}

struct write_data {
	struct bt_conn *conn;
	struct net_buf *buf;
	uint8_t req;
	const void *value;
	uint16_t len;
	uint16_t offset;
	uint8_t err;
};

static uint8_t write_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct write_data *data = user_data;
	int write;
	uint8_t flags = 0U;

	BT_DBG("handle 0x%04x offset %u", attr->handle, data->offset);

	/* Check attribute permissions */
	data->err = bt_gatt_check_perm(data->conn, attr,
				       BT_GATT_PERM_WRITE_MASK);
	if (data->err) {
		return BT_GATT_ITER_STOP;
	}

	/* Set command flag if not a request */
	if (!data->req) {
		flags |= BT_GATT_WRITE_FLAG_CMD;
	}

	/* Write attribute value */
	write = attr->write(data->conn, attr, data->value, data->len,
			    data->offset, flags);
	if (write < 0 || write != data->len) {
		data->err = err_to_att(write);
		return BT_GATT_ITER_STOP;
	}

	data->err = 0U;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_write_rsp(struct bt_att_chan *chan, uint8_t req, uint8_t rsp,
			  uint16_t handle, uint16_t offset, const void *value,
			  uint16_t len)
{
	struct write_data data;

	if (!bt_gatt_change_aware(chan->att->conn, req ? true : false)) {
		return BT_ATT_ERR_DB_OUT_OF_SYNC;
	}

	if (!handle) {
		return BT_ATT_ERR_INVALID_HANDLE;
	}

	(void)memset(&data, 0, sizeof(data));

	/* Only allocate buf if required to respond */
	if (rsp) {
		data.buf = bt_att_chan_create_pdu(chan, rsp, 0);
		if (!data.buf) {
			return BT_ATT_ERR_UNLIKELY;
		}
	}

	data.conn = chan->att->conn;
	data.req = req;
	data.offset = offset;
	data.value = value;
	data.len = len;
	data.err = BT_ATT_ERR_INVALID_HANDLE;

	bt_gatt_foreach_attr(handle, handle, write_cb, &data);

	if (data.err) {
		/* In case of error discard data and respond with an error */
		if (rsp) {
			net_buf_unref(data.buf);
			/* Respond here since handle is set */
			send_err_rsp(chan, req, handle, data.err);
		}
		return req == BT_ATT_OP_EXEC_WRITE_REQ ? data.err : 0;
	}

	if (data.buf) {
		bt_att_chan_send_rsp(chan, data.buf, chan_rsp_sent);
	}

	return 0;
}

static uint8_t att_write_req(struct bt_att_chan *chan, struct net_buf *buf)
{
	uint16_t handle;

	handle = net_buf_pull_le16(buf);

	BT_DBG("handle 0x%04x", handle);

	return att_write_rsp(chan, BT_ATT_OP_WRITE_REQ, BT_ATT_OP_WRITE_RSP,
			     handle, 0, buf->data, buf->len);
}

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
struct prep_data {
	struct bt_conn *conn;
	struct net_buf *buf;
	const void *value;
	uint16_t len;
	uint16_t offset;
	uint8_t err;
};

static uint8_t prep_write_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct prep_data *data = user_data;
	struct bt_attr_data *attr_data;
	int write;

	BT_DBG("handle 0x%04x offset %u", attr->handle, data->offset);

	/* Check attribute permissions */
	data->err = bt_gatt_check_perm(data->conn, attr,
				       BT_GATT_PERM_WRITE_MASK);
	if (data->err) {
		return BT_GATT_ITER_STOP;
	}

	/* Check if attribute requires handler to accept the data */
	if (!(attr->perm & BT_GATT_PERM_PREPARE_WRITE)) {
		goto append;
	}

	/* Write attribute value to check if device is authorized */
	write = attr->write(data->conn, attr, data->value, data->len,
			    data->offset, BT_GATT_WRITE_FLAG_PREPARE);
	if (write != 0) {
		data->err = err_to_att(write);
		return BT_GATT_ITER_STOP;
	}

append:
	/* Copy data into the outstanding queue */
	data->buf = net_buf_alloc(&prep_pool, K_NO_WAIT);
	if (!data->buf) {
		data->err = BT_ATT_ERR_PREPARE_QUEUE_FULL;
		return BT_GATT_ITER_STOP;
	}

	attr_data = net_buf_user_data(data->buf);
	attr_data->handle = attr->handle;
	attr_data->offset = data->offset;

	net_buf_add_mem(data->buf, data->value, data->len);

	data->err = 0U;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t att_prep_write_rsp(struct bt_att_chan *chan, uint16_t handle,
			       uint16_t offset, const void *value, uint8_t len)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct prep_data data;
	struct bt_att_prepare_write_rsp *rsp;

	if (!bt_gatt_change_aware(conn, true)) {
		return BT_ATT_ERR_DB_OUT_OF_SYNC;
	}

	if (!handle) {
		return BT_ATT_ERR_INVALID_HANDLE;
	}

	(void)memset(&data, 0, sizeof(data));

	data.conn = conn;
	data.offset = offset;
	data.value = value;
	data.len = len;
	data.err = BT_ATT_ERR_INVALID_HANDLE;

	bt_gatt_foreach_attr(handle, handle, prep_write_cb, &data);

	if (data.err) {
		/* Respond here since handle is set */
		send_err_rsp(chan, BT_ATT_OP_PREPARE_WRITE_REQ, handle,
			     data.err);
		return 0;
	}

	BT_DBG("buf %p handle 0x%04x offset %u", data.buf, handle, offset);

	/* Store buffer in the outstanding queue */
	net_buf_put(&chan->att->prep_queue, data.buf);

	/* Generate response */
	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_PREPARE_WRITE_RSP, 0);
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	rsp = net_buf_add(data.buf, sizeof(*rsp));
	rsp->handle = sys_cpu_to_le16(handle);
	rsp->offset = sys_cpu_to_le16(offset);
	net_buf_add(data.buf, len);
	memcpy(rsp->value, value, len);

	bt_att_chan_send_rsp(chan, data.buf, chan_rsp_sent);

	return 0;
}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */

static uint8_t att_prepare_write_req(struct bt_att_chan *chan, struct net_buf *buf)
{
#if CONFIG_BT_ATT_PREPARE_COUNT == 0
	return BT_ATT_ERR_NOT_SUPPORTED;
#else
	struct bt_att_prepare_write_req *req;
	uint16_t handle, offset;

	req = net_buf_pull_mem(buf, sizeof(*req));

	handle = sys_le16_to_cpu(req->handle);
	offset = sys_le16_to_cpu(req->offset);

	BT_DBG("handle 0x%04x offset %u", handle, offset);

	return att_prep_write_rsp(chan, handle, offset, buf->data, buf->len);
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */
}

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
static uint8_t att_exec_write_rsp(struct bt_att_chan *chan, uint8_t flags)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct net_buf *buf;
	uint8_t err = 0U;

	while ((buf = net_buf_get(&chan->att->prep_queue, K_NO_WAIT))) {
		struct bt_attr_data *data = net_buf_user_data(buf);

		BT_DBG("buf %p handle 0x%04x offset %u", buf, data->handle,
		       data->offset);

		/* Just discard the data if an error was set */
		if (!err && flags == BT_ATT_FLAG_EXEC) {
			err = att_write_rsp(chan, BT_ATT_OP_EXEC_WRITE_REQ, 0,
					    data->handle, data->offset,
					    buf->data, buf->len);
			if (err) {
				/* Respond here since handle is set */
				send_err_rsp(chan, BT_ATT_OP_EXEC_WRITE_REQ,
					     data->handle, err);
			}
		}

		net_buf_unref(buf);
	}

	if (err) {
		return 0;
	}

	/* Generate response */
	buf = bt_att_create_pdu(conn, BT_ATT_OP_EXEC_WRITE_RSP, 0);
	if (!buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	bt_att_chan_send_rsp(chan, buf, chan_rsp_sent);

	return 0;
}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */


static uint8_t att_exec_write_req(struct bt_att_chan *chan, struct net_buf *buf)
{
#if CONFIG_BT_ATT_PREPARE_COUNT == 0
	return BT_ATT_ERR_NOT_SUPPORTED;
#else
	struct bt_att_exec_write_req *req;

	req = (void *)buf->data;

	BT_DBG("flags 0x%02x", req->flags);

	return att_exec_write_rsp(chan, req->flags);
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */
}

static uint8_t att_write_cmd(struct bt_att_chan *chan, struct net_buf *buf)
{
	uint16_t handle;

	handle = net_buf_pull_le16(buf);

	BT_DBG("handle 0x%04x", handle);

	return att_write_rsp(chan, 0, 0, handle, 0, buf->data, buf->len);
}

#if defined(CONFIG_BT_SIGNING)
static uint8_t att_signed_write_cmd(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_conn *conn = chan->chan.chan.conn;
	struct bt_att_signed_write_cmd *req;
	uint16_t handle;
	int err;

	req = (void *)buf->data;

	handle = sys_le16_to_cpu(req->handle);

	BT_DBG("handle 0x%04x", handle);

	/* Verifying data requires full buffer including attribute header */
	net_buf_push(buf, sizeof(struct bt_att_hdr));
	err = bt_smp_sign_verify(conn, buf);
	if (err) {
		BT_ERR("Error verifying data");
		/* No response for this command */
		return 0;
	}

	net_buf_pull(buf, sizeof(struct bt_att_hdr));
	net_buf_pull(buf, sizeof(*req));

	return att_write_rsp(chan, 0, 0, handle, 0, buf->data,
			     buf->len - sizeof(struct bt_att_signature));
}
#endif /* CONFIG_BT_SIGNING */

#if defined(CONFIG_BT_GATT_CLIENT)
#if defined(CONFIG_BT_SMP)
static int att_change_security(struct bt_conn *conn, uint8_t err)
{
	bt_security_t sec;

	switch (err) {
	case BT_ATT_ERR_INSUFFICIENT_ENCRYPTION:
		if (conn->sec_level >= BT_SECURITY_L2)
			return -EALREADY;
		sec = BT_SECURITY_L2;
		break;
	case BT_ATT_ERR_AUTHENTICATION:
		if (conn->sec_level < BT_SECURITY_L2) {
			/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part C]
			 * page 375:
			 *
			 * If an LTK is not available, the service request
			 * shall be rejected with the error code 'Insufficient
			 * Authentication'.
			 * Note: When the link is not encrypted, the error code
			 * "Insufficient Authentication" does not indicate that
			 * MITM protection is required.
			 */
			sec = BT_SECURITY_L2;
		} else if (conn->sec_level < BT_SECURITY_L3) {
			/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part C]
			 * page 375:
			 *
			 * If an authenticated pairing is required but only an
			 * unauthenticated pairing has occurred and the link is
			 * currently encrypted, the service request shall be
			 * rejected with the error code 'Insufficient
			 * Authentication'.
			 * Note: When unauthenticated pairing has occurred and
			 * the link is currently encrypted, the error code
			 * 'Insufficient Authentication' indicates that MITM
			 * protection is required.
			 */
			sec = BT_SECURITY_L3;
		} else if (conn->sec_level < BT_SECURITY_L4) {
			/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part C]
			 * page 375:
			 *
			 * If LE Secure Connections authenticated pairing is
			 * required but LE legacy pairing has occurred and the
			 * link is currently encrypted, the service request
			 * shall be rejected with the error code ''Insufficient
			 * Authentication'.
			 */
			sec = BT_SECURITY_L4;
		} else {
			return -EALREADY;
		}
		break;
	default:
		return -EINVAL;
	}

	return bt_conn_set_security(conn, sec);
}
#endif /* CONFIG_BT_SMP */

static uint8_t att_error_rsp(struct bt_att_chan *chan, struct net_buf *buf)
{
	struct bt_att_error_rsp *rsp;
	uint8_t err;

	rsp = (void *)buf->data;

	BT_DBG("request 0x%02x handle 0x%04x error 0x%02x", rsp->request,
	       sys_le16_to_cpu(rsp->handle), rsp->error);

	/* Don't retry if there is no req pending or it has been cancelled */
	if (!chan->req || chan->req == &cancel) {
		err = BT_ATT_ERR_UNLIKELY;
		goto done;
	}

	if (chan->req->buf) {
		/* Restore state to be resent */
		net_buf_simple_restore(&chan->req->buf->b, &chan->req->state);
	}

	err = rsp->error;
#if defined(CONFIG_BT_SMP)
	if (chan->req->retrying) {
		goto done;
	}

	/* Check if security needs to be changed */
	if (!att_change_security(chan->chan.chan.conn, err)) {
		chan->req->retrying = true;
		/* Wait security_changed: TODO: Handle fail case */
		return 0;
	}
#endif /* CONFIG_BT_SMP */

done:
	return att_handle_rsp(chan, NULL, 0, err);
}

static uint8_t att_handle_find_info_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_find_type_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_read_type_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_read_rsp(struct bt_att_chan *chan,
				struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_read_blob_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

#if defined(CONFIG_BT_GATT_READ_MULTIPLE)
static uint8_t att_handle_read_mult_rsp(struct bt_att_chan *chan,
				     struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

#if defined(CONFIG_BT_EATT)
static uint8_t att_handle_read_mult_vl_rsp(struct bt_att_chan *chan,
					struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}
#endif /* CONFIG_BT_EATT */
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */

static uint8_t att_handle_read_group_rsp(struct bt_att_chan *chan,
				      struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_write_rsp(struct bt_att_chan *chan,
				 struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_prepare_write_rsp(struct bt_att_chan *chan,
					 struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_handle_exec_write_rsp(struct bt_att_chan *chan,
				      struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static uint8_t att_notify(struct bt_att_chan *chan, struct net_buf *buf)
{
	uint16_t handle;

	handle = net_buf_pull_le16(buf);

	BT_DBG("chan %p handle 0x%04x", chan, handle);

	bt_gatt_notification(chan->att->conn, handle, buf->data, buf->len);

	return 0;
}

static uint8_t att_indicate(struct bt_att_chan *chan, struct net_buf *buf)
{
	uint16_t handle;

	handle = net_buf_pull_le16(buf);

	BT_DBG("chan %p handle 0x%04x", chan, handle);

	bt_gatt_notification(chan->att->conn, handle, buf->data, buf->len);

	buf = bt_att_chan_create_pdu(chan, BT_ATT_OP_CONFIRM, 0);
	if (!buf) {
		return 0;
	}

	bt_att_chan_send_rsp(chan, buf, chan_cfm_sent);

	return 0;
}

static uint8_t att_notify_mult(struct bt_att_chan *chan, struct net_buf *buf)
{
	BT_DBG("chan %p", chan);

	bt_gatt_mult_notification(chan->att->conn, buf->data, buf->len);

	return 0;
}
#endif /* CONFIG_BT_GATT_CLIENT */

static uint8_t att_confirm(struct bt_att_chan *chan, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(chan, buf->data, buf->len, 0);
}

static const struct att_handler {
	uint8_t       op;
	uint8_t       expect_len;
	att_type_t type;
	uint8_t       (*func)(struct bt_att_chan *chan, struct net_buf *buf);
} handlers[] = {
	{ BT_ATT_OP_MTU_REQ,
		sizeof(struct bt_att_exchange_mtu_req),
		ATT_REQUEST,
		att_mtu_req },
	{ BT_ATT_OP_FIND_INFO_REQ,
		sizeof(struct bt_att_find_info_req),
		ATT_REQUEST,
		att_find_info_req },
	{ BT_ATT_OP_FIND_TYPE_REQ,
		sizeof(struct bt_att_find_type_req),
		ATT_REQUEST,
		att_find_type_req },
	{ BT_ATT_OP_READ_TYPE_REQ,
		sizeof(struct bt_att_read_type_req),
		ATT_REQUEST,
		att_read_type_req },
	{ BT_ATT_OP_READ_REQ,
		sizeof(struct bt_att_read_req),
		ATT_REQUEST,
		att_read_req },
	{ BT_ATT_OP_READ_BLOB_REQ,
		sizeof(struct bt_att_read_blob_req),
		ATT_REQUEST,
		att_read_blob_req },
#if defined(CONFIG_BT_GATT_READ_MULTIPLE)
	{ BT_ATT_OP_READ_MULT_REQ,
		BT_ATT_READ_MULT_MIN_LEN_REQ,
		ATT_REQUEST,
		att_read_mult_req },
#if defined(CONFIG_BT_EATT)
	{ BT_ATT_OP_READ_MULT_VL_REQ,
		BT_ATT_READ_MULT_MIN_LEN_REQ,
		ATT_REQUEST,
		att_read_mult_vl_req },
#endif /* CONFIG_BT_EATT */
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */
	{ BT_ATT_OP_READ_GROUP_REQ,
		sizeof(struct bt_att_read_group_req),
		ATT_REQUEST,
		att_read_group_req },
	{ BT_ATT_OP_WRITE_REQ,
		sizeof(struct bt_att_write_req),
		ATT_REQUEST,
		att_write_req },
	{ BT_ATT_OP_PREPARE_WRITE_REQ,
		sizeof(struct bt_att_prepare_write_req),
		ATT_REQUEST,
		att_prepare_write_req },
	{ BT_ATT_OP_EXEC_WRITE_REQ,
		sizeof(struct bt_att_exec_write_req),
		ATT_REQUEST,
		att_exec_write_req },
	{ BT_ATT_OP_CONFIRM,
		0,
		ATT_CONFIRMATION,
		att_confirm },
	{ BT_ATT_OP_WRITE_CMD,
		sizeof(struct bt_att_write_cmd),
		ATT_COMMAND,
		att_write_cmd },
#if defined(CONFIG_BT_SIGNING)
	{ BT_ATT_OP_SIGNED_WRITE_CMD,
		(sizeof(struct bt_att_write_cmd) +
		 sizeof(struct bt_att_signature)),
		ATT_COMMAND,
		att_signed_write_cmd },
#endif /* CONFIG_BT_SIGNING */
#if defined(CONFIG_BT_GATT_CLIENT)
	{ BT_ATT_OP_ERROR_RSP,
		sizeof(struct bt_att_error_rsp),
		ATT_RESPONSE,
		att_error_rsp },
	{ BT_ATT_OP_MTU_RSP,
		sizeof(struct bt_att_exchange_mtu_rsp),
		ATT_RESPONSE,
		att_mtu_rsp },
	{ BT_ATT_OP_FIND_INFO_RSP,
		sizeof(struct bt_att_find_info_rsp),
		ATT_RESPONSE,
		att_handle_find_info_rsp },
	{ BT_ATT_OP_FIND_TYPE_RSP,
		sizeof(struct bt_att_find_type_rsp),
		ATT_RESPONSE,
		att_handle_find_type_rsp },
	{ BT_ATT_OP_READ_TYPE_RSP,
		sizeof(struct bt_att_read_type_rsp),
		ATT_RESPONSE,
		att_handle_read_type_rsp },
	{ BT_ATT_OP_READ_RSP,
		sizeof(struct bt_att_read_rsp),
		ATT_RESPONSE,
		att_handle_read_rsp },
	{ BT_ATT_OP_READ_BLOB_RSP,
		sizeof(struct bt_att_read_blob_rsp),
		ATT_RESPONSE,
		att_handle_read_blob_rsp },
#if defined(CONFIG_BT_GATT_READ_MULTIPLE)
	{ BT_ATT_OP_READ_MULT_RSP,
		sizeof(struct bt_att_read_mult_rsp),
		ATT_RESPONSE,
		att_handle_read_mult_rsp },
#if defined(CONFIG_BT_EATT)
	{ BT_ATT_OP_READ_MULT_VL_RSP,
		sizeof(struct bt_att_read_mult_vl_rsp),
		ATT_RESPONSE,
		att_handle_read_mult_vl_rsp },
#endif /* CONFIG_BT_EATT */
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */
	{ BT_ATT_OP_READ_GROUP_RSP,
		sizeof(struct bt_att_read_group_rsp),
		ATT_RESPONSE,
		att_handle_read_group_rsp },
	{ BT_ATT_OP_WRITE_RSP,
		0,
		ATT_RESPONSE,
		att_handle_write_rsp },
	{ BT_ATT_OP_PREPARE_WRITE_RSP,
		sizeof(struct bt_att_prepare_write_rsp),
		ATT_RESPONSE,
		att_handle_prepare_write_rsp },
	{ BT_ATT_OP_EXEC_WRITE_RSP,
		0,
		ATT_RESPONSE,
		att_handle_exec_write_rsp },
	{ BT_ATT_OP_NOTIFY,
		sizeof(struct bt_att_notify),
		ATT_NOTIFICATION,
		att_notify },
	{ BT_ATT_OP_INDICATE,
		sizeof(struct bt_att_indicate),
		ATT_INDICATION,
		att_indicate },
	{ BT_ATT_OP_NOTIFY_MULT,
		sizeof(struct bt_att_notify_mult),
		ATT_NOTIFICATION,
		att_notify_mult },
#endif /* CONFIG_BT_GATT_CLIENT */
};

static att_type_t att_op_get_type(uint8_t op)
{
	switch (op) {
	case BT_ATT_OP_MTU_REQ:
	case BT_ATT_OP_FIND_INFO_REQ:
	case BT_ATT_OP_FIND_TYPE_REQ:
	case BT_ATT_OP_READ_TYPE_REQ:
	case BT_ATT_OP_READ_REQ:
	case BT_ATT_OP_READ_BLOB_REQ:
	case BT_ATT_OP_READ_MULT_REQ:
	case BT_ATT_OP_READ_GROUP_REQ:
	case BT_ATT_OP_WRITE_REQ:
	case BT_ATT_OP_PREPARE_WRITE_REQ:
	case BT_ATT_OP_EXEC_WRITE_REQ:
		return ATT_REQUEST;
	case BT_ATT_OP_CONFIRM:
		return ATT_CONFIRMATION;
	case BT_ATT_OP_WRITE_CMD:
	case BT_ATT_OP_SIGNED_WRITE_CMD:
		return ATT_COMMAND;
	case BT_ATT_OP_ERROR_RSP:
	case BT_ATT_OP_MTU_RSP:
	case BT_ATT_OP_FIND_INFO_RSP:
	case BT_ATT_OP_FIND_TYPE_RSP:
	case BT_ATT_OP_READ_TYPE_RSP:
	case BT_ATT_OP_READ_RSP:
	case BT_ATT_OP_READ_BLOB_RSP:
	case BT_ATT_OP_READ_MULT_RSP:
	case BT_ATT_OP_READ_GROUP_RSP:
	case BT_ATT_OP_WRITE_RSP:
	case BT_ATT_OP_PREPARE_WRITE_RSP:
	case BT_ATT_OP_EXEC_WRITE_RSP:
		return ATT_RESPONSE;
	case BT_ATT_OP_NOTIFY:
		return ATT_NOTIFICATION;
	case BT_ATT_OP_INDICATE:
		return ATT_INDICATION;
	}

	if (op & ATT_CMD_MASK) {
		return ATT_COMMAND;
	}

	return ATT_UNKNOWN;
}

static int bt_att_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_att_chan *att_chan = ATT_CHAN(chan);
	struct bt_att_hdr *hdr;
	const struct att_handler *handler;
	uint8_t err;
	size_t i;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small ATT PDU received");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	BT_DBG("Received ATT chan %p code 0x%02x len %zu", att_chan, hdr->code,
	       net_buf_frags_len(buf));

	for (i = 0, handler = NULL; i < ARRAY_SIZE(handlers); i++) {
		if (hdr->code == handlers[i].op) {
			handler = &handlers[i];
			break;
		}
	}

	if (!handler) {
		BT_WARN("Unhandled ATT code 0x%02x", hdr->code);
		if (att_op_get_type(hdr->code) != ATT_COMMAND) {
			send_err_rsp(att_chan, hdr->code, 0,
				     BT_ATT_ERR_NOT_SUPPORTED);
		}
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_ATT_ENFORCE_FLOW)) {
		if (handler->type == ATT_REQUEST &&
		    atomic_test_and_set_bit(att_chan->flags, ATT_PENDING_RSP)) {
			BT_WARN("Ignoring unexpected request");
			return 0;
		} else if (handler->type == ATT_INDICATION &&
			   atomic_test_and_set_bit(att_chan->flags,
						   ATT_PENDING_CFM)) {
			BT_WARN("Ignoring unexpected indication");
			return 0;
		}
	}

	if (buf->len < handler->expect_len) {
		BT_ERR("Invalid len %u for code 0x%02x", buf->len, hdr->code);
		err = BT_ATT_ERR_INVALID_PDU;
	} else {
		err = handler->func(att_chan, buf);
	}

	if (handler->type == ATT_REQUEST && err) {
		BT_DBG("ATT error 0x%02x", err);
		send_err_rsp(att_chan, hdr->code, 0, err);
	}

	return 0;
}

static struct bt_att *att_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;
	struct bt_att_chan *att_chan;

	if (conn->state != BT_CONN_CONNECTED) {
		BT_WARN("Not connected");
		return NULL;
	}

	chan = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_ATT);
	if (!chan) {
		BT_ERR("Unable to find ATT channel");
		return NULL;
	}

	att_chan = ATT_CHAN(chan);
	if (atomic_test_bit(att_chan->flags, ATT_DISCONNECTED)) {
		BT_WARN("ATT channel flagged as disconnected");
		return NULL;
	}

	return att_chan->att;
}

struct net_buf *bt_att_create_pdu(struct bt_conn *conn, uint8_t op, size_t len)
{
	struct bt_att *att;
	struct bt_att_chan *chan, *tmp;

	att = att_get(conn);
	if (!att) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		if (len + sizeof(op) > chan->chan.tx.mtu) {
			continue;
		}

		return bt_att_chan_create_pdu(chan, op, len);
	}

	BT_WARN("No ATT channel for MTU %zu", len + sizeof(op));

	return NULL;
}

static void att_reset(struct bt_att *att)
{
	struct bt_att_req *req, *tmp;
	struct net_buf *buf;

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
	/* Discard queued buffers */
	while ((buf = net_buf_get(&att->prep_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT > 0 */

	while ((buf = net_buf_get(&att->tx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	att->conn = NULL;

	/* Notify pending requests */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->reqs, req, tmp, node) {
		if (req->func) {
			req->func(NULL, BT_ATT_ERR_UNLIKELY, NULL, 0,
				  req->user_data);
		}

		att_req_destroy(req);
	}

	k_mem_slab_free(&att_slab, (void **)&att);
}

static void att_chan_detach(struct bt_att_chan *chan)
{
	struct net_buf *buf;
	int i;

	BT_DBG("chan %p", chan);

	sys_slist_find_and_remove(&chan->att->chans, &chan->node);

	/* Ensure that any waiters are woken up */
	for (i = 0; i < CONFIG_BT_ATT_TX_MAX; i++) {
		k_sem_give(&chan->tx_sem);
	}

	/* Release pending buffers */
	while ((buf = net_buf_get(&chan->tx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	if (chan->req) {
		/* Notify outstanding request */
		att_handle_rsp(chan, NULL, 0, BT_ATT_ERR_UNLIKELY);
	}

	chan->att = NULL;
}

static void att_timeout(struct k_work *work)
{
	struct bt_att_chan *chan = CONTAINER_OF(work, struct bt_att_chan,
						timeout_work);
	struct bt_att *att = chan->att;
	struct bt_l2cap_le_chan *ch = &chan->chan;

	BT_ERR("ATT Timeout");

	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part F] page 480:
	 *
	 * A transaction not completed within 30 seconds shall time out. Such a
	 * transaction shall be considered to have failed and the local higher
	 * layers shall be informed of this failure. No more attribute protocol
	 * requests, commands, indications or notifications shall be sent to the
	 * target device on this ATT Bearer.
	 */
	att_chan_detach(chan);

	/* Don't notify if there are still channels to be used */
	if (!sys_slist_is_empty(&att->chans)) {
		return;
	}

	att_reset(att);

	/* Consider the channel disconnected */
	bt_gatt_disconnected(ch->chan.conn);
	ch->chan.conn = NULL;
}

static struct bt_att_chan *att_get_fixed_chan(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_ATT);
	__ASSERT(chan, "No ATT channel found");

	return ATT_CHAN(chan);
}

static void att_chan_attach(struct bt_att *att, struct bt_att_chan *chan)
{
	BT_DBG("att %p chan %p flags %u", att, chan, atomic_get(chan->flags));

	if (sys_slist_is_empty(&att->chans)) {
		/* Init general queues when attaching the first channel */
		k_fifo_init(&att->tx_queue);
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
		k_fifo_init(&att->prep_queue);
#endif
	}

	sys_slist_prepend(&att->chans, &chan->node);
}

static void bt_att_connected(struct bt_l2cap_chan *chan)
{
	struct bt_att_chan *att_chan = att_get_fixed_chan(chan->conn);
	struct bt_att *att = att_chan->att;
	struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);

	BT_DBG("chan %p cid 0x%04x", ch, ch->tx.cid);

	att_chan = ATT_CHAN(chan);

	att_chan_attach(att, att_chan);

	if (!atomic_test_bit(att_chan->flags, ATT_ENHANCED)) {
		ch->tx.mtu = BT_ATT_DEFAULT_LE_MTU;
		ch->rx.mtu = BT_ATT_DEFAULT_LE_MTU;
	}

	k_delayed_work_init(&att_chan->timeout_work, att_timeout);
}

static void bt_att_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_att_chan *att_chan = ATT_CHAN(chan);
	struct bt_att *att = att_chan->att;
	struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);

	BT_DBG("chan %p cid 0x%04x", ch, ch->tx.cid);

	att_chan_detach(att_chan);

	/* Don't reset if there are still channels to be used */
	if (!sys_slist_is_empty(&att->chans)) {
		return;
	}

	att_reset(att);

	bt_gatt_disconnected(ch->chan.conn);
}

#if defined(CONFIG_BT_SMP)
static void bt_att_encrypt_change(struct bt_l2cap_chan *chan,
				  uint8_t hci_status)
{
	struct bt_att_chan *att_chan = ATT_CHAN(chan);
	struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);
	struct bt_conn *conn = ch->chan.conn;

	BT_DBG("chan %p conn %p handle %u sec_level 0x%02x status 0x%02x", ch,
	       conn, conn->handle, conn->sec_level, hci_status);

	/*
	 * If status (HCI status of security procedure) is non-zero, notify
	 * outstanding request about security failure.
	 */
	if (hci_status) {
		if (att_chan->req) {
			att_handle_rsp(att_chan, NULL, 0,
				       BT_ATT_ERR_AUTHENTICATION);
		}

		return;
	}

	bt_gatt_encrypt_change(conn);

	if (conn->sec_level == BT_SECURITY_L1) {
		return;
	}

	if (!att_chan->req || !att_chan->req->retrying) {
		return;
	}

	BT_DBG("Retrying");

	/* Resend buffer */
	bt_att_chan_send_rsp(att_chan, att_chan->req->buf,
			     chan_cb(att_chan->req->buf));

	att_chan->req->buf = NULL;
}
#endif /* CONFIG_BT_SMP */

static void bt_att_status(struct bt_l2cap_chan *ch, atomic_t *status)
{
	struct bt_att_chan *chan = ATT_CHAN(ch);
	sys_snode_t *node;

	BT_DBG("chan %p status %p", ch, status);

	if (!atomic_test_bit(status, BT_L2CAP_STATUS_OUT)) {
		return;
	}

	/* If there is a request pending don't attempt to send */
	if (chan->req) {
		return;
	}

	/* Pull next request from the list */
	node = sys_slist_get(&chan->att->reqs);
	if (!node) {
		return;
	}

	if (bt_att_chan_req_send(chan, ATT_REQ(node)) >= 0) {
		return;
	}

	/* Prepend back to the list as it could not be sent */
	sys_slist_prepend(&chan->att->reqs, node);
}

static void bt_att_released(struct bt_l2cap_chan *ch)
{
	struct bt_att_chan *chan = ATT_CHAN(ch);

	BT_DBG("chan %p", chan);

	k_mem_slab_free(&chan_slab, (void **)&chan);
}

static struct bt_att_chan *att_chan_new(struct bt_att *att, atomic_val_t flags)
{
	int quota = 0;
	static struct bt_l2cap_chan_ops ops = {
		.connected = bt_att_connected,
		.disconnected = bt_att_disconnected,
		.recv = bt_att_recv,
		.sent = bt_att_sent,
		.status = bt_att_status,
	#if defined(CONFIG_BT_SMP)
		.encrypt_change = bt_att_encrypt_change,
	#endif /* CONFIG_BT_SMP */
		.released = bt_att_released,
	};
	struct bt_att_chan *chan;

	SYS_SLIST_FOR_EACH_CONTAINER(&att->chans, chan, node) {
		if (chan->att == att) {
			quota++;
		}

		if (quota == ATT_CHAN_MAX) {
			BT_ERR("Maximum number of channels reached: %d", quota);
			return NULL;
		}
	}

	if (k_mem_slab_alloc(&chan_slab, (void **)&chan, K_NO_WAIT)) {
		BT_ERR("No available ATT channel for conn %p", att->conn);
		return NULL;
	}

	(void)memset(chan, 0, sizeof(*chan));
	chan->chan.chan.ops = &ops;
	k_fifo_init(&chan->tx_queue);
	k_sem_init(&chan->tx_sem, CONFIG_BT_ATT_TX_MAX, CONFIG_BT_ATT_TX_MAX);
	atomic_set(chan->flags, flags);
	chan->att = att;

	return chan;
}

static int bt_att_accept(struct bt_conn *conn, struct bt_l2cap_chan **ch)
{
	struct bt_att *att;
	struct bt_att_chan *chan;

	BT_DBG("conn %p handle %u", conn, conn->handle);

	if (k_mem_slab_alloc(&att_slab, (void **)&att, K_NO_WAIT)) {
		BT_ERR("No available ATT context for conn %p", conn);
		return -ENOMEM;
	}

	(void)memset(att, 0, sizeof(*att));
	att->conn = conn;
	sys_slist_init(&att->reqs);
	sys_slist_init(&att->chans);

	chan = att_chan_new(att, 0);
	if (!chan) {
		return -ENOMEM;
	}

	*ch = &chan->chan.chan;

	return 0;
}

BT_L2CAP_CHANNEL_DEFINE(att_fixed_chan, BT_L2CAP_CID_ATT, bt_att_accept, NULL);

#if defined(CONFIG_BT_EATT)
int bt_eatt_connect(struct bt_conn *conn, uint8_t num_channels)
{
	struct bt_att_chan *att_chan = att_get_fixed_chan(conn);
	struct bt_att *att = att_chan->att;
	struct bt_l2cap_chan *chan[CONFIG_BT_EATT_MAX] = {};
	int i = 0;

	if (num_channels > CONFIG_BT_EATT_MAX) {
		return -EINVAL;
	}

	while (num_channels--) {
		att_chan = att_chan_new(att, BIT(ATT_ENHANCED));
		if (!att_chan) {
			break;
		}

		chan[i] = &att_chan->chan.chan;
		i++;
	}

	if (!i) {
		return -ENOMEM;
	}

	return bt_l2cap_ecred_chan_connect(conn, chan, BT_EATT_PSM);
}

int bt_eatt_disconnect(struct bt_conn *conn)
{
	struct bt_att_chan *chan = att_get_fixed_chan(conn);
	struct bt_att *att = chan->att;
	int err = -ENOTCONN;

	if (!conn) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&att->chans, chan, node) {
		if (atomic_test_bit(chan->flags, ATT_ENHANCED)) {
			err = bt_l2cap_chan_disconnect(&chan->chan.chan);
		}
	}

	return err;
}

#endif /* CONFIG_BT_EATT */

static int bt_eatt_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	struct bt_att_chan *att_chan = att_get_fixed_chan(conn);
	struct bt_att *att = att_chan->att;

	BT_DBG("conn %p handle %u", conn, conn->handle);

	att_chan = att_chan_new(att, BIT(ATT_ENHANCED));
	if (att_chan) {
		*chan = &att_chan->chan.chan;
		return 0;
	}

	return -ENOMEM;
}

static void bt_eatt_init(void)
{
	int err;
	static struct bt_l2cap_server eatt_l2cap = {
		.psm = BT_EATT_PSM,
#if defined(CONFIG_BT_EATT_SEC_LEVEL)
		.sec_level = CONFIG_BT_EATT_SEC_LEVEL,
#endif
		.accept = bt_eatt_accept,
	};

	BT_DBG("");

	err = bt_l2cap_server_register(&eatt_l2cap);
	if (err < 0) {
		BT_ERR("EATT Server registration failed %d", err);
	}
}

void bt_att_init(void)
{
	bt_gatt_init();

	if (IS_ENABLED(CONFIG_BT_EATT)) {
		bt_eatt_init();
	}
}

uint16_t bt_att_get_mtu(struct bt_conn *conn)
{
	struct bt_att_chan *chan, *tmp;
	struct bt_att *att;
	uint16_t mtu = 0;

	att = att_get(conn);
	if (!att) {
		return 0;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		if (chan->chan.tx.mtu > mtu) {
			mtu = chan->chan.tx.mtu;
		}
	}

	return mtu;
}

struct bt_att_req *bt_att_req_alloc(k_timeout_t timeout)
{
	struct bt_att_req *req = NULL;

	/* Reserve space for request */
	if (k_mem_slab_alloc(&req_slab, (void **)&req, timeout)) {
		return NULL;
	}

	BT_DBG("req %p", req);

	memset(req, 0, sizeof(*req));

	return req;
}

void bt_att_req_free(struct bt_att_req *req)
{
	BT_DBG("req %p", req);

	k_mem_slab_free(&req_slab, (void **)&req);
}

int bt_att_send(struct bt_conn *conn, struct net_buf *buf, bt_conn_tx_cb_t cb,
		void *user_data)
{
	struct bt_att_chan *chan, *tmp;
	struct bt_att *att;
	int ret;

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(buf);

	att = att_get(conn);
	if (!att) {
		net_buf_unref(buf);
		return -ENOTCONN;
	}

	/* If callback is set use the fixed channel since bt_l2cap_chan_send
	 * cannot be used with a custom user_data.
	 */
	if (cb) {
		return bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, buf, cb,
					user_data);
	}

	ret = 0;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		ret = bt_att_chan_send(chan, buf, NULL);
		if (ret >= 0) {
			break;
		}
	}

	if (ret < 0) {
		/* Queue buffer to be send later */
		BT_DBG("Queueing buffer %p", buf);
		net_buf_put(&att->tx_queue, buf);
	}

	return 0;
}

int bt_att_req_send(struct bt_conn *conn, struct bt_att_req *req)
{
	struct bt_att *att;
	struct bt_att_chan *chan, *tmp;

	BT_DBG("conn %p req %p", conn, req);

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(req);

	att = att_get(conn);
	if (!att) {
		net_buf_unref(req->buf);
		req->buf = NULL;
		return -ENOTCONN;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		/* If there is nothing pending use the channel */
		if (!chan->req) {
			int ret;

			ret = bt_att_chan_req_send(chan, req);
			if (ret >= 0) {
				return ret;
			}
		}
	}

	/* Queue the request to be send later */
	sys_slist_append(&att->reqs, &req->node);

	BT_DBG("req %p queued", req);

	return 0;
}

static bool bt_att_chan_req_cancel(struct bt_att_chan *chan,
				   struct bt_att_req *req)
{
	if (chan->req != req) {
		return false;
	}

	chan->req = &cancel;

	att_req_destroy(req);

	return true;
}

void bt_att_req_cancel(struct bt_conn *conn, struct bt_att_req *req)
{
	struct bt_att *att;
	struct bt_att_chan *chan, *tmp;

	BT_DBG("req %p", req);

	if (!conn || !req) {
		return;
	}

	att = att_get(conn);
	if (!att) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->chans, chan, tmp, node) {
		/* Check if request is outstanding */
		if (bt_att_chan_req_cancel(chan, req)) {
			return;
		}
	}

	/* Remove request from the list */
	sys_slist_find_and_remove(&att->reqs, &req->node);

	att_req_destroy(req);
}
