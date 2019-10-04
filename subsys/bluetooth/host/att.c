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
#include <bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_ATT)
#define LOG_MODULE_NAME bt_att
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "smp.h"
#include "att_internal.h"
#include "gatt_internal.h"

#define ATT_CHAN(_ch) CONTAINER_OF(_ch, struct bt_att, chan.chan)
#define ATT_REQ(_node) CONTAINER_OF(_node, struct bt_att_req, node)

#define ATT_CMD_MASK				0x40

#define ATT_TIMEOUT				K_SECONDS(30)

typedef enum __packed {
		ATT_COMMAND,
		ATT_REQUEST,
		ATT_RESPONSE,
		ATT_NOTIFICATION,
		ATT_CONFIRMATION,
		ATT_INDICATION,
		ATT_UNKNOWN,
} att_type_t;

static att_type_t att_op_get_type(u8_t op);

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
struct bt_attr_data {
	u16_t handle;
	u16_t offset;
};

/* Pool for incoming ATT packets */
NET_BUF_POOL_DEFINE(prep_pool, CONFIG_BT_ATT_PREPARE_COUNT, BT_ATT_MTU,
		    sizeof(struct bt_attr_data), NULL);
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */

enum {
	ATT_PENDING_RSP,
	ATT_PENDING_CFM,
	ATT_DISCONNECTED,

	/* Total number of flags - must be at the end of the enum */
	ATT_NUM_FLAGS,
};

/* ATT channel specific context */
struct bt_att {
	/* The channel this context is associated with */
	struct bt_l2cap_le_chan	chan;
	ATOMIC_DEFINE(flags, ATT_NUM_FLAGS);
	struct bt_att_req	*req;
	sys_slist_t		reqs;
	struct k_delayed_work	timeout_work;
	struct k_sem		tx_sem;
	struct k_fifo		tx_queue;
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
	struct k_fifo		prep_queue;
#endif
};

static struct bt_att bt_req_pool[CONFIG_BT_MAX_CONN];
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

	(void)memset(req, 0, sizeof(*req));
}

static struct bt_att *att_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;

	chan = bt_l2cap_le_lookup_tx_cid(conn, BT_L2CAP_CID_ATT);
	__ASSERT(chan, "No ATT channel found");

	return CONTAINER_OF(chan, struct bt_att, chan);
}

static bt_conn_tx_cb_t att_cb(struct net_buf *buf);

static int att_send(struct bt_conn *conn, struct net_buf *buf,
		    bt_conn_tx_cb_t cb, void *user_data)
{
	struct bt_att_hdr *hdr;

	hdr = (void *)buf->data;

	BT_DBG("code 0x%02x", hdr->code);

	if (hdr->code == BT_ATT_OP_SIGNED_WRITE_CMD) {
		int err;

		err = bt_smp_sign(conn, buf);
		if (err) {
			BT_ERR("Error signing data");
			return err;
		}
	}

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, buf, cb ? cb : att_cb(buf),
			 user_data);

	return 0;
}

void att_pdu_sent(struct bt_conn *conn, void *user_data)
{
	struct bt_att *att = att_get(conn);
	struct net_buf *buf;

	BT_DBG("conn %p att %p", conn, att);

	while ((buf = net_buf_get(&att->tx_queue, K_NO_WAIT))) {
		/* Check if the queued buf is a request */
		if (att->req && att->req->buf == buf) {
			/* Save request state so it can be resent */
			net_buf_simple_save(&att->req->buf->b,
					    &att->req->state);
		}

		if (!att_send(conn, buf, NULL, NULL)) {
			return;
		}
		/* If the buffer cannot be send unref it */
		net_buf_unref(buf);
	}

	k_sem_give(&att->tx_sem);
}

void att_cfm_sent(struct bt_conn *conn, void *user_data)
{
	struct bt_att *att = att_get(conn);

	BT_DBG("conn %p att %p", conn, att);

	if (IS_ENABLED(CONFIG_BT_ATT_ENFORCE_FLOW)) {
		atomic_clear_bit(att->flags, ATT_PENDING_CFM);
	}

	att_pdu_sent(conn, user_data);
}

void att_rsp_sent(struct bt_conn *conn, void *user_data)
{
	struct bt_att *att = att_get(conn);

	BT_DBG("conn %p att %p", conn, att);

	if (IS_ENABLED(CONFIG_BT_ATT_ENFORCE_FLOW)) {
		atomic_clear_bit(att->flags, ATT_PENDING_RSP);
	}

	att_pdu_sent(conn, user_data);
}

void att_req_sent(struct bt_conn *conn, void *user_data)
{
	struct bt_att *att = att_get(conn);

	BT_DBG("conn %p att %p att->req %p", conn, att, att->req);

	/* Start timeout work */
	if (att->req) {
		k_delayed_work_submit(&att->timeout_work, ATT_TIMEOUT);
	}

	att_pdu_sent(conn, user_data);
}

static bt_conn_tx_cb_t att_cb(struct net_buf *buf)
{
	switch (att_op_get_type(buf->data[0])) {
	case ATT_RESPONSE:
		return att_rsp_sent;
	case ATT_CONFIRMATION:
		return att_cfm_sent;
	case ATT_REQUEST:
	case ATT_INDICATION:
		return att_req_sent;
	default:
		return att_pdu_sent;
	}
}

static void send_err_rsp(struct bt_conn *conn, u8_t req, u16_t handle,
			 u8_t err)
{
	struct bt_att_error_rsp *rsp;
	struct net_buf *buf;

	/* Ignore opcode 0x00 */
	if (!req) {
		return;
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_ERROR_RSP, sizeof(*rsp));
	if (!buf) {
		return;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->request = req;
	rsp->handle = sys_cpu_to_le16(handle);
	rsp->error = err;

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, buf, att_rsp_sent, NULL);
}

static u8_t att_mtu_req(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct bt_att_exchange_mtu_req *req;
	struct bt_att_exchange_mtu_rsp *rsp;
	struct net_buf *pdu;
	u16_t mtu_client, mtu_server;

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

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, pdu, att_rsp_sent, NULL);

	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part F] page 484:
	 *
	 * A device's Exchange MTU Request shall contain the same MTU as the
	 * device's Exchange MTU Response (i.e. the MTU shall be symmetric).
	 */
	att->chan.rx.mtu = MIN(mtu_client, mtu_server);
	att->chan.tx.mtu = att->chan.rx.mtu;

	BT_DBG("Negotiated MTU %u", att->chan.rx.mtu);
	return 0;
}

static inline bool att_is_connected(struct bt_att *att)
{
	return (att->chan.chan.conn->state != BT_CONN_CONNECTED ||
		!atomic_test_bit(att->flags, ATT_DISCONNECTED));
}

static int att_send_req(struct bt_att *att, struct bt_att_req *req)
{
	__ASSERT_NO_MSG(req);
	__ASSERT_NO_MSG(req->func);
	__ASSERT_NO_MSG(!att->req);

	BT_DBG("req %p", req);

	att->req = req;

	if (k_sem_take(&att->tx_sem, K_NO_WAIT) < 0) {
		k_fifo_put(&att->tx_queue, req->buf);
		return 0;
	}

	/* Save request state so it can be resent */
	net_buf_simple_save(&req->buf->b, &req->state);

	/* Keep a reference for resending in case of an error */
	bt_l2cap_send_cb(att->chan.chan.conn, BT_L2CAP_CID_ATT,
			 net_buf_ref(req->buf), att_cb(req->buf), NULL);

	return 0;
}

static void att_process(struct bt_att *att)
{
	sys_snode_t *node;

	BT_DBG("");

	/* Pull next request from the list */
	node = sys_slist_get(&att->reqs);
	if (!node) {
		return;
	}

	att_send_req(att, ATT_REQ(node));
}

static u8_t att_handle_rsp(struct bt_att *att, void *pdu, u16_t len, u8_t err)
{
	bt_att_func_t func;

	BT_DBG("err 0x%02x len %u: %s", err, len, bt_hex(pdu, len));

	/* Cancel timeout if ongoing */
	k_delayed_work_cancel(&att->timeout_work);

	if (!att->req) {
		BT_WARN("No pending ATT request");
		goto process;
	}

	/* Check if request has been cancelled */
	if (att->req == &cancel) {
		att->req = NULL;
		goto process;
	}

	/* Release original buffer */
	if (att->req->buf) {
		net_buf_unref(att->req->buf);
		att->req->buf = NULL;
	}

	/* Reset func so it can be reused by the callback */
	func = att->req->func;
	att->req->func = NULL;

	func(att->chan.chan.conn, err, pdu, len, att->req);

	/* Don't destroy if callback had reused the request */
	if (!att->req->func) {
		att_req_destroy(att->req);
	}

	att->req = NULL;

process:
	/* Process pending requests */
	att_process(att);

	return 0;
}

#if defined(CONFIG_BT_GATT_CLIENT)
static u8_t att_mtu_rsp(struct bt_att *att, struct net_buf *buf)
{
	struct bt_att_exchange_mtu_rsp *rsp;
	u16_t mtu;

	if (!att) {
		return 0;
	}

	rsp = (void *)buf->data;

	mtu = sys_le16_to_cpu(rsp->mtu);

	BT_DBG("Server MTU %u", mtu);

	/* Check if MTU is valid */
	if (mtu < BT_ATT_DEFAULT_LE_MTU) {
		return att_handle_rsp(att, NULL, 0, BT_ATT_ERR_INVALID_PDU);
	}

	att->chan.rx.mtu = MIN(mtu, BT_ATT_MTU);

	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part F] page 484:
	 *
	 * A device's Exchange MTU Request shall contain the same MTU as the
	 * device's Exchange MTU Response (i.e. the MTU shall be symmetric).
	 */
	att->chan.tx.mtu = att->chan.rx.mtu;

	BT_DBG("Negotiated MTU %u", att->chan.rx.mtu);

	return att_handle_rsp(att, rsp, buf->len, 0);
}
#endif /* CONFIG_BT_GATT_CLIENT */

static bool range_is_valid(u16_t start, u16_t end, u16_t *err)
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
	struct bt_att *att;
	struct net_buf *buf;
	struct bt_att_find_info_rsp *rsp;
	union {
		struct bt_att_info_16 *info16;
		struct bt_att_info_128 *info128;
	};
};

static u8_t find_info_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct find_info_data *data = user_data;
	struct bt_att *att = data->att;

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

		if (att->chan.tx.mtu - data->buf->len >
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

		if (att->chan.tx.mtu - data->buf->len >
		    sizeof(*data->info128)) {
			return BT_GATT_ITER_CONTINUE;
		}
	}

	return BT_GATT_ITER_STOP;
}

static u8_t att_find_info_rsp(struct bt_att *att, u16_t start_handle,
			      u16_t end_handle)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct find_info_data data;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_INFO_RSP, 0);
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.att = att;
	bt_gatt_foreach_attr(start_handle, end_handle, find_info_cb, &data);

	if (!data.rsp) {
		net_buf_unref(data.buf);
		/* Respond since handle is set */
		send_err_rsp(conn, BT_ATT_OP_FIND_INFO_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return 0;
	}

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, data.buf, att_rsp_sent, NULL);

	return 0;
}

static u8_t att_find_info_req(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct bt_att_find_info_req *req;
	u16_t start_handle, end_handle, err_handle;

	req = (void *)buf->data;

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);

	BT_DBG("start_handle 0x%04x end_handle 0x%04x", start_handle,
	       end_handle);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(conn, BT_ATT_OP_FIND_INFO_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	return att_find_info_rsp(att, start_handle, end_handle);
}

struct find_type_data {
	struct bt_att *att;
	struct net_buf *buf;
	struct bt_att_handle_group *group;
	const void *value;
	u8_t value_len;
	u8_t err;
};

static u8_t find_type_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct find_type_data *data = user_data;
	struct bt_att *att = data->att;
	struct bt_conn *conn = att->chan.chan.conn;
	int read;
	u8_t uuid[16];

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
	if (att->chan.tx.mtu - data->buf->len < sizeof(*data->group)) {
		return BT_GATT_ITER_STOP;
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
	data->group = net_buf_add(data->buf, sizeof(*data->group));
	data->group->start_handle = sys_cpu_to_le16(attr->handle);
	data->group->end_handle = sys_cpu_to_le16(attr->handle);

	/* continue to find the end_handle */
	return BT_GATT_ITER_CONTINUE;

skip:
	data->group = NULL;
	return BT_GATT_ITER_CONTINUE;
}

static u8_t att_find_type_rsp(struct bt_att *att, u16_t start_handle,
			      u16_t end_handle, const void *value,
			      u8_t value_len)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct find_type_data data;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_TYPE_RSP, 0);
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.att = att;
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
		send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, start_handle,
			     data.err);
		return 0;
	}

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, data.buf, att_rsp_sent, NULL);

	return 0;
}

static u8_t att_find_type_req(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct bt_att_find_type_req *req;
	u16_t start_handle, end_handle, err_handle, type;
	u8_t *value;

	req = net_buf_pull_mem(buf, sizeof(*req));

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);
	type = sys_le16_to_cpu(req->type);
	value = buf->data;

	BT_DBG("start_handle 0x%04x end_handle 0x%04x type %u", start_handle,
	       end_handle, type);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	/* The Attribute Protocol Find By Type Value Request shall be used with
	 * the Attribute Type parameter set to the UUID for "Primary Service"
	 * and the Attribute Value set to the 16-bit Bluetooth UUID or 128-bit
	 * UUID for the specific primary service.
	 */
	if (bt_uuid_cmp(BT_UUID_DECLARE_16(type), BT_UUID_GATT_PRIMARY)) {
		send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return 0;
	}

	return att_find_type_rsp(att, start_handle, end_handle, value,
				 buf->len);
}

static u8_t err_to_att(int err)
{
	BT_DBG("%d", err);

	if (err < 0 && err >= -0xff) {
		return -err;
	}

	return BT_ATT_ERR_UNLIKELY;
}

struct read_type_data {
	struct bt_att *att;
	struct bt_uuid *uuid;
	struct net_buf *buf;
	struct bt_att_read_type_rsp *rsp;
	struct bt_att_data *item;
	u8_t err;
};

static u8_t read_type_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_type_data *data = user_data;
	struct bt_att *att = data->att;
	struct bt_conn *conn = att->chan.chan.conn;
	int read;

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

	/* Fast forward to next item position */
	data->item = net_buf_add(data->buf, sizeof(*data->item));
	data->item->handle = sys_cpu_to_le16(attr->handle);

	/* Read attribute value and store in the buffer */
	read = attr->read(conn, attr, data->buf->data + data->buf->len,
			  att->chan.tx.mtu - data->buf->len, 0);
	if (read < 0) {
		data->err = err_to_att(read);
		return BT_GATT_ITER_STOP;
	}

	if (!data->rsp->len) {
		/* Set len to be the first item found */
		data->rsp->len = read + sizeof(*data->item);
	} else if (data->rsp->len != read + sizeof(*data->item)) {
		/* All items should have the same size */
		data->buf->len -= sizeof(*data->item);
		return BT_GATT_ITER_STOP;
	}

	net_buf_add(data->buf, read);

	/* return true only if there are still space for more items */
	return att->chan.tx.mtu - data->buf->len > data->rsp->len ?
	       BT_GATT_ITER_CONTINUE : BT_GATT_ITER_STOP;
}

static u8_t att_read_type_rsp(struct bt_att *att, struct bt_uuid *uuid,
			      u16_t start_handle, u16_t end_handle)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct read_type_data data;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_TYPE_RSP,
				     sizeof(*data.rsp));
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.att = att;
	data.uuid = uuid;
	data.rsp = net_buf_add(data.buf, sizeof(*data.rsp));
	data.rsp->len = 0U;

	/* Pre-set error if no attr will be found in handle */
	data.err = BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;

	bt_gatt_foreach_attr(start_handle, end_handle, read_type_cb, &data);

	if (data.err) {
		net_buf_unref(data.buf);
		/* Response here since handle is set */
		send_err_rsp(conn, BT_ATT_OP_READ_TYPE_REQ, start_handle,
			     data.err);
		return 0;
	}

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, data.buf, att_rsp_sent, NULL);

	return 0;
}

static u8_t att_read_type_req(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct bt_att_read_type_req *req;
	u16_t start_handle, end_handle, err_handle;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;
	u8_t uuid_len = buf->len - sizeof(*req);

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
		send_err_rsp(conn, BT_ATT_OP_READ_TYPE_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return 0;
	}

	return att_read_type_rsp(att, &u.uuid, start_handle, end_handle);
}

struct read_data {
	struct bt_att *att;
	u16_t offset;
	struct net_buf *buf;
	struct bt_att_read_rsp *rsp;
	u8_t err;
};

static u8_t read_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_data *data = user_data;
	struct bt_att *att = data->att;
	struct bt_conn *conn = att->chan.chan.conn;
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

	/* Read attribute value and store in the buffer */
	read = attr->read(conn, attr, data->buf->data + data->buf->len,
			  att->chan.tx.mtu - data->buf->len, data->offset);
	if (read < 0) {
		data->err = err_to_att(read);
		return BT_GATT_ITER_STOP;
	}

	net_buf_add(data->buf, read);

	return BT_GATT_ITER_CONTINUE;
}

static u8_t att_read_rsp(struct bt_att *att, u8_t op, u8_t rsp, u16_t handle,
			 u16_t offset)
{
	struct bt_conn *conn = att->chan.chan.conn;
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

	data.att = att;
	data.offset = offset;

	/* Pre-set error if no attr will be found in handle */
	data.err = BT_ATT_ERR_INVALID_HANDLE;

	bt_gatt_foreach_attr(handle, handle, read_cb, &data);

	/* In case of error discard data and respond with an error */
	if (data.err) {
		net_buf_unref(data.buf);
		/* Respond here since handle is set */
		send_err_rsp(conn, op, handle, data.err);
		return 0;
	}

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, data.buf, att_rsp_sent, NULL);

	return 0;
}

static u8_t att_read_req(struct bt_att *att, struct net_buf *buf)
{
	struct bt_att_read_req *req;
	u16_t handle;

	req = (void *)buf->data;

	handle = sys_le16_to_cpu(req->handle);

	BT_DBG("handle 0x%04x", handle);

	return att_read_rsp(att, BT_ATT_OP_READ_REQ, BT_ATT_OP_READ_RSP,
			    handle, 0);
}

static u8_t att_read_blob_req(struct bt_att *att, struct net_buf *buf)
{
	struct bt_att_read_blob_req *req;
	u16_t handle, offset;

	req = (void *)buf->data;

	handle = sys_le16_to_cpu(req->handle);
	offset = sys_le16_to_cpu(req->offset);

	BT_DBG("handle 0x%04x offset %u", handle, offset);

	return att_read_rsp(att, BT_ATT_OP_READ_BLOB_REQ,
			    BT_ATT_OP_READ_BLOB_RSP, handle, offset);
}

#if defined(CONFIG_BT_GATT_READ_MULTIPLE)
static u8_t att_read_mult_req(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct read_data data;
	u16_t handle;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_MULT_RSP, 0);
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.att = att;

	while (buf->len >= sizeof(u16_t)) {
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
			send_err_rsp(conn, BT_ATT_OP_READ_MULT_REQ, handle,
				     data.err);
			return 0;
		}
	}

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, data.buf, att_rsp_sent, NULL);

	return 0;
}
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */

struct read_group_data {
	struct bt_att *att;
	struct bt_uuid *uuid;
	struct net_buf *buf;
	struct bt_att_read_group_rsp *rsp;
	struct bt_att_group_data *group;
};

static u8_t read_group_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_group_data *data = user_data;
	struct bt_att *att = data->att;
	struct bt_conn *conn = att->chan.chan.conn;
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
	    att->chan.tx.mtu - data->buf->len < data->rsp->len) {
		return BT_GATT_ITER_STOP;
	}

	/* Fast forward to next group position */
	data->group = net_buf_add(data->buf, sizeof(*data->group));

	/* Initialize group handle range */
	data->group->start_handle = sys_cpu_to_le16(attr->handle);
	data->group->end_handle = sys_cpu_to_le16(attr->handle);

	/* Read attribute value and store in the buffer */
	read = attr->read(conn, attr, data->buf->data + data->buf->len,
			  att->chan.tx.mtu - data->buf->len, 0);
	if (read < 0) {
		/* TODO: Handle read errors */
		return BT_GATT_ITER_STOP;
	}

	if (!data->rsp->len) {
		/* Set len to be the first group found */
		data->rsp->len = read + sizeof(*data->group);
	} else if (data->rsp->len != read + sizeof(*data->group)) {
		/* All groups entries should have the same size */
		data->buf->len -= sizeof(*data->group);
		return false;
	}

	net_buf_add(data->buf, read);

	/* Continue to find the end handle */
	return BT_GATT_ITER_CONTINUE;
}

static u8_t att_read_group_rsp(struct bt_att *att, struct bt_uuid *uuid,
			       u16_t start_handle, u16_t end_handle)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct read_group_data data;

	(void)memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_GROUP_RSP,
				     sizeof(*data.rsp));
	if (!data.buf) {
		return BT_ATT_ERR_UNLIKELY;
	}

	data.att = att;
	data.uuid = uuid;
	data.rsp = net_buf_add(data.buf, sizeof(*data.rsp));
	data.rsp->len = 0U;
	data.group = NULL;

	bt_gatt_foreach_attr(start_handle, end_handle, read_group_cb, &data);

	if (!data.rsp->len) {
		net_buf_unref(data.buf);
		/* Respond here since handle is set */
		send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return 0;
	}

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, data.buf, att_rsp_sent, NULL);

	return 0;
}

static u8_t att_read_group_req(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct bt_att_read_group_req *req;
	u16_t start_handle, end_handle, err_handle;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;
	u8_t uuid_len = buf->len - sizeof(*req);

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
		send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, err_handle,
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
		send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, start_handle,
			     BT_ATT_ERR_UNSUPPORTED_GROUP_TYPE);
		return 0;
	}

	return att_read_group_rsp(att, &u.uuid, start_handle, end_handle);
}

struct write_data {
	struct bt_conn *conn;
	struct net_buf *buf;
	u8_t req;
	const void *value;
	u8_t len;
	u16_t offset;
	u8_t err;
};

static u8_t write_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct write_data *data = user_data;
	int write;
	u8_t flags = 0U;

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

static u8_t att_write_rsp(struct bt_conn *conn, u8_t req, u8_t rsp,
			  u16_t handle, u16_t offset, const void *value,
			  u8_t len)
{
	struct write_data data;

	if (!bt_gatt_change_aware(conn, req ? true : false)) {
		return BT_ATT_ERR_DB_OUT_OF_SYNC;
	}

	if (!handle) {
		return BT_ATT_ERR_INVALID_HANDLE;
	}

	(void)memset(&data, 0, sizeof(data));

	/* Only allocate buf if required to respond */
	if (rsp) {
		data.buf = bt_att_create_pdu(conn, rsp, 0);
		if (!data.buf) {
			return BT_ATT_ERR_UNLIKELY;
		}
	}

	data.conn = conn;
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
			send_err_rsp(conn, req, handle, data.err);
		}
		return req == BT_ATT_OP_EXEC_WRITE_REQ ? data.err : 0;
	}

	if (data.buf) {
		bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, data.buf,
				 att_rsp_sent, NULL);
	}

	return 0;
}

static u8_t att_write_req(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	u16_t handle;

	handle = net_buf_pull_le16(buf);

	BT_DBG("handle 0x%04x", handle);

	return att_write_rsp(conn, BT_ATT_OP_WRITE_REQ, BT_ATT_OP_WRITE_RSP,
			     handle, 0, buf->data, buf->len);
}

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
struct prep_data {
	struct bt_conn *conn;
	struct net_buf *buf;
	const void *value;
	u8_t len;
	u16_t offset;
	u8_t err;
};

static u8_t prep_write_cb(const struct bt_gatt_attr *attr, void *user_data)
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

static u8_t att_prep_write_rsp(struct bt_att *att, u16_t handle, u16_t offset,
			       const void *value, u8_t len)
{
	struct bt_conn *conn = att->chan.chan.conn;
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
		send_err_rsp(conn, BT_ATT_OP_PREPARE_WRITE_REQ, handle,
			     data.err);
		return 0;
	}

	BT_DBG("buf %p handle 0x%04x offset %u", data.buf, handle, offset);

	/* Store buffer in the outstanding queue */
	net_buf_put(&att->prep_queue, data.buf);

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

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, data.buf, att_rsp_sent, NULL);

	return 0;
}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */

static u8_t att_prepare_write_req(struct bt_att *att, struct net_buf *buf)
{
#if CONFIG_BT_ATT_PREPARE_COUNT == 0
	return BT_ATT_ERR_NOT_SUPPORTED;
#else
	struct bt_att_prepare_write_req *req;
	u16_t handle, offset;

	req = net_buf_pull_mem(buf, sizeof(*req));

	handle = sys_le16_to_cpu(req->handle);
	offset = sys_le16_to_cpu(req->offset);

	BT_DBG("handle 0x%04x offset %u", handle, offset);

	return att_prep_write_rsp(att, handle, offset, buf->data, buf->len);
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */
}

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
static u8_t att_exec_write_rsp(struct bt_att *att, u8_t flags)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct net_buf *buf;
	u8_t err = 0U;

	while ((buf = net_buf_get(&att->prep_queue, K_NO_WAIT))) {
		struct bt_attr_data *data = net_buf_user_data(buf);

		BT_DBG("buf %p handle 0x%04x offset %u", buf, data->handle,
		       data->offset);

		/* Just discard the data if an error was set */
		if (!err && flags == BT_ATT_FLAG_EXEC) {
			err = att_write_rsp(conn, BT_ATT_OP_EXEC_WRITE_REQ, 0,
					    data->handle, data->offset,
					    buf->data, buf->len);
			if (err) {
				/* Respond here since handle is set */
				send_err_rsp(conn, BT_ATT_OP_EXEC_WRITE_REQ,
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

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, buf, att_rsp_sent, NULL);

	return 0;
}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */


static u8_t att_exec_write_req(struct bt_att *att, struct net_buf *buf)
{
#if CONFIG_BT_ATT_PREPARE_COUNT == 0
	return BT_ATT_ERR_NOT_SUPPORTED;
#else
	struct bt_att_exec_write_req *req;

	req = (void *)buf->data;

	BT_DBG("flags 0x%02x", req->flags);

	return att_exec_write_rsp(att, req->flags);
#endif /* CONFIG_BT_ATT_PREPARE_COUNT */
}

static u8_t att_write_cmd(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	u16_t handle;

	handle = net_buf_pull_le16(buf);

	BT_DBG("handle 0x%04x", handle);

	return att_write_rsp(conn, 0, 0, handle, 0, buf->data, buf->len);
}

#if defined(CONFIG_BT_SIGNING)
static u8_t att_signed_write_cmd(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	struct bt_att_signed_write_cmd *req;
	u16_t handle;
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

	return att_write_rsp(conn, 0, 0, handle, 0, buf->data,
			     buf->len - sizeof(struct bt_att_signature));
}
#endif /* CONFIG_BT_SIGNING */

#if defined(CONFIG_BT_GATT_CLIENT)
#if defined(CONFIG_BT_SMP)
static int att_change_security(struct bt_conn *conn, u8_t err)
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

static u8_t att_error_rsp(struct bt_att *att, struct net_buf *buf)
{
	struct bt_att_error_rsp *rsp;
	u8_t err;

	rsp = (void *)buf->data;

	BT_DBG("request 0x%02x handle 0x%04x error 0x%02x", rsp->request,
	       sys_le16_to_cpu(rsp->handle), rsp->error);

	/* Don't retry if there is no req pending or it has been cancelled */
	if (!att->req || att->req == &cancel) {
		err = BT_ATT_ERR_UNLIKELY;
		goto done;
	}

	if (att->req->buf) {
		/* Restore state to be resent */
		net_buf_simple_restore(&att->req->buf->b, &att->req->state);
	}

	err = rsp->error;
#if defined(CONFIG_BT_SMP)
	if (att->req->retrying) {
		goto done;
	}

	/* Check if security needs to be changed */
	if (!att_change_security(att->chan.chan.conn, err)) {
		att->req->retrying = true;
		/* Wait security_changed: TODO: Handle fail case */
		return 0;
	}
#endif /* CONFIG_BT_SMP */

done:
	return att_handle_rsp(att, NULL, 0, err);
}

static u8_t att_handle_find_info_rsp(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

static u8_t att_handle_find_type_rsp(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

static u8_t att_handle_read_type_rsp(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

static u8_t att_handle_read_rsp(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

static u8_t att_handle_read_blob_rsp(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

#if defined(CONFIG_BT_GATT_READ_MULTIPLE)
static u8_t att_handle_read_mult_rsp(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}
#endif /* CONFIG_BT_GATT_READ_MULTIPLE */

static u8_t att_handle_read_group_rsp(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

static u8_t att_handle_write_rsp(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

static u8_t att_handle_prepare_write_rsp(struct bt_att *att,
					 struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

static u8_t att_handle_exec_write_rsp(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

static u8_t att_notify(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	u16_t handle;

	handle = net_buf_pull_le16(buf);

	BT_DBG("handle 0x%04x", handle);

	bt_gatt_notification(conn, handle, buf->data, buf->len);

	return 0;
}

static u8_t att_indicate(struct bt_att *att, struct net_buf *buf)
{
	struct bt_conn *conn = att->chan.chan.conn;
	u16_t handle;

	handle = net_buf_pull_le16(buf);

	BT_DBG("handle 0x%04x", handle);

	bt_gatt_notification(conn, handle, buf->data, buf->len);

	buf = bt_att_create_pdu(conn, BT_ATT_OP_CONFIRM, 0);
	if (!buf) {
		return 0;
	}

	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, buf, att_cfm_sent, NULL);

	return 0;
}
#endif /* CONFIG_BT_GATT_CLIENT */

static u8_t att_confirm(struct bt_att *att, struct net_buf *buf)
{
	BT_DBG("");

	return att_handle_rsp(att, buf->data, buf->len, 0);
}

static const struct att_handler {
	u8_t       op;
	u8_t       expect_len;
	att_type_t type;
	u8_t       (*func)(struct bt_att *att, struct net_buf *buf);
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
#endif /* CONFIG_BT_GATT_CLIENT */
};

static att_type_t att_op_get_type(u8_t op)
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
	struct bt_att *att = ATT_CHAN(chan);
	struct bt_att_hdr *hdr;
	const struct att_handler *handler;
	u8_t err;
	size_t i;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small ATT PDU received");
		return 0;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	BT_DBG("Received ATT code 0x%02x len %u", hdr->code, buf->len);

	for (i = 0, handler = NULL; i < ARRAY_SIZE(handlers); i++) {
		if (hdr->code == handlers[i].op) {
			handler = &handlers[i];
			break;
		}
	}

	if (!handler) {
		BT_WARN("Unhandled ATT code 0x%02x", hdr->code);
		if (att_op_get_type(hdr->code) != ATT_COMMAND) {
			send_err_rsp(chan->conn, hdr->code, 0,
				     BT_ATT_ERR_NOT_SUPPORTED);
		}
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_ATT_ENFORCE_FLOW)) {
		if (handler->type == ATT_REQUEST &&
		    atomic_test_and_set_bit(att->flags, ATT_PENDING_RSP)) {
			BT_WARN("Ignoring unexpected request");
			return 0;
		} else if (handler->type == ATT_INDICATION &&
			   atomic_test_and_set_bit(att->flags,
						   ATT_PENDING_CFM)) {
			BT_WARN("Ignoring unexpected indication");
			return 0;
		}
	}

	if (buf->len < handler->expect_len) {
		BT_ERR("Invalid len %u for code 0x%02x", buf->len, hdr->code);
		err = BT_ATT_ERR_INVALID_PDU;
	} else {
		err = handler->func(att, buf);
	}

	if (handler->type == ATT_REQUEST && err) {
		BT_DBG("ATT error 0x%02x", err);
		send_err_rsp(chan->conn, hdr->code, 0, err);
	}

	return 0;
}

static struct bt_att *att_chan_get(struct bt_conn *conn)
{
	struct bt_l2cap_chan *chan;
	struct bt_att *att;

	if (conn->state != BT_CONN_CONNECTED) {
		BT_WARN("Not connected");
		return NULL;
	}

	chan = bt_l2cap_le_lookup_rx_cid(conn, BT_L2CAP_CID_ATT);
	if (!chan) {
		BT_ERR("Unable to find ATT channel");
		return NULL;
	}

	att = ATT_CHAN(chan);
	if (atomic_test_bit(att->flags, ATT_DISCONNECTED)) {
		BT_WARN("ATT context flagged as disconnected");
		return NULL;
	}

	return att;
}

struct net_buf *bt_att_create_pdu(struct bt_conn *conn, u8_t op, size_t len)
{
	struct bt_att_hdr *hdr;
	struct net_buf *buf;
	struct bt_att *att;

	att = att_chan_get(conn);
	if (!att) {
		return NULL;
	}

	if (len + sizeof(op) > att->chan.tx.mtu) {
		BT_WARN("ATT MTU exceeded, max %u, wanted %zu",
			att->chan.tx.mtu, len + sizeof(op));
		return NULL;
	}

	switch (att_op_get_type(op)) {
	case ATT_RESPONSE:
	case ATT_CONFIRMATION:
		/* Use a timeout only when responding/confirming */
		buf = bt_l2cap_create_pdu_timeout(NULL, 0, ATT_TIMEOUT);
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

static void att_reset(struct bt_att *att)
{
	struct bt_att_req *req, *tmp;
	int i;
	struct net_buf *buf;

#if CONFIG_BT_ATT_PREPARE_COUNT > 0
	/* Discard queued buffers */
	while ((buf = k_fifo_get(&att->prep_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}
#endif /* CONFIG_BT_ATT_PREPARE_COUNT > 0 */

	while ((buf = k_fifo_get(&att->tx_queue, K_NO_WAIT))) {
		net_buf_unref(buf);
	}

	atomic_set_bit(att->flags, ATT_DISCONNECTED);

	/* Ensure that any waiters are woken up */
	for (i = 0; i < CONFIG_BT_ATT_TX_MAX; i++) {
		k_sem_give(&att->tx_sem);
	}

	/* Notify pending requests */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&att->reqs, req, tmp, node) {
		if (req->func) {
			req->func(NULL, BT_ATT_ERR_UNLIKELY, NULL, 0, req);
		}

		att_req_destroy(req);
	}

	/* Reset list */
	sys_slist_init(&att->reqs);

	if (!att->req) {
		return;
	}

	/* Notify outstanding request */
	att_handle_rsp(att, NULL, 0, BT_ATT_ERR_UNLIKELY);
}

static void att_timeout(struct k_work *work)
{
	struct bt_att *att = CONTAINER_OF(work, struct bt_att, timeout_work);
	struct bt_l2cap_le_chan *ch = &att->chan;

	BT_ERR("ATT Timeout");

	/* BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part F] page 480:
	 *
	 * A transaction not completed within 30 seconds shall time out. Such a
	 * transaction shall be considered to have failed and the local higher
	 * layers shall be informed of this failure. No more attribute protocol
	 * requests, commands, indications or notifications shall be sent to the
	 * target device on this ATT Bearer.
	 */
	att_reset(att);

	/* Consider the channel disconnected */
	bt_gatt_disconnected(ch->chan.conn);
	ch->chan.conn = NULL;
}

static void bt_att_connected(struct bt_l2cap_chan *chan)
{
	struct bt_att *att = ATT_CHAN(chan);
	struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);

	BT_DBG("chan %p cid 0x%04x", ch, ch->tx.cid);

	k_fifo_init(&att->tx_queue);
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
	k_fifo_init(&att->prep_queue);
#endif

	ch->tx.mtu = BT_ATT_DEFAULT_LE_MTU;
	ch->rx.mtu = BT_ATT_DEFAULT_LE_MTU;

	k_delayed_work_init(&att->timeout_work, att_timeout);
	sys_slist_init(&att->reqs);
}

static void bt_att_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_att *att = ATT_CHAN(chan);
	struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);

	BT_DBG("chan %p cid 0x%04x", ch, ch->tx.cid);

	att_reset(att);

	bt_gatt_disconnected(ch->chan.conn);
}

#if defined(CONFIG_BT_SMP)
static void bt_att_encrypt_change(struct bt_l2cap_chan *chan,
				  u8_t hci_status)
{
	struct bt_att *att = ATT_CHAN(chan);
	struct bt_l2cap_le_chan *ch = BT_L2CAP_LE_CHAN(chan);
	struct bt_conn *conn = ch->chan.conn;

	BT_DBG("chan %p conn %p handle %u sec_level 0x%02x status 0x%02x", ch,
	       conn, conn->handle, conn->sec_level, hci_status);

	/*
	 * If status (HCI status of security procedure) is non-zero, notify
	 * outstanding request about security failure.
	 */
	if (hci_status) {
		att_handle_rsp(att, NULL, 0, BT_ATT_ERR_AUTHENTICATION);
		return;
	}

	bt_gatt_encrypt_change(conn);

	if (conn->sec_level == BT_SECURITY_L1) {
		return;
	}

	if (!att->req || !att->req->retrying) {
		return;
	}

	k_sem_take(&att->tx_sem, K_FOREVER);
	if (!att_is_connected(att)) {
		BT_WARN("Disconnected");
		k_sem_give(&att->tx_sem);
		return;
	}

	BT_DBG("Retrying");

	/* Resend buffer */
	bt_l2cap_send_cb(conn, BT_L2CAP_CID_ATT, att->req->buf,
			 att_cb(att->req->buf), NULL);
	att->req->buf = NULL;
}
#endif /* CONFIG_BT_SMP */

static int bt_att_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	int i;
	static struct bt_l2cap_chan_ops ops = {
		.connected = bt_att_connected,
		.disconnected = bt_att_disconnected,
		.recv = bt_att_recv,
#if defined(CONFIG_BT_SMP)
		.encrypt_change = bt_att_encrypt_change,
#endif /* CONFIG_BT_SMP */
	};

	BT_DBG("conn %p handle %u", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_req_pool); i++) {
		struct bt_att *att = &bt_req_pool[i];

		if (att->chan.chan.conn) {
			continue;
		}

		(void)memset(att, 0, sizeof(*att));
		att->chan.chan.ops = &ops;
		k_sem_init(&att->tx_sem, CONFIG_BT_ATT_TX_MAX,
			   CONFIG_BT_ATT_TX_MAX);

		*chan = &att->chan.chan;

		return 0;
	}

	BT_ERR("No available ATT context for conn %p", conn);

	return -ENOMEM;
}

BT_L2CAP_CHANNEL_DEFINE(att_fixed_chan, BT_L2CAP_CID_ATT, bt_att_accept);

void bt_att_init(void)
{
	bt_gatt_init();
}

u16_t bt_att_get_mtu(struct bt_conn *conn)
{
	struct bt_att *att;

	att = att_chan_get(conn);
	if (!att) {
		return 0;
	}

	/* tx and rx MTU shall be symmetric */
	return att->chan.tx.mtu;
}

int bt_att_send(struct bt_conn *conn, struct net_buf *buf, bt_conn_tx_cb_t cb,
		void *user_data)
{
	struct bt_att *att;
	int err;

	if (!conn || !buf) {
		return -EINVAL;
	}

	att = att_chan_get(conn);
	if (!att) {
		return -ENOTCONN;
	}

	/* Don't use tx_sem if caller has set it own callback */
	if (!cb) {
		/* Queue buffer to be send later */
		if (k_sem_take(&att->tx_sem, K_NO_WAIT) < 0) {
			k_fifo_put(&att->tx_queue, buf);
			return 0;
		}
	}

	err = att_send(conn, buf, cb, user_data);
	if (err) {
		if (!cb) {
			k_sem_give(&att->tx_sem);
		}
		return err;
	}

	return 0;
}

int bt_att_req_send(struct bt_conn *conn, struct bt_att_req *req)
{
	struct bt_att *att;

	BT_DBG("conn %p req %p", conn, req);

	if (!conn || !req) {
		return -EINVAL;
	}

	att = att_chan_get(conn);
	if (!att) {
		return -ENOTCONN;
	}

	/* Check if there is a request outstanding */
	if (att->req) {
		/* Queue the request to be send later */
		sys_slist_append(&att->reqs, &req->node);
		return 0;
	}

	return att_send_req(att, req);
}

void bt_att_req_cancel(struct bt_conn *conn, struct bt_att_req *req)
{
	struct bt_att *att;

	BT_DBG("req %p", req);

	if (!conn || !req) {
		return;
	}

	att = att_chan_get(conn);
	if (!att) {
		return;
	}

	/* Check if request is outstanding */
	if (att->req == req) {
		att->req = &cancel;
	} else {
		/* Remove request from the list */
		sys_slist_find_and_remove(&att->reqs, &req->node);
	}

	att_req_destroy(req);
}
