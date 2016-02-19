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
#define MAX_ATTRIBUTES 50
#define MAX_BUFFER_SIZE 2048

#define GATT_PERM_ENC_READ_MASK		(BT_GATT_PERM_READ_ENCRYPT | \
					 BT_GATT_PERM_READ_AUTHEN)
#define GATT_PERM_ENC_WRITE_MASK	(BT_GATT_PERM_WRITE_ENCRYPT | \
					 BT_GATT_PERM_WRITE_AUTHEN)

static struct bt_gatt_attr gatt_db[MAX_ATTRIBUTES];

/*
 * gatt_buf - cache used by a gatt client (to cache data read/discovered)
 * and gatt server (to store attribute user_data).
 * It is not intended to be used by client and server at the same time.
 */
static struct {
	uint16_t len;
	uint8_t buf[MAX_BUFFER_SIZE];
} gatt_buf;

static void *gatt_buf_add(const void *data, size_t len)
{
	void *ptr = gatt_buf.buf + gatt_buf.len;

	if ((len + gatt_buf.len) > MAX_BUFFER_SIZE) {
		return NULL;
	}

	if (data) {
		memcpy(ptr, data, len);
	} else {
		memset(ptr, 0, len);
	}

	gatt_buf.len += len;

	BTTESTER_DBG("%d/%d used", gatt_buf.len, MAX_BUFFER_SIZE);

	return ptr;
}

static void *gatt_buf_reserve(size_t len)
{
	return gatt_buf_add(NULL, len);
}

static void gatt_buf_clear(void)
{
	memset(&gatt_buf, 0, sizeof(gatt_buf));
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

	BTTESTER_DBG("handle 0x%04x", attr->handle);

	return attr++;
}

/* Convert UUID from BTP command to bt_uuid */
static uint8_t btp2bt_uuid(const uint8_t *uuid, uint8_t len,
			   struct bt_uuid *bt_uuid)
{
	uint16_t le16;

	switch (len) {
	case 0x02: /* UUID 16 */
		bt_uuid->type = BT_UUID_TYPE_16;
		memcpy(&le16, uuid, sizeof(le16));
		BT_UUID_16(bt_uuid)->val = sys_le16_to_cpu(le16);
		break;
	case 0x10: /* UUID 128*/
		bt_uuid->type = BT_UUID_TYPE_128;
		memcpy(BT_UUID_128(bt_uuid)->val, uuid, 16);
		break;
	default:
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static void supported_commands(uint8_t *data, uint16_t len)
{
	uint64_t cmds[2];
	struct gatt_read_supported_commands_rp *rp = (void *) cmds;

	cmds[0] = BIT(GATT_READ_SUPPORTED_COMMANDS);
	cmds[0] |= BIT(GATT_ADD_SERVICE);
	cmds[0] |= BIT(GATT_ADD_CHARACTERISTIC);
	cmds[0] |= BIT(GATT_ADD_DESCRIPTOR);
	cmds[0] |= BIT(GATT_ADD_INCLUDED_SERVICE);
	cmds[0] |= BIT(GATT_SET_VALUE);
	cmds[0] |= BIT(GATT_START_SERVER);
	cmds[0] |= BIT(GATT_SET_ENC_KEY_SIZE);
	cmds[1] = BIT(GATT_EXCHANGE_MTU - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_DISC_PRIM_UUID - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_FIND_INCLUDED - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_DISC_ALL_CHRC - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_DISC_CHRC_UUID - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_DISC_ALL_DESC - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_READ - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_READ_LONG - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_READ_MULTIPLE - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_WRITE_WITHOUT_RSP - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_SIGNED_WRITE_WITHOUT_RSP - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_WRITE - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_WRITE_LONG - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_CFG_NOTIFY - GATT_CLIENT_OP_OFFSET);
	cmds[1] |= BIT(GATT_CFG_INDICATE - GATT_CLIENT_OP_OFFSET);

	tester_send(BTP_SERVICE_ID_GATT, GATT_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, (uint8_t *) rp, sizeof(cmds));
}

static struct bt_gatt_attr svc_pri = BT_GATT_PRIMARY_SERVICE(NULL);
static struct bt_gatt_attr svc_sec = BT_GATT_SECONDARY_SERVICE(NULL);

union uuid {
	struct bt_uuid uuid;
	struct bt_uuid_16 u16;
	struct bt_uuid_128 u128;
};

static void add_service(uint8_t *data, uint16_t len)
{
	const struct gatt_add_service_cmd *cmd = (void *) data;
	struct gatt_add_service_rp rp;
	struct bt_gatt_attr *attr_svc;
	union uuid uuid;

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
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

	tester_send(BTP_SERVICE_ID_GATT, GATT_ADD_SERVICE, CONTROLLER_INDEX,
		    (uint8_t *) &rp, sizeof(rp));

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
	bool has_ccc;
};

static ssize_t read_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t len, uint16_t offset)
{
	const struct gatt_value *value = attr->user_data;

	if ((attr->perm & GATT_PERM_ENC_READ_MASK) &&
	    (value->enc_key_size > bt_conn_enc_key_size(conn))) {
		return BT_GATT_ERR(BT_ATT_ERR_ENCRYPTION_KEY_SIZE);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,
				 value->len);
}

static ssize_t write_value(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, const void *buf,
			   uint16_t len, uint16_t offset)
{
	struct gatt_value *value = attr->user_data;

	if ((attr->perm & GATT_PERM_ENC_WRITE_MASK) &&
	    (value->enc_key_size > bt_conn_enc_key_size(conn))) {
		return BT_GATT_ERR(BT_ATT_ERR_ENCRYPTION_KEY_SIZE);
	}

	/*
	 * If the prepare Value Offset is greater than the current length of
	 * the attribute value Error Response shall be sent with the
	 * «Invalid Offset».
	 */
	if (offset > value->len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (offset + len > value->len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value->prep_data + offset, buf, len);

	return len;
}

static ssize_t flush_value(struct bt_conn *conn,
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

	return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
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
	union uuid uuid;

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
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
	tester_send(BTP_SERVICE_ID_GATT, GATT_ADD_CHARACTERISTIC,
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

static bool ccc_added;

static struct bt_gatt_ccc_cfg ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};

static void ccc_cfg_changed(uint16_t value)
{
	/* NOP */
}

static struct bt_gatt_attr ccc = BT_GATT_CCC(ccc_cfg, ccc_cfg_changed);

static struct bt_gatt_attr *add_ccc(const struct bt_gatt_attr *attr_chrc)
{
	struct bt_gatt_attr *attr_desc, *attr_value;
	struct bt_gatt_chrc *chrc = attr_chrc->user_data;
	struct gatt_value *value;

	/* Fail if another CCC already exist on server */
	if (ccc_added) {
		return NULL;
	}

	/* Check characteristic properties */
	if (!(chrc->properties &
	    (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE))) {
		return NULL;
	}

	/*
	 * Look for characteristic value (stored under next handle) to set
	 * 'has_ccc' flag
	 */
	attr_value = bt_gatt_attr_next(attr_chrc);
	if (!attr_value) {
		return NULL;
	}

	value = attr_value->user_data;
	if (!value) {
		return NULL;
	}

	/* Add CCC descriptor to GATT database */
	attr_desc = gatt_db_add(&ccc);
	if (!attr_desc) {
		return NULL;
	}

	value->has_ccc = true;
	ccc_added = true;

	return attr_desc;
}

static struct bt_gatt_attr cep = BT_GATT_CEP(NULL);

static struct bt_gatt_attr *add_cep(const struct bt_gatt_attr *attr_chrc)
{
	struct bt_gatt_attr *attr_desc;
	struct bt_gatt_chrc *chrc = attr_chrc->user_data;
	struct bt_gatt_cep cep_value;

	/* Extended Properties bit shall be set */
	if (!(chrc->properties & BT_GATT_CHRC_EXT_PROP)) {
		return NULL;
	}

	/* Add CEP descriptor to GATT database */
	attr_desc = gatt_db_add(&cep);
	if (!attr_desc) {
		return NULL;
	}

	attr_desc->user_data = gatt_buf_add(&cep_value, sizeof(cep_value));
	if (!attr_desc->user_data) {
		return NULL;
	}

	return attr_desc;
}

static struct bt_gatt_attr *dsc = &chr_val;

static uint8_t add_descriptor_cb(const struct bt_gatt_attr *attr,
				 void *user_data)
{
	const struct gatt_add_descriptor_cmd *cmd = user_data;
	struct gatt_add_descriptor_rp rp;
	struct bt_gatt_attr *attr_desc;
	union uuid uuid;

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
		goto fail;
	}

	if (!bt_uuid_cmp(&uuid.uuid, BT_UUID_GATT_CEP)) {
		attr_desc = add_cep(attr);
	} else if (!bt_uuid_cmp(&uuid.uuid, BT_UUID_GATT_CCC)) {
		attr_desc = add_ccc(attr);
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

	tester_send(BTP_SERVICE_ID_GATT, GATT_ADD_DESCRIPTOR, CONTROLLER_INDEX,
		    (uint8_t *) &rp, sizeof(rp));

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
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) ||
	    !bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
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
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) &&
	    bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
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

	tester_send(BTP_SERVICE_ID_GATT, GATT_ADD_CHARACTERISTIC,
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

static uint8_t set_ccc_value(struct bt_gatt_attr *attr, const void *value,
			     const uint16_t len)
{
	uint16_t ccc_val;

	if (len != sizeof(ccc_val)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(&ccc_val, value, sizeof(ccc_val));

	/*
	 * CCC Data has been already set, so we can only verify if the
	 * requested data is correct
	 */
	if (sys_le16_to_cpu(ccc_val) != 0) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_cep_value(struct bt_gatt_attr *attr, const void *value,
			     const uint16_t len)
{
	struct bt_gatt_cep *cep_value = attr->user_data;

	if (len != sizeof(cep_value->properties)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(&cep_value->properties, value, len);

	return BTP_STATUS_SUCCESS;
}

static uint8_t set_value_cb(struct bt_gatt_attr *attr, void *user_data)
{
	const struct gatt_set_value_cmd *cmd = user_data;
	struct gatt_value value;
	uint8_t status;

	/* Handle CCC value */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC)) {
		status = set_ccc_value(attr, cmd->value,
				       sys_le16_to_cpu(cmd->len));
		goto rsp;
	}

	/* Set CEP value */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CEP)) {
		status = set_cep_value(attr, cmd->value,
				       sys_le16_to_cpu(cmd->len));
		goto rsp;
	}

	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC)) {
		attr = bt_gatt_attr_next(attr);
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

		if (gatt_value->has_ccc) {
			bt_gatt_notify(NULL, attr, gatt_value->data,
				       gatt_value->len);
		}

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

	value.has_ccc = false;
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
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) ||
	    !bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY) ||
	    !bt_uuid_cmp(attr->uuid, BT_UUID_GATT_INCLUDE)) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	/* Lookup for characteristic value attribute */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC)) {
		attr = bt_gatt_attr_next(attr);
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
	if (err) {
		tester_rsp(BTP_SERVICE_ID_GATT, GATT_EXCHANGE_MTU,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);

		return;
	}

	tester_rsp(BTP_SERVICE_ID_GATT, GATT_EXCHANGE_MTU, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void exchange_mtu(uint8_t *data, uint16_t len)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail;
	}

	if (bt_gatt_exchange_mtu(conn, exchange_mtu_rsp) < 0) {
		bt_conn_unref(conn);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_EXCHANGE_MTU,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static struct bt_gatt_discover_params discover_params;
static union uuid uuid;
static uint8_t btp_opcode;

static void discover_destroy(struct bt_gatt_discover_params *params)
{
	memset(params, 0, sizeof(*params));
	gatt_buf_clear();
}

static uint8_t disc_prim_uuid_cb(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service *data;
	struct gatt_disc_prim_uuid_rp *rp = (void *) gatt_buf.buf;
	struct gatt_service *service;
	uint8_t uuid_length;

	if (!attr) {
		tester_send(BTP_SERVICE_ID_GATT, GATT_DISC_PRIM_UUID,
			    CONTROLLER_INDEX, gatt_buf.buf, gatt_buf.len);
		discover_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	data = attr->user_data;

	uuid_length = data->uuid->type == BT_UUID_TYPE_16 ? 2 : 16;

	service = gatt_buf_reserve(sizeof(*service) + uuid_length);
	if (!service) {
		tester_rsp(BTP_SERVICE_ID_GATT, GATT_DISC_PRIM_UUID,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		discover_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	service->start_handle = sys_cpu_to_le16(attr->handle);
	service->end_handle = sys_cpu_to_le16(data->end_handle);
	service->uuid_length = uuid_length;

	if (data->uuid->type == BT_UUID_TYPE_16) {
		uint16_t u16 = sys_cpu_to_le16(BT_UUID_16(data->uuid)->val);

		memcpy(service->uuid, &u16, uuid_length);
	} else {
		memcpy(service->uuid, BT_UUID_128(data->uuid)->val,
		       uuid_length);
	}

	rp->services_count++;

	return BT_GATT_ITER_CONTINUE;
}

static void disc_prim_uuid(uint8_t *data, uint16_t len)
{
	const struct gatt_disc_prim_uuid_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail_conn;
	}

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
		goto fail;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_disc_prim_uuid_rp))) {
		goto fail;
	}

	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.func = disc_prim_uuid_cb;

	if (bt_gatt_discover(conn, &discover_params) < 0) {
		discover_destroy(&discover_params);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_DISC_PRIM_UUID, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static uint8_t find_included_cb(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	struct bt_gatt_include *data;
	struct gatt_find_included_rp *rp = (void *) gatt_buf.buf;
	struct gatt_included *included;
	uint8_t uuid_length;

	if (!attr) {
		tester_send(BTP_SERVICE_ID_GATT, GATT_FIND_INCLUDED,
			    CONTROLLER_INDEX, gatt_buf.buf, gatt_buf.len);
		discover_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	data = attr->user_data;

	uuid_length = data->uuid->type == BT_UUID_TYPE_16 ? 2 : 16;

	included = gatt_buf_reserve(sizeof(*included) + uuid_length);
	if (!included) {
		tester_rsp(BTP_SERVICE_ID_GATT, GATT_FIND_INCLUDED,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		discover_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	included->included_handle = attr->handle;
	included->service.start_handle = sys_cpu_to_le16(data->start_handle);
	included->service.end_handle = sys_cpu_to_le16(data->end_handle);
	included->service.uuid_length = uuid_length;

	if (data->uuid->type == BT_UUID_TYPE_16) {
		uint16_t u16 = sys_cpu_to_le16(BT_UUID_16(data->uuid)->val);

		memcpy(included->service.uuid, &u16, uuid_length);
	} else {
		/* TODO Read this 128bit UUID */
		memset(included->service.uuid, 0, uuid_length);
	}

	rp->services_count++;

	return BT_GATT_ITER_CONTINUE;
}

static void find_included(uint8_t *data, uint16_t len)
{
	const struct gatt_find_included_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail_conn;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_find_included_rp))) {
		goto fail;
	}

	discover_params.start_handle = sys_le16_to_cpu(cmd->start_handle);
	discover_params.end_handle = sys_le16_to_cpu(cmd->end_handle);
	discover_params.type = BT_GATT_DISCOVER_INCLUDE;
	discover_params.func = find_included_cb;

	if (bt_gatt_discover(conn, &discover_params) < 0) {
		discover_destroy(&discover_params);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_FIND_INCLUDED, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static uint8_t disc_chrc_cb(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *data;
	struct gatt_disc_chrc_rp *rp = (void *) gatt_buf.buf;
	struct gatt_characteristic *chrc;
	uint8_t uuid_length;

	if (!attr) {
		tester_send(BTP_SERVICE_ID_GATT, btp_opcode,
			    CONTROLLER_INDEX, gatt_buf.buf, gatt_buf.len);
		discover_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	data = attr->user_data;

	uuid_length = data->uuid->type == BT_UUID_TYPE_16 ? 2 : 16;

	chrc = gatt_buf_reserve(sizeof(*chrc) + uuid_length);
	if (!chrc) {
		tester_rsp(BTP_SERVICE_ID_GATT, btp_opcode,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		discover_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	chrc->characteristic_handle = sys_cpu_to_le16(attr->handle);
	chrc->properties = data->properties;
	chrc->value_handle = sys_cpu_to_le16(attr->handle + 1);
	chrc->uuid_length = uuid_length;

	if (data->uuid->type == BT_UUID_TYPE_16) {
		uint16_t u16 = sys_cpu_to_le16(BT_UUID_16(data->uuid)->val);

		memcpy(chrc->uuid, &u16, uuid_length);
	} else {
		memcpy(chrc->uuid, BT_UUID_128(data->uuid)->val, uuid_length);
	}

	rp->characteristics_count++;

	return BT_GATT_ITER_CONTINUE;
}

static void disc_all_chrc(uint8_t *data, uint16_t len)
{
	const struct gatt_disc_all_chrc_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail_conn;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_disc_chrc_rp))) {
		goto fail;
	}

	discover_params.start_handle = sys_le16_to_cpu(cmd->start_handle);
	discover_params.end_handle = sys_le16_to_cpu(cmd->end_handle);
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	discover_params.func = disc_chrc_cb;

	/* TODO should be handled as user_data via CONTAINER_OF macro */
	btp_opcode = GATT_DISC_ALL_CHRC;

	if (bt_gatt_discover(conn, &discover_params) < 0) {
		discover_destroy(&discover_params);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_DISC_ALL_CHRC, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void disc_chrc_uuid(uint8_t *data, uint16_t len)
{
	const struct gatt_disc_chrc_uuid_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail_conn;
	}

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
		goto fail;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_disc_chrc_rp))) {
		goto fail;
	}

	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle = sys_le16_to_cpu(cmd->start_handle);
	discover_params.end_handle = sys_le16_to_cpu(cmd->end_handle);
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	discover_params.func = disc_chrc_cb;

	/* TODO should be handled as user_data via CONTAINER_OF macro */
	btp_opcode = GATT_DISC_CHRC_UUID;

	if (bt_gatt_discover(conn, &discover_params) < 0) {
		discover_destroy(&discover_params);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_DISC_CHRC_UUID, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static uint8_t disc_all_desc_cb(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	struct gatt_disc_all_desc_rp *rp = (void *) gatt_buf.buf;
	struct gatt_descriptor *descriptor;
	uint8_t uuid_length;

	if (!attr) {
		tester_send(BTP_SERVICE_ID_GATT, GATT_DISC_ALL_DESC,
			    CONTROLLER_INDEX, gatt_buf.buf, gatt_buf.len);
		discover_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	uuid_length = attr->uuid->type == BT_UUID_TYPE_16 ? 2 : 16;

	descriptor = gatt_buf_reserve(sizeof(*descriptor) + uuid_length);
	if (!descriptor) {
		tester_rsp(BTP_SERVICE_ID_GATT, GATT_DISC_ALL_DESC,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		discover_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	descriptor->descriptor_handle = sys_cpu_to_le16(attr->handle);
	descriptor->uuid_length = uuid_length;

	if (attr->uuid->type == BT_UUID_TYPE_16) {
		uint16_t u16 = sys_cpu_to_le16(BT_UUID_16(attr->uuid)->val);

		memcpy(descriptor->uuid, &u16, uuid_length);
	} else {
		memcpy(descriptor->uuid, BT_UUID_128(attr->uuid)->val,
		       uuid_length);
	}

	rp->descriptors_count++;

	return BT_GATT_ITER_CONTINUE;
}

static void disc_all_desc(uint8_t *data, uint16_t len)
{
	const struct gatt_disc_all_desc_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail_conn;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_disc_all_desc_rp))) {
		goto fail;
	}

	discover_params.start_handle = sys_le16_to_cpu(cmd->start_handle);
	discover_params.end_handle = sys_le16_to_cpu(cmd->end_handle);
	discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
	discover_params.func = disc_all_desc_cb;

	if (bt_gatt_discover(conn, &discover_params) < 0) {
		discover_destroy(&discover_params);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_DISC_ALL_DESC, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static struct bt_gatt_read_params read_params;

static void read_destroy(struct bt_gatt_read_params *params)
{
	memset(params, 0, sizeof(*params));
	gatt_buf_clear();
}

static uint8_t read_cb(struct bt_conn *conn, int err,
		       struct bt_gatt_read_params *params, const void *data,
		       uint16_t length)
{
	struct gatt_read_rp *rp = (void *) gatt_buf.buf;

	/* Respond to the Lower Tester with ATT Error received */
	if (err) {
		rp->att_response = err;
	}

	/* read complete */
	if (!data) {
		tester_send(BTP_SERVICE_ID_GATT, btp_opcode, CONTROLLER_INDEX,
			    gatt_buf.buf, gatt_buf.len);
		read_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	if (!gatt_buf_add(data, length)) {
		tester_rsp(BTP_SERVICE_ID_GATT, btp_opcode,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		read_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	rp->data_length += length;

	return BT_GATT_ITER_CONTINUE;
}

static void read(uint8_t *data, uint16_t len)
{
	const struct gatt_read_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail_conn;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_read_rp))) {
		goto fail;
	}

	read_params.handle_count = 1;
	read_params.single.handle = sys_le16_to_cpu(cmd->handle);
	read_params.single.offset = 0x0000;
	read_params.func = read_cb;

	/* TODO should be handled as user_data via CONTAINER_OF macro */
	btp_opcode = GATT_READ;

	if (bt_gatt_read(conn, &read_params) < 0) {
		read_destroy(&read_params);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_READ, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void read_long(uint8_t *data, uint16_t len)
{
	const struct gatt_read_long_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail_conn;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_read_rp))) {
		goto fail;
	}

	read_params.handle_count = 1;
	read_params.single.handle = sys_le16_to_cpu(cmd->handle);
	read_params.single.offset = sys_le16_to_cpu(cmd->offset);
	read_params.func = read_cb;

	/* TODO should be handled as user_data via CONTAINER_OF macro */
	btp_opcode = GATT_READ_LONG;

	if (bt_gatt_read(conn, &read_params) < 0) {
		read_destroy(&read_params);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_READ_LONG, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void read_multiple(uint8_t *data, uint16_t len)
{
	const struct gatt_read_multiple_cmd *cmd = (void *) data;
	uint16_t handles[cmd->handles_count];
	struct bt_conn *conn;
	int i;

	for (i = 0; i < ARRAY_SIZE(handles); i++) {
		handles[i] = sys_le16_to_cpu(cmd->handles[i]);
	}

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail_conn;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_read_rp))) {
		goto fail;
	}

	read_params.func = read_cb;
	read_params.handle_count = i;
	read_params.handles = handles; /* not used in read func */

	/* TODO should be handled as user_data via CONTAINER_OF macro */
	btp_opcode = GATT_READ_MULTIPLE;

	if (bt_gatt_read(conn, &read_params) < 0) {
		gatt_buf_clear();
		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_READ_MULTIPLE, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void write_without_rsp(uint8_t *data, uint16_t len, uint8_t op,
			      bool sign)
{
	const struct gatt_write_without_rsp_cmd *cmd = (void *) data;
	struct bt_conn *conn;
	uint8_t status = BTP_STATUS_SUCCESS;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	if (bt_gatt_write_without_response(conn, sys_le16_to_cpu(cmd->handle),
					   cmd->data,
					   sys_le16_to_cpu(cmd->data_length),
					   sign) < 0) {
		status = BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);
rsp:
	tester_rsp(BTP_SERVICE_ID_GATT, op, CONTROLLER_INDEX, status);
}

static void write_rsp(struct bt_conn *conn, uint8_t err)
{
	tester_send(BTP_SERVICE_ID_GATT, GATT_WRITE, CONTROLLER_INDEX, &err,
		    sizeof(err));
}

static void write(uint8_t *data, uint16_t len)
{
	const struct gatt_write_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail;
	}

	if (bt_gatt_write(conn, sys_le16_to_cpu(cmd->handle), 0, cmd->data,
			  sys_le16_to_cpu(cmd->data_length), write_rsp) < 0) {
		bt_conn_unref(conn);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_WRITE, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void write_long_rsp(struct bt_conn *conn, uint8_t err)
{
	tester_send(BTP_SERVICE_ID_GATT, GATT_WRITE_LONG, CONTROLLER_INDEX,
		    &err, sizeof(err));
}

static void write_long(uint8_t *data, uint16_t len)
{
	const struct gatt_write_long_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		goto fail;
	}

	if (bt_gatt_write(conn, sys_le16_to_cpu(cmd->handle),
			  sys_le16_to_cpu(cmd->offset), cmd->data,
			  sys_le16_to_cpu(cmd->data_length),
			  write_long_rsp) < 0) {
		bt_conn_unref(conn);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_WRITE_LONG, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static struct bt_gatt_subscribe_params subscribe_params;

/* ev header + default MTU_ATT-3 */
static uint8_t ev_buf[33];

static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	struct gatt_notification_ev *ev = (void *) ev_buf;
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);

	if (!data) {
		BTTESTER_DBG("Unsubscribed");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	ev->type = (uint8_t) subscribe_params.value;
	ev->handle = sys_cpu_to_le16(subscribe_params.value_handle);
	ev->data_length = sys_cpu_to_le16(length);
	memcpy(ev->data, data, length);
	memcpy(ev->address, addr->val, sizeof(ev->address));
	ev->address_type = addr->type;

	tester_send(BTP_SERVICE_ID_GATT, GATT_EV_NOTIFICATION,
		    CONTROLLER_INDEX, ev_buf, sizeof(*ev) + length);

	return BT_GATT_ITER_CONTINUE;
}

static void discover_complete(struct bt_conn *conn,
			      struct bt_gatt_discover_params *params)
{
	uint8_t op, status;

	/* If no value handle it means that chrc has not been found */
	if (!subscribe_params.value_handle) {
		status = BTP_STATUS_FAILED;
		goto fail;
	}

	if (bt_gatt_subscribe(conn, &subscribe_params) < 0) {
		status = BTP_STATUS_FAILED;
		goto fail;
	}

	status = BTP_STATUS_SUCCESS;
fail:
	op = subscribe_params.value == BT_GATT_CCC_NOTIFY ? GATT_CFG_NOTIFY :
							    GATT_CFG_INDICATE;

	if (status == BTP_STATUS_FAILED) {
		memset(&subscribe_params, 0, sizeof(subscribe_params));
	}

	tester_rsp(BTP_SERVICE_ID_GATT, op, CONTROLLER_INDEX, status);
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	if (!attr) {
		discover_complete(conn, params);
		return BT_GATT_ITER_STOP;
	}

	/* Characteristic Value Handle is the next handle beyond declaration */
	subscribe_params.value_handle = attr->handle + 1;

	/*
	 * Continue characteristic discovery to get last characteristic
	 * preceding this CCC descriptor
	 */
	return BT_GATT_ITER_CONTINUE;
}

static int enable_subscription(struct bt_conn *conn, uint16_t ccc_handle,
			       uint16_t value)
{
	/* Fail if there is another subscription enabled */
	if (subscribe_params.ccc_handle) {
		BTTESTER_DBG("Another subscription already enabled");
		return -EEXIST;
	}

	/* Discover Characteristic Value this CCC Descriptor refers to */
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = ccc_handle;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	discover_params.func = discover_func;

	subscribe_params.ccc_handle = ccc_handle;
	subscribe_params.value = value;
	subscribe_params.notify = notify_func;

	return bt_gatt_discover(conn, &discover_params);
}

static int disable_subscription(struct bt_conn *conn, uint16_t ccc_handle)
{
	/* Fail if CCC handle doesn't match */
	if (ccc_handle != subscribe_params.ccc_handle) {
		BTTESTER_DBG("CCC handle doesn't match");
		return -EINVAL;
	}

	if (bt_gatt_unsubscribe(conn, &subscribe_params) < 0) {
		return -EBUSY;
	}

	subscribe_params.ccc_handle = 0;

	return 0;
}

static void config_subscription(uint8_t *data, uint16_t len, uint16_t op)
{
	const struct gatt_cfg_notify_cmd *cmd = (void *) data;
	struct bt_conn *conn;
	uint16_t ccc_handle = sys_le16_to_cpu(cmd->ccc_handle);
	uint8_t status;

	conn = bt_conn_lookup_addr_le((bt_addr_le_t *) data);
	if (!conn) {
		tester_rsp(BTP_SERVICE_ID_GATT, op, CONTROLLER_INDEX,
			   BTP_STATUS_FAILED);
		return;
	}

	if (cmd->enable) {
		uint16_t value;

		if (op == GATT_CFG_NOTIFY) {
			value = BT_GATT_CCC_NOTIFY;
		} else {
			value = BT_GATT_CCC_INDICATE;
		}

		/* on success response will be sent from callback */
		if (enable_subscription(conn, ccc_handle, value) == 0) {
			bt_conn_unref(conn);
			return;
		}

		status = BTP_STATUS_FAILED;
	} else {
		if (disable_subscription(conn, ccc_handle) < 0) {
			status = BTP_STATUS_FAILED;
		} else {
			status = BTP_STATUS_SUCCESS;
		}
	}

	BTTESTER_DBG("Config subscription (op %u) status %u", op, status);

	bt_conn_unref(conn);
	tester_rsp(BTP_SERVICE_ID_GATT, op, CONTROLLER_INDEX, status);
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
	case GATT_DISC_PRIM_UUID:
		disc_prim_uuid(data, len);
		return;
	case GATT_FIND_INCLUDED:
		find_included(data, len);
		return;
	case GATT_DISC_ALL_CHRC:
		disc_all_chrc(data, len);
		return;
	case GATT_DISC_CHRC_UUID:
		disc_chrc_uuid(data, len);
		return;
	case GATT_DISC_ALL_DESC:
		disc_all_desc(data, len);
		return;
	case GATT_READ:
		read(data, len);
		return;
	case GATT_READ_LONG:
		read_long(data, len);
		return;
	case GATT_READ_MULTIPLE:
		read_multiple(data, len);
		return;
	case GATT_WRITE_WITHOUT_RSP:
		write_without_rsp(data, len, opcode, false);
		return;
	case GATT_SIGNED_WRITE_WITHOUT_RSP:
		write_without_rsp(data, len, opcode, true);
		return;
	case GATT_WRITE:
		write(data, len);
		return;
	case GATT_WRITE_LONG:
		write_long(data, len);
		return;
	case GATT_CFG_NOTIFY:
	case GATT_CFG_INDICATE:
		config_subscription(data, len, opcode);
		return;
	default:
		tester_rsp(BTP_SERVICE_ID_GATT, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

uint8_t tester_init_gatt(void)
{
	return BTP_STATUS_SUCCESS;
}
