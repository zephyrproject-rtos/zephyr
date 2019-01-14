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
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include <shell/shell.h>

#include "bt.h"

#define CHAR_SIZE_MAX           512

#if defined(CONFIG_BT_GATT_CLIENT)
static void exchange_func(struct bt_conn *conn, u8_t err,
			  struct bt_gatt_exchange_params *params)
{
	shell_print(ctx_shell, "Exchange %s", err == 0 ? "successful" :
		    "failed");
}

static struct bt_gatt_exchange_params exchange_params;

static int cmd_exchange_mtu(const struct shell *shell,
			     size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_print(shell, "Not connected");
		return -ENOEXEC;
	}

	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(default_conn, &exchange_params);
	if (err) {
		shell_print(shell, "Exchange failed (err %d)", err);
	} else {
		shell_print(shell, "Exchange pending");
	}

	return err;
}

static struct bt_gatt_discover_params discover_params;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static void print_chrc_props(const struct shell *shell, u8_t properties)
{
	shell_print(shell, "Properties: ");

	if (properties & BT_GATT_CHRC_BROADCAST) {
		shell_print(shell, "[bcast]");
	}

	if (properties & BT_GATT_CHRC_READ) {
		shell_print(shell, "[read]");
	}

	if (properties & BT_GATT_CHRC_WRITE) {
		shell_print(shell, "[write]");
	}

	if (properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
		shell_print(shell, "[write w/w rsp]");
	}

	if (properties & BT_GATT_CHRC_NOTIFY) {
		shell_print(shell, "[notify]");
	}

	if (properties & BT_GATT_CHRC_INDICATE) {
		shell_print(shell, "[indicate]");
	}

	if (properties & BT_GATT_CHRC_AUTH) {
		shell_print(shell, "[auth]");
	}

	if (properties & BT_GATT_CHRC_EXT_PROP) {
		shell_print(shell, "[ext prop]");
	}

	shell_print(shell, "");
}

static u8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service_val *gatt_service;
	struct bt_gatt_chrc *gatt_chrc;
	struct bt_gatt_include *gatt_include;
	char str[37];

	if (!attr) {
		shell_print(ctx_shell, "Discover complete");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_SECONDARY:
	case BT_GATT_DISCOVER_PRIMARY:
		gatt_service = attr->user_data;
		bt_uuid_to_str(gatt_service->uuid, str, sizeof(str));
		shell_print(ctx_shell, "Service %s found: start handle %x, "
			    "end_handle %x", str, attr->handle,
			    gatt_service->end_handle);
		break;
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		gatt_chrc = attr->user_data;
		bt_uuid_to_str(gatt_chrc->uuid, str, sizeof(str));
		shell_print(ctx_shell, "Characteristic %s found: handle %x",
			    str, attr->handle);
		print_chrc_props(ctx_shell, gatt_chrc->properties);
		break;
	case BT_GATT_DISCOVER_INCLUDE:
		gatt_include = attr->user_data;
		bt_uuid_to_str(gatt_include->uuid, str, sizeof(str));
		shell_print(ctx_shell, "Include %s found: handle %x, start %x, "
			    "end %x", str, attr->handle,
			    gatt_include->start_handle,
			    gatt_include->end_handle);
		break;
	default:
		bt_uuid_to_str(attr->uuid, str, sizeof(str));
		shell_print(ctx_shell, "Descriptor %s found: handle %x", str,
			    attr->handle);
		break;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int cmd_discover(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	discover_params.func = discover_func;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;

	if (argc > 1) {
		/* Only set the UUID if the value is valid (non zero) */
		uuid.val = strtoul(argv[1], NULL, 16);
		if (uuid.val) {
			discover_params.uuid = &uuid.uuid;
		}
	}

	if (argc > 2) {
		discover_params.start_handle = strtoul(argv[2], NULL, 16);
		if (argc > 3) {
			discover_params.end_handle = strtoul(argv[3], NULL, 16);
		}
	}

	if (!strcmp(argv[0], "discover-secondary")) {
		discover_params.type = BT_GATT_DISCOVER_SECONDARY;
	} else if (!strcmp(argv[0], "discover-include")) {
		discover_params.type = BT_GATT_DISCOVER_INCLUDE;
	} else if (!strcmp(argv[0], "discover-characteristic")) {
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	} else if (!strcmp(argv[0], "discover-descriptor")) {
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
	} else {
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	}

	err = bt_gatt_discover(default_conn, &discover_params);
	if (err) {
		shell_error(shell, "Discover failed (err %d)", err);
	} else {
		shell_print(shell, "Discover pending");
	}

	return err;
}

static struct bt_gatt_read_params read_params;

static u8_t read_func(struct bt_conn *conn, u8_t err,
			 struct bt_gatt_read_params *params,
			 const void *data, u16_t length)
{
	shell_print(ctx_shell, "Read complete: err %u length %u", err, length);

	if (!data) {
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int cmd_read(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	read_params.func = read_func;
	read_params.handle_count = 1;
	read_params.single.handle = strtoul(argv[1], NULL, 16);
	read_params.single.offset = 0;

	if (argc > 2) {
		read_params.single.offset = strtoul(argv[2], NULL, 16);
	}

	err = bt_gatt_read(default_conn, &read_params);
	if (err) {
		shell_error(shell, "Read failed (err %d)", err);
	} else {
		shell_print(shell, "Read pending");
	}

	return err;
}

static int cmd_mread(const struct shell *shell, size_t argc, char *argv[])
{
	u16_t h[8];
	int i, err;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	if ((argc - 1) >  ARRAY_SIZE(h)) {
		shell_print(shell, "Enter max %lu handle items to read",
			    ARRAY_SIZE(h));
		return -EINVAL;
	}

	for (i = 0; i < argc - 1; i++) {
		h[i] = strtoul(argv[i + 1], NULL, 16);
	}

	read_params.func = read_func;
	read_params.handle_count = i;
	read_params.handles = h; /* not used in read func */

	err = bt_gatt_read(default_conn, &read_params);
	if (err) {
		shell_error(shell, "GATT multiple read request failed (err %d)",
			    err);
	}

	return err;
}

static struct bt_gatt_write_params write_params;
static u8_t gatt_write_buf[CHAR_SIZE_MAX];

static void write_func(struct bt_conn *conn, u8_t err,
		       struct bt_gatt_write_params *params)
{
	shell_print(ctx_shell, "Write complete: err %u", err);

	(void)memset(&write_params, 0, sizeof(write_params));
}

static int cmd_write(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	u16_t handle, offset;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	if (write_params.func) {
		shell_error(shell, "Write ongoing");
		return -ENOEXEC;
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
		shell_error(shell, "Write failed (err %d)", err);
	} else {
		shell_print(shell, "Write pending");
	}

	return err;
}

static void write_without_rsp_cb(struct bt_conn *conn)
{
	shell_print(ctx_shell, "Write transmission complete");
}

static int cmd_write_without_rsp(const struct shell *shell,
				 size_t argc, char *argv[])
{
	u16_t handle;
	u16_t repeat;
	int err;
	u16_t len;
	bool sign;
	bt_gatt_complete_func_t func = NULL;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	sign = !strcmp(argv[0], "signed-write");
	if (!sign) {
		if (!strcmp(argv[0], "write-without-response-cb")) {
			func = write_without_rsp_cb;
		}
	}

	handle = strtoul(argv[1], NULL, 16);
	gatt_write_buf[0] = strtoul(argv[2], NULL, 16);
	len = 1U;

	if (argc > 3) {
		int i;

		len = min(strtoul(argv[3], NULL, 16), sizeof(gatt_write_buf));

		for (i = 1; i < len; i++) {
			gatt_write_buf[i] = gatt_write_buf[0];
		}
	}

	repeat = 0U;

	if (argc > 4) {
		repeat = strtoul(argv[4], NULL, 16);
	}

	if (!repeat) {
		repeat = 1U;
	}

	while (repeat--) {
		err = bt_gatt_write_without_response_cb(default_conn, handle,
							gatt_write_buf, len,
							sign, func);
		if (err) {
			break;
		}
	}

	shell_print(shell, "Write Complete (err %d)", err);
	return err;
}

static struct bt_gatt_subscribe_params subscribe_params;

static u8_t notify_func(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, u16_t length)
{
	if (!data) {
		shell_print(ctx_shell, "Unsubscribed");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	shell_print(ctx_shell, "Notification: data %p length %u", data, length);

	return BT_GATT_ITER_CONTINUE;
}

static int cmd_subscribe(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (subscribe_params.value_handle) {
		shell_error(shell, "Cannot subscribe: subscription to %x "
			    "already exists", subscribe_params.value_handle);
		return -ENOEXEC;
	}

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
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
		shell_error(shell, "Subscribe failed (err %d)", err);
	} else {
		shell_print(shell, "Subscribed");
	}

	return err;
}

static int cmd_unsubscribe(const struct shell *shell,
			   size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	if (!subscribe_params.value_handle) {
		shell_error(shell, "No subscription found");
		return -ENOEXEC;
	}

	err = bt_gatt_unsubscribe(default_conn, &subscribe_params);
	if (err) {
		shell_error(shell, "Unsubscribe failed (err %d)", err);
	} else {
		shell_print(shell, "Unsubscribe success");
	}

	return err;
}
#endif /* CONFIG_BT_GATT_CLIENT */

static u8_t print_attr(const struct bt_gatt_attr *attr, void *user_data)
{
	const struct shell *shell = user_data;

	shell_print(shell, "attr %p handle 0x%04x uuid %s perm 0x%02x",
	      attr, attr->handle, bt_uuid_str(attr->uuid), attr->perm);

	return BT_GATT_ITER_CONTINUE;
}

static int cmd_show_db(const struct shell *shell, size_t argc, char *argv[])
{
	bt_gatt_foreach_attr(0x0001, 0xffff, print_attr, (void *)shell);
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

static struct bt_uuid_128 vnd1_uuid = BT_UUID_INIT_128(
	0xf4, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 vnd1_echo_uuid = BT_UUID_INIT_128(
	0xf5, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_gatt_ccc_cfg vnd1_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t echo_enabled;

static void vnd1_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	echo_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t write_vnd1(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, u16_t len, u16_t offset,
			  u8_t flags)
{
	if (echo_enabled) {
		shell_print(ctx_shell, "Echo attr len %u", len);
		bt_gatt_notify(conn, attr, buf, len);
	}

	return len;
}

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
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       read_vnd, write_vnd, vnd_value),

	BT_GATT_CHARACTERISTIC(&vnd_long_uuid1.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
			       BT_GATT_PERM_PREPARE_WRITE,
			       read_long_vnd, write_long_vnd,
			       &vnd_long_value1),

	BT_GATT_CHARACTERISTIC(&vnd_long_uuid2.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
			       BT_GATT_PERM_PREPARE_WRITE,
			       read_long_vnd, write_long_vnd,
			       &vnd_long_value2),
};

static struct bt_gatt_service vnd_svc = BT_GATT_SERVICE(vnd_attrs);

static struct bt_gatt_attr vnd1_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&vnd1_uuid),

	BT_GATT_CHARACTERISTIC(&vnd1_echo_uuid.uuid,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP |
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_WRITE, NULL, write_vnd1, NULL),
	BT_GATT_CCC(vnd1_ccc_cfg, vnd1_ccc_cfg_changed),
};

static struct bt_gatt_service vnd1_svc = BT_GATT_SERVICE(vnd1_attrs);

static int cmd_register_test_svc(const struct shell *shell,
				  size_t argc, char *argv[])
{
	bt_gatt_service_register(&vnd_svc);
	bt_gatt_service_register(&vnd1_svc);

	shell_print(shell, "Registering test vendor services");
	return 0;
}

static int cmd_unregister_test_svc(const struct shell *shell,
				    size_t argc, char *argv[])
{
	bt_gatt_service_unregister(&vnd_svc);
	bt_gatt_service_unregister(&vnd1_svc);

	shell_print(shell, "Unregistering test vendor services");
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
		write_count = 0U;
		write_len = 0U;
		write_rate = 0U;
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
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_met, write_met, met_char_value),
};

static struct bt_gatt_service met_svc = BT_GATT_SERVICE(met_attrs);

static int cmd_metrics(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;

	if (argc < 2) {
		shell_print(shell, "Write: count= %u, len= %u, rate= %u bps.",
		       write_count, write_len, write_rate);

		return -ENOEXEC;
	}

	if (!strcmp(argv[1], "on")) {
		static bool registered;

		if (!registered) {
			shell_print(shell, "Registering GATT metrics test "
			      "Service.");
			err = bt_gatt_service_register(&met_svc);
			registered = true;
		}
	} else if (!strcmp(argv[1], "off")) {
		shell_print(shell, "Unregistering GATT metrics test Service.");
		err = bt_gatt_service_unregister(&met_svc);
	} else {
		shell_error(shell, "Incorrect value: %s", argv[1]);
		shell_help(shell);
		return -ENOEXEC;
	}

	if (!err) {
		shell_print(shell, "GATT write cmd metrics %s.", argv[1]);
	}

	return err;
}

#define HELP_NONE "[none]"

SHELL_CREATE_STATIC_SUBCMD_SET(gatt_cmds) {
#if defined(CONFIG_BT_GATT_CLIENT)
	SHELL_CMD_ARG(discover-characteristic, NULL,
		      "[UUID] [start handle] [end handle]", cmd_discover, 1, 3),
	SHELL_CMD_ARG(discover-descriptor, NULL,
		      "[UUID] [start handle] [end handle]", cmd_discover, 1, 3),
	SHELL_CMD_ARG(discover-include, NULL,
		      "[UUID] [start handle] [end handle]", cmd_discover, 1, 3),
	SHELL_CMD_ARG(discover-primary, NULL,
		      "[UUID] [start handle] [end handle]", cmd_discover, 1, 3),
	SHELL_CMD_ARG(discover-secondary, NULL,
		      "[UUID] [start handle] [end handle]", cmd_discover, 1, 3),
	SHELL_CMD_ARG(exchange-mtu, NULL, HELP_NONE, cmd_exchange_mtu, 1, 0),
	SHELL_CMD_ARG(read, NULL, "<handle> [offset]", cmd_read, 2, 1),
	SHELL_CMD_ARG(read-multiple, NULL, "<handle 1> <handle 2> ...",
		      cmd_mread, 2, -1),
	SHELL_CMD_ARG(signed-write, NULL, "<handle> <data> [length] [repeat]",
		      cmd_write_without_rsp, 3, 2),
	SHELL_CMD_ARG(subscribe, NULL, "<CCC handle> <value handle> [ind]",
		      cmd_subscribe, 3, 1),
	SHELL_CMD_ARG(write, NULL, "<handle> <offset> <data> [length]",
		      cmd_write, 4, 1),
	SHELL_CMD_ARG(write-without-response, NULL,
		      "<handle> <data> [length] [repeat]",
		      cmd_write_without_rsp, 3, 2),
	SHELL_CMD_ARG(write-without-response-cb, NULL,
		      "<handle> <data> [length] [repeat]",
		      cmd_write_without_rsp, 3, 2),
	SHELL_CMD_ARG(unsubscribe, NULL, HELP_NONE, cmd_unsubscribe, 1, 0),
#endif /* CONFIG_BT_GATT_CLIENT */
	SHELL_CMD_ARG(metrics, NULL,
		      "register vendr char and measure rx <value: on, off>",
		      cmd_metrics, 2, 0),
	SHELL_CMD_ARG(register, NULL,
		      "register pre-predefined test service",
		      cmd_register_test_svc, 1, 0),
	SHELL_CMD_ARG(show-db, NULL, HELP_NONE, cmd_show_db, 1, 0),
	SHELL_CMD_ARG(unregister, NULL,
		      "unregister pre-predefined test service",
		      cmd_unregister_test_svc, 1, 0),
	SHELL_SUBCMD_SET_END
};

static int cmd_gatt(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(gatt, &gatt_cmds, "Bluetooth GATT shell commands",
		       cmd_gatt, 1, 1);
