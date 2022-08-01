/* Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file gap_svc.c
 *  @brief GATT Service for Generic Access Profile (GAP)
 *
 *  The GAP service exposes parts of the GAP API over GATT.
 *
 *  The GAP service is mandatory on all GATT servers.
 *
 *  This file should not be compiled in when there is no GATT server.
 */

#include <stdint.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

static ssize_t read_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *name = bt_get_name();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name,
				 strlen(name));
}

#if defined(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE)
static ssize_t write_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	char value[CONFIG_BT_DEVICE_NAME_MAX] = {};

	if (offset >= sizeof(value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (offset + len >= sizeof(value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value, buf, len);

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
	uint16_t appearance_le = sys_cpu_to_le16(bt_get_appearance());
	char * const appearance_le_bytes = (char *)&appearance_le;
	uint16_t appearance;
	int err;

	if (offset >= sizeof(appearance_le)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if ((offset + len) > sizeof(appearance_le)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&appearance_le_bytes[offset], buf, len);
	appearance = sys_le16_to_cpu(appearance_le);

	err = bt_set_appearance(appearance);

	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return len;
}
#endif /* CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE */

#if CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE
	#define GAP_APPEARANCE_PROPS (BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE)
	#define GAP_APPEARANCE_PERMS (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN)
	#define GAP_APPEARANCE_WRITE_HANDLER write_appearance
#else
	#define GAP_APPEARANCE_PROPS BT_GATT_CHRC_READ
	#define GAP_APPEARANCE_PERMS BT_GATT_PERM_READ
	#define GAP_APPEARANCE_WRITE_HANDLER NULL
#endif

#if defined (CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS)
/* This checks if the range entered is valid */
BUILD_ASSERT(!(CONFIG_BT_PERIPHERAL_PREF_MIN_INT > 3200 &&
	     CONFIG_BT_PERIPHERAL_PREF_MIN_INT < 0xffff));
BUILD_ASSERT(!(CONFIG_BT_PERIPHERAL_PREF_MAX_INT > 3200 &&
	     CONFIG_BT_PERIPHERAL_PREF_MAX_INT < 0xffff));
BUILD_ASSERT(!(CONFIG_BT_PERIPHERAL_PREF_TIMEOUT > 3200 &&
	     CONFIG_BT_PERIPHERAL_PREF_TIMEOUT < 0xffff));
BUILD_ASSERT((CONFIG_BT_PERIPHERAL_PREF_MIN_INT == 0xffff) ||
	     (CONFIG_BT_PERIPHERAL_PREF_MIN_INT <=
	     CONFIG_BT_PERIPHERAL_PREF_MAX_INT));
BUILD_ASSERT((CONFIG_BT_PERIPHERAL_PREF_TIMEOUT * 4U) >
	     ((1U + CONFIG_BT_PERIPHERAL_PREF_LATENCY) *
	      CONFIG_BT_PERIPHERAL_PREF_MAX_INT));

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
	/* Always has value 1 = "supported". Lack of support is communicated by
	 * not having this attribute.
	 */
	uint8_t central_addr_res = 1;

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
#if defined(CONFIG_DEVICE_NAME_GATT_WRITABLE_AUTHEN)
			       BT_GATT_PERM_WRITE_AUTHEN,
#elif defined(CONFIG_DEVICE_NAME_GATT_WRITABLE_ENCRYPT)
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
