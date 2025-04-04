/* btp_ots.c - Bluetooth OTS Tester */

/*
 * Copyright (c) 2024 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/services/ots.h>

#include <zephyr/sys/byteorder.h>
#include <stdint.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_ots
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#define OBJ_POOL_SIZE CONFIG_BT_OTS_MAX_OBJ_CNT
#define OBJ_MAX_SIZE  100

static struct object {
	uint8_t data[OBJ_MAX_SIZE];
	char name[CONFIG_BT_OTS_OBJ_MAX_NAME_LEN + 1];
	bool in_use;
} objects[OBJ_POOL_SIZE];

struct object_creation_data {
	struct object *object;
	struct bt_ots_obj_size size;
	uint32_t props;
};

#define OTS_OBJ_ID_TO_OBJ_IDX(id) (((id) - BT_OTS_OBJ_ID_MIN) % ARRAY_SIZE(objects))

static struct object_creation_data *object_being_created;

static struct bt_ots *ots;

static uint8_t ots_supported_commands(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_ots_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_OTS, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static struct object *get_object(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(objects); i++) {
		if (!objects[i].in_use) {
			objects[i].in_use = true;
			return &objects[i];
		}
	}

	return NULL;
}

static uint8_t register_object(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_ots_register_object_cmd *cp = cmd;
	struct btp_ots_register_object_rp *rp = rsp;
	struct object_creation_data obj_data;
	struct bt_ots_obj_add_param param;
	uint32_t supported_props = 0;
	struct object *obj;
	uint32_t props;
	int err;

	if ((cmd_len < sizeof(*cp)) || (cmd_len != sizeof(*cp) + cp->name_len)) {
		return BTP_STATUS_FAILED;
	}

	if (cp->name_len == 0 || cp->name_len > CONFIG_BT_OTS_OBJ_MAX_NAME_LEN) {
		return BTP_STATUS_FAILED;
	}

	/* all supported props (execute, append, truncate not supported) */
	BT_OTS_OBJ_SET_PROP_DELETE(supported_props);
	BT_OTS_OBJ_SET_PROP_READ(supported_props);
	BT_OTS_OBJ_SET_PROP_WRITE(supported_props);
	BT_OTS_OBJ_SET_PROP_PATCH(supported_props);

	props = sys_le32_to_cpu(cp->ots_props);
	if (cp->flags & BTP_OTS_REGISTER_OBJECT_FLAGS_SKIP_UNSUPPORTED_PROPS) {
		props &= supported_props;
	}

	obj = get_object();
	if (!obj) {
		return BTP_STATUS_FAILED;
	}

	(void)memset(&obj_data, 0, sizeof(obj_data));

	memcpy(obj->name, cp->name, cp->name_len);
	obj_data.object = obj;
	obj_data.size.cur = sys_le32_to_cpu(cp->current_size);
	obj_data.size.alloc = sys_le32_to_cpu(cp->alloc_size);
	obj_data.props = props;

	/* bt_ots_obj_add() lacks user_data so we need to use global for
	 * passing this
	 */
	object_being_created = &obj_data;

	param.size = obj_data.size.alloc;
	param.type.uuid.type = BT_UUID_TYPE_16;
	param.type.uuid_16.val = BT_UUID_OTS_TYPE_UNSPECIFIED_VAL;

	err = bt_ots_obj_add(ots, &param);
	object_being_created = NULL;

	if (err < 0) {
		memset(obj, 0, sizeof(*obj));
		return BTP_STATUS_FAILED;
	}

	rp->object_id = sys_cpu_to_le64(err);
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler ots_handlers[] = {
	{
		.opcode = BTP_OTS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = ots_supported_commands
	},
	{
		.opcode = BTP_OTS_REGISTER_OBJECT,
		.index = 0,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = register_object
	},
};

static int ots_obj_created(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
			   const struct bt_ots_obj_add_param *add_param,
			   struct bt_ots_obj_created_desc *created_desc)
{
	struct object *obj;

	LOG_DBG("id=%"PRIu64" size=%u", id, add_param->size);

	/* TS suggests to use OTS service UUID for testing this */
	if (conn && bt_uuid_cmp(&add_param->type.uuid, BT_UUID_OTS) == 0) {
		return -ENOTSUP;
	}

	if (add_param->size > OBJ_MAX_SIZE) {
		return -ENOMEM;
	}

	if (conn || !object_being_created) {
		uint32_t obj_index = OTS_OBJ_ID_TO_OBJ_IDX(id);

		if (obj_index >= OBJ_POOL_SIZE) {
			return -ENOMEM;
		}

		obj = &objects[obj_index];
		if (obj->in_use) {
			return -ENOMEM;
		}

		obj->in_use = false;
		created_desc->name = obj->name;
		created_desc->size.alloc = OBJ_MAX_SIZE;
		BT_OTS_OBJ_SET_PROP_READ(created_desc->props);
		BT_OTS_OBJ_SET_PROP_WRITE(created_desc->props);
		BT_OTS_OBJ_SET_PROP_PATCH(created_desc->props);
		BT_OTS_OBJ_SET_PROP_DELETE(created_desc->props);
	} else {
		obj = object_being_created->object;
		created_desc->name = obj->name;
		created_desc->size = object_being_created->size;
		created_desc->props = object_being_created->props;
	}

	return 0;
}

static int ots_obj_deleted(struct bt_ots *ots, struct bt_conn *conn,
			    uint64_t id)
{
	uint32_t obj_index = OTS_OBJ_ID_TO_OBJ_IDX(id);
	struct object *obj;

	LOG_DBG("id=%"PRIu64, id);

	if (obj_index >= OBJ_POOL_SIZE) {
		return -ENOENT;
	}

	obj = &objects[obj_index];
	memset(obj, 0, sizeof(*obj));

	return 0;
}

static void ots_obj_selected(struct bt_ots *ots, struct bt_conn *conn,
			     uint64_t id)
{
	LOG_DBG("id=%"PRIu64, id);
}

static ssize_t ots_obj_read(struct bt_ots *ots, struct bt_conn *conn,
			    uint64_t id, void **data, size_t len,
			    off_t offset)
{
	uint32_t obj_index = OTS_OBJ_ID_TO_OBJ_IDX(id);

	LOG_DBG("id=%"PRIu64" data=%p offset=%ld len=%zu", id, data, (long)offset, len);

	if (!data) {
		return 0;
	}

	if (obj_index >= OBJ_POOL_SIZE) {
		return -ENOENT;
	}

	*data = &objects[obj_index].data[offset];

	return len;
}

static ssize_t ots_obj_write(struct bt_ots *ots, struct bt_conn *conn,
			     uint64_t id, const void *data, size_t len,
			     off_t offset, size_t rem)
{
	uint32_t obj_index = OTS_OBJ_ID_TO_OBJ_IDX(id);

	LOG_DBG("id=%"PRIu64" data=%p offset=%ld len=%zu", id, data, (long)offset, len);

	if (obj_index >= OBJ_POOL_SIZE) {
		return -ENOENT;
	}

	(void)memcpy(&objects[obj_index].data[offset], data, len);

	return len;
}

static void ots_obj_name_written(struct bt_ots *ots, struct bt_conn *conn,
				 uint64_t id, const char *cur_name, const char *new_name)
{
	LOG_DBG("id=%"PRIu64"cur_name=%s new_name=%s", id, cur_name, new_name);
}

static int ots_obj_cal_checksum(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
				off_t offset, size_t len, void **data)
{
	uint32_t obj_index = OTS_OBJ_ID_TO_OBJ_IDX(id);

	if (obj_index >= OBJ_POOL_SIZE) {
		return -ENOENT;
	}

	*data = &objects[obj_index].data[offset];
	return 0;
}

static struct bt_ots_cb ots_callbacks = {
	.obj_created = ots_obj_created,
	.obj_deleted = ots_obj_deleted,
	.obj_selected = ots_obj_selected,
	.obj_read = ots_obj_read,
	.obj_write = ots_obj_write,
	.obj_name_written = ots_obj_name_written,
	.obj_cal_checksum = ots_obj_cal_checksum,
};

static int ots_init(void)
{
	int err;
	struct bt_ots_init_param ots_init;

	/* Configure OTS initialization. */
	(void)memset(&ots_init, 0, sizeof(ots_init));
	BT_OTS_OACP_SET_FEAT_READ(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_WRITE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_CREATE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_DELETE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_CHECKSUM(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_PATCH(ots_init.features.oacp);
	BT_OTS_OLCP_SET_FEAT_GO_TO(ots_init.features.olcp);
	ots_init.cb = &ots_callbacks;

	/* Initialize OTS instance. */
	err = bt_ots_init(ots, &ots_init);
	if (err) {
		LOG_ERR("Failed to init OTS (err:%d)\n", err);
		return err;
	}

	return 0;
}

uint8_t tester_init_ots(void)
{
	int err;

	/* TODO there is no API to return OTS instance to pool */
	if (!ots) {
		ots = bt_ots_free_instance_get();
	}

	if (!ots) {
		return BTP_STATUS_FAILED;
	}

	err = ots_init();
	if (err) {
		return BTP_STATUS_VAL(err);
	}

	tester_register_command_handlers(BTP_SERVICE_ID_OTS, ots_handlers,
					 ARRAY_SIZE(ots_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_ots(void)
{
	memset(objects, 0, sizeof(objects));

	return BTP_STATUS_SUCCESS;
}
