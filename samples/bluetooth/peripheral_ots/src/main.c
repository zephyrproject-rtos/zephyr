/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/ots.h>

#define DEVICE_NAME      CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN  (sizeof(DEVICE_NAME) - 1)

#define OBJ_POOL_SIZE 2
#define OBJ_MAX_SIZE  100

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_OTS_VAL)),
};

static uint8_t objects[OBJ_POOL_SIZE][OBJ_MAX_SIZE];
static uint32_t obj_cnt;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected        = connected,
	.disconnected     = disconnected,
};

static int ots_obj_created(struct bt_ots *ots, struct bt_conn *conn,
			   uint64_t id,
			   const struct bt_ots_obj_metadata *init)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	if (obj_cnt >= ARRAY_SIZE(objects)) {
		printk("No item from Object pool is available for Object "
		       "with %s ID\n", id_str);
		return -ENOMEM;
	}

	if (init->size.alloc > OBJ_MAX_SIZE) {
		printk("Object pool item is too small for Object with %s ID\n",
		       id_str);
		return -ENOMEM;
	}

	printk("Object with %s ID has been created\n", id_str);
	obj_cnt++;

	return 0;
}

static void ots_obj_deleted(struct bt_ots *ots, struct bt_conn *conn,
			    uint64_t id)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	printk("Object with %s ID has been deleted\n", id_str);

	obj_cnt--;
}

static void ots_obj_selected(struct bt_ots *ots, struct bt_conn *conn,
			     uint64_t id)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	printk("Object with %s ID has been selected\n", id_str);
}

static uint32_t ots_obj_read(struct bt_ots *ots, struct bt_conn *conn,
			     uint64_t id, uint8_t **data, uint32_t len,
			     uint32_t offset)
{
	char id_str[BT_OTS_OBJ_ID_STR_LEN];
	uint32_t obj_index = (id % ARRAY_SIZE(objects));

	bt_ots_obj_id_to_str(id, id_str, sizeof(id_str));

	if (!data) {
		printk("Object with %s ID has been successfully read\n",
		       id_str);

		return 0;
	}

	*data = &objects[obj_index][offset];

	/* Send even-indexed objects in 20 byte packets
	 * to demonstrate fragmented transmission.
	 */
	if ((obj_index % 2) == 0) {
		len = (len < 20) ? len : 20;
	}

	printk("Object with %s ID is being read\n"
		"Offset = %d, Length = %d\n",
		id_str, offset, len);

	return len;
}

static struct bt_ots_cb ots_callbacks = {
	.obj_created = ots_obj_created,
	.obj_deleted = ots_obj_deleted,
	.obj_selected = ots_obj_selected,
	.obj_read = ots_obj_read,
};

static int ots_init(void)
{
	int err;
	struct bt_ots *ots;
	struct bt_ots_init ots_init;
	struct bt_ots_obj_metadata obj_init;

	ots = bt_ots_free_instance_get();
	if (!ots) {
		printk("Failed to retrieve OTS instance\n");
		return -ENOMEM;
	}

	/* Configure OTS initialization. */
	memset(&ots_init, 0, sizeof(ots_init));
	BT_OTS_OACP_SET_FEAT_READ(ots_init.features.oacp);
	BT_OTS_OLCP_SET_FEAT_GO_TO(ots_init.features.olcp);
	ots_init.cb = &ots_callbacks;

	/* Initialize OTS instance. */
	err = bt_ots_init(ots, &ots_init);
	if (err) {
		printk("Failed to init OTS (err:%d)\n", err);
		return err;
	}

	/* Prepare first object demo data and add it to the instance. */
	for (uint32_t i = 0; i < sizeof(objects[0]); i++) {
		objects[0][i] = i + 1;
	}

	memset(&obj_init, 0, sizeof(obj_init));
	obj_init.name = "first_object.txt";
	obj_init.type.uuid.type = BT_UUID_TYPE_16;
	obj_init.type.uuid_16.val = BT_UUID_OTS_TYPE_UNSPECIFIED_VAL;
	obj_init.size.cur = sizeof(objects[0]);
	obj_init.size.alloc = sizeof(objects[0]);
	BT_OTS_OBJ_SET_PROP_READ(obj_init.props);

	err = bt_ots_obj_add(ots, &obj_init);
	if (err) {
		printk("Failed to add an object to OTS (err: %d)\n", err);
		return err;
	}

	/* Prepare second object demo data and add it to the instance. */
	for (uint32_t i = 0; i < sizeof(objects[1]); i++) {
		objects[1][i] = i * 2;
	}

	memset(&obj_init, 0, sizeof(obj_init));
	obj_init.name = "second_object.gif";
	obj_init.type.uuid.type = BT_UUID_TYPE_16;
	obj_init.type.uuid_16.val = BT_UUID_OTS_TYPE_UNSPECIFIED_VAL;
	obj_init.size.cur = sizeof(objects[1]);
	obj_init.size.alloc = sizeof(objects[1]);
	BT_OTS_OBJ_SET_PROP_READ(obj_init.props);

	err = bt_ots_obj_add(ots, &obj_init);
	if (err) {
		printk("Failed to add an object to OTS (err: %d)\n", err);
		return err;
	}

	return 0;
}

void main(void)
{
	int err;

	printk("Starting Bluetooth Peripheral OTS example\n");

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = ots_init();
	if (err) {
		printk("Failed to init OTS (err:%d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}
