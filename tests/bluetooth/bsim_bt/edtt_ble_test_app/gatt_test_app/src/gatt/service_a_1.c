/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service A
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 1'
 */
#include <sys/byteorder.h>
#include <sys/printk.h>

#include <bluetooth/gatt.h>

#include "gatt_macs.h"

extern struct bt_gatt_attr service_d_1_attrs[];

/** @def BT_UUID_SERVICE_A
 *  @brief UUID for the Service A
 */
#define BT_UUID_SERVICE_A               BT_UUID_DECLARE_16(0xa00a)

/** @def BT_UUID_VALUE_V1
 *  @brief UUID for the Value V1 Characteristic
 */
#define BT_UUID_VALUE_V1                BT_UUID_DECLARE_16(0xb001)

/** @def BT_UUID_VALUE_V2
 *  @brief UUID for the Value V2 Characteristic
 */
#define BT_UUID_VALUE_V2                BT_UUID_DECLARE_16(0xb002)

/** @def BT_UUID_VALUE_V3
 *  @brief UUID for the Value V3 Characteristic
 */
#define BT_UUID_VALUE_V3                BT_UUID_DECLARE_16(0xb003)

static uint8_t   value_v1_value = 0x01;
static uint8_t   value_v2_value[] = {
	      '1', '1', '1', '1', '1', '2', '2', '2', '2', '2', '3', '3', '3',
	      '3', '3', '4', '4', '4', '4', '4', '5', '5', '5', '5', '5', '6',
	      '6', '6', '6', '6', '7', '7', '7', '7', '7', '8', '8', '8', '8',
	      '8', '9', '9', '9', '9', '9', '0', '0', '0', '0', '0', '1', '1',
	      '1', '1', '1', '2', '2', '2', '2', '2', '3', '3', '3', '3', '3',
	      '4', '4', '4', '4', '4', '5', '5', '5', '5', '5', '6', '6', '6',
	      '6', '6', '7', '7', '7', '7', '7', '8', '8', '8', '8', '8', '9',
	      '9', '9', '9', '9', '0', '0', '0', '0', '0', '1', '1', '1', '1',
	      '1', '2', '2', '2', '2', '2', '3', '3', '3', '3', '3', '4', '4',
	      '4', '4', '4', '5', '5', '5', '5', '5', '6', '6', '6', '6', '6',
	      '7', '7', '7', '7', '7', '8', '8', '8', '8', '8', '9', '9', '9',
	      '9', '9', '0', '0', '0', '0', '0', '1', '1', '1', '1', '1', '2',
	      '2', '2', '2', '2', '3', '3', '3', '3', '3', '4', '4', '4', '4',
	      '4', '5', '5', '5', '5', '5', '6', '6', '6', '6', '6', '7', '7',
	      '7', '7', '7', '8', '8', '8', '8', '8', '9', '9', '9', '9', '9',
	      '0', '0', '0', '0', '0', '1', '1', '1', '1', '1', '2', '2', '2',
	      '2', '2', '3', '3', '3', '3', '3', '4', '4', '4', '4', '4', '5',
	      '5', '5', '5', '5', '6', '6', '6', '6', '6', '7', '7', '7', '7',
	      '7', '8', '8', '8', '8', '8', '9', '9', '9', '9', '9', '0', '0',
	      '0', '0', '0', '1', '1', '1', '1', '1', '2', '2', '2', '2', '2',
	      '3', '3', '3', '3', '3', '4', '4', '4', '4', '4', '5', '5', '5',
	      '5', '5', '6', '6', '6', '6', '6', '7', '7', '7', '7', '7', '8',
	      '8', '8', '8', '8', '9', '9', '9', '9', '9', '0', '0', '0', '0',
	      '0', '1', '1', '1', '1', '1', '2', '2', '2', '2', '2', '3', '3',
	      '3', '3', '3', '4', '4', '4', '4', '4', '5', '5', '5', '5', '5',
	      '6', '6', '6', '6', '6', '7', '7', '7', '7', '7', '8', '8', '8',
	      '8', '8', '9', '9', '9', '9', '9', '0', '0', '0', '0', '0', '1',
	      '1', '1', '1', '1', '2', '2', '2', '2', '2', '3', '3', '3', '3',
	      '3', '4', '4', '4', '4', '4', '5', '5', '5', '5', '5', '6', '6',
	      '6', '6', '6', '7', '7', '7', '7', '7', '8', '8', '8', '8', '8',
	      '9', '9', '9', '9', '9', '0', '0', '0', '0', '0', '1', '1', '1',
	      '1', '1', '2', '2', '2', '2', '2', '3', '3', '3', '3', '3', '4',
	      '4', '4', '4', '4', '5', '5', '5', '5', '5', '6', '6', '6', '6',
	      '6', '7', '7', '7', '7', '7', '8', '8', '8', '8', '8', '9', '9',
	      '9', '9', '9', '0', '0', '0', '0', '0', '1', '1', '1', '1', '1',
	      '2', '2', '2', '2', '2', '3', '3', '3', '3', '3', '4', '4', '4',
	      '4', '4', '5', '5', '5', '5', '5', '6', '6', '6', '6', '6', '7',
	      '7', '7', '7', '7', '8', '8', '8', '8', '8', '9', '9', '9', '9',
	      '9', '0', '0', '0', '0', '0', '1', '2', '3', '4', '5', '6', '7',
	      '8', '9', '0', '1', '2', '\0'
};
static uint8_t   value_v2_1_value[] = {
	      '1', '1', '1', '1', '1', '2', '2', '2', '2', '2', '3', '3', '3',
	      '3', '3', '4', '4', '4', '4', '4', '5', '5', '5', '5', '5', '6',
	      '6', '6', '6', '6', '7', '7', '7', '7', '7', '8', '8', '8', '8',
	      '8', '9', '9', '9', '9', '9', '0', '0', '0', '0', '0', '\0'
};
static uint8_t   value_v3_value = 0x03;

/**
 * @brief Attribute read call back for the Value V1 attribute
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
static ssize_t read_value_v1(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v1_value));
}

/**
 * @brief Attribute read call back for the Value V2 string attribute
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
static ssize_t read_str_value(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

/**
 * @brief Attribute write call back for the Value V2 attribute
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
static ssize_t write_value_v2(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	char *value = attr->user_data;

	if (offset >= sizeof(value_v2_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v2_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute write call back for the Value V2 attribute
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
static ssize_t write_value_v2_1(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	char *value = attr->user_data;

	if (offset >= sizeof(value_v2_1_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v2_1_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute write call back for the Value V3 attribute
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
static ssize_t write_value_v3(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v3_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v3_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr service_a_1_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_A, 0x20),
	BT_GATT_H_INCLUDE_SERVICE(service_d_1_attrs, 0x21),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V1,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ_ENCRYPT,
		read_value_v1, NULL, &value_v1_value, 0x22),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2, &value_v2_value, 0x24),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE,
		NULL, write_value_v2_1, &value_v2_1_value, 0x26),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V3,
		BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE,
		NULL, write_value_v3, &value_v3_value, 0x28)
};

static struct bt_gatt_service service_a_1_svc =
		    BT_GATT_SERVICE(service_a_1_attrs);

/**
 * @brief Register the Service A and all its Characteristics...
 */
void service_a_1_init(void)
{
	bt_gatt_service_register(&service_a_1_svc);
}

/**
 * @brief Un-Register the Service A and all its Characteristics...
 */
void service_a_1_remove(void)
{
	bt_gatt_service_unregister(&service_a_1_svc);
}
