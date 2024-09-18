/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service C.2
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 1'
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/gatt.h>

#include "gatt_macs.h"

/**
 *  @brief UUID for the Service C.2
 */
#define BT_UUID_SERVICE_C_2             BT_UUID_DECLARE_128( \
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, \
		0x00, 0x00, 0x00, 0x00, 0x0c, 0xa0, 0x00, 0x00)

/**
 *  @brief UUID for the Value V10 Characteristic
 */
#define BT_UUID_VALUE_V10               BT_UUID_DECLARE_16(0xb00a)

/**
 *  @brief UUID for the Value V2 Characteristic
 */
#define BT_UUID_VALUE_V2                BT_UUID_DECLARE_16(0xb002)

/**
 *  @brief UUID for the Long descriptor V2D1 Characteristic
 */
#define BT_UUID_LONG_DES_V2D1           BT_UUID_DECLARE_16(0xb012)

/**
 *  @brief UUID for the Long descriptor V2D2 Characteristic
 */
#define BT_UUID_LONG_DES_V2D2           BT_UUID_DECLARE_16(0xb013)

/**
 *  @brief UUID for the Long descriptor V2D3 Characteristic
 */
#define BT_UUID_LONG_DES_V2D3           BT_UUID_DECLARE_16(0xb014)

static uint8_t   value_v10_value = 0x0A;
static uint8_t   value_v2_value[] = {
	      '1', '1', '1', '1', '1', '2', '2', '2', '2', '2', '3', '3', '3',
	      '3', '3', '4', '4', '4', '4', '4', '5', '\0'
};
static uint8_t   long_des_v2d1_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11
};
static uint8_t   value_v2_1_value[] = {
	      '2', '2', '2', '2', '2', '3', '3', '3', '3', '3', '4', '4', '4',
	      '4', '4', '5', '5', '5', '5', '5', '6', '6', '\0'
};
static uint8_t   long_des_v2d2_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22
};
static uint8_t   value_v2_2_value[] = {
	      '3', '3', '3', '3', '3', '4', '4', '4', '4', '4', '5', '5', '5',
	      '5', '5', '6', '6', '6', '6', '6', '7', '7', '7', '\0'
};
static uint8_t   long_des_v2d3_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22,
	      0x33
};
static uint8_t   value_v2_3_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33
};
static uint8_t   long_des_v2d1_1_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33
};
static uint8_t   value_v2_4_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44
};
static uint8_t   long_des_v2d2_1_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44
};
static uint8_t   value_v2_5_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44,
	      0x55
};
static uint8_t   long_des_v2d3_1_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44,
	      0x55
};
static uint8_t   value_v2_6_value[] = {
	      '1', '1', '1', '1', '1', '2', '2', '2', '2', '2', '3', '3', '3',
	      '3', '3', '4', '4', '4', '4', '4', '5', '5', '5', '5', '5', '6',
	      '6', '6', '6', '6', '7', '7', '7', '7', '7', '8', '8', '8', '8',
	      '8', '9', '9', '9', '\0'
};
static uint8_t   long_des_v2d1_2_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33
};
static uint8_t   value_v2_7_value[] = {
	      '2', '2', '2', '2', '2', '3', '3', '3', '3', '3', '4', '4', '4',
	      '4', '4', '5', '5', '5', '5', '5', '6', '6', '6', '6', '6', '7',
	      '7', '7', '7', '7', '8', '8', '8', '8', '8', '9', '9', '9', '9',
	      '9', '0', '0', '0', '0', '\0'
};
static uint8_t   long_des_v2d2_2_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44
};
static uint8_t   value_v2_8_value[] = {
	      '3', '3', '3', '3', '3', '4', '4', '4', '4', '4', '5', '5', '5',
	      '5', '5', '6', '6', '6', '6', '6', '7', '7', '7', '7', '7', '8',
	      '8', '8', '8', '8', '9', '9', '9', '9', '9', '0', '0', '0', '0',
	      '0', '1', '1', '1', '1', '1', '\0'
};
static uint8_t   long_des_v2d3_2_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44,
	      0x55
};
static bool   bAuthorized;

/**
 * @brief Attribute read call back for the Value V10 attribute
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
static ssize_t read_value_v10(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v10_value));
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

	if (offset >= sizeof(value_v2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Long descriptor V2D1 attribute
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
static ssize_t read_long_des_v2d1(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(long_des_v2d1_value));
}

/**
 * @brief Attribute write call back for the Long descriptor V2D1 attribute
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
static ssize_t write_long_des_v2d1(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(long_des_v2d1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(long_des_v2d1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

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

	if (offset >= sizeof(value_v2_1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v2_1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Long descriptor V2D2 attribute
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
static ssize_t read_long_des_v2d2(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(long_des_v2d2_value));
}

/**
 * @brief Attribute write call back for the Long descriptor V2D2 attribute
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
static ssize_t write_long_des_v2d2(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(long_des_v2d2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(long_des_v2d2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

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
static ssize_t write_value_v2_2(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	char *value = attr->user_data;

	if (offset >= sizeof(value_v2_2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v2_2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Long descriptor V2D3 attribute
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
static ssize_t read_long_des_v2d3(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(long_des_v2d3_value));
}

/**
 * @brief Attribute write call back for the Long descriptor V2D3 attribute
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
static ssize_t write_long_des_v2d3(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset,
				   uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(long_des_v2d3_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(long_des_v2d3_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Value V2 attribute
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
static ssize_t read_value_v2_3(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v2_3_value));
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
static ssize_t write_value_v2_3(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v2_3_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v2_3_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Long descriptor V2D1 attribute
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
static ssize_t read_long_des_v2d1_1(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(long_des_v2d1_1_value));
}

/**
 * @brief Attribute write call back for the Long descriptor V2D1 attribute
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
static ssize_t write_long_des_v2d1_1(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf, uint16_t len, uint16_t offset,
				     uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(long_des_v2d1_1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(long_des_v2d1_1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Value V2 attribute
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
static ssize_t read_value_v2_4(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v2_4_value));
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
static ssize_t write_value_v2_4(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v2_4_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v2_4_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Long descriptor V2D2 attribute
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
static ssize_t read_long_des_v2d2_1(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(long_des_v2d2_1_value));
}

/**
 * @brief Attribute write call back for the Long descriptor V2D2 attribute
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
static ssize_t write_long_des_v2d2_1(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf, uint16_t len, uint16_t offset,
				     uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(long_des_v2d2_1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(long_des_v2d2_1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Value V2 attribute
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
static ssize_t read_value_v2_5(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v2_5_value));
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
static ssize_t write_value_v2_5(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v2_5_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v2_5_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Long descriptor V2D3 attribute
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
static ssize_t read_long_des_v2d3_1(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(long_des_v2d3_1_value));
}

/**
 * @brief Attribute write call back for the Long descriptor V2D3 attribute
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
static ssize_t write_long_des_v2d3_1(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf, uint16_t len, uint16_t offset,
				     uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(long_des_v2d3_1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(long_des_v2d3_1_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

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
static ssize_t write_value_v2_6(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	char *value = attr->user_data;

	if (offset >= sizeof(value_v2_6_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v2_6_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Long descriptor V2D1 attribute
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
static ssize_t read_long_des_v2d1_2(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(long_des_v2d1_2_value));
}

/**
 * @brief Attribute write call back for the Long descriptor V2D1 attribute
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
static ssize_t write_long_des_v2d1_2(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf, uint16_t len, uint16_t offset,
				     uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(long_des_v2d1_2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(long_des_v2d1_2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
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
static ssize_t read_str_auth_value(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	if (!bAuthorized) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}
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
static ssize_t write_value_v2_7(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	char *value = attr->user_data;

	if (offset >= sizeof(value_v2_7_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v2_7_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (!bAuthorized) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Long descriptor V2D2 attribute
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
static ssize_t read_long_des_v2d2_2(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	if (!bAuthorized) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(long_des_v2d2_2_value));
}

/**
 * @brief Attribute write call back for the Long descriptor V2D2 attribute
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
static ssize_t write_long_des_v2d2_2(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf, uint16_t len, uint16_t offset,
				     uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(long_des_v2d2_2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(long_des_v2d2_2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (!bAuthorized) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

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
static ssize_t write_value_v2_8(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	char *value = attr->user_data;

	if (offset >= sizeof(value_v2_8_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v2_8_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Long descriptor V2D3 attribute
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
static ssize_t read_long_des_v2d3_2(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr, void *buf,
				    uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(long_des_v2d3_2_value));
}

/**
 * @brief Attribute write call back for the Long descriptor V2D3 attribute
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
static ssize_t write_long_des_v2d3_2(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf, uint16_t len, uint16_t offset,
				     uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(long_des_v2d3_2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(long_des_v2d3_2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr service_c_2_1_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_C_2, 0xC0),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V10,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_value_v10, NULL, &value_v10_value, 0xC1),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2, &value_v2_value, 0xC3),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D1,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_long_des_v2d1, write_long_des_v2d1, &long_des_v2d1_value,
		0xC5),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2_1, &value_v2_1_value, 0xC6),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D2,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_long_des_v2d2, write_long_des_v2d2, &long_des_v2d2_value,
		0xC8),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2_2, &value_v2_2_value, 0xC9),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D3,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_long_des_v2d3, write_long_des_v2d3, &long_des_v2d3_value,
		0xCB),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v2_3, write_value_v2_3, &value_v2_3_value, 0xCC),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D1,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_long_des_v2d1_1, write_long_des_v2d1_1,
		&long_des_v2d1_1_value, 0xCE),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v2_4, write_value_v2_4, &value_v2_4_value, 0xCF),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D2,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_long_des_v2d2_1, write_long_des_v2d2_1,
		&long_des_v2d2_1_value, 0xD1),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v2_5, write_value_v2_5, &value_v2_5_value, 0xD2),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D3,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_long_des_v2d3_1, write_long_des_v2d3_1,
		&long_des_v2d3_1_value, 0xD4),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
		read_str_value, write_value_v2_6, &value_v2_6_value, 0xD5),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D1,
		BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
		read_long_des_v2d1_2, write_long_des_v2d1_2,
		&long_des_v2d1_2_value, 0xD7),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_auth_value, write_value_v2_7, &value_v2_7_value, 0xD8),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D2,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_long_des_v2d2_2, write_long_des_v2d2_2,
		&long_des_v2d2_2_value, 0xDA),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
		read_str_value, write_value_v2_8, &value_v2_8_value, 0xDB),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D3,
		BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
		read_long_des_v2d3_2, write_long_des_v2d3_2,
		&long_des_v2d3_2_value, 0xDD)
};

static struct bt_gatt_service service_c_2_1_svc =
		    BT_GATT_SERVICE(service_c_2_1_attrs);

/**
 * @brief Register the Service C.2 and all its Characteristics...
 */
void service_c_2_1_init(void)
{
	bt_gatt_service_register(&service_c_2_1_svc);
}

/**
 * @brief Un-Register the Service C.2 and all its Characteristics...
 */
void service_c_2_1_remove(void)
{
	bt_gatt_service_unregister(&service_c_2_1_svc);
}

/**
 * @brief Set authorization for Characteristics and Descriptors in Service C.2.
 */
void service_c_2_1_authorize(bool authorized)
{
	bAuthorized = authorized;
}
