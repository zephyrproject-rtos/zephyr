/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service B.1
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 1'
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/gatt.h>

#include "gatt_macs.h"

/** @def BT_UUID_SERVICE_B_1
 *  @brief UUID for the Service B.1
 */
#define BT_UUID_SERVICE_B_1             BT_UUID_DECLARE_16(0xa00b)

/** @def BT_UUID_VALUE_V4
 *  @brief UUID for the Value V4 Characteristic
 */
#define BT_UUID_VALUE_V4                BT_UUID_DECLARE_16(0xb004)

/** @def BT_UUID_LONG_DES_V2D1
 *  @brief UUID for the Long descriptor V2D1 Characteristic
 */
#define BT_UUID_LONG_DES_V2D1           BT_UUID_DECLARE_16(0xb012)

static uint8_t   value_v4_value = 0x04;
static uint8_t   value_v4_1_value = 0x04;
static uint16_t  server_cha_con_value;
static uint8_t   value_v4_2_value = 0x04;
static uint8_t   long_des_v2d1_value[] = {
	      0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x12,
	      0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34,
	      0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x11, 0x22, 0x33,
	      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33
};
static uint8_t   value_v4_3_value[] = {
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

/**
 * @brief Attribute read call back for the Value V4 attribute
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
static ssize_t read_value_v4(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v4_value));
}

/**
 * @brief Attribute write call back for the Value V4 attribute
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
static ssize_t write_value_v4(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v4_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v4_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Value V4 attribute
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
static ssize_t read_value_v4_1(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v4_1_value));
}

/**
 * @brief Attribute write call back for the Value V4 attribute
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
static ssize_t write_value_v4_1(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v4_1_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v4_1_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Server Characteristic Configuration
 *        attribute
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
static ssize_t read_server_cha_con(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	const uint16_t *value = attr->user_data;
	uint16_t server_cha_con_conv = sys_cpu_to_le16(*value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &server_cha_con_conv,
				 sizeof(server_cha_con_conv));
}

/**
 * @brief Attribute write call back for the Server Characteristic Configuration
 *        attribute
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
static ssize_t write_server_cha_con(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len, uint16_t offset,
				    uint8_t flags)
{
	uint16_t *value = attr->user_data;
	uint16_t server_cha_con_conv = sys_cpu_to_le16(*value);

	if (offset >= sizeof(server_cha_con_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(server_cha_con_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy((uint8_t *)&server_cha_con_conv + offset, buf, len);

	*value = sys_le16_to_cpu(server_cha_con_conv);

	return len;
}

/**
 * @brief Attribute read call back for the Value V4 attribute
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
static ssize_t read_value_v4_3(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v4_3_value));
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

#define BT_GATT_CHRC_NONE 0

static struct bt_gatt_attr service_b_1_1_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_B_1, 0x60),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V4,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_AUTHEN,
		read_value_v4, write_value_v4, &value_v4_value, 0x61),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V4,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v4_1, write_value_v4_1, &value_v4_1_value, 0x63),
	BT_GATT_H_DESCRIPTOR(BT_UUID_GATT_SCC,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_server_cha_con, write_server_cha_con,
		&server_cha_con_value, 0x65),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V4,
		BT_GATT_CHRC_NONE,
		BT_GATT_PERM_NONE,
		NULL, NULL, &value_v4_2_value, 0x66),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D1,
		BT_GATT_PERM_NONE,
		NULL, NULL, &long_des_v2d1_value, 0x68),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V4,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_value_v4_3, NULL, &value_v4_3_value, 0x69),
	BT_GATT_H_DESCRIPTOR(BT_UUID_LONG_DES_V2D1,
		BT_GATT_PERM_READ,
		read_long_des_v2d1_1, NULL, &long_des_v2d1_1_value, 0x6B)
};

static struct bt_gatt_service service_b_1_1_svc =
		    BT_GATT_SERVICE(service_b_1_1_attrs);

/**
 * @brief Register the Service B.1 and all its Characteristics...
 */
void service_b_1_1_init(void)
{
	bt_gatt_service_register(&service_b_1_1_svc);
}

/**
 * @brief Un-Register the Service B.1 and all its Characteristics...
 */
void service_b_1_1_remove(void)
{
	bt_gatt_service_unregister(&service_b_1_1_svc);
}
