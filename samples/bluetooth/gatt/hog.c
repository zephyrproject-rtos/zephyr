/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

enum {
	HIDS_REMOTE_WAKE = BIT(0),
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info {
	u16_t version; /* version number of base USB HID Specification */
	u8_t code; /* country HID Device hardware is localized for. */
	u8_t flags;
} __packed;

struct hids_report {
	u8_t id; /* report id */
	u8_t type; /* report type */
} __packed;

static struct hids_info info = {
	.version = 0x0000,
	.code = 0x00,
	.flags = HIDS_NORMALLY_CONNECTABLE,
};

enum {
	HIDS_INPUT = 0x01,
	HIDS_OUTPUT = 0x02,
	HIDS_FEATURE = 0x03,
};

static struct hids_report input = {
	.id = 0x01,
	.type = HIDS_INPUT,
};

static struct bt_gatt_ccc_cfg input_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t simulate_input;
static u8_t ctrl_point;
static u8_t report_map[] = {
	0x05, 0x01, /* Usage Page (Generic Desktop Ctrls) */
	0x09, 0x02, /* Usage (Mouse) */
	0xA1, 0x01, /* Collection (Application) */
	0x09, 0x01, /*   Usage (Pointer) */
	0xA1, 0x00, /*   Collection (Physical) */
	0x05, 0x09, /*     Usage Page (Button) */
	0x19, 0x01, /*     Usage Minimum (0x01) */
	0x29, 0x03, /*     Usage Maximum (0x03) */
	0x15, 0x00, /*     Logical Minimum (0) */
	0x25, 0x01, /*     Logical Maximum (1) */
	0x95, 0x03, /*     Report Count (3) */
	0x75, 0x01, /*     Report Size (1) */
	0x81, 0x02, /*     Input (Data,Var,Abs,No Wrap,Linear,...) */
	0x95, 0x01, /*     Report Count (1) */
	0x75, 0x05, /*     Report Size (5) */
	0x81, 0x03, /*     Input (Const,Var,Abs,No Wrap,Linear,...) */
	0x05, 0x01, /*     Usage Page (Generic Desktop Ctrls) */
	0x09, 0x30, /*     Usage (X) */
	0x09, 0x31, /*     Usage (Y) */
	0x15, 0x81, /*     Logical Minimum (129) */
	0x25, 0x7F, /*     Logical Maximum (127) */
	0x75, 0x08, /*     Report Size (8) */
	0x95, 0x02, /*     Report Count (2) */
	0x81, 0x06, /*     Input (Data,Var,Rel,No Wrap,Linear,...) */
	0xC0,       /*   End Collection */
	0xC0,       /* End Collection */
};


static ssize_t read_info(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_map,
				 sizeof(report_map));
}

static ssize_t read_report(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_report));
}

static void input_ccc_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	simulate_input = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_input_report(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(ctrl_point)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/* HID Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_info, NULL, &info),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_report_map, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_AUTHEN,
			       read_input_report, NULL, NULL),
	BT_GATT_CCC(input_ccc_cfg, input_ccc_changed),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
			   read_report, NULL, &input),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE,
			       NULL, write_ctrl_point, &ctrl_point),
};

static struct bt_gatt_service hog_svc = BT_GATT_SERVICE(attrs);

void hog_init(void)
{
	bt_gatt_service_register(&hog_svc);
}
