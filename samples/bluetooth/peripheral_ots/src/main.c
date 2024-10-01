/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/bluetooth/services/ots.h>

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

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed, err %u %s\n", err, bt_hci_err_to_str(err));
		return;
	}

	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason %u %s\n", reason, bt_hci_err_to_str(reason));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static int ots_obj_created(struct bt_ots *ots, struct bt_conn *conn, uint64_t id,
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
		printk("Object pool item is too small for Object with %s ID\n",
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

	printk("Object with %s ID has been created\n", id_str);
	obj_cnt++;

	return 0;
}

static int ots_obj_deleted(struct bt_ots *ots, struct bt_conn *conn,
			    uint64_t id)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	printk("Object with %s ID has been deleted\n", id_str);

	obj_cnt--;

	return 0;
}

static void ots_obj_selected(struct bt_ots *ots, struct bt_conn *conn,
			     uint64_t id)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	printk("Object with %s ID has been selected\n", id_str);
}

static ssize_t ots_obj_read(struct bt_ots *ots, struct bt_conn *conn,
			    uint64_t id, void **data, size_t len,
			    off_t offset)
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

static ssize_t ots_obj_write(struct bt_ots *ots, struct bt_conn *conn,
			     uint64_t id, const void *data, size_t len,
			     off_t offset, size_t rem)
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

static void ots_obj_name_written(struct bt_ots *ots, struct bt_conn *conn,
				 uint64_t id, const char *cur_name, const char *new_name)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	printk("Name for object with %s ID is being changed from '%s' to '%s'\n",
		id_str, cur_name, new_name);
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
	struct bt_ots *ots;
	struct object_creation_data obj_data;
	struct bt_ots_init_param ots_init;
	struct bt_ots_obj_add_param param;
	const char * const first_object_name = "first_object.txt";
	const char * const second_object_name = "second_object.gif";
	uint32_t cur_size;
	uint32_t alloc_size;

	ots = bt_ots_free_instance_get();
	if (!ots) {
		printk("Failed to retrieve OTS instance\n");
		return -ENOMEM;
	}

	/* Configure OTS initialization. */
	(void)memset(&ots_init, 0, sizeof(ots_init));
	BT_OTS_OACP_SET_FEAT_READ(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_WRITE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_CREATE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_DELETE(ots_init.features.oacp);
	BT_OTS_OACP_SET_FEAT_PATCH(ots_init.features.oacp);
	BT_OTS_OLCP_SET_FEAT_GO_TO(ots_init.features.olcp);
	ots_init.cb = &ots_callbacks;

	/* Initialize OTS instance. */
	err = bt_ots_init(ots, &ots_init);
	if (err) {
		printk("Failed to init OTS (err:%d)\n", err);
		return err;
	}

	/* Prepare first object demo data and add it to the instance. */
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

	param.size = alloc_size;
	param.type.uuid.type = BT_UUID_TYPE_16;
	param.type.uuid_16.val = BT_UUID_OTS_TYPE_UNSPECIFIED_VAL;
	err = bt_ots_obj_add(ots, &param);
	object_being_created = NULL;
	if (err < 0) {
		printk("Failed to add an object to OTS (err: %d)\n", err);
		return err;
	}

	/* Prepare second object demo data and add it to the instance. */
	cur_size = sizeof(objects[0].data);
	alloc_size = sizeof(objects[0].data);
	for (uint32_t i = 0; i < cur_size; i++) {
		objects[1].data[i] = i * 2;
	}

	(void)memset(&obj_data, 0, sizeof(obj_data));
	__ASSERT(strlen(second_object_name) <= CONFIG_BT_OTS_OBJ_MAX_NAME_LEN,
		 "Object name length is larger than the allowed maximum of %u",
		 CONFIG_BT_OTS_OBJ_MAX_NAME_LEN);
	(void)strcpy(objects[1].name, second_object_name);
	obj_data.name = objects[1].name;
	obj_data.size.cur = cur_size;
	obj_data.size.alloc = alloc_size;
	BT_OTS_OBJ_SET_PROP_READ(obj_data.props);
	object_being_created = &obj_data;

	param.size = alloc_size;
	param.type.uuid.type = BT_UUID_TYPE_16;
	param.type.uuid_16.val = BT_UUID_OTS_TYPE_UNSPECIFIED_VAL;
	err = bt_ots_obj_add(ots, &param);
	object_being_created = NULL;
	if (err < 0) {
		printk("Failed to add an object to OTS (err: %d)\n", err);
		return err;
	}

	return 0;
}

int main(void)
{
	int err;

	printk("Starting Bluetooth Peripheral OTS example\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	err = ots_init();
	if (err) {
		printk("Failed to init OTS (err:%d)\n", err);
		return 0;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	printk("Advertising successfully started\n");
	return 0;
}
