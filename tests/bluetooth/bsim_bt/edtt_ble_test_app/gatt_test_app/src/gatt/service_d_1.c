/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service D
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 1'
 */
#include <sys/byteorder.h>
#include <sys/printk.h>

#include <bluetooth/gatt.h>

#include "gatt_macs.h"

extern struct bt_gatt_attr service_b_5_1_attrs[];

/** @def BT_UUID_SERVICE_D
 *  @brief UUID for the Service D
 */
#define BT_UUID_SERVICE_D               BT_UUID_DECLARE_16(0xa00d)

/** @def BT_UUID_VALUE_V12
 *  @brief UUID for the Value V12 Characteristic
 */
#define BT_UUID_VALUE_V12               BT_UUID_DECLARE_16(0xb00c)

/** @def BT_UUID_VALUE_V11__128_BIT_UUID
 *  @brief UUID for the Value V11 (128-bit UUID) Characteristic
 */
#define BT_UUID_VALUE_V11__128_BIT_UUID BT_UUID_DECLARE_128( \
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, \
		0x00, 0x00, 0x00, 0x00, 0x0b, 0xb0, 0x00, 0x00)

static uint8_t   value_v12_value = 0x0C;
static uint8_t   value_v11__128_bit_uuid_value = 0x0B;
static bool   bAuthorized;

/**
 * @brief Attribute read call back for the Value V12 attribute
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
static ssize_t read_value_v12(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v12_value));
}

/**
 * @brief Attribute read call back for the Value V11 (128-bit UUID) attribute
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
static ssize_t read_value_v11__128_bit_uuid(struct bt_conn *conn,
					    const struct bt_gatt_attr *attr,
					    void *buf, uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	if (!bAuthorized)
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v11__128_bit_uuid_value));
}

struct bt_gatt_attr service_d_1_attrs[] = {
	BT_GATT_H_SECONDARY_SERVICE(BT_UUID_SERVICE_D, 0x10),
	BT_GATT_H_INCLUDE_SERVICE(service_b_5_1_attrs, 0x11),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V12,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ_AUTHEN,
		read_value_v12, NULL, &value_v12_value, 0x12),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V11__128_BIT_UUID,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_value_v11__128_bit_uuid, NULL,
		&value_v11__128_bit_uuid_value, 0x14)
};

static struct bt_gatt_service service_d_1_svc =
		    BT_GATT_SERVICE(service_d_1_attrs);

/**
 * @brief Register the Service D and all its Characteristics...
 */
void service_d_1_init(void)
{
	bt_gatt_service_register(&service_d_1_svc);
}

/**
 * @brief Un-Register the Service D and all its Characteristics...
 */
void service_d_1_remove(void)
{
	bt_gatt_service_unregister(&service_d_1_svc);
}

/**
 * @brief Set authorization for Characteristics and Descriptors in Service D.
 */
void service_d_1_authorize(bool authorized)
{
	bAuthorized = authorized;
}
