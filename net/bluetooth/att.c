/* att.c - Attribute protocol handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "hci_core.h"
#include "conn.h"
#include "l2cap.h"
#include "att.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_ATT)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

/* ATT channel specific context */
struct bt_att {
	/* The connection this context is associated with */
	struct bt_conn		*conn;
	uint16_t		mtu;
};

static struct bt_att bt_att_pool[CONFIG_BLUETOOTH_MAX_CONN];

static const struct bt_uuid primary_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_GATT_PRIMARY,
};

static const struct bt_uuid secondary_uuid = {
	.type = BT_UUID_16,
	.u16 = BT_UUID_GATT_SECONDARY,
};

static void send_err_rsp(struct bt_conn *conn, uint8_t req, uint16_t handle,
			 uint8_t err)
{
	struct bt_att_error_rsp *rsp;
	struct bt_buf *buf;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_ERROR_RSP, sizeof(*rsp));
	if (!buf) {
		return;
	}

	rsp = bt_buf_add(buf, sizeof(*rsp));
	rsp->request = req;
	rsp->handle = sys_cpu_to_le16(handle);
	rsp->error = err;

	bt_l2cap_send(conn, BT_L2CAP_CID_ATT, buf);
}

static void att_mtu_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att *att = conn->att;
	struct bt_att_exchange_mtu_req *req;
	struct bt_att_exchange_mtu_rsp *rsp;
	struct bt_buf *buf;
	uint16_t mtu;

	if (data->len != sizeof(*req)) {
		send_err_rsp(conn, BT_ATT_OP_MTU_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	mtu = sys_le16_to_cpu(req->mtu);

	BT_DBG("Client MTU %u\n", mtu);

	if (mtu > BT_ATT_MAX_LE_MTU || mtu < BT_ATT_DEFAULT_LE_MTU) {
		send_err_rsp(conn, BT_ATT_OP_MTU_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_MTU_RSP, sizeof(*rsp));
	if (!buf) {
		return;
	}

	/* Select MTU based on the amount of room we have in bt_buf including
	 * one extra byte for ATT header.
	 */
	mtu = min(mtu, bt_buf_tailroom(buf) + 1);

	BT_DBG("Server MTU %u\n", mtu);

	att->mtu = mtu;

	rsp = bt_buf_add(buf, sizeof(*rsp));
	rsp->mtu = sys_cpu_to_le16(mtu);

	bt_l2cap_send(conn, BT_L2CAP_CID_ATT, buf);
}

static bool range_is_valid(uint16_t start, uint16_t end, uint16_t *err)
{
	/* Handle 0 is invalid */
	if (!start || !end) {
		if (err) {
			*err = 0;
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
	struct bt_conn *conn;
	struct bt_buf *buf;
	struct bt_att_find_info_rsp *rsp;
	union {
		struct bt_att_info_16 *info16;
		struct bt_att_info_128 *info128;
	};
};

static uint8_t find_info_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct find_info_data *data = user_data;
	struct bt_att *att = data->conn->att;

	BT_DBG("handle %u\n", attr->handle);

	/* Initialize rsp at first entry */
	if (!data->rsp) {
		data->rsp = bt_buf_add(data->buf, sizeof(*data->rsp));
		data->rsp->format = (attr->uuid->type == BT_UUID_16) ?
				    BT_ATT_INFO_16 : BT_ATT_INFO_128;
	}

	switch (data->rsp->format) {
	case BT_ATT_INFO_16:
		if (attr->uuid->type != BT_UUID_16) {
			return BT_GATT_ITER_STOP;
		}

		/* Fast foward to next item position */
		data->info16 = bt_buf_add(data->buf, sizeof(*data->info16));
		data->info16->handle = sys_cpu_to_le16(attr->handle);
		data->info16->uuid = sys_cpu_to_le16(attr->uuid->u16);

		return att->mtu - data->buf->len > sizeof(*data->info16) ?
			BT_GATT_ITER_CONTINUE : BT_GATT_ITER_STOP;
	case BT_ATT_INFO_128:
		if (attr->uuid->type != BT_UUID_128) {
			return BT_GATT_ITER_STOP;
		}

		/* Fast foward to next item position */
		data->info128 = bt_buf_add(data->buf, sizeof(*data->info128));
		data->info128->handle = sys_cpu_to_le16(attr->handle);
		memcpy(data->info128->uuid, attr->uuid->u128,
		       sizeof(data->info128->uuid));

		return att->mtu - data->buf->len > sizeof(*data->info128) ?
			BT_GATT_ITER_CONTINUE : BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static void att_find_info_rsp(struct bt_conn *conn, uint16_t start_handle,
			      uint16_t end_handle)
{
	struct find_info_data data;

	memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_INFO_RSP, 0);
	if (!data.buf) {
		return;
	}

	bt_gatt_foreach_attr(start_handle, end_handle, find_info_cb, &data);

	if (!data.rsp) {
		bt_buf_put(data.buf);
		send_err_rsp(conn, BT_ATT_OP_FIND_INFO_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return;
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_ATT, data.buf);
}

static void att_find_info_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_find_info_req *req;
	uint16_t start_handle, end_handle, err_handle;

	if (data->len != sizeof(*req)) {
		send_err_rsp(conn, BT_ATT_OP_FIND_INFO_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);

	BT_DBG("start_handle %u end_handle %u\n", start_handle, end_handle);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return;
	}

	att_find_info_rsp(conn, start_handle, end_handle);
}

struct find_type_data {
	struct bt_conn *conn;
	struct bt_buf *buf;
	struct bt_att_handle_group *group;
	const void *value;
	uint8_t value_len;
};

static uint8_t find_type_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct find_type_data *data = user_data;
	struct bt_att *att = data->conn->att;
	int read;
	uint8_t uuid[16];

	/* Skip if not a primary service */
	if (bt_uuid_cmp(attr->uuid, &primary_uuid)) {
		if (data->group && attr->handle > data->group->end_handle) {
			data->group->end_handle = sys_cpu_to_le16(attr->handle);
		}
		return BT_GATT_ITER_CONTINUE;
	}

	BT_DBG("handle %u\n", attr->handle);

	/* stop if there is no space left */
	if (att->mtu - data->buf->len < sizeof(*data->group))
		return BT_GATT_ITER_STOP;

	/* Read attribute value and store in the buffer */
	read = attr->read(attr, uuid, sizeof(uuid), 0);
	if (read < 0) {
		/* TODO: Return an error if this fails */
		return BT_GATT_ITER_STOP;
	}

	/* Check if data matches */
	if (read != data->value_len || memcmp(data->value, uuid, read)) {
		/* If a group exists stop otherwise continue */
		return data->group ? BT_GATT_ITER_STOP : BT_GATT_ITER_CONTINUE;
	}

	/* Fast foward to next item position */
	data->group = bt_buf_add(data->buf, sizeof(*data->group));
	data->group->start_handle = sys_cpu_to_le16(attr->handle);
	data->group->end_handle = sys_cpu_to_le16(attr->handle);

	/* continue to find the end_handle */
	return BT_GATT_ITER_CONTINUE;
}

static void att_find_type_rsp(struct bt_conn *conn, uint16_t start_handle,
			      uint16_t end_handle, const void *value,
			      uint8_t value_len)
{
	struct find_type_data data;

	memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_TYPE_RSP, 0);
	if (!data.buf) {
		return;
	}

	data.value = value;
	data.value_len = value_len;

	bt_gatt_foreach_attr(start_handle, end_handle, find_type_cb, &data);

	if (!data.group) {
		bt_buf_put(data.buf);
		send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return;
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_ATT, data.buf);
}

static void att_find_type_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_find_type_req *req;
	uint16_t start_handle, end_handle, err_handle, type;
	uint8_t *value;

	if (data->len < sizeof(*req)) {
		send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);
	type = sys_le16_to_cpu(req->type);
	value = bt_buf_pull(data, sizeof(*req));

	BT_DBG("start_handle %u end_handle %u type %u\n", start_handle,
	       end_handle, type);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return;
	}

	/* The Attribute Protocol Find By Type Value Request shall be used with
	 * the Attribute Type parameter set to the UUID for «Primary Service»
	 * and the Attribute Value set to the 16-bit Bluetooth UUID or 128-bit
	 * UUID for the specific primary service.
	 */
	if (type != BT_UUID_GATT_PRIMARY) {
		send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, start_handle,
				     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return;
	}

	att_find_type_rsp(conn, start_handle, end_handle, value, data->len);
}

static bool uuid_create(struct bt_uuid *uuid, struct bt_buf *data)
{
	if (data->len > sizeof(uuid->u128)) {
		return false;
	}

	switch (data->len) {
	case 2:
		uuid->type = BT_UUID_16;
		uuid->u16 = bt_buf_pull_le16(data);
		return true;
	case 16:
		uuid->type = BT_UUID_128;
		memcpy(uuid->u128, data->data, data->len);
		return true;
	}

	return false;
}

struct read_type_data {
	struct bt_conn *conn;
	struct bt_uuid *uuid;
	struct bt_buf *buf;
	struct bt_att_read_type_rsp *rsp;
	struct bt_att_data *item;
};

static uint8_t read_type_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_type_data *data = user_data;
	struct bt_att *att = data->conn->att;
	int read;

	/* Skip if doesn't match */
	if (bt_uuid_cmp(attr->uuid, data->uuid)) {
		return BT_GATT_ITER_CONTINUE;
	}

	BT_DBG("handle %u\n", attr->handle);

	/* Fast foward to next item position */
	data->item = bt_buf_add(data->buf, sizeof(*data->item));
	data->item->handle = sys_cpu_to_le16(attr->handle);

	/* Read attribute value and store in the buffer */
	read = attr->read(attr, data->buf->data + data->buf->len,
			  att->mtu - data->buf->len, 0);
	if (read < 0) {
		/* TODO: Handle read errors */
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

	bt_buf_add(data->buf, read);

	/* return true only if there are still space for more items */
	return att->mtu - data->buf->len > data->rsp->len ?
	       BT_GATT_ITER_CONTINUE : BT_GATT_ITER_STOP;
}

static void att_read_type_rsp(struct bt_conn *conn, struct bt_uuid *uuid,
			       uint16_t start_handle, uint16_t end_handle)
{
	struct read_type_data data;

	memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_TYPE_RSP,
				     sizeof(*data.rsp));
	if (!data.buf) {
		return;
	}

	data.uuid = uuid;
	data.rsp = bt_buf_add(data.buf, sizeof(*data.rsp));
	data.rsp->len = 0;

	bt_gatt_foreach_attr(start_handle, end_handle, read_type_cb, &data);

	if (!data.rsp->len) {
		bt_buf_put(data.buf);
		send_err_rsp(conn, BT_ATT_OP_READ_TYPE_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return;
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_ATT, data.buf);
}

static void att_read_type_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_read_type_req *req;
	uint16_t start_handle, end_handle, err_handle;
	struct bt_uuid uuid;

	/* Type can only be UUID16 or UUID128 */
	if (data->len != sizeof(*req) + sizeof(uuid.u16) &&
	    data->len != sizeof(*req) + sizeof(uuid.u128)) {
		send_err_rsp(conn, BT_ATT_OP_READ_TYPE_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);
	bt_buf_pull(data, sizeof(*req));

	if (!uuid_create(&uuid, data)) {
		send_err_rsp(conn, BT_ATT_OP_READ_TYPE_REQ, 0,
			     BT_ATT_ERR_UNLIKELY);
		return;
	}

	BT_DBG("start_handle %u end_handle %u type %u\n",
	       start_handle, end_handle, uuid.u16);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(conn, BT_ATT_OP_READ_TYPE_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return;
	}

	att_read_type_rsp(conn, &uuid, start_handle, end_handle);
}

struct read_data {
	struct bt_conn *conn;
	uint16_t offset;
	struct bt_buf *buf;
	struct bt_att_read_rsp *rsp;
};

static uint8_t read_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_data *data = user_data;
	struct bt_att *att = data->conn->att;
	int read;

	BT_DBG("handle %u\n", attr->handle);

	data->rsp = bt_buf_add(data->buf, sizeof(*data->rsp));

	if (!attr->read) {
		/* TODO: Respond with BT_ATT_ERR_READ_NOT_PERMITTED */
		return BT_GATT_ITER_STOP;
	}

	/* Read attribute value and store in the buffer */
	read = attr->read(attr, data->buf->data + data->buf->len,
			  att->mtu - data->buf->len, data->offset);
	if (read < 0) {
		/* TODO: Handle read error */
		return BT_GATT_ITER_STOP;
	}

	bt_buf_add(data->buf, read);

	return BT_GATT_ITER_CONTINUE;
}

static void att_read_rsp(struct bt_conn *conn, uint8_t op, uint8_t rsp,
			 uint16_t handle, uint16_t offset)
{
	struct read_data data;

	if (!handle) {
		send_err_rsp(conn, op, 0, BT_ATT_ERR_INVALID_HANDLE);
		return;
	}

	memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, rsp, 0);
	if (!data.buf) {
		return;
	}

	data.offset = offset;

	bt_gatt_foreach_attr(handle, handle, read_cb, &data);

	if (!data.rsp) {
		bt_buf_put(data.buf);
		send_err_rsp(conn, op, handle, BT_ATT_ERR_INVALID_HANDLE);
		return;
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_ATT, data.buf);
}

static void att_read_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_read_req *req;
	uint16_t handle;

	if (data->len != sizeof(*req)) {
		send_err_rsp(conn, BT_ATT_OP_READ_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	handle = sys_le16_to_cpu(req->handle);

	BT_DBG("handle %u\n", handle);

	att_read_rsp(conn, BT_ATT_OP_READ_REQ, BT_ATT_OP_READ_RSP, handle, 0);
}

static void att_read_blob_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_read_blob_req *req;
	uint16_t handle, offset;

	if (data->len != sizeof(*req)) {
		send_err_rsp(conn, BT_ATT_OP_READ_BLOB_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	handle = sys_le16_to_cpu(req->handle);
	offset = sys_le16_to_cpu(req->offset);

	BT_DBG("handle %u offset %u\n", handle, offset);

	att_read_rsp(conn, BT_ATT_OP_READ_BLOB_REQ, BT_ATT_OP_READ_BLOB_RSP,
		     handle, offset);
}

static void att_read_mult_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_read_mult_req *req;
	uint16_t handle1, handle2;

	if (data->len < sizeof(*req)) {
		send_err_rsp(conn, BT_ATT_OP_READ_MULT_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	handle1 = sys_le16_to_cpu(req->handle1);
	handle2 = sys_le16_to_cpu(req->handle2);

	BT_DBG("handle1 %u handle2 %u ...\n", handle1, handle2);

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_READ_MULT_REQ, handle1,
		     BT_ATT_ERR_INVALID_HANDLE);
}

struct read_group_data {
	struct bt_conn *conn;
	struct bt_uuid *uuid;
	struct bt_buf *buf;
	struct bt_att_read_group_rsp *rsp;
	struct bt_att_group_data *group;
};

static uint8_t read_group_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct read_group_data *data = user_data;
	struct bt_att *att = data->conn->att;
	int read;

	/* If UUID don't match update group end_handle */
	if (bt_uuid_cmp(attr->uuid, data->uuid)) {
		if (data->group && attr->handle > data->group->end_handle) {
			data->group->end_handle = sys_cpu_to_le16(attr->handle);
		}
		return BT_GATT_ITER_CONTINUE;
	}

	BT_DBG("handle %u\n", attr->handle);

	/* Stop if there is no space left */
	if (data->rsp->len && att->mtu - data->buf->len < data->rsp->len)
		return BT_GATT_ITER_STOP;

	/* Fast foward to next group position */
	data->group = bt_buf_add(data->buf, sizeof(*data->group));

	/* Initialize group handle range */
	data->group->start_handle = sys_cpu_to_le16(attr->handle);
	data->group->end_handle = sys_cpu_to_le16(attr->handle);

	/* Read attribute value and store in the buffer */
	read = attr->read(attr, data->buf->data + data->buf->len,
			  att->mtu - data->buf->len, 0);
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

	bt_buf_add(data->buf, read);

	/* Continue to find the end handle */
	return BT_GATT_ITER_CONTINUE;
}

static void att_read_group_rsp(struct bt_conn *conn, struct bt_uuid *uuid,
			       uint16_t start_handle, uint16_t end_handle)
{
	struct read_group_data data;

	memset(&data, 0, sizeof(data));

	data.buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_GROUP_RSP,
				     sizeof(*data.rsp));
	if (!data.buf) {
		return;
	}

	data.conn = conn;
	data.uuid = uuid;
	data.rsp = bt_buf_add(data.buf, sizeof(*data.rsp));
	data.rsp->len = 0;

	bt_gatt_foreach_attr(start_handle, end_handle, read_group_cb, &data);

	if (!data.rsp->len) {
		bt_buf_put(data.buf);
		send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, start_handle,
			     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
		return;
	}

	bt_l2cap_send(conn, BT_L2CAP_CID_ATT, data.buf);
}

static void att_read_group_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_read_group_req *req;
	uint16_t start_handle, end_handle, err_handle;
	struct bt_uuid uuid;

	/* Type can only be UUID16 or UUID128 */
	if (data->len != sizeof(*req) + sizeof(uuid.u16) &&
	    data->len != sizeof(*req) + sizeof(uuid.u128)) {
		send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	start_handle = sys_le16_to_cpu(req->start_handle);
	end_handle = sys_le16_to_cpu(req->end_handle);
	bt_buf_pull(data, sizeof(*req));

	if (!uuid_create(&uuid, data)) {
		send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, 0,
			     BT_ATT_ERR_UNLIKELY);
		return;
	}

	BT_DBG("start_handle %u end_handle %u type %u\n",
	       start_handle, end_handle, uuid.u16);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, err_handle,
			     BT_ATT_ERR_INVALID_HANDLE);
		return;
	}

	/* Core v4.2, Vol 3, sec 2.5.3 Attribute Grouping:
	 * Not all of the grouping attributes can be used in the ATT
	 * Read By Group Type Request. The «Primary Service» and «Secondary
	 * Service» grouping types may be used in the Read By Group Type
	 * Request. The «Characteristic» grouping type shall not be used in
	 * the ATT Read By Group Type Request.
	 */
	if (bt_uuid_cmp(&uuid, &primary_uuid) &&
	    bt_uuid_cmp(&uuid, &secondary_uuid)) {
		send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, start_handle,
				     BT_ATT_ERR_UNSUPPORTED_GROUP_TYPE);
		return;
	}

	att_read_group_rsp(conn, &uuid, start_handle, end_handle);
}

static void att_write_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_write_req *req;
	uint16_t handle;

	if (data->len < sizeof(*req)) {
		send_err_rsp(conn, BT_ATT_OP_WRITE_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	handle = sys_le16_to_cpu(req->handle);
	bt_buf_pull(data, sizeof(*req));

	BT_DBG("handle %u\n", handle);

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_WRITE_REQ, handle,
		     BT_ATT_ERR_INVALID_HANDLE);
}

static void att_prepare_write_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_prepare_write_req *req;
	uint16_t handle, offset;

	if (data->len < sizeof(*req)) {
		send_err_rsp(conn, BT_ATT_OP_PREPARE_WRITE_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	handle = sys_le16_to_cpu(req->handle);
	offset = sys_le16_to_cpu(req->offset);

	BT_DBG("handle %u offset %u\n", handle);

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_PREPARE_WRITE_REQ, handle,
		     BT_ATT_ERR_INVALID_HANDLE);
}

static void att_exec_write_req(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_exec_write_req *req;

	if (data->len != sizeof(*req)) {
		send_err_rsp(conn, BT_ATT_OP_EXEC_WRITE_REQ, 0,
			     BT_ATT_ERR_INVALID_PDU);
		return;
	}

	req = (void *)data->data;

	BT_DBG("flags %u\n", req->flags);

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_EXEC_WRITE_REQ, 0,
		     BT_ATT_ERR_NOT_SUPPORTED);
}

static void att_write_cmd(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_write_cmd *req;
	uint16_t handle;

	if (data->len < sizeof(*req)) {
		return;
	}

	req = (void *)data->data;

	handle = sys_le16_to_cpu(req->handle);

	BT_DBG("handle %u\n", handle);

	/* TODO: Perform write once database is defined */
}

static void att_signed_write_cmd(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_signed_write_cmd *req;
	uint16_t handle;

	if (data->len < sizeof(*req) + sizeof(struct bt_att_signature)) {
		return;
	}

	req = (void *)data->data;

	handle = sys_le16_to_cpu(req->handle);
	bt_buf_pull(data, sizeof(*req));

	BT_DBG("handle %u\n", handle);

	/* TODO: Perform write once database is defined */
}

static void bt_att_recv(struct bt_conn *conn, struct bt_buf *buf)
{
	struct bt_att_hdr *hdr = (void *)buf->data;

	if (buf->len < sizeof(*hdr)) {
		BT_ERR("Too small ATT PDU received\n");
		goto done;
	}

	BT_DBG("Received ATT code %u len %u\n", hdr->code, buf->len);

	bt_buf_pull(buf, sizeof(*hdr));

	switch (hdr->code) {
	case BT_ATT_OP_MTU_REQ:
		att_mtu_req(conn, buf);
		break;
	case BT_ATT_OP_FIND_INFO_REQ:
		att_find_info_req(conn, buf);
		break;
	case BT_ATT_OP_FIND_TYPE_REQ:
		att_find_type_req(conn, buf);
		break;
	case BT_ATT_OP_READ_TYPE_REQ:
		att_read_type_req(conn, buf);
		break;
	case BT_ATT_OP_READ_REQ:
		att_read_req(conn, buf);
		break;
	case BT_ATT_OP_READ_BLOB_REQ:
		att_read_blob_req(conn, buf);
		break;
	case BT_ATT_OP_READ_MULT_REQ:
		att_read_mult_req(conn, buf);
		break;
	case BT_ATT_OP_READ_GROUP_REQ:
		att_read_group_req(conn, buf);
		break;
	case BT_ATT_OP_WRITE_REQ:
		att_write_req(conn, buf);
		break;
	case BT_ATT_OP_PREPARE_WRITE_REQ:
		att_prepare_write_req(conn, buf);
		break;
	case BT_ATT_OP_EXEC_WRITE_REQ:
		att_exec_write_req(conn, buf);
		break;
	case BT_ATT_OP_WRITE_CMD:
		att_write_cmd(conn, buf);
		break;
	case BT_ATT_OP_SIGNED_WRITE_CMD:
		att_signed_write_cmd(conn, buf);
		break;
	default:
		BT_WARN("Unhandled ATT code %u\n", hdr->code);
		send_err_rsp(conn, hdr->code, 0, BT_ATT_ERR_NOT_SUPPORTED);
		break;
	}

done:
	bt_buf_put(buf);
}

struct bt_buf *bt_att_create_pdu(struct bt_conn *conn, uint8_t op, size_t len)
{
	struct bt_att_hdr *hdr;
	struct bt_buf *buf;

	buf = bt_l2cap_create_pdu(conn);
	if (!buf) {
		return NULL;
	}

	hdr = bt_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}

static void bt_att_connected(struct bt_conn *conn)
{
	int i;

	BT_DBG("conn %p handle %u\n", conn, conn->handle);

	for (i = 0; i < ARRAY_SIZE(bt_att_pool); i++) {
		struct bt_att *att = &bt_att_pool[i];

		if (!att->conn) {
			att->conn = conn;
			conn->att = att;
			att->mtu = BT_ATT_DEFAULT_LE_MTU;
			return;
		}
	}

	BT_ERR("No available ATT context for conn %p\n", conn);
}

static void bt_att_disconnected(struct bt_conn *conn)
{
	struct bt_att *att = conn->att;

	if (!att) {
		return;
	}

	BT_DBG("conn %p handle %u\n", conn, conn->handle);

	conn->att = NULL;
	memset(att, 0, sizeof(*att));
}

void bt_att_init(void)
{
	static struct bt_l2cap_chan chan = {
		.cid		= BT_L2CAP_CID_ATT,
		.recv		= bt_att_recv,
		.connected	= bt_att_connected,
		.disconnected	= bt_att_disconnected,
	};

	bt_l2cap_chan_register(&chan);
}
