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
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
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

#if defined(CONFIG_BT_GAP_LE_GATT_SECURITY_LEVELS)
/* Core Vol 3 Part C 12.7: Security Mode values, and the level of TAK encryption in mode 4 */
#define GAP_SECURITY_MODE_1	0x01U
#define GAP_SECURITY_MODE_4	0x04U
#define GAP_SECURITY_MODE_4_LEVEL_TAK	0x02U

struct security_levels {
	/* Highest LE security mode 1 level that an attribute requires */
	uint8_t mode_1_level;
	/* Every attribute that requires security permits a TAK */
	bool is_tak_sufficient;
};

static uint8_t find_security_levels(const struct bt_gatt_attr *attr, uint16_t handle,
				    void *user_data)
{
	struct security_levels *levels = user_data;
	uint8_t required = BT_SECURITY_L1;

	ARG_UNUSED(handle);

	/* LE Secure Connections without authentication has no level of its own: level 4 */
	if ((attr->perm & (BT_GATT_PERM_READ_LESC | BT_GATT_PERM_WRITE_LESC)) != 0U) {
		required = BT_SECURITY_L4;
	} else if ((attr->perm & (BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN)) != 0U) {
		required = BT_SECURITY_L3;
	} else if ((attr->perm & (BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT)) != 0U) {
		required = BT_SECURITY_L2;
	} else {
		/* No security requirement */
	}

	levels->mode_1_level = MAX(levels->mode_1_level, required);

	if (required > BT_SECURITY_L1 && (attr->perm & BT_GATT_PERM_TAK) == 0U) {
		levels->is_tak_sufficient = false;
	}

	return BT_GATT_ITER_CONTINUE;
}

static ssize_t read_security_levels(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset)
{
	struct security_levels levels = {
		.mode_1_level = BT_SECURITY_L1,
		.is_tak_sufficient = IS_ENABLED(CONFIG_BT_TAK),
	};
	uint8_t value[] = {
		GAP_SECURITY_MODE_1, BT_SECURITY_L1,
		GAP_SECURITY_MODE_4, GAP_SECURITY_MODE_4_LEVEL_TAK,
	};
	uint16_t size = sizeof(value);

	bt_gatt_foreach_attr(BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE,
			     find_security_levels, &levels);

	value[1] = levels.mode_1_level;

	/* An entry promises that meeting it opens every attribute (Core Vol 3 Part C 12.7), so
	 * mode 4 is left out as long as one of them asks for more than a TAK.
	 */
	if (!levels.is_tak_sufficient) {
		size -= 2U;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, size);
}
#endif /* CONFIG_BT_GAP_LE_GATT_SECURITY_LEVELS */

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
#if defined(CONFIG_BT_GAP_LE_GATT_SECURITY_LEVELS)
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_SL, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_security_levels, NULL, NULL),
#endif /* CONFIG_BT_GAP_LE_GATT_SECURITY_LEVELS */
);
