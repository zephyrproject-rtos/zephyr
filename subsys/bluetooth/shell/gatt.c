/** @file
 * @brief Bluetooth GATT shell functions
 *
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include "bt.h"
#include "gatt.h"

#define CHAR_SIZE_MAX           512

#if defined(CONFIG_BLUETOOTH_GATT_CLIENT)
static void exchange_func(struct bt_conn *conn, u8_t err,
			  struct bt_gatt_exchange_params *params)
{
	printk("Exchange %s\n", err == 0 ? "successful" : "failed");
}

static struct bt_gatt_exchange_params exchange_params;

int cmd_gatt_exchange_mtu(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(default_conn, &exchange_params);
	if (err) {
		printk("Exchange failed (err %d)\n", err);
	} else {
		printk("Exchange pending\n");
	}

	return 0;
}

static struct bt_gatt_discover_params discover_params;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static void print_chrc_props(u8_t properties)
{
	printk("Properties: ");

	if (properties & BT_GATT_CHRC_BROADCAST) {
		printk("[bcast]");
	}

	if (properties & BT_GATT_CHRC_READ) {
		printk("[read]");
	}

	if (properties & BT_GATT_CHRC_WRITE) {
		printk("[write]");
	}

	if (properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
		printk("[write w/w rsp]");
	}

	if (properties & BT_GATT_CHRC_NOTIFY) {
		printk("[notify]");
	}

	if (properties & BT_GATT_CHRC_INDICATE) {
		printk("[indicate]");
	}

	if (properties & BT_GATT_CHRC_AUTH) {
		printk("[auth]");
	}

	if (properties & BT_GATT_CHRC_EXT_PROP) {
		printk("[ext prop]");
	}

	printk("\n");
}

static u8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service_val *gatt_service;
	struct bt_gatt_chrc *gatt_chrc;
	struct bt_gatt_include *gatt_include;
	char uuid[37];

	if (!attr) {
		printk("Discover complete\n");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_SECONDARY:
	case BT_GATT_DISCOVER_PRIMARY:
		gatt_service = attr->user_data;
		bt_uuid_to_str(gatt_service->uuid, uuid, sizeof(uuid));
		printk("Service %s found: start handle %x, end_handle %x\n",
		       uuid, attr->handle, gatt_service->end_handle);
		break;
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		gatt_chrc = attr->user_data;
		bt_uuid_to_str(gatt_chrc->uuid, uuid, sizeof(uuid));
		printk("Characteristic %s found: handle %x\n", uuid,
		       attr->handle);
		print_chrc_props(gatt_chrc->properties);
		break;
	case BT_GATT_DISCOVER_INCLUDE:
		gatt_include = attr->user_data;
		bt_uuid_to_str(gatt_include->uuid, uuid, sizeof(uuid));
		printk("Include %s found: handle %x, start %x, end %x\n",
		       uuid, attr->handle, gatt_include->start_handle,
		       gatt_include->end_handle);
		break;
	default:
		bt_uuid_to_str(attr->uuid, uuid, sizeof(uuid));
		printk("Descriptor %s found: handle %x\n", uuid, attr->handle);
		break;
	}

	return BT_GATT_ITER_CONTINUE;
}

int cmd_gatt_discover(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	discover_params.func = discover_func;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;

	if (argc < 2) {
		if (!strcmp(argv[0], "gatt-discover-primary") ||
		    !strcmp(argv[0], "gatt-discover-secondary")) {
			return -EINVAL;
		}
		goto done;
	}

	/* Only set the UUID if the value is valid (non zero) */
	uuid.val = strtoul(argv[1], NULL, 16);
	if (uuid.val) {
		discover_params.uuid = &uuid.uuid;
	}

	if (argc > 2) {
		discover_params.start_handle = strtoul(argv[2], NULL, 16);
		if (argc > 3) {
			discover_params.end_handle = strtoul(argv[3], NULL, 16);
		}
	}

done:
	if (!strcmp(argv[0], "gatt-discover-secondary")) {
		discover_params.type = BT_GATT_DISCOVER_SECONDARY;
	} else if (!strcmp(argv[0], "gatt-discover-include")) {
		discover_params.type = BT_GATT_DISCOVER_INCLUDE;
	} else if (!strcmp(argv[0], "gatt-discover-characteristic")) {
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	} else if (!strcmp(argv[0], "gatt-discover-descriptor")) {
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
	} else {
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	}

	err = bt_gatt_discover(default_conn, &discover_params);
	if (err) {
		printk("Discover failed (err %d)\n", err);
	} else {
		printk("Discover pending\n");
	}

	return 0;
}

static struct bt_gatt_read_params read_params;

static u8_t read_func(struct bt_conn *conn, u8_t err,
			 struct bt_gatt_read_params *params,
			 const void *data, u16_t length)
{
	printk("Read complete: err %u length %u\n", err, length);

	if (!data) {
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

int cmd_gatt_read(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	read_params.func = read_func;

	if (argc < 2) {
		return -EINVAL;
	}

	read_params.handle_count = 1;
	read_params.single.handle = strtoul(argv[1], NULL, 16);

	if (argc > 2) {
		read_params.single.offset = strtoul(argv[2], NULL, 16);
	}

	err = bt_gatt_read(default_conn, &read_params);
	if (err) {
		printk("Read failed (err %d)\n", err);
	} else {
		printk("Read pending\n");
	}

	return 0;
}

int cmd_gatt_mread(int argc, char *argv[])
{
	u16_t h[8];
	int i, err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 3) {
		return -EINVAL;
	}

	if (argc - 1 >  ARRAY_SIZE(h)) {
		printk("Enter max %lu handle items to read\n", ARRAY_SIZE(h));
		return 0;
	}

	for (i = 0; i < argc - 1; i++) {
		h[i] = strtoul(argv[i + 1], NULL, 16);
	}

	read_params.func = read_func;
	read_params.handle_count = i;
	read_params.handles = h; /* not used in read func */

	err = bt_gatt_read(default_conn, &read_params);
	if (err) {
		printk("GATT multiple read request failed (err %d)\n", err);
	}

	return 0;
}

static struct bt_gatt_write_params write_params;
static u8_t gatt_write_buf[CHAR_SIZE_MAX];

static void write_func(struct bt_conn *conn, u8_t err,
		       struct bt_gatt_write_params *params)
{
	printk("Write complete: err %u\n", err);

	memset(&write_params, 0, sizeof(write_params));
}

int cmd_gatt_write(int argc, char *argv[])
{
	int err;
	u16_t handle, offset;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (write_params.func) {
		printk("Write ongoing\n");
		return 0;
	}

	if (argc < 4) {
		return -EINVAL;
	}

	handle = strtoul(argv[1], NULL, 16);
	offset = strtoul(argv[2], NULL, 16);

	gatt_write_buf[0] = strtoul(argv[3], NULL, 16);
	write_params.data = gatt_write_buf;
	write_params.length = 1;
	write_params.handle = handle;
	write_params.offset = offset;
	write_params.func = write_func;

	if (argc == 5) {
		size_t len;
		int i;

		len = min(strtoul(argv[4], NULL, 16), sizeof(gatt_write_buf));

		for (i = 1; i < len; i++) {
			gatt_write_buf[i] = gatt_write_buf[0];
		}

		write_params.length = len;
	}

	err = bt_gatt_write(default_conn, &write_params);
	if (err) {
		printk("Write failed (err %d)\n", err);
	} else {
		printk("Write pending\n");
	}

	return 0;
}

int cmd_gatt_write_without_rsp(int argc, char *argv[])
{
	int err;
	bool sign;
	u16_t handle;
	u16_t len;
	u16_t repeat;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 3) {
		return -EINVAL;
	}

	sign = !strcmp(argv[0], "gatt-write-signed");
	handle = strtoul(argv[1], NULL, 16);
	gatt_write_buf[0] = strtoul(argv[2], NULL, 16);
	len = 1;

	if (argc > 3) {
		int i;

		len = min(strtoul(argv[3], NULL, 16), sizeof(gatt_write_buf));

		for (i = 1; i < len; i++) {
			gatt_write_buf[i] = gatt_write_buf[0];
		}
	}

	repeat = 0;

	if (argc > 4) {
		repeat = strtoul(argv[4], NULL, 16);
	}

	if (!repeat) {
		repeat = 1;
	}

	while (repeat--) {
		err = bt_gatt_write_without_response(default_conn, handle,
						     gatt_write_buf, len, sign);
		if (err) {
			break;
		}
	}

	printk("Write Complete (err %d)\n", err);

	return 0;
}

static struct bt_gatt_subscribe_params subscribe_params;

static u8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, u16_t length)
{
	if (!data) {
		printk("Unsubscribed\n");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	printk("Notification: data %p length %u\n", data, length);

	return BT_GATT_ITER_CONTINUE;
}

int cmd_gatt_subscribe(int argc, char *argv[])
{
	int err;

	if (subscribe_params.value_handle) {
		printk("Cannot subscribe: subscription to %x already exists\n",
		       subscribe_params.value_handle);
		return 0;
	}

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (argc < 3) {
		return -EINVAL;
	}

	subscribe_params.ccc_handle = strtoul(argv[1], NULL, 16);
	subscribe_params.value_handle = strtoul(argv[2], NULL, 16);
	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	subscribe_params.notify = notify_func;

	if (argc > 3 && !strcmp(argv[3], "ind")) {
		subscribe_params.value = BT_GATT_CCC_INDICATE;
	}

	err = bt_gatt_subscribe(default_conn, &subscribe_params);
	if (err) {
		printk("Subscribe failed (err %d)\n", err);
	} else {
		printk("Subscribed\n");
	}

	return 0;
}

int cmd_gatt_unsubscribe(int argc, char *argv[])
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return 0;
	}

	if (!subscribe_params.value_handle) {
		printk("No subscription found\n");
		return 0;
	}

	err = bt_gatt_unsubscribe(default_conn, &subscribe_params);
	if (err) {
		printk("Unsubscribe failed (err %d)\n", err);
	} else {
		printk("Unsubscribe success\n");
	}

	return 0;
}
#endif /* CONFIG_BLUETOOTH_GATT_CLIENT */

static u8_t print_attr(const struct bt_gatt_attr *attr, void *user_data)
{
	printk("attr %p handle 0x%04x uuid %s perm 0x%02x\n",
		attr, attr->handle, bt_uuid_str(attr->uuid), attr->perm);

	return BT_GATT_ITER_CONTINUE;
}

int cmd_gatt_show_db(int argc, char *argv[])
{
	bt_gatt_foreach_attr(0x0001, 0xffff, print_attr, NULL);

	return 0;
}

/* Custom Service Variables */
static struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static struct bt_uuid_128 vnd_auth_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static const struct bt_uuid_128 vnd_long_uuid1 = BT_UUID_INIT_128(
	0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static const struct bt_uuid_128 vnd_long_uuid2 = BT_UUID_INIT_128(
	0xde, 0xad, 0xfa, 0xce, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static u8_t vnd_value[] = { 'V', 'e', 'n', 'd', 'o', 'r' };

static ssize_t read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset,
			 u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(vnd_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

#define MAX_DATA 30
static u8_t vnd_long_value1[MAX_DATA] = { 'V', 'e', 'n', 'd', 'o', 'r' };
static u8_t vnd_long_value2[MAX_DATA] = { 'S', 't', 'r', 'i', 'n', 'g' };

static ssize_t read_long_vnd(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     u16_t len, u16_t offset)
{
	u8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(vnd_long_value1));
}

static ssize_t write_long_vnd(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      u16_t len, u16_t offset, u8_t flags)
{
	u8_t *value = attr->user_data;

	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		return 0;
	}

	if (offset + len > sizeof(vnd_long_value1)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* Copy to buffer */
	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr vnd_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&vnd_uuid),

	BT_GATT_CHARACTERISTIC(&vnd_auth_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(&vnd_auth_uuid.uuid,
			   BT_GATT_PERM_READ_AUTHEN |
			   BT_GATT_PERM_WRITE_AUTHEN,
			   read_vnd, write_vnd, vnd_value),

	BT_GATT_CHARACTERISTIC(&vnd_long_uuid1.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP),
	BT_GATT_DESCRIPTOR(&vnd_long_uuid1.uuid,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
				BT_GATT_PERM_PREPARE_WRITE,
				read_long_vnd, write_long_vnd,
				&vnd_long_value1),

	BT_GATT_CHARACTERISTIC(&vnd_long_uuid2.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP),
	BT_GATT_DESCRIPTOR(&vnd_long_uuid2.uuid,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
				BT_GATT_PERM_PREPARE_WRITE,
				read_long_vnd, write_long_vnd,
				&vnd_long_value2),
};

static struct bt_gatt_service vnd_svc = BT_GATT_SERVICE(vnd_attrs);

int cmd_gatt_register_test_svc(int argc, char *argv[])
{
	bt_gatt_service_register(&vnd_svc);

	printk("Registering test vendor service\n");

	return 0;
}

int cmd_gatt_unregister_test_svc(int argc, char *argv[])
{
	bt_gatt_unregister_service(&vnd_svc);

	printk("Unregistering test vendor service\n");

	return 0;
}

static struct bt_uuid_128 met_svc_uuid = BT_UUID_INIT_128(
	0x01, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 met_char_uuid = BT_UUID_INIT_128(
	0x02, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static u8_t met_char_value[CHAR_SIZE_MAX] = {
	'M', 'e', 't', 'r', 'i', 'c', 's' };

static ssize_t read_met(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;
	u16_t value_len;

	value_len = min(strlen(value), CHAR_SIZE_MAX);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 value_len);
}

static u32_t write_count;
static u32_t write_len;
static u32_t write_rate;

static ssize_t write_met(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset,
			 u8_t flags)
{
	u8_t *value = attr->user_data;
	static u32_t cycle_stamp;
	u32_t delta;

	if (offset + len > sizeof(met_char_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	delta = k_cycle_get_32() - cycle_stamp;
	delta = SYS_CLOCK_HW_CYCLES_TO_NS(delta);

	/* if last data rx-ed was greater than 1 second in the past,
	 * reset the metrics.
	 */
	if (delta > 1000000000) {
		write_count = 0;
		write_len = 0;
		write_rate = 0;
		cycle_stamp = k_cycle_get_32();
	} else {
		write_count++;
		write_len += len;
		write_rate = ((u64_t)write_len << 3) * 1000000000 / delta;
	}

	return len;
}

static struct bt_gatt_attr met_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&met_svc_uuid),

	BT_GATT_CHARACTERISTIC(&met_char_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(&met_char_uuid.uuid,
			   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   read_met, write_met, met_char_value),
};

static struct bt_gatt_service met_svc = BT_GATT_SERVICE(met_attrs);

int cmd_gatt_write_cmd_metrics(int argc, char *argv[])
{
	int err = 0;

	if (argc < 2) {
		printk("Write: count= %u, len= %u, rate= %u bps.\n",
		       write_count, write_len, write_rate);

		return 0;
	}

	if (!strcmp(argv[1], "on")) {
		static bool registered;

		if (!registered) {
			printk("Registering GATT metrics test Service.\n");
			err = bt_gatt_service_register(&met_svc);
			registered = true;
		}
	} else if (!strcmp(argv[1], "off")) {
		printk("Not supported.\n");
		err = -EINVAL;
	} else {
		printk("Incorrect value: %s\n", argv[1]);
		return -EINVAL;
	}

	if (!err) {
		printk("GATT write cmd metrics %s.\n", argv[1]);
	}

	return err;
}
