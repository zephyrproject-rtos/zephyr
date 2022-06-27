/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service B.5
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 1'
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/gatt.h>

#include "gatt_macs.h"

/** @def BT_UUID_SERVICE_B_5
 *  @brief UUID for the Service B.5
 */
#define BT_UUID_SERVICE_B_5             BT_UUID_DECLARE_16(0xa00b)

/** @def BT_UUID_VALUE_V8
 *  @brief UUID for the Value V8 Characteristic
 */
#define BT_UUID_VALUE_V8                BT_UUID_DECLARE_16(0xb008)

/** @def BT_UUID_DES_V8D1
 *  @brief UUID for the Descriptor V8D1 Characteristic
 */
#define BT_UUID_DES_V8D1                BT_UUID_DECLARE_16(0xb015)

/** @def BT_UUID_DES_V8D2
 *  @brief UUID for the Descriptor V8D2 Characteristic
 */
#define BT_UUID_DES_V8D2                BT_UUID_DECLARE_16(0xb016)

/** @def BT_UUID_DES_V8D3
 *  @brief UUID for the Descriptor V8D3 Characteristic
 */
#define BT_UUID_DES_V8D3                BT_UUID_DECLARE_16(0xb017)

static uint8_t   value_v8_value = 0x08;
static uint8_t   des_v8d1_value = 0x01;
static uint8_t   des_v8d2_value = 0x02;
static uint8_t   des_v8d3_value = 0x03;
static bool   bAuthorized;

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

/**
 * @brief Attribute write call back for the Value V8 attribute
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
static ssize_t write_value_v8(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v8_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v8_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Descriptor V8D1 attribute
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
static ssize_t read_des_v8d1(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(des_v8d1_value));
}

/**
 * @brief Attribute write call back for the Descriptor V8D1 attribute
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
static ssize_t write_des_v8d1(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(des_v8d1_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(des_v8d1_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Descriptor V8D2 attribute
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
static ssize_t read_des_v8d2(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	if (!bAuthorized)
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(des_v8d2_value));
}

/**
 * @brief Attribute write call back for the Descriptor V8D2 attribute
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
static ssize_t write_des_v8d2(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(des_v8d2_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(des_v8d2_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	if (!bAuthorized)
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Descriptor V8D3 attribute
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
static ssize_t read_des_v8d3(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(des_v8d3_value));
}

/**
 * @brief Attribute write call back for the Descriptor V8D3 attribute
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
static ssize_t write_des_v8d3(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(des_v8d3_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(des_v8d3_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

struct bt_gatt_attr service_b_5_1_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_B_5, 0x80),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V8,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT,
		read_value_v8, write_value_v8, &value_v8_value, 0x81),
	BT_GATT_H_DESCRIPTOR(BT_UUID_DES_V8D1,
		BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
		read_des_v8d1, write_des_v8d1, &des_v8d1_value, 0x83),
	BT_GATT_H_DESCRIPTOR(BT_UUID_DES_V8D2,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_des_v8d2, write_des_v8d2, &des_v8d2_value, 0x84),
	BT_GATT_H_DESCRIPTOR(BT_UUID_DES_V8D3,
		BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
		read_des_v8d3, write_des_v8d3, &des_v8d3_value, 0x85)
};

static struct bt_gatt_service service_b_5_1_svc =
		    BT_GATT_SERVICE(service_b_5_1_attrs);

/**
 * @brief Register the Service B.5 and all its Characteristics...
 */
void service_b_5_1_init(void)
{
	bt_gatt_service_register(&service_b_5_1_svc);
}

/**
 * @brief Un-Register the Service B.5 and all its Characteristics...
 */
void service_b_5_1_remove(void)
{
	bt_gatt_service_unregister(&service_b_5_1_svc);
}

/**
 * @brief Set authorization for Characteristics and Descriptors in Service B.5.
 */
void service_b_5_1_authorize(bool authorized)
{
	bAuthorized = authorized;
}
