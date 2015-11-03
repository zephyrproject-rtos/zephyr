/* gatt.c - Bluetooth GATT Server Tester */

/*
 * Copyright (c) 2015 Intel Corporation
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

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <toolchain.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>
#include <misc/byteorder.h>
#include <misc/printk.h>

#include "bttester.h"

#define CONTROLLER_INDEX 0
#define MAX_ATTRIBUTES 20
#define MAX_BUFFER_SIZE 512

#define GATT_PERM_ENC_READ_MASK		(BT_GATT_PERM_READ_ENCRYPT | \
					 BT_GATT_PERM_READ_AUTHEN)
#define GATT_PERM_ENC_WRITE_MASK	(BT_GATT_PERM_WRITE_ENCRYPT | \
					 BT_GATT_PERM_WRITE_AUTHEN)

static struct bt_gatt_attr gatt_db[MAX_ATTRIBUTES];

static struct {
	uint16_t len;
	uint8_t buf[MAX_BUFFER_SIZE];
} gatt_buf;

static void *gatt_buf_reserve(size_t len)
{
	void *ptr;

	if ((len + gatt_buf.len) > ARRAY_SIZE(gatt_buf.buf)) {
		return NULL;
	}

	ptr = memset(gatt_buf.buf + gatt_buf.len, 0, len);
	gatt_buf.len += len;

	printk("gatt_buf: %d/%d used\n", gatt_buf.len, MAX_BUFFER_SIZE);

	return ptr;
}

static void *gatt_buf_add(const void *data, size_t len)
{
	void *ptr;

	if ((len + gatt_buf.len) > ARRAY_SIZE(gatt_buf.buf)) {
		return NULL;
	}

	ptr = memcpy(gatt_buf.buf + gatt_buf.len, data, len);
	gatt_buf.len += len;

	printk("gatt_buf: %d/%d used\n", gatt_buf.len, MAX_BUFFER_SIZE);

	return ptr;
}

static struct bt_gatt_attr *gatt_db_add(const struct bt_gatt_attr *pattern)
{
	static struct bt_gatt_attr *attr = gatt_db;

	/* Return NULL if gatt_db is full */
	if (attr == &gatt_db[ARRAY_SIZE(gatt_db)]) {
		return NULL;
	}

	memcpy(attr, pattern, sizeof(*attr));

	/* Register attribute in GATT database, this will assign it a handle */
	if (bt_gatt_register(attr, 1)) {
		return NULL;
	}

	printk("gatt_db: attribute added, handle %x\n", attr->handle);

	return attr++;
}

/* Convert UUID from BTP command to bt_uuid */
static uint8_t btp2bt_uuid(const uint8_t *uuid, uint8_t len,
			   struct bt_uuid *bt_uuid)
{
	switch (len) {
	case 0x02: { /* UUID 16 */
		uint16_t u16;

		bt_uuid->type = BT_UUID_16;
		memcpy(&u16, uuid, sizeof(u16));
		bt_uuid->u16 = sys_le16_to_cpu(u16);
		break;
	}
	case 0x10: /* UUID 128*/
		bt_uuid->type = BT_UUID_128;
		memcpy(&bt_uuid->u128, uuid, sizeof(bt_uuid->u128));
		break;
	default:
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static void supported_commands(uint8_t *data, uint16_t len)
{
	uint16_t cmds;
	struct gatt_read_supported_commands_rp *rp = (void *) &cmds;

	cmds = 1 << GATT_READ_SUPPORTED_COMMANDS;
	cmds |= 1 << GATT_ADD_SERVICE;
	cmds |= 1 << GATT_ADD_CHARACTERISTIC;
	cmds |= 1 << GATT_ADD_DESCRIPTOR;
	cmds |= 1 << GATT_ADD_INCLUDED_SERVICE;
	cmds |= 1 << GATT_SET_VALUE;
	cmds |= 1 << GATT_START_SERVER;

	tester_rsp_full(BTP_SERVICE_ID_GATT, GATT_READ_SUPPORTED_COMMANDS,
			CONTROLLER_INDEX, (uint8_t *) rp, sizeof(cmds));
}

static struct bt_gatt_attr svc_pri = BT_GATT_PRIMARY_SERVICE(NULL);
static struct bt_gatt_attr svc_sec = BT_GATT_SECONDARY_SERVICE(NULL);

static void add_service(uint8_t *data, uint16_t len)
{
	const struct gatt_add_service_cmd *cmd = (void *) data;
	struct gatt_add_service_rp rp;
	struct bt_gatt_attr *attr_svc;
	struct bt_uuid uuid;

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid)) {
		goto fail;
	}

	switch (cmd->type) {
	case GATT_SERVICE_PRIMARY:
		attr_svc = gatt_db_add(&svc_pri);
		break;
	case GATT_SERVICE_SECONDARY:
		attr_svc = gatt_db_add(&svc_sec);
		break;
	default:
		goto fail;
	}

	if (!attr_svc) {
		goto fail;
	}

	attr_svc->user_data = gatt_buf_add(&uuid, sizeof(uuid));
	if (!attr_svc->user_data) {
		goto fail;
	}

	rp.svc_id = sys_cpu_to_le16(attr_svc->handle);

	tester_rsp_full(BTP_SERVICE_ID_GATT, GATT_ADD_SERVICE,
			CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));

	return;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_ADD_SERVICE, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

struct gatt_value {
	uint16_t len;
	uint8_t *data;
	uint8_t *prep_data;
	uint8_t enc_key_size;
};

static int read_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		      void *buf, uint16_t len, uint16_t offset)
{
	const struct gatt_value *value = attr->user_data;

	if ((attr->perm & GATT_PERM_ENC_READ_MASK) &&
	    (value->enc_key_size > bt_conn_enc_key_size(conn))) {
		return -EACCES;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,
				 value->len);
}

static int write_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		       const void *buf, uint16_t len, uint16_t offset)
{
	struct gatt_value *value = attr->user_data;

	if ((attr->perm & GATT_PERM_ENC_WRITE_MASK) &&
	    (value->enc_key_size > bt_conn_enc_key_size(conn))) {
		return -EACCES;
	}

	/*
	 * If the prepare Value Offset is greater than the current length of
	 * the attribute value Error Response shall be sent with the
	 * «Invalid Offset».
	 */
	if (offset > value->len) {
		return -EINVAL;
	}

	if (offset + len > value->len) {
		return -EFBIG;
	}

	memcpy(value->prep_data + offset, buf, len);

	return len;
}

static int flush_value(struct bt_conn *conn,
		       const struct bt_gatt_attr *attr, uint8_t flags)
{
	struct gatt_value *value = attr->user_data;

	switch (flags) {
	case BT_GATT_FLUSH_SYNC:
		/* Sync buffer to data */
		memcpy(value->data, value->prep_data, value->len);
		/* Fallthrough */
	case BT_GATT_FLUSH_DISCARD:
		memset(value->prep_data, 0, value->len);
		return 0;
	}

	return -EINVAL;
}

static struct bt_gatt_attr chr = BT_GATT_CHARACTERISTIC(NULL, 0);
static struct bt_gatt_attr chr_val = BT_GATT_LONG_DESCRIPTOR(NULL, 0,
							     read_value,
							     write_value,
							     flush_value, NULL);

static uint8_t add_characteristic_cb(const struct bt_gatt_attr *attr,
				     void *user_data)
{
	const struct gatt_add_characteristic_cmd *cmd = user_data;
	struct gatt_add_characteristic_rp rp;
	struct bt_gatt_attr *attr_chrc, *attr_value;
	struct bt_gatt_chrc chrc;
	struct bt_uuid uuid;

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid)) {
		goto fail;
	}

	attr_chrc = gatt_db_add(&chr);
	if (!attr_chrc) {
		goto fail;
	}

	attr_value = gatt_db_add(&chr_val);
	if (!attr_value) {
		goto fail;
	}

	chrc.properties = cmd->properties;
	chrc.uuid = gatt_buf_add(&uuid, sizeof(uuid));
	if (!chrc.uuid) {
		goto fail;
	}

	attr_chrc->user_data = gatt_buf_add(&chrc, sizeof(chrc));
	if (!attr_chrc->user_data) {
		goto fail;
	}

	attr_value->uuid = chrc.uuid;
	attr_value->perm = cmd->permissions;

	rp.char_id = sys_cpu_to_le16(attr_chrc->handle);
	tester_rsp_full(BTP_SERVICE_ID_GATT, GATT_ADD_CHARACTERISTIC,
			CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));

	return BT_GATT_ITER_STOP;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_ADD_CHARACTERISTIC,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);

	return BT_GATT_ITER_STOP;
}

static void add_characteristic(uint8_t *data, uint16_t len)
{
	const struct gatt_add_characteristic_cmd *cmd = (void *) data;
	uint16_t handle = sys_le16_to_cpu(cmd->svc_id);

	/* TODO Return error if no attribute found */
	bt_gatt_foreach_attr(handle, handle, add_characteristic_cb, data);
}

static struct bt_gatt_attr ccc = BT_GATT_CCC(NULL, NULL);
static struct bt_gatt_attr cep = BT_GATT_CEP(NULL);
static struct bt_gatt_attr *dsc = &chr_val;

static uint8_t add_descriptor_cb(const struct bt_gatt_attr *attr,
				 void *user_data)
{
	const struct gatt_add_descriptor_cmd *cmd = user_data;
	struct gatt_add_descriptor_rp rp;
	struct bt_gatt_attr *attr_desc;
	struct bt_uuid uuid;

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid)) {
		goto fail;
	}

	if (!bt_uuid_cmp(&uuid, cep.uuid)) {
		/* TODO Add CEP descriptor */
		attr_desc = NULL;
	} else if (!bt_uuid_cmp(&uuid, ccc.uuid)) {
		/* TODO Add CCC descriptor */
		attr_desc = NULL;
	} else {
		attr_desc = gatt_db_add(dsc);
	}

	if (!attr_desc) {
		goto fail;
	}

	/* CCC and CEP have permissions already set */
	if (!attr_desc->perm) {
		attr_desc->perm = cmd->permissions;
	}

	/* CCC and CEP have UUID already set */
	if (!attr_desc->uuid) {
		attr_desc->uuid = gatt_buf_add(&uuid, sizeof(uuid));
		if (!attr_desc->uuid) {
			goto fail;
		}
	}

	rp.desc_id = sys_cpu_to_le16(attr_desc->handle);

	tester_rsp_full(BTP_SERVICE_ID_GATT, GATT_ADD_DESCRIPTOR,
			CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));

	return BT_GATT_ITER_STOP;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_ADD_DESCRIPTOR, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);

	return BT_GATT_ITER_STOP;
}

static void add_descriptor(uint8_t *data, uint16_t len)
{
	const struct gatt_add_descriptor_cmd *cmd = (void *) data;
	uint16_t handle = sys_le16_to_cpu(cmd->char_id);

	/* TODO Return error if no attribute found */
	bt_gatt_foreach_attr(handle, handle, add_descriptor_cb, data);
}

static uint8_t get_service_handles(const struct bt_gatt_attr *attr,
				   void *user_data)
{
	struct bt_gatt_include *include = user_data;

	/*
	 * The first attribute found is service declaration.
	 * Preset end handle - next attribute can be a service.
	 */
	if (!include->start_handle) {
		include->start_handle = attr->handle;
		include->end_handle = attr->handle;

		return BT_GATT_ITER_CONTINUE;
	}

	/* Stop if attribute is a service */
	if (!bt_uuid_cmp(attr->uuid, svc_pri.uuid) ||
	    !bt_uuid_cmp(attr->uuid, svc_sec.uuid)) {
		return BT_GATT_ITER_STOP;
	}

	include->end_handle = attr->handle;

	return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_attr svc_inc = BT_GATT_INCLUDE_SERVICE(NULL);

static uint8_t add_included_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct gatt_add_included_service_rp rp;
	struct bt_gatt_attr *attr_incl;
	struct bt_gatt_include include;

	/* Fail if attribute stored under requested handle is not a service */
	if (bt_uuid_cmp(attr->uuid, svc_pri.uuid) &&
	    bt_uuid_cmp(attr->uuid, svc_sec.uuid)) {
		goto fail;
	}

	attr_incl = gatt_db_add(&svc_inc);
	if (!attr_incl) {
		goto fail;
	}

	include.uuid = attr->user_data;
	include.start_handle = 0;

	attr_incl->user_data = gatt_buf_add(&include, sizeof(include));
	if (!attr_incl->user_data) {
		goto fail;
	}

	/* Lookup for service end handle */
	bt_gatt_foreach_attr(attr->handle, 0xffff, get_service_handles,
			     attr_incl->user_data);

	rp.included_service_id = sys_cpu_to_le16(attr_incl->handle);

	tester_rsp_full(BTP_SERVICE_ID_GATT, GATT_ADD_CHARACTERISTIC,
			CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));

	return BT_GATT_ITER_STOP;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_ADD_CHARACTERISTIC,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);

	return BT_GATT_ITER_STOP;
}

static void add_included(uint8_t *data, uint16_t len)
{
	const struct gatt_add_included_service_cmd *cmd = (void *) data;
	uint16_t handle = sys_le16_to_cpu(cmd->svc_id);

	/* TODO Return error if no attribute found */
	bt_gatt_foreach_attr(handle, handle, add_included_cb, data);
}

static uint8_t set_value_cb(struct bt_gatt_attr *attr, void *user_data)
{
	const struct gatt_set_value_cmd *cmd = user_data;
	struct gatt_value value;
	uint8_t status;

	if (!bt_uuid_cmp(attr->uuid, chr.uuid)) {
		attr = attr->_next;
		if (!attr) {
			status = BTP_STATUS_FAILED;
			goto rsp;
		}
	}

	value.len = sys_le16_to_cpu(cmd->len);

	/* Check if attribute value has been already set */
	if (attr->user_data) {
		struct gatt_value *gatt_value = attr->user_data;

		/* Fail if value length doesn't match  */
		if (value.len != gatt_value->len) {
			status = BTP_STATUS_FAILED;
			goto rsp;
		}

		memcpy(gatt_value->data, cmd->value, gatt_value->len);

		/* TODO Send notification */

		status = BTP_STATUS_SUCCESS;
		goto rsp;
	}

	value.data = gatt_buf_add(cmd->value, value.len);
	if (!value.data) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	value.prep_data = gatt_buf_reserve(value.len);
	if (!value.prep_data) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	value.enc_key_size = 0x00;

	attr->user_data = gatt_buf_add(&value, sizeof(value));
	if (!attr->user_data) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	status = BTP_STATUS_SUCCESS;
rsp:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_SET_VALUE, CONTROLLER_INDEX,
		   status);

	return BT_GATT_ITER_STOP;
}

static void set_value(uint8_t *data, uint16_t len)
{
	const struct gatt_set_value_cmd *cmd = (void *) data;
	uint16_t handle = sys_le16_to_cpu(cmd->attr_id);

	/* TODO Return error if no attribute found */
	bt_gatt_foreach_attr(handle, handle, (bt_gatt_attr_func_t) set_value_cb,
			     data);
}

static void start_server(uint8_t *data, uint16_t len)
{
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_START_SERVER,
		   CONTROLLER_INDEX, BTP_STATUS_SUCCESS);
}

static uint8_t set_enc_key_size_cb(const struct bt_gatt_attr *attr,
				   void *user_data)
{
	const struct gatt_set_enc_key_size_cmd *cmd = user_data;
	struct gatt_value *value;
	uint8_t status;

	/* Fail if requested key size is invalid */
	if (cmd->key_size < 0x07 || cmd->key_size > 0x0f) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	/* Fail if requested attribute is a service */
	if (!bt_uuid_cmp(attr->uuid, svc_pri.uuid) ||
	    !bt_uuid_cmp(attr->uuid, svc_sec.uuid) ||
	    !bt_uuid_cmp(attr->uuid, svc_inc.uuid)) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	/* Lookup for characteristic value attribute */
	if (!bt_uuid_cmp(attr->uuid, chr.uuid)) {
		attr = attr->_next;
		if (!attr) {
			status = BTP_STATUS_FAILED;
			goto rsp;
		}
	}

	/* Fail if permissions are not set */
	if (!(attr->perm & (GATT_PERM_ENC_READ_MASK |
			    GATT_PERM_ENC_WRITE_MASK))) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	/* Fail if there is no attribute value */
	if (!attr->user_data) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	value = attr->user_data;
	value->enc_key_size = cmd->key_size;

	status = BTP_STATUS_SUCCESS;
rsp:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_SET_ENC_KEY_SIZE, CONTROLLER_INDEX,
		   status);

	return BT_GATT_ITER_STOP;
}

static void set_enc_key_size(uint8_t *data, uint16_t len)
{
	const struct gatt_set_enc_key_size_cmd *cmd = (void *) data;
	uint16_t handle = sys_le16_to_cpu(cmd->attr_id);

	/* TODO Return error if no attribute found */
	bt_gatt_foreach_attr(handle, handle, set_enc_key_size_cb, data);
}

static void exchange_mtu_rsp(struct bt_conn *conn, uint8_t err)
{
	/* NOP */
}

static void exchange_mtu(uint8_t *data, uint16_t len)
{
	struct bt_conn *conn;
	uint8_t status;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	if (bt_gatt_exchange_mtu(conn, exchange_mtu_rsp) < 0) {
		status = BTP_STATUS_FAILED;
	} else {
		status = BTP_STATUS_SUCCESS;
	}

	bt_conn_unref(conn);
rsp:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_EXCHANGE_MTU, CONTROLLER_INDEX,
		   status);
}

void tester_handle_gatt(uint8_t opcode, uint8_t index, uint8_t *data,
			 uint16_t len)
{
	switch (opcode) {
	case GATT_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		return;
	case GATT_ADD_SERVICE:
		add_service(data, len);
		return;
	case GATT_ADD_CHARACTERISTIC:
		add_characteristic(data, len);
		return;
	case GATT_ADD_DESCRIPTOR:
		add_descriptor(data, len);
		return;
	case GATT_ADD_INCLUDED_SERVICE:
		add_included(data, len);
		return;
	case GATT_SET_VALUE:
		set_value(data, len);
		return;
	case GATT_START_SERVER:
		start_server(data, len);
		return;
	case GATT_SET_ENC_KEY_SIZE:
		set_enc_key_size(data, len);
		return;
	case GATT_EXCHANGE_MTU:
		exchange_mtu(data, len);
		return;
	default:
		tester_rsp(BTP_SERVICE_ID_GATT, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

uint8_t tester_init_gatt(void)
{
	memset(&gatt_buf, 0, sizeof(gatt_buf));
	memset(&gatt_db, 0, sizeof(gatt_db));

	return BTP_STATUS_SUCCESS;
}
