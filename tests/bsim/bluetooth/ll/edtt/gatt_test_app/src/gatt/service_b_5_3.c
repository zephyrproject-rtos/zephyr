/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service B.5
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 3'
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/gatt.h>

#include "gatt_macs.h"

/**
 *  @brief UUID for the Service B.5
 */
#define BT_UUID_SERVICE_B_5             BT_UUID_DECLARE_16(0xa00b)

/**
 *  @brief UUID for the Value V8 Characteristic
 */
#define BT_UUID_VALUE_V8                BT_UUID_DECLARE_16(0xb008)

static uint8_t   value_v8_value = 0x08;

/**
 * @brief Attribute read call back for the Value V8 attribute
 *
 * @param conn   The connection that is requesting to read
 * @param attr   The attribute that's being read
 * @param buf    Buffer to place the read result in
 * @param len    Length of data to read
 * @param offset Offset to start reading from
 *
 * @return       Number of bytes read, or in case of an error - BT_GATT_ERR()
 *               with a specific ATT error code.
 */
static ssize_t read_value_v8(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v8_value));
}

static struct bt_gatt_attr service_b_5_3_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_B_5, 0x30),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V8,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_value_v8, NULL, &value_v8_value, 0x31)
};

static struct bt_gatt_service service_b_5_3_svc =
		    BT_GATT_SERVICE(service_b_5_3_attrs);

/**
 * @brief Register the Service B.5 and all its Characteristics...
 */
void service_b_5_3_init(void)
{
	bt_gatt_service_register(&service_b_5_3_svc);
}

/**
 * @brief Un-Register the Service B.5 and all its Characteristics...
 */
void service_b_5_3_remove(void)
{
	bt_gatt_service_unregister(&service_b_5_3_svc);
}
