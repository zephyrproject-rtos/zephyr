/** @file
 *  @brief GATT Generic Access Service - default implementation
 */
/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2025 Koppel Electronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

#define BT_GATT_CENTRAL_ADDR_RES_NOT_SUPP	0
#define BT_GATT_CENTRAL_ADDR_RES_SUPP		1


static ssize_t read_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *name = bt_get_name();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name,
				 strlen(name));
}

#if defined(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE)

static ssize_t write_name(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	/* adding one to fit the terminating null character */
	char value[CONFIG_BT_DEVICE_NAME_MAX + 1] = {};

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len > CONFIG_BT_DEVICE_NAME_MAX) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value, buf, len);

	value[len] = '\0';

	bt_set_name(value);

	return len;
}

#endif /* CONFIG_BT_DEVICE_NAME_GATT_WRITABLE */

static ssize_t read_appearance(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint16_t appearance = sys_cpu_to_le16(bt_get_appearance());

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &appearance,
				 sizeof(appearance));
}

#if defined(CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE)
static ssize_t write_appearance(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint16_t appearance;
	int err;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(appearance)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	appearance = sys_get_le16(buf);

	err = bt_set_appearance(appearance);

	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return len;
}
#endif /* CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE */

#if defined(CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE)
	#define GAP_APPEARANCE_PROPS (BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE)
#if defined(CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE_AUTHEN)
	#define GAP_APPEARANCE_PERMS (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN)
#elif defined(CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE_ENCRYPT)
	#define GAP_APPEARANCE_PERMS (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)
#else
	#define GAP_APPEARANCE_PERMS (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
#endif
	#define GAP_APPEARANCE_WRITE_HANDLER write_appearance
#else
	#define GAP_APPEARANCE_PROPS BT_GATT_CHRC_READ
	#define GAP_APPEARANCE_PERMS BT_GATT_PERM_READ
	#define GAP_APPEARANCE_WRITE_HANDLER NULL
#endif

#if defined(CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS)
static ssize_t read_ppcp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	struct __packed {
		uint16_t min_int;
		uint16_t max_int;
		uint16_t latency;
		uint16_t timeout;
	} ppcp;

	ppcp.min_int = sys_cpu_to_le16(CONFIG_BT_PERIPHERAL_PREF_MIN_INT);
	ppcp.max_int = sys_cpu_to_le16(CONFIG_BT_PERIPHERAL_PREF_MAX_INT);
	ppcp.latency = sys_cpu_to_le16(CONFIG_BT_PERIPHERAL_PREF_LATENCY);
	ppcp.timeout = sys_cpu_to_le16(CONFIG_BT_PERIPHERAL_PREF_TIMEOUT);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &ppcp,
				 sizeof(ppcp));
}
#endif

#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PRIVACY)
static ssize_t read_central_addr_res(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr, void *buf,
				     uint16_t len, uint16_t offset)
{
	uint8_t central_addr_res = BT_GATT_CENTRAL_ADDR_RES_SUPP;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &central_addr_res, sizeof(central_addr_res));
}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_PRIVACY */

BT_GATT_SERVICE_DEFINE(_2_gap_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GAP),
#if defined(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE)
	/* Require pairing for writes to device name */
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_DEVICE_NAME,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ |
#if defined(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE_AUTHEN)
			       BT_GATT_PERM_WRITE_AUTHEN,
#elif defined(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE_ENCRYPT)
			       BT_GATT_PERM_WRITE_ENCRYPT,
#else
			       BT_GATT_PERM_WRITE,
#endif
			       read_name, write_name, NULL),
#else
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_DEVICE_NAME, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_name, NULL, NULL),
#endif /* CONFIG_BT_DEVICE_NAME_GATT_WRITABLE */
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_APPEARANCE, GAP_APPEARANCE_PROPS,
			       GAP_APPEARANCE_PERMS, read_appearance,
			       GAP_APPEARANCE_WRITE_HANDLER, NULL),
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PRIVACY)
	BT_GATT_CHARACTERISTIC(BT_UUID_CENTRAL_ADDR_RES,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_central_addr_res, NULL, NULL),
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_PRIVACY */
#if defined(CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS)
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_PPCP, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_ppcp, NULL, NULL),
#endif
);
