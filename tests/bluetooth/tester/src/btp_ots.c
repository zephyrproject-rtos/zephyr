/* btp_ots.c - Bluetooth OTS Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/services/ots.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <../../subsys/bluetooth/services/ots/ots_internal.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_ots
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#define DEVICE_NAME      CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN  (sizeof(DEVICE_NAME) - 1)

#define OBJ_POOL_SIZE CONFIG_BT_OTS_MAX_OBJ_CNT
#define OBJ_MAX_SIZE  100

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_OTS_VAL)),
};

static struct {
	uint8_t data[OBJ_MAX_SIZE];
	char name[CONFIG_BT_OTS_OBJ_MAX_NAME_LEN + 1];
} objects[OBJ_POOL_SIZE];
static uint32_t obj_cnt;

struct object_creation_data {
	struct bt_ots_obj_size size;
	char *name;
	uint32_t props;
};

#define OTS_OBJ_ID_TO_OBJ_IDX(id) (((id) - BT_OTS_OBJ_ID_MIN) % ARRAY_SIZE(objects))

static struct object_creation_data *object_being_created;

/* Object Transfer Service */

static uint8_t ots_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	struct btp_ots_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_OTS_READ_SUPPORTED_COMMANDS);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler ots_handlers[] = {
	{
		.opcode = BTP_OTS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = ots_supported_commands,
	},
};

static int ots_obj_created_cb(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
			      const struct bt_ots_obj_add_param *add_param,
			      struct bt_ots_obj_created_desc *created_desc)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];
	uint64_t index;

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	if (obj_cnt >= ARRAY_SIZE(objects)) {
		printk("No item from Object pool is available for Object "
		       "with %s ID\n", id_str);
		return -ENOMEM;
	}

	if (add_param->size > OBJ_MAX_SIZE) {
		LOG_DBG("Object pool item is too small for Object with %s ID\n",
		        id_str);
		return -ENOMEM;
	}

	if (object_being_created) {
		created_desc->name = object_being_created->name;
		created_desc->size = object_being_created->size;
		created_desc->props = object_being_created->props;
	} else {
		index = id - BT_OTS_OBJ_ID_MIN;
		objects[index].name[0] = '\0';

		created_desc->name = objects[index].name;
		created_desc->size.alloc = OBJ_MAX_SIZE;
		BT_OTS_OBJ_SET_PROP_READ(created_desc->props);
		BT_OTS_OBJ_SET_PROP_WRITE(created_desc->props);
		BT_OTS_OBJ_SET_PROP_PATCH(created_desc->props);
		BT_OTS_OBJ_SET_PROP_DELETE(created_desc->props);
	}

	LOG_DBG("Object with %s ID has been created\n", id_str);
	obj_cnt++;

	return 0;
}

static int ots_obj_deleted_cb(struct bt_ots *ots, struct bt_conn *conn, uint64_t id)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	LOG_DBG("Object with %s ID has been deleted\n", id_str);

	obj_cnt--;

	return 0;
}

static ssize_t ots_obj_read_cb(struct bt_ots *ots, struct bt_conn *conn, uint64_t id, void **data,
			       size_t len, off_t offset)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];
	uint32_t obj_index = OTS_OBJ_ID_TO_OBJ_IDX(id);

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	if (!data) {
		printk("Object with %s ID has been successfully read\n",
		       id_str);

		return 0;
	}

	*data = &objects[obj_index].data[offset];

	/* Send even-indexed objects in 20 byte packets
	 * to demonstrate fragmented transmission.
	 */
	if ((obj_index % 2) == 0) {
		len = (len < 20) ? len : 20;
	}

	printk("Object with %s ID is being read\n"
	       "Offset = %lu, Length = %zu\n",
	       id_str, (long)offset, len);

	return len;
}

static ssize_t ots_obj_write_cb(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
				const void *data, size_t len, off_t offset, size_t rem)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];
	uint32_t obj_index = OTS_OBJ_ID_TO_OBJ_IDX(id);

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	printk("Object with %s ID is being written\n"
	       "Offset = %lu, Length = %zu, Remaining= %zu\n",
	       id_str, (long)offset, len, rem);

	(void)memcpy(&objects[obj_index].data[offset], data, len);

	return len;
}

static struct bt_ots_cb ots_cbs = {
	.obj_created = ots_obj_created_cb,
	.obj_deleted = ots_obj_deleted_cb,
	.obj_read = ots_obj_read_cb,
	.obj_write = ots_obj_write_cb,
};

uint8_t tester_init_ots(void)
{
	int err;
	struct bt_ots *ots;
	struct bt_ots_init_param ots_init;
	struct object_creation_data obj_data;
	struct bt_ots_obj_add_param obj;
	const char * const first_object_name = "first_object";
	uint32_t cur_size;
	uint32_t alloc_size;

	ots = bt_ots_free_instance_get();
	if (!ots) {
		printk("Failed to retrieve OTS instance\n");
		return BTP_STATUS_FAILED;
	}

	memset(&ots_init, 0, sizeof(ots_init));
	BT_OTS_OACP_SET_FEAT_READ(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_WRITE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_CREATE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_DELETE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_PATCH(ots_init.features.oacp);
	BT_OTS_OLCP_SET_FEAT_GO_TO(ots_init.features.olcp);
	ots_init.cb = &ots_cbs;

	err = bt_ots_init(ots, &ots_init);
	if (err) {
		LOG_DBG("Failed to register callbacks: %d", err);
		return BTP_STATUS_FAILED;
	}

	/* Add test object. */
	cur_size = sizeof(objects[0].data) / 2;
	alloc_size = sizeof(objects[0].data);
	for (uint32_t i = 0; i < cur_size; i++) {
		objects[0].data[i] = i + 1;
	}

	(void)memset(&obj_data, 0, sizeof(obj_data));
	__ASSERT(strlen(first_object_name) <= CONFIG_BT_OTS_OBJ_MAX_NAME_LEN,
		 "Object name length is larger than the allowed maximum of %u",
		 CONFIG_BT_OTS_OBJ_MAX_NAME_LEN);
	(void)strcpy(objects[0].name, first_object_name);
	obj_data.name = objects[0].name;
	obj_data.size.cur = cur_size;
	obj_data.size.alloc = alloc_size;
	BT_OTS_OBJ_SET_PROP_READ(obj_data.props);
	BT_OTS_OBJ_SET_PROP_WRITE(obj_data.props);
	BT_OTS_OBJ_SET_PROP_PATCH(obj_data.props);
	object_being_created = &obj_data;

	obj.size = alloc_size;
	obj.type.uuid.type = BT_UUID_TYPE_16;
	obj.type.uuid_16.val = BT_UUID_OTS_TYPE_UNSPECIFIED_VAL;
	err = bt_ots_obj_add(ots, &obj);
	object_being_created = NULL;
	if (err < 0) {
		LOG_DBG("Failed to add an object to OTS (err: %d)\n", err);
		return err;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_OTS, ots_handlers,
					 ARRAY_SIZE(ots_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_ots(void)
{
	return BTP_STATUS_SUCCESS;
}
