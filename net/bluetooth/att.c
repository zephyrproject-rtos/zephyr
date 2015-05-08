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
#include <toolchain.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>

#include "hci_core.h"
#include "conn.h"
#include "l2cap.h"
#include "att.h"

static void send_err_rsp(struct bt_conn *conn, uint8_t req, uint16_t handle,
			 uint8_t err)
{
	struct bt_att_error_rsp *rsp;
	struct bt_buf *buf;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_ERROR_RSP, sizeof(*rsp));
	if (!buf) {
		return;
	}

	rsp = (void *)bt_buf_add(buf, sizeof(*rsp));
	rsp->request = req;
	rsp->handle = sys_cpu_to_le16(handle);
	rsp->error = err;

	bt_conn_send(conn, buf);
}

static void att_mtu_req(struct bt_conn *conn, struct bt_buf *data)
{
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

	/* TODO: Store the MTU negotiated */

	rsp = (void *)bt_buf_add(buf, sizeof(*rsp));
	rsp->mtu = sys_cpu_to_le16(mtu);

	bt_conn_send(conn, buf);
}

static bool range_is_valid(uint16_t start, uint16_t end, uint16_t *err)
{
	/* Handle 0 is invalid */
	if (!start || !end) {
		if (err)
			*err = 0;
		return false;
	}

	/* Check if range is valid */
	if (start > end) {
		if (err)
			*err = start;
		return false;
	}

	return true;
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

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_FIND_INFO_REQ, start_handle,
		     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	return;
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

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, start_handle,
		     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);

	return;
}

static bool uuid_create(struct bt_uuid *uuid, uint8_t *data, uint8_t len)
{
	uint16_t u16;

	if (len > sizeof(uuid->u128))
		return false;

	switch (len) {
	case 2:
		uuid->type = BT_UUID_16;
		/* TODO: Add unaligned helpers for these operations */
		memcpy(&u16, data, len);
		uuid->u16 = sys_le16_to_cpu(u16);
		return true;
	case 16:
		uuid->type = BT_UUID_128;
		memcpy(uuid->u128, data, len);
		return true;
	}

	return false;
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

	if (!uuid_create(&uuid, data->data, data->len)) {
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

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_READ_TYPE_REQ, start_handle,
		     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	return;
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

	if (!handle) {
		send_err_rsp(conn, BT_ATT_OP_READ_REQ, 0,
			     BT_ATT_ERR_INVALID_HANDLE);
		return;
	}

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_READ_REQ, handle,
		     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
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

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_READ_BLOB_REQ, handle,
		     BT_ATT_ERR_INVALID_HANDLE);
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

	if (!uuid_create(&uuid, data->data, data->len)) {
		send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, 0,
			     BT_ATT_ERR_UNLIKELY);
		return;
	}

	BT_DBG("start_handle %u end_handle %u type %u\n",
	       start_handle, end_handle, uuid.u16);

	if (!range_is_valid(start_handle, end_handle, &err_handle)) {
		send_err_rsp(conn, BT_ATT_OP_FIND_TYPE_REQ, err_handle,
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
	if (uuid.type == BT_UUID_16) {
		if (uuid.u16 != 0x2800 && uuid.u16 != 0x2801) {
			send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ,
				     start_handle,
				     BT_ATT_ERR_UNSUPPORTED_GROUP_TYPE);
			return;
		}
	} /* TODO: Add UUID helpers for UUID formats */

	/* TODO: Generate proper response once a database is defined */

	send_err_rsp(conn, BT_ATT_OP_READ_GROUP_REQ, start_handle,
		     BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
	return;
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

static void att_write_cmd(struct bt_conn *conn, struct bt_buf *data)
{
	struct bt_att_write_req *req;
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
	struct bt_att_write_req *req;
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

void bt_att_recv(struct bt_conn *conn, struct bt_buf *buf)
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
	case BT_ATT_OP_WRITE_CMD:
		att_write_cmd(conn, buf);
		break;
	case BT_ATT_OP_SIGNED_WRITE_CMD:
		att_signed_write_cmd(conn, buf);
		break;
	default:
		BT_DBG("Unhandled ATT code %u\n", hdr->code);
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

	buf = bt_l2cap_create_pdu(conn, BT_L2CAP_CID_ATT, sizeof(*hdr) + len);
	if (!buf) {
		return NULL;
	}

	hdr = (void *)bt_buf_add(buf, sizeof(*hdr));
	hdr->code = op;

	return buf;
}
