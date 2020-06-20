/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service E
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 2'
 */
#include <sys/byteorder.h>
#include <sys/printk.h>

#include <bluetooth/gatt.h>

#include "gatt_macs.h"

/** @def BT_UUID_SERVICE_E
 *  @brief UUID for the Service E
 */
#define BT_UUID_SERVICE_E               BT_UUID_DECLARE_16(0xa00e)

/** @def BT_UUID_VALUE_V13
 *  @brief UUID for the Value V13 Characteristic
 */
#define BT_UUID_VALUE_V13               BT_UUID_DECLARE_16(0xb00d)

static uint8_t   value_v13_value = 0x0D;

/**
 * @brief Attribute read call back for the Value V13 attribute
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
static ssize_t read_value_v13(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v13_value));
}

static struct bt_gatt_attr service_e_2_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_E, 0xFFFD),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V13,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_value_v13, NULL, &value_v13_value, 0xFFFE)
};

static struct bt_gatt_service service_e_2_svc =
		    BT_GATT_SERVICE(service_e_2_attrs);

/**
 * @brief Register the Service E and all its Characteristics...
 */
void service_e_2_init(void)
{
	bt_gatt_service_register(&service_e_2_svc);
}

/**
 * @brief Un-Register the Service E and all its Characteristics...
 */
void service_e_2_remove(void)
{
	bt_gatt_service_unregister(&service_e_2_svc);
}
