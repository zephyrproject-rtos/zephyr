/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service B.4
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 1'
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/gatt.h>

#include "gatt_macs.h"

/**
 *  @brief UUID for the Service B.4
 */
#define BT_UUID_SERVICE_B_4             BT_UUID_DECLARE_16(0xa00b)

/**
 *  @brief UUID for the Value V7 Characteristic
 */
#define BT_UUID_VALUE_V7                BT_UUID_DECLARE_16(0xb007)

static uint8_t   value_v7_value = 0x07;

/**
 * @brief Attribute write call back for the Value V7 attribute
 *
 * @param conn   The connection that is requesting to write
 * @param attr   The attribute that's being written
 * @param buf    Buffer with the data to write
 * @param len    Number of bytes in the buffer
 * @param offset Offset to start writing from
 * @param flags  Flags (BT_GATT_WRITE_*)
 *
 * @return       Number of bytes written, or in case of an error - BT_GATT_ERR()
 *               with a specific ATT error code.
 */
static k_ssize_t write_value_v7(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v7_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v7_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr service_b_4_1_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_B_4, 0x30),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V7,
		BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE,
		NULL, write_value_v7, &value_v7_value, 0x31)
};

static struct bt_gatt_service service_b_4_1_svc =
		    BT_GATT_SERVICE(service_b_4_1_attrs);

/**
 * @brief Register the Service B.4 and all its Characteristics...
 */
void service_b_4_1_init(void)
{
	bt_gatt_service_register(&service_b_4_1_svc);
}

/**
 * @brief Un-Register the Service B.4 and all its Characteristics...
 */
void service_b_4_1_remove(void)
{
	bt_gatt_service_unregister(&service_b_4_1_svc);
}
