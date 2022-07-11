/* gatt.c - Bluetooth GATT Server Tester */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <string.h>
#include <errno.h>

#include <zephyr/toolchain.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/net/buf.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_gatt
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "bttester.h"

#define CONTROLLER_INDEX 0
#define MAX_BUFFER_SIZE 2048
#define MAX_UUID_LEN 16

#define MAX_SUBSCRIPTIONS 2

#define UNUSED_SUBSCRIBE_CCC_HANDLE 0x0000

/* This masks Permission bits from GATT API */
#define GATT_PERM_MASK			(BT_GATT_PERM_READ | \
					 BT_GATT_PERM_READ_AUTHEN | \
					 BT_GATT_PERM_READ_ENCRYPT | \
					 BT_GATT_PERM_WRITE | \
					 BT_GATT_PERM_WRITE_AUTHEN | \
					 BT_GATT_PERM_WRITE_ENCRYPT | \
					 BT_GATT_PERM_PREPARE_WRITE)
#define GATT_PERM_ENC_READ_MASK		(BT_GATT_PERM_READ_ENCRYPT | \
					 BT_GATT_PERM_READ_AUTHEN)
#define GATT_PERM_ENC_WRITE_MASK	(BT_GATT_PERM_WRITE_ENCRYPT | \
					 BT_GATT_PERM_WRITE_AUTHEN)
#define GATT_PERM_READ_AUTHORIZATION	0x40
#define GATT_PERM_WRITE_AUTHORIZATION	0x80

/* GATT server context */
#define SERVER_MAX_SERVICES		10
#define SERVER_MAX_ATTRIBUTES		50
#define SERVER_BUF_SIZE			2048

/* bt_gatt_attr_next cannot be used on non-registered services */
#define NEXT_DB_ATTR(attr) (attr + 1)
#define LAST_DB_ATTR (server_db + (attr_count - 1))

#define server_buf_push(_len)	net_buf_push(server_buf, ROUND_UP(_len, 4))
#define server_buf_pull(_len)	net_buf_pull(server_buf, ROUND_UP(_len, 4))

static struct bt_gatt_service server_svcs[SERVER_MAX_SERVICES];
static struct bt_gatt_attr server_db[SERVER_MAX_ATTRIBUTES];
static struct net_buf *server_buf;
NET_BUF_POOL_DEFINE(server_pool, 1, SERVER_BUF_SIZE, 0, NULL);

static uint8_t attr_count;
static uint8_t svc_attr_count;
static uint8_t svc_count;

/*
 * gatt_buf - cache used by a gatt client (to cache data read/discovered)
 * and gatt server (to store attribute user_data).
 * It is not intended to be used by client and server at the same time.
 */
static struct {
	uint16_t len;
	uint8_t buf[MAX_BUFFER_SIZE];
} gatt_buf;

struct get_attr_data {
	struct net_buf_simple *buf;
	struct bt_conn *conn;
};

static void *gatt_buf_add(const void *data, size_t len)
{
	void *ptr = gatt_buf.buf + gatt_buf.len;

	if ((len + gatt_buf.len) > MAX_BUFFER_SIZE) {
		return NULL;
	}

	if (data) {
		memcpy(ptr, data, len);
	} else {
		(void)memset(ptr, 0, len);
	}

	gatt_buf.len += len;

	LOG_DBG("%d/%d used", gatt_buf.len, MAX_BUFFER_SIZE);

	return ptr;
}

static void *gatt_buf_reserve(size_t len)
{
	return gatt_buf_add(NULL, len);
}

static void gatt_buf_clear(void)
{
	(void)memset(&gatt_buf, 0, sizeof(gatt_buf));
}

union uuid {
	struct bt_uuid uuid;
	struct bt_uuid_16 u16;
	struct bt_uuid_128 u128;
};

static struct bt_gatt_attr *gatt_db_add(const struct bt_gatt_attr *pattern,
					size_t user_data_len)
{
	static struct bt_gatt_attr *attr = server_db;
	const union uuid *u = CONTAINER_OF(pattern->uuid, union uuid, uuid);
	size_t uuid_size = u->uuid.type == BT_UUID_TYPE_16 ? sizeof(u->u16) :
							     sizeof(u->u128);

	/* Return NULL if database is full */
	if (attr == &server_db[SERVER_MAX_ATTRIBUTES - 1]) {
		return NULL;
	}

	/* First attribute in db must be service */
	if (!svc_count) {
		return NULL;
	}

	memcpy(attr, pattern, sizeof(*attr));

	/* Store the UUID. */
	attr->uuid = server_buf_push(uuid_size);
	memcpy((void *) attr->uuid, &u->uuid, uuid_size);

	/* Copy user_data to the buffer. */
	if (user_data_len) {
		attr->user_data = server_buf_push(user_data_len);
		memcpy(attr->user_data, pattern->user_data, user_data_len);
	}

	LOG_DBG("handle 0x%04x", attr->handle);

	attr_count++;
	svc_attr_count++;

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
	uint8_t cmds[5];
	struct gatt_read_supported_commands_rp *rp = (void *) cmds;

	(void)memset(cmds, 0, sizeof(cmds));

	tester_set_bit(cmds, GATT_READ_SUPPORTED_COMMANDS);
	tester_set_bit(cmds, GATT_ADD_SERVICE);
	tester_set_bit(cmds, GATT_ADD_CHARACTERISTIC);
	tester_set_bit(cmds, GATT_ADD_DESCRIPTOR);
	tester_set_bit(cmds, GATT_ADD_INCLUDED_SERVICE);
	tester_set_bit(cmds, GATT_SET_VALUE);
	tester_set_bit(cmds, GATT_START_SERVER);
	tester_set_bit(cmds, GATT_SET_ENC_KEY_SIZE);
	tester_set_bit(cmds, GATT_EXCHANGE_MTU);
	tester_set_bit(cmds, GATT_DISC_PRIM_UUID);
	tester_set_bit(cmds, GATT_FIND_INCLUDED);
	tester_set_bit(cmds, GATT_DISC_ALL_CHRC);
	tester_set_bit(cmds, GATT_DISC_CHRC_UUID);
	tester_set_bit(cmds, GATT_DISC_ALL_DESC);
	tester_set_bit(cmds, GATT_READ);
	tester_set_bit(cmds, GATT_READ_LONG);
	tester_set_bit(cmds, GATT_READ_MULTIPLE);
	tester_set_bit(cmds, GATT_WRITE_WITHOUT_RSP);
	tester_set_bit(cmds, GATT_SIGNED_WRITE_WITHOUT_RSP);
	tester_set_bit(cmds, GATT_WRITE);
	tester_set_bit(cmds, GATT_WRITE_LONG);
	tester_set_bit(cmds, GATT_CFG_NOTIFY);
	tester_set_bit(cmds, GATT_CFG_INDICATE);
	tester_set_bit(cmds, GATT_GET_ATTRIBUTES);
	tester_set_bit(cmds, GATT_GET_ATTRIBUTE_VALUE);
	tester_set_bit(cmds, GATT_DISC_ALL_PRIM);
	tester_set_bit(cmds, GATT_READ_MULTIPLE_VAR);
	tester_set_bit(cmds, GATT_EATT_CONNECT);

	tester_send(BTP_SERVICE_ID_GATT, GATT_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, (uint8_t *) rp, sizeof(cmds));
}

static int register_service(void)
{
	int err;

	server_svcs[svc_count].attrs = server_db +
				       (attr_count - svc_attr_count);
	server_svcs[svc_count].attr_count = svc_attr_count;

	err = bt_gatt_service_register(&server_svcs[svc_count]);
	if (!err) {
		/* Service registered, reset the counter */
		svc_attr_count = 0U;
	}

	return err;
}

static void add_service(uint8_t *data, uint16_t len)
{
	const struct gatt_add_service_cmd *cmd = (void *) data;
	struct gatt_add_service_rp rp;
	struct bt_gatt_attr *attr_svc = NULL;
	union uuid uuid;
	size_t uuid_size;

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
		goto fail;
	}

	uuid_size = uuid.uuid.type == BT_UUID_TYPE_16 ? sizeof(uuid.u16) :
							sizeof(uuid.u128);

	/* Register last defined service */
	if (svc_attr_count) {
		if (register_service()) {
			goto fail;
		}
	}

	svc_count++;

	switch (cmd->type) {
	case GATT_SERVICE_PRIMARY:
		attr_svc = gatt_db_add(&(struct bt_gatt_attr)
				       BT_GATT_PRIMARY_SERVICE(&uuid.uuid),
				       uuid_size);
		break;
	case GATT_SERVICE_SECONDARY:
		attr_svc = gatt_db_add(&(struct bt_gatt_attr)
				       BT_GATT_SECONDARY_SERVICE(&uuid.uuid),
				       uuid_size);
		break;
	}

	if (!attr_svc) {
		svc_count--;
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
	uint8_t enc_key_size;
	uint8_t flags[1];
};

enum {
	GATT_VALUE_CCC_FLAG,
	GATT_VALUE_READ_AUTHOR_FLAG,
	GATT_VALUE_WRITE_AUTHOR_FLAG,
};

static ssize_t read_value(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t len, uint16_t offset)
{
	const struct gatt_value *value = attr->user_data;

	if (tester_test_bit(value->flags, GATT_VALUE_READ_AUTHOR_FLAG)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	if ((attr->perm & GATT_PERM_ENC_READ_MASK) && (conn != NULL) &&
	    (value->enc_key_size > bt_conn_enc_key_size(conn))) {
		return BT_GATT_ERR(BT_ATT_ERR_ENCRYPTION_KEY_SIZE);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value->data,
				 value->len);
}

static void attr_value_changed_ev(uint16_t handle, const uint8_t *value, uint16_t len)
{
	uint8_t buf[len + sizeof(struct gatt_attr_value_changed_ev)];
	struct gatt_attr_value_changed_ev *ev = (void *) buf;

	ev->handle = sys_cpu_to_le16(handle);
	ev->data_length = sys_cpu_to_le16(len);
	memcpy(ev->data, value, len);

	tester_send(BTP_SERVICE_ID_GATT, GATT_EV_ATTR_VALUE_CHANGED,
		    CONTROLLER_INDEX, buf, sizeof(buf));
}

static ssize_t write_value(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, const void *buf,
			   uint16_t len, uint16_t offset, uint8_t flags)
{
	struct gatt_value *value = attr->user_data;

	if (tester_test_bit(value->flags, GATT_VALUE_WRITE_AUTHOR_FLAG)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	if ((attr->perm & GATT_PERM_ENC_WRITE_MASK) &&
	    (value->enc_key_size > bt_conn_enc_key_size(conn))) {
		return BT_GATT_ERR(BT_ATT_ERR_ENCRYPTION_KEY_SIZE);
	}

	/* Don't write anything if prepare flag is set */
	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		return 0;
	}

	if (offset > value->len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (offset + len > value->len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value->data + offset, buf, len);

	/* Maximum attribute value size is 512 bytes */
	__ASSERT_NO_MSG(value->len < 512);

	attr_value_changed_ev(attr->handle, value->data, value->len);

	return len;
}

struct add_characteristic {
	uint16_t char_id;
	uint8_t properties;
	uint8_t permissions;
	const struct bt_uuid *uuid;
};

static int alloc_characteristic(struct add_characteristic *ch)
{
	struct bt_gatt_attr *attr_chrc, *attr_value;
	struct bt_gatt_chrc *chrc_data;
	struct gatt_value value;

	/* Add Characteristic Declaration */
	attr_chrc = gatt_db_add(&(struct bt_gatt_attr)
				BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC,
						  BT_GATT_PERM_READ,
						  bt_gatt_attr_read_chrc, NULL,
						  (&(struct bt_gatt_chrc){})),
				sizeof(*chrc_data));
	if (!attr_chrc) {
		return -EINVAL;
	}

	(void)memset(&value, 0, sizeof(value));

	if (ch->permissions & GATT_PERM_READ_AUTHORIZATION) {
		tester_set_bit(value.flags, GATT_VALUE_READ_AUTHOR_FLAG);

		/* To maintain backward compatibility, set Read Permission */
		if (!(ch->permissions & GATT_PERM_ENC_READ_MASK)) {
			ch->permissions |= BT_GATT_PERM_READ;
		}
	}

	if (ch->permissions & GATT_PERM_WRITE_AUTHORIZATION) {
		tester_set_bit(value.flags, GATT_VALUE_WRITE_AUTHOR_FLAG);

		/* To maintain backward compatibility, set Write Permission */
		if (!(ch->permissions & GATT_PERM_ENC_WRITE_MASK)) {
			ch->permissions |= BT_GATT_PERM_WRITE;
		}
	}

	/* Allow prepare writes */
	ch->permissions |= BT_GATT_PERM_PREPARE_WRITE;

	/* Add Characteristic Value */
	attr_value = gatt_db_add(&(struct bt_gatt_attr)
				 BT_GATT_ATTRIBUTE(ch->uuid,
					ch->permissions & GATT_PERM_MASK,
					read_value, write_value, &value),
					sizeof(value));
	if (!attr_value) {
		server_buf_pull(sizeof(*chrc_data));
		/* Characteristic attribute uuid has constant length */
		server_buf_pull(sizeof(uint16_t));
		return -EINVAL;
	}

	chrc_data = attr_chrc->user_data;
	chrc_data->properties = ch->properties;
	chrc_data->uuid = attr_value->uuid;

	ch->char_id = attr_chrc->handle;
	return 0;
}

static void add_characteristic(uint8_t *data, uint16_t len)
{
	const struct gatt_add_characteristic_cmd *cmd = (void *) data;
	struct gatt_add_characteristic_rp rp;
	struct add_characteristic cmd_data;
	union uuid uuid;

	/* Pre-set char_id */
	cmd_data.char_id = 0U;
	cmd_data.permissions = cmd->permissions;
	cmd_data.properties = cmd->properties;
	cmd_data.uuid = &uuid.uuid;

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
		goto fail;
	}

	/* characteristic must be added only sequential */
	if (cmd->svc_id) {
		goto fail;
	}

	if (alloc_characteristic(&cmd_data)) {
		goto fail;
	}

	rp.char_id = sys_cpu_to_le16(cmd_data.char_id);
	tester_send(BTP_SERVICE_ID_GATT, GATT_ADD_CHARACTERISTIC,
		    CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_ADD_CHARACTERISTIC,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static bool ccc_added;

static uint8_t ccc_value;

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ccc_value = value;
}

static struct bt_gatt_attr ccc = BT_GATT_CCC(ccc_cfg_changed,
					     BT_GATT_PERM_READ |
					     BT_GATT_PERM_WRITE);

static struct bt_gatt_attr *add_ccc(const struct bt_gatt_attr *attr)
{
	struct bt_gatt_attr *attr_desc;
	struct bt_gatt_chrc *chrc = attr->user_data;
	struct gatt_value *value = NEXT_DB_ATTR(attr)->user_data;

	/* Fail if another CCC already exist on server */
	if (ccc_added) {
		return NULL;
	}

	/* Check characteristic properties */
	if (!(chrc->properties &
	    (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE))) {
		return NULL;
	}

	/* Add CCC descriptor to GATT database */
	attr_desc = gatt_db_add(&ccc, 0);
	if (!attr_desc) {
		return NULL;
	}

	tester_set_bit(value->flags, GATT_VALUE_CCC_FLAG);
	ccc_added = true;

	return attr_desc;
}

static struct bt_gatt_attr *add_cep(const struct bt_gatt_attr *attr_chrc)
{
	struct bt_gatt_chrc *chrc = attr_chrc->user_data;
	struct bt_gatt_cep cep_value;

	/* Extended Properties bit shall be set */
	if (!(chrc->properties & BT_GATT_CHRC_EXT_PROP)) {
		return NULL;
	}

	cep_value.properties = 0x0000;

	/* Add CEP descriptor to GATT database */
	return gatt_db_add(&(struct bt_gatt_attr) BT_GATT_CEP(&cep_value),
			   sizeof(cep_value));
}

struct add_descriptor {
	uint16_t desc_id;
	uint8_t permissions;
	const struct bt_uuid *uuid;
};

static int alloc_descriptor(const struct bt_gatt_attr *attr,
			    struct add_descriptor *d)
{
	struct bt_gatt_attr *attr_desc;
	struct gatt_value value;

	if (!bt_uuid_cmp(d->uuid, BT_UUID_GATT_CEP)) {
		attr_desc = add_cep(attr);
	} else if (!bt_uuid_cmp(d->uuid, BT_UUID_GATT_CCC)) {
		attr_desc = add_ccc(attr);
	} else {
		(void)memset(&value, 0, sizeof(value));

		if (d->permissions & GATT_PERM_READ_AUTHORIZATION) {
			tester_set_bit(value.flags,
				       GATT_VALUE_READ_AUTHOR_FLAG);

			/*
			 * To maintain backward compatibility,
			 * set Read Permission
			 */
			if (!(d->permissions & GATT_PERM_ENC_READ_MASK)) {
				d->permissions |= BT_GATT_PERM_READ;
			}
		}

		if (d->permissions & GATT_PERM_WRITE_AUTHORIZATION) {
			tester_set_bit(value.flags,
				       GATT_VALUE_WRITE_AUTHOR_FLAG);

			/*
			 * To maintain backward compatibility,
			 * set Write Permission
			 */
			if (!(d->permissions & GATT_PERM_ENC_WRITE_MASK)) {
				d->permissions |= BT_GATT_PERM_WRITE;
			}
		}

		/* Allow prepare writes */
		d->permissions |= BT_GATT_PERM_PREPARE_WRITE;

		attr_desc = gatt_db_add(&(struct bt_gatt_attr)
					BT_GATT_DESCRIPTOR(d->uuid,
						d->permissions & GATT_PERM_MASK,
						read_value, write_value,
						&value), sizeof(value));
	}

	if (!attr_desc) {
		return -EINVAL;
	}

	d->desc_id = attr_desc->handle;
	return 0;
}

static struct bt_gatt_attr *get_base_chrc(struct bt_gatt_attr *attr)
{
	struct bt_gatt_attr *tmp;

	for (tmp = attr; tmp > server_db; tmp--) {
		/* Service Declaration cannot precede Descriptor declaration */
		if (!bt_uuid_cmp(tmp->uuid, BT_UUID_GATT_PRIMARY) ||
		    !bt_uuid_cmp(tmp->uuid, BT_UUID_GATT_SECONDARY)) {
			break;
		}

		if (!bt_uuid_cmp(tmp->uuid, BT_UUID_GATT_CHRC)) {
			return tmp;
		}
	}

	return NULL;
}

static void add_descriptor(uint8_t *data, uint16_t len)
{
	const struct gatt_add_descriptor_cmd *cmd = (void *) data;
	struct gatt_add_descriptor_rp rp;
	struct add_descriptor cmd_data;
	struct bt_gatt_attr *chrc;
	union uuid uuid;

	/* Must be declared first svc or at least 3 attrs (svc+char+char val) */
	if (!svc_count || attr_count < 3) {
		goto fail;
	}

	/* Pre-set desc_id */
	cmd_data.desc_id = 0U;
	cmd_data.permissions = cmd->permissions;
	cmd_data.uuid = &uuid.uuid;

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
		goto fail;
	}

	/* descriptor can be added only sequential */
	if (cmd->char_id) {
		goto fail;
	}

	/* Lookup preceding Characteristic Declaration here */
	chrc = get_base_chrc(LAST_DB_ATTR);
	if (!chrc) {
		goto fail;
	}

	if (alloc_descriptor(chrc, &cmd_data)) {
		goto fail;
	}

	rp.desc_id = sys_cpu_to_le16(cmd_data.desc_id);
	tester_send(BTP_SERVICE_ID_GATT, GATT_ADD_DESCRIPTOR, CONTROLLER_INDEX,
		    (uint8_t *) &rp, sizeof(rp));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_ADD_DESCRIPTOR,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static int alloc_included(struct bt_gatt_attr *attr,
			  uint16_t *included_service_id, uint16_t svc_handle)
{
	struct bt_gatt_attr *attr_incl;

	/*
	 * user_data_len is set to 0 to NOT allocate memory in server_buf for
	 * user_data, just to assign to it attr pointer.
	 */
	attr_incl = gatt_db_add(&(struct bt_gatt_attr)
				BT_GATT_INCLUDE_SERVICE(attr), 0);

	if (!attr_incl) {
		return -EINVAL;
	}

	attr_incl->user_data = attr;

	*included_service_id = attr_incl->handle;
	return 0;
}

static void add_included(uint8_t *data, uint16_t len)
{
	const struct gatt_add_included_service_cmd *cmd = (void *) data;
	struct gatt_add_included_service_rp rp;
	struct bt_gatt_attr *svc;
	uint16_t included_service_id = 0U;

	if (!svc_count || !cmd->svc_id) {
		goto fail;
	}

	svc = &server_db[cmd->svc_id - 1];

	/* Fail if attribute stored under requested handle is not a service */
	if (bt_uuid_cmp(svc->uuid, BT_UUID_GATT_PRIMARY) &&
	    bt_uuid_cmp(svc->uuid, BT_UUID_GATT_SECONDARY)) {
		goto fail;
	}

	if (alloc_included(svc, &included_service_id, cmd->svc_id)) {
		goto fail;
	}

	rp.included_service_id = sys_cpu_to_le16(included_service_id);
	tester_send(BTP_SERVICE_ID_GATT, GATT_ADD_INCLUDED_SERVICE,
		    CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_ADD_INCLUDED_SERVICE,
		   CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static uint8_t set_cep_value(struct bt_gatt_attr *attr, const void *value,
			     const uint16_t len)
{
	struct bt_gatt_cep *cep_value = attr->user_data;
	uint16_t properties;

	if (len != sizeof(properties)) {
		return BTP_STATUS_FAILED;
	}

	memcpy(&properties, value, len);
	cep_value->properties = sys_le16_to_cpu(properties);

	return BTP_STATUS_SUCCESS;
}

struct set_value {
	const uint8_t *value;
	uint16_t len;
};

struct bt_gatt_indicate_params indicate_params;

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	if (err != 0U) {
		LOG_ERR("Indication fail");
	} else {
		LOG_DBG("Indication success");
	}
}

static uint8_t alloc_value(struct bt_gatt_attr *attr, struct set_value *data)
{
	struct gatt_value *value;

	/* Value has been already set while adding CCC to the gatt_db */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC)) {
		return BTP_STATUS_SUCCESS;
	}

	/* Set CEP value */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CEP)) {
		return set_cep_value(attr, data->value, data->len);
	}

	value = attr->user_data;

	/* Check if attribute value has been already set */
	if (!value->len) {
		value->data = server_buf_push(data->len);
		value->len = data->len;
	}

	/* Fail if value length doesn't match  */
	if (value->len != data->len) {
		return BTP_STATUS_FAILED;
	}

	memcpy(value->data, data->value, value->len);

	if (tester_test_bit(value->flags, GATT_VALUE_CCC_FLAG) && ccc_value) {
		if (ccc_value == BT_GATT_CCC_NOTIFY) {
			bt_gatt_notify(NULL, attr, value->data, value->len);
		} else {
			indicate_params.attr = attr;
			indicate_params.data = value->data;
			indicate_params.len = value->len;
			indicate_params.func = indicate_cb;
			indicate_params.destroy = NULL;

			bt_gatt_indicate(NULL, &indicate_params);
		}
	}

	return BTP_STATUS_SUCCESS;
}

static void set_value(uint8_t *data, uint16_t len)
{
	const struct gatt_set_value_cmd *cmd = (void *) data;
	struct set_value cmd_data;
	uint8_t status;

	/* Pre-set btp_status */
	cmd_data.value = cmd->value;
	cmd_data.len = sys_le16_to_cpu(cmd->len);

	if (!cmd->attr_id) {
		status = alloc_value(LAST_DB_ATTR, &cmd_data);
	} else {
		/* set value of local attr, corrected by pre set attr handles */
		status = alloc_value(&server_db[cmd->attr_id -
				     server_db[0].handle], &cmd_data);
	}

	tester_rsp(BTP_SERVICE_ID_GATT, GATT_SET_VALUE, CONTROLLER_INDEX,
		   status);
}

static void start_server(uint8_t *data, uint16_t len)
{
	struct gatt_start_server_rp rp;

	/* Register last defined service */
	if (svc_attr_count) {
		if (register_service()) {
			tester_rsp(BTP_SERVICE_ID_GATT, GATT_START_SERVER,
				   CONTROLLER_INDEX, BTP_STATUS_FAILED);
			return;
		}
	}

	tester_send(BTP_SERVICE_ID_GATT, GATT_START_SERVER, CONTROLLER_INDEX,
		    (uint8_t *) &rp, sizeof(rp));
}

static int set_attr_enc_key_size(const struct bt_gatt_attr *attr,
				 uint8_t key_size)
{
	struct gatt_value *value;

	/* Fail if requested attribute is a service */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) ||
	    !bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY) ||
	    !bt_uuid_cmp(attr->uuid, BT_UUID_GATT_INCLUDE)) {
		return -EINVAL;
	}

	/* Fail if permissions are not set */
	if (!(attr->perm & (GATT_PERM_ENC_READ_MASK |
			    GATT_PERM_ENC_WRITE_MASK))) {
		return -EINVAL;
	}

	value = attr->user_data;
	value->enc_key_size = key_size;

	return 0;
}

static void set_enc_key_size(uint8_t *data, uint16_t len)
{
	const struct gatt_set_enc_key_size_cmd *cmd = (void *) data;
	uint8_t status;

	/* Fail if requested key size is invalid */
	if (cmd->key_size < 0x07 || cmd->key_size > 0x0f) {
		status = BTP_STATUS_FAILED;
		goto fail;
	}

	if (!cmd->attr_id) {
		status = set_attr_enc_key_size(LAST_DB_ATTR, cmd->key_size);
	} else {
		/* set value of local attr, corrected by pre set attr handles */
		status = set_attr_enc_key_size(&server_db[cmd->attr_id -
					       server_db[0].handle],
					       cmd->key_size);
	}

fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_SET_ENC_KEY_SIZE, CONTROLLER_INDEX,
		   status);
}

static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
	if (err) {
		tester_rsp(BTP_SERVICE_ID_GATT, GATT_EXCHANGE_MTU,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);

		return;
	}

	tester_rsp(BTP_SERVICE_ID_GATT, GATT_EXCHANGE_MTU, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static struct bt_gatt_exchange_params exchange_params;

static void exchange_mtu(uint8_t *data, uint16_t len)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		goto fail;
	}

	exchange_params.func = exchange_func;

	if (bt_gatt_exchange_mtu(conn, &exchange_params) < 0) {
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
	(void)memset(params, 0, sizeof(*params));
	gatt_buf_clear();
}

static uint8_t disc_prim_cb(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service_val *data;
	struct gatt_disc_prim_rp *rp = (void *) gatt_buf.buf;
	struct gatt_service *service;
	uint8_t uuid_length;

	if (!attr) {
		tester_send(BTP_SERVICE_ID_GATT, btp_opcode,
			    CONTROLLER_INDEX, gatt_buf.buf, gatt_buf.len);
		discover_destroy(params);
		return BT_GATT_ITER_STOP;
	}

	data = attr->user_data;

	uuid_length = data->uuid->type == BT_UUID_TYPE_16 ? 2 : 16;

	service = gatt_buf_reserve(sizeof(*service) + uuid_length);
	if (!service) {
		tester_rsp(BTP_SERVICE_ID_GATT, btp_opcode,
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

static void disc_all_prim(uint8_t *data, uint16_t len)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		goto fail_conn;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_disc_prim_rp))) {
		goto fail;
	}

	discover_params.uuid = NULL;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.func = disc_prim_cb;

	btp_opcode = GATT_DISC_ALL_PRIM;

	if (bt_gatt_discover(conn, &discover_params) < 0) {
		discover_destroy(&discover_params);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_DISC_ALL_PRIM, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void disc_prim_uuid(uint8_t *data, uint16_t len)
{
	const struct gatt_disc_prim_uuid_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		goto fail_conn;
	}

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
		goto fail;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_disc_prim_rp))) {
		goto fail;
	}

	discover_params.uuid = &uuid.uuid;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.func = disc_prim_cb;

	btp_opcode = GATT_DISC_PRIM_UUID;

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
		memcpy(included->service.uuid, BT_UUID_128(data->uuid)->val,
		       uuid_length);
	}

	rp->services_count++;

	return BT_GATT_ITER_CONTINUE;
}

static void find_included(uint8_t *data, uint16_t len)
{
	const struct gatt_find_included_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
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

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
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

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
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

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
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
	(void)memset(params, 0, sizeof(*params));
	gatt_buf_clear();
}

static uint8_t read_cb(struct bt_conn *conn, uint8_t err,
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

static uint8_t read_uuid_cb(struct bt_conn *conn, uint8_t err,
		       struct bt_gatt_read_params *params, const void *data,
		       uint16_t length)
{
	struct gatt_read_uuid_rp *rp = (void *)gatt_buf.buf;
	struct gatt_char_value value;

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

	value.handle = params->by_uuid.start_handle;
	value.data_len = length;

	if (!gatt_buf_add(&value, sizeof(struct gatt_char_value))) {
		tester_rsp(BTP_SERVICE_ID_GATT, btp_opcode,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		read_destroy(params);

		return BT_GATT_ITER_STOP;
	}

	if (!gatt_buf_add(data, length)) {
		tester_rsp(BTP_SERVICE_ID_GATT, btp_opcode,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
		read_destroy(params);

		return BT_GATT_ITER_STOP;
	}

	rp->values_count++;

	return BT_GATT_ITER_CONTINUE;
}

static void read_data(uint8_t *data, uint16_t len)
{
	const struct gatt_read_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
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

static void read_uuid(uint8_t *data, uint16_t len)
{
	const struct gatt_read_uuid_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		goto fail_conn;
	}

	if (btp2bt_uuid(cmd->uuid, cmd->uuid_length, &uuid.uuid)) {
		goto fail;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_read_uuid_rp))) {
		goto fail;
	}

	read_params.by_uuid.uuid = &uuid.uuid;
	read_params.handle_count = 0;
	read_params.by_uuid.start_handle = sys_le16_to_cpu(cmd->start_handle);
	read_params.by_uuid.end_handle = sys_le16_to_cpu(cmd->end_handle);
	read_params.func = read_uuid_cb;

	btp_opcode = GATT_READ_UUID;

	if (bt_gatt_read(conn, &read_params) < 0) {
		read_destroy(&read_params);

		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	bt_conn_unref(conn);

fail_conn:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_READ_UUID, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void read_long(uint8_t *data, uint16_t len)
{
	const struct gatt_read_long_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
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

static void read_multiple(uint8_t *data, uint16_t len, uint8_t opcode)
{
	const struct gatt_read_multiple_cmd *cmd = (void *) data;
	uint16_t handles[cmd->handles_count];
	struct bt_conn *conn;
	int i;

	for (i = 0; i < ARRAY_SIZE(handles); i++) {
		handles[i] = sys_le16_to_cpu(cmd->handles[i]);
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		goto fail_conn;
	}

	if (!gatt_buf_reserve(sizeof(struct gatt_read_rp))) {
		goto fail;
	}

	read_params.func = read_cb;
	read_params.handle_count = i;
	read_params.multiple.handles = handles; /* not used in read func */
	read_params.multiple.variable = (opcode == GATT_READ_MULTIPLE_VAR);

	/* TODO should be handled as user_data via CONTAINER_OF macro */
	btp_opcode = opcode;

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

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
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

static void write_rsp(struct bt_conn *conn, uint8_t err,
		      struct bt_gatt_write_params *params)
{
	tester_send(BTP_SERVICE_ID_GATT, GATT_WRITE, CONTROLLER_INDEX, &err,
		    sizeof(err));
}

static struct bt_gatt_write_params write_params;

static void write_data(uint8_t *data, uint16_t len)
{
	const struct gatt_write_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		goto fail;
	}

	write_params.handle = sys_le16_to_cpu(cmd->handle);
	write_params.func = write_rsp;
	write_params.offset = 0U;
	write_params.data = cmd->data;
	write_params.length = sys_le16_to_cpu(cmd->data_length);

	if (bt_gatt_write(conn, &write_params) < 0) {
		bt_conn_unref(conn);
		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_WRITE, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void write_long_rsp(struct bt_conn *conn, uint8_t err,
			   struct bt_gatt_write_params *params)
{
	tester_send(BTP_SERVICE_ID_GATT, GATT_WRITE_LONG, CONTROLLER_INDEX,
		    &err, sizeof(err));
}

static void write_long(uint8_t *data, uint16_t len)
{
	const struct gatt_write_long_cmd *cmd = (void *) data;
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
	if (!conn) {
		goto fail;
	}

	write_params.handle = sys_le16_to_cpu(cmd->handle);
	write_params.func = write_long_rsp;
	write_params.offset = cmd->offset;
	write_params.data = cmd->data;
	write_params.length = sys_le16_to_cpu(cmd->data_length);

	if (bt_gatt_write(conn, &write_params) < 0) {
		bt_conn_unref(conn);
		goto fail;
	}

	bt_conn_unref(conn);

	return;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_WRITE_LONG, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static struct bt_gatt_subscribe_params subscriptions[MAX_SUBSCRIPTIONS];

static struct bt_gatt_subscribe_params *find_subscription(uint16_t ccc_handle)
{
	for (int i = 0; i < MAX_SUBSCRIPTIONS; i++) {
		if (subscriptions[i].ccc_handle == ccc_handle) {
			return &subscriptions[i];
		}
	}

	return NULL;
}

/* TODO there should be better way of determining max supported MTU */
#define MAX_NOTIF_DATA (MIN(BT_L2CAP_RX_MTU, BT_L2CAP_TX_MTU) - 3)

static uint8_t ev_buf[sizeof(struct gatt_notification_ev) + MAX_NOTIF_DATA];

static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	struct gatt_notification_ev *ev = (void *) ev_buf;
	const bt_addr_le_t *addr;

	if (!conn || !data) {
		LOG_DBG("Unsubscribed");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}
	addr = bt_conn_get_dst(conn);
	ev->type = (uint8_t)params->value;
	ev->handle = sys_cpu_to_le16(params->value_handle);

	length = MIN(length, MAX_NOTIF_DATA);

	ev->data_length = sys_cpu_to_le16(length);
	memcpy(ev->data, data, length);
	memcpy(ev->address, addr->a.val, sizeof(ev->address));
	ev->address_type = addr->type;

	tester_send(BTP_SERVICE_ID_GATT, GATT_EV_NOTIFICATION,
		    CONTROLLER_INDEX, ev_buf, sizeof(*ev) + length);

	return BT_GATT_ITER_CONTINUE;
}

static void discover_complete(struct bt_conn *conn,
			      struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscription;
	uint8_t op, status;

	subscription = find_subscription(discover_params.end_handle);
	__ASSERT_NO_MSG(subscription);

	/* If no value handle it means that chrc has not been found */
	if (!subscription->value_handle) {
		status = BTP_STATUS_FAILED;
		goto fail;
	}

	if (bt_gatt_subscribe(conn, subscription) < 0) {
		status = BTP_STATUS_FAILED;
		goto fail;
	}

	status = BTP_STATUS_SUCCESS;
fail:
	op = subscription->value == BT_GATT_CCC_NOTIFY ? GATT_CFG_NOTIFY :
							 GATT_CFG_INDICATE;

	if (status == BTP_STATUS_FAILED) {
		(void)memset(subscription, 0, sizeof(*subscription));
	}

	tester_rsp(BTP_SERVICE_ID_GATT, op, CONTROLLER_INDEX, status);

	(void)memset(params, 0, sizeof(*params));
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscription;

	if (!attr) {
		discover_complete(conn, params);
		return BT_GATT_ITER_STOP;
	}

	subscription = find_subscription(discover_params.end_handle);
	__ASSERT_NO_MSG(subscription);

	/* Characteristic Value Handle is the next handle beyond declaration */
	subscription->value_handle = attr->handle + 1;

	/*
	 * Continue characteristic discovery to get last characteristic
	 * preceding this CCC descriptor
	 */
	return BT_GATT_ITER_CONTINUE;
}

static int enable_subscription(struct bt_conn *conn, uint16_t ccc_handle,
			       uint16_t value)
{
	struct bt_gatt_subscribe_params *subscription;

	/* find unused subscription */
	subscription = find_subscription(UNUSED_SUBSCRIBE_CCC_HANDLE);
	if (!subscription) {
		return -ENOMEM;
	}

	/* if discovery is busy fail */
	if (discover_params.start_handle) {
		return -EBUSY;
	}

	/* Discover Characteristic Value this CCC Descriptor refers to */
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = ccc_handle;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	discover_params.func = discover_func;

	subscription->ccc_handle = ccc_handle;
	subscription->value = value;
	subscription->notify = notify_func;

	/* require security level from time of subscription */
	subscription->min_security = bt_conn_get_security(conn);

	return bt_gatt_discover(conn, &discover_params);
}

static int disable_subscription(struct bt_conn *conn, uint16_t ccc_handle)
{
	struct bt_gatt_subscribe_params *subscription;

	/* Fail if CCC handle doesn't match */
	subscription = find_subscription(ccc_handle);
	if (!subscription) {
		LOG_ERR("CCC handle doesn't match");
		return -EINVAL;
	}

	if (bt_gatt_unsubscribe(conn, subscription) < 0) {
		return -EBUSY;
	}

	(void)memset(subscription, 0, sizeof(*subscription));

	return 0;
}

static void config_subscription(uint8_t *data, uint16_t len, uint16_t op)
{
	const struct gatt_cfg_notify_cmd *cmd = (void *) data;
	struct bt_conn *conn;
	uint16_t ccc_handle = sys_le16_to_cpu(cmd->ccc_handle);
	uint8_t status;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)data);
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

	LOG_DBG("Config subscription (op %u) status %u", op, status);

	bt_conn_unref(conn);
	tester_rsp(BTP_SERVICE_ID_GATT, op, CONTROLLER_INDEX, status);
}

struct get_attrs_foreach_data {
	struct net_buf_simple *buf;
	struct bt_uuid *uuid;
	uint8_t count;
};

static uint8_t get_attrs_rp(const struct bt_gatt_attr *attr, uint16_t handle,
			    void *user_data)
{
	struct get_attrs_foreach_data *foreach = user_data;
	struct gatt_attr *gatt_attr;

	if (foreach->uuid && bt_uuid_cmp(foreach->uuid, attr->uuid)) {

		return BT_GATT_ITER_CONTINUE;
	}

	gatt_attr = net_buf_simple_add(foreach->buf, sizeof(*gatt_attr));
	gatt_attr->handle = sys_cpu_to_le16(handle);
	gatt_attr->permission = attr->perm;

	if (attr->uuid->type == BT_UUID_TYPE_16) {
		gatt_attr->type_length = 2U;
		net_buf_simple_add_le16(foreach->buf,
					BT_UUID_16(attr->uuid)->val);
	} else {
		gatt_attr->type_length = 16U;
		net_buf_simple_add_mem(foreach->buf,
				       BT_UUID_128(attr->uuid)->val,
				       gatt_attr->type_length);
	}

	foreach->count++;

	return BT_GATT_ITER_CONTINUE;
}

static void get_attrs(uint8_t *data, uint16_t len)
{
	const struct gatt_get_attributes_cmd *cmd = (void *) data;
	struct gatt_get_attributes_rp *rp;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);
	struct get_attrs_foreach_data foreach;
	uint16_t start_handle, end_handle;
	union uuid uuid;

	start_handle = sys_le16_to_cpu(cmd->start_handle);
	end_handle = sys_le16_to_cpu(cmd->end_handle);

	if (cmd->type_length) {
		char uuid_str[BT_UUID_STR_LEN];

		if (btp2bt_uuid(cmd->type, cmd->type_length, &uuid.uuid)) {
			goto fail;
		}

		bt_uuid_to_str(&uuid.uuid, uuid_str, sizeof(uuid_str));
		LOG_DBG("start 0x%04x end 0x%04x, uuid %s", start_handle,
			end_handle, uuid_str);

		foreach.uuid = &uuid.uuid;
	} else {
		LOG_DBG("start 0x%04x end 0x%04x", start_handle, end_handle);

		foreach.uuid = NULL;
	}

	net_buf_simple_init(buf, sizeof(*rp));

	foreach.buf = buf;
	foreach.count = 0U;

	bt_gatt_foreach_attr(start_handle, end_handle, get_attrs_rp, &foreach);

	rp = net_buf_simple_push(buf, sizeof(*rp));
	rp->attrs_count = foreach.count;

	tester_send(BTP_SERVICE_ID_GATT, GATT_GET_ATTRIBUTES, CONTROLLER_INDEX,
		    buf->data, buf->len);

	return;
fail:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_GET_ATTRIBUTES, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static uint8_t err_to_att(int err)
{
	if (err < 0 && err >= -0xff) {
		return -err;
	}

	return BT_ATT_ERR_UNLIKELY;
}

static uint8_t get_attr_val_rp(const struct bt_gatt_attr *attr, uint16_t handle,
			       void *user_data)
{
	struct get_attr_data *u_data = user_data;
	struct net_buf_simple *buf = u_data->buf;
	struct bt_conn *conn = u_data->conn;
	struct gatt_get_attribute_value_rp *rp;
	ssize_t read, to_read;

	rp = net_buf_simple_add(buf, sizeof(*rp));
	rp->value_length = 0x0000;
	rp->att_response = 0x00;

	do {
		to_read = net_buf_simple_tailroom(buf);

		if (!attr->read) {
			rp->att_response = BT_ATT_ERR_READ_NOT_PERMITTED;
			break;
		}

		read = attr->read(conn, attr, buf->data + buf->len, to_read,
				  rp->value_length);
		if (read < 0) {
			rp->att_response = err_to_att(read);
			break;
		}

		rp->value_length += read;

		net_buf_simple_add(buf, read);
	} while (read == to_read);

	return BT_GATT_ITER_STOP;
}

static void get_attr_val(uint8_t *data, uint16_t len)
{
	const struct gatt_get_attribute_value_cmd *cmd = (void *) data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);
	uint16_t handle = sys_le16_to_cpu(cmd->handle);
	struct bt_conn *conn;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)cmd);

	net_buf_simple_init(buf, 0);

	struct get_attr_data cb_data = { .buf = buf, .conn = conn };

	bt_gatt_foreach_attr(handle, handle, get_attr_val_rp, &cb_data);

	if (buf->len) {
		tester_send(BTP_SERVICE_ID_GATT, GATT_GET_ATTRIBUTE_VALUE,
			    CONTROLLER_INDEX, buf->data, buf->len);
	} else {
		tester_rsp(BTP_SERVICE_ID_GATT, GATT_GET_ATTRIBUTE_VALUE,
			   CONTROLLER_INDEX, BTP_STATUS_FAILED);
	}
}

static void eatt_connect(uint8_t *data, uint16_t len)
{
	const struct gatt_eatt_connect_cmd *cmd = (void *)data;
	struct bt_conn *conn;
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, (bt_addr_le_t *)cmd);
	if (!conn) {
		status = BTP_STATUS_FAILED;
		goto response;
	}

	err = bt_eatt_connect(conn, cmd->num_channels);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	bt_conn_unref(conn);

response:
	tester_rsp(BTP_SERVICE_ID_GATT, GATT_EATT_CONNECT, CONTROLLER_INDEX, status);
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
	case GATT_DISC_ALL_PRIM:
		disc_all_prim(data, len);
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
		read_data(data, len);
		return;
	case GATT_READ_UUID:
		read_uuid(data, len);
		return;
	case GATT_READ_LONG:
		read_long(data, len);
		return;
	case GATT_READ_MULTIPLE:
	case GATT_READ_MULTIPLE_VAR:
		read_multiple(data, len, opcode);
		return;
	case GATT_WRITE_WITHOUT_RSP:
		write_without_rsp(data, len, opcode, false);
		return;
	case GATT_SIGNED_WRITE_WITHOUT_RSP:
		write_without_rsp(data, len, opcode, true);
		return;
	case GATT_WRITE:
		write_data(data, len);
		return;
	case GATT_WRITE_LONG:
		write_long(data, len);
		return;
	case GATT_CFG_NOTIFY:
	case GATT_CFG_INDICATE:
		config_subscription(data, len, opcode);
		return;
	case GATT_GET_ATTRIBUTES:
		get_attrs(data, len);
		return;
	case GATT_GET_ATTRIBUTE_VALUE:
		get_attr_val(data, len);
		return;
	case GATT_EATT_CONNECT:
		eatt_connect(data, len);
		return;
	default:
		tester_rsp(BTP_SERVICE_ID_GATT, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

uint8_t tester_init_gatt(void)
{
	server_buf = net_buf_alloc(&server_pool, K_NO_WAIT);
	if (!server_buf) {
		return BTP_STATUS_FAILED;
	}

	net_buf_reserve(server_buf, SERVER_BUF_SIZE);

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_gatt(void)
{
	return BTP_STATUS_SUCCESS;
}
