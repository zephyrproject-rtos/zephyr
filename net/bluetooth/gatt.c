/* gatt.c - Generic Attribute Profile handling */

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
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/buf.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "hci_core.h"
#include "conn.h"
#include "l2cap.h"
#include "att.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_GATT)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

static const struct bt_gatt_attr *db = NULL;
static size_t attr_count = 0;

void bt_gatt_register(const struct bt_gatt_attr *attrs, size_t count)
{
	db = attrs;
	attr_count = count;
}

int bt_gatt_attr_read(const bt_addr_le_t *peer, const struct bt_gatt_attr *attr,
		      void *buf, uint8_t buf_len, uint16_t offset,
		      const void *value, uint8_t value_len)
{
	uint8_t len;

	if (offset > value_len) {
		return -EINVAL;
	}

	len = min(buf_len, value_len - offset);

	BT_DBG("handle %u offset %u length %u\n", attr->handle, offset, len);

	memcpy(buf, value + offset, len);

	return len;
}

int bt_gatt_attr_read_service(const bt_addr_le_t *peer,
			      const struct bt_gatt_attr *attr,
			      void *buf, uint8_t len, uint16_t offset)
{
	struct bt_uuid *uuid = attr->user_data;

	if (uuid->type == BT_UUID_16) {
		uint16_t uuid16 = sys_cpu_to_le16(uuid->u16);

		return bt_gatt_attr_read(peer, attr, buf, len, offset, &uuid16,
					 sizeof(uuid16));
	}

	return bt_gatt_attr_read(peer, attr, buf, len, offset, uuid->u128,
				 sizeof(uuid->u128));
}

struct gatt_incl {
	uint16_t start_handle;
	uint16_t end_handle;
	union {
		uint16_t uuid16;
		uint8_t  uuid[16];
	};
} __packed;

int bt_gatt_attr_read_include(const bt_addr_le_t *peer,
			      const struct bt_gatt_attr *attr,
			      void *buf, uint8_t len, uint16_t offset)
{
	struct bt_gatt_include *incl = attr->user_data;
	struct gatt_incl pdu;
	uint8_t value_len;

	pdu.start_handle = sys_cpu_to_le16(incl->start_handle);
	pdu.end_handle = sys_cpu_to_le16(incl->end_handle);
	value_len = sizeof(pdu.start_handle) + sizeof(pdu.end_handle);

	if (incl->uuid->type == BT_UUID_16) {
		pdu.uuid16 = sys_cpu_to_le16(incl->uuid->u16);
		value_len += sizeof(pdu.uuid16);
	} else {
		memcpy(pdu.uuid, incl->uuid->u128, sizeof(incl->uuid->u128));
		value_len += sizeof(incl->uuid->u128);
	}

	return bt_gatt_attr_read(peer, attr, buf, len, offset, &pdu, value_len);
}

struct gatt_chrc {
	uint8_t properties;
	uint16_t value_handle;
	union {
		uint16_t uuid16;
		uint8_t  uuid[16];
	};
} __packed;

int bt_gatt_attr_read_chrc(const bt_addr_le_t *peer,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint8_t len, uint16_t offset)
{
	struct bt_gatt_chrc *chrc = attr->user_data;
	struct gatt_chrc pdu;
	uint8_t value_len;

	pdu.properties = chrc->properties;
	pdu.value_handle = sys_cpu_to_le16(chrc->value_handle);
	value_len = sizeof(pdu.properties) + sizeof(pdu.value_handle);

	if (chrc->uuid->type == BT_UUID_16) {
		pdu.uuid16 = sys_cpu_to_le16(chrc->uuid->u16);
		value_len += sizeof(pdu.uuid16);
	} else {
		memcpy(pdu.uuid, chrc->uuid->u128, sizeof(chrc->uuid->u128));
		value_len += sizeof(chrc->uuid->u128);
	}

	return bt_gatt_attr_read(peer, attr, buf, len, offset, &pdu, value_len);
}

void bt_gatt_foreach_attr(uint16_t start_handle, uint16_t end_handle,
			  bt_gatt_attr_func_t func, void *user_data)
{
	size_t i;

	for (i = 0; i < attr_count; i++) {
		const struct bt_gatt_attr *attr = &db[i];

		/* Check if attribute handle is within range */
		if (attr->handle < start_handle || attr->handle > end_handle)
			continue;

		if (func(attr, user_data) == BT_GATT_ITER_STOP)
			break;
	}
}

int bt_gatt_attr_read_ccc(const bt_addr_le_t *peer,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint8_t len, uint16_t offset)
{
	struct _bt_gatt_ccc *ccc = attr->user_data;
	uint16_t value;
	size_t i;

	for (i = 0; i < ccc->cfg_len; i++) {
		if (bt_addr_le_cmp(&ccc->cfg[i].peer, peer)) {
			continue;
		}

		value = sys_cpu_to_le16(ccc->cfg[i].value);
		break;
	}

	/* Default to disable if there is no cfg for the peer */
	if (i == ccc->cfg_len) {
		value = 0x0000;
	}

	return bt_gatt_attr_read(peer, attr, buf, len, offset, &value,
				 sizeof(value));
}

static void gatt_ccc_changed(struct _bt_gatt_ccc *ccc)
{
	int i;
	uint16_t value = 0x0000;

	for (i = 0; i < ccc->cfg_len; i++) {
		if (ccc->cfg[i].value > value) {
			value = ccc->cfg[i].value;
		}
	}

	if (value != ccc->value) {
		ccc->value = value;
		ccc->cfg_changed(value);
	}
}

int bt_gatt_attr_write_ccc(const bt_addr_le_t *peer,
			   const struct bt_gatt_attr *attr, const void *buf,
			   uint8_t len, uint16_t offset)
{
	struct _bt_gatt_ccc *ccc = attr->user_data;
	struct bt_conn *conn;
	const uint16_t *data = buf;
	size_t i;

	if (len != sizeof(*data) || offset) {
		return -EINVAL;
	}

	conn = bt_conn_lookup_addr_le(peer);
	if (!conn) {
		BT_WARN("%s not connected", bt_addr_le_str(peer));
		return -ENOTCONN;
	}

	for (i = 0; i < ccc->cfg_len; i++) {
		/* Check for existing configuration */
		if (!bt_addr_le_cmp(&ccc->cfg[i].peer, peer)) {
			break;
		}
	}

	if (i == ccc->cfg_len) {
		for (i = 0; i < ccc->cfg_len; i++) {
			/* Check for unused configuration */
			if (!ccc->cfg[i].valid) {
				bt_addr_le_copy(&ccc->cfg[i].peer, peer);
				/* Only set valid if bonded */
				ccc->cfg[i].valid = conn->keys ? 1 : 0;
				break;
			}
		}

		if (i == ccc->cfg_len) {
			BT_WARN("No space to store CCC cfg\n");
			return -ENOMEM;
		}
	}

	ccc->cfg[i].value = sys_le16_to_cpu(*data);

	BT_DBG("handle %u value %u\n", attr->handle, ccc->cfg[i].value);

	/* Update cfg if don't match */
	if (ccc->cfg[i].value != ccc->value) {
		gatt_ccc_changed(ccc);
	}

	return len;
}

int bt_gatt_attr_read_cep(const bt_addr_le_t *peer,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint8_t len, uint16_t offset)
{
	struct bt_gatt_cep *value = attr->user_data;
	uint16_t props = sys_cpu_to_le16(value->properties);

	return bt_gatt_attr_read(peer, attr, buf, len, offset, &props,
				 sizeof(props));
}

struct notify_data {
	uint8_t handle;
	const void *data;
	size_t len;
};

static uint8_t notify_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct notify_data *data = user_data;
	struct bt_uuid uuid = { .type = BT_UUID_16, .u16 = BT_UUID_GATT_CCC };
	struct bt_uuid chrc = { .type = BT_UUID_16, .u16 = BT_UUID_GATT_CHRC };
	struct _bt_gatt_ccc *ccc;
	size_t i;

	if (bt_uuid_cmp(attr->uuid, &uuid)) {
		/* Stop if we reach the next characteristic */
		if (!bt_uuid_cmp(attr->uuid, &chrc)) {
			return BT_GATT_ITER_STOP;
		}
		return BT_GATT_ITER_CONTINUE;
	}

	/* Check attribute user_data must be of type struct _bt_gatt_ccc */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Notify all peers configured */
	for (i = 0; i < ccc->cfg_len; i++) {
		struct bt_conn *conn;
		struct bt_buf *buf;
		struct bt_att_notify *nfy;

		/* TODO: Handle indications */
		if (ccc->value != BT_GATT_CCC_NOTIFY) {
			continue;
		}

		conn = bt_conn_lookup_addr_le(&ccc->cfg[i].peer);
		if (!conn) {
			continue;
		}

		buf = bt_att_create_pdu(conn, BT_ATT_OP_NOTIFY,
					sizeof(*nfy) + data->len);
		if (!buf) {
			BT_WARN("No buffer available to send notification");
			return BT_GATT_ITER_STOP;
		}

		BT_DBG("conn %p handle %u\n", conn, data->handle);

		nfy = bt_buf_add(buf, sizeof(*nfy));
		nfy->handle = sys_cpu_to_le16(data->handle);

		bt_buf_add(buf, data->len);
		memcpy(nfy->value, data->data, data->len);

		bt_l2cap_send(conn, BT_L2CAP_CID_ATT, buf);
	}

	return BT_GATT_ITER_CONTINUE;
}

void bt_gatt_notify(uint16_t handle, const void *data, size_t len)
{
	struct notify_data nfy;

	nfy.handle = handle;
	nfy.data = data;
	nfy.len = len;

	bt_gatt_foreach_attr(handle, 0xffff, notify_cb, &nfy);
}
