/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service C.2
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 2'
 */
#include <sys/byteorder.h>
#include <sys/printk.h>

#include <bluetooth/gatt.h>

#include "gatt_macs.h"

/** @def BT_UUID_SERVICE_C_2
 *  @brief UUID for the Service C.2
 */
#define BT_UUID_SERVICE_C_2             BT_UUID_DECLARE_128( \
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, \
		0x00, 0x00, 0x00, 0x00, 0x0c, 0xa0, 0x00, 0x00)

/** @def BT_UUID_VALUE_V10
 *  @brief UUID for the Value V10 Characteristic
 */
#define BT_UUID_VALUE_V10               BT_UUID_DECLARE_16(0xb00a)

/** @def BT_UUID_VALUE_V2
 *  @brief UUID for the Value V2 Characteristic
 */
#define BT_UUID_VALUE_V2                BT_UUID_DECLARE_16(0xb002)

static uint8_t   value_v10_value = 0x0A;
static uint8_t   value_v2_value[] = {
	      '1', '1', '1', '1', '1', '2', '2', '2', '2', '2', '3', '3', '3',
	      '3', '3', '4', '4', '4', '4', '4', '5', '\0'
};
static uint8_t   value_v2_1_value[] = {
	      '2', '2', '2', '2', '2', '3', '3', '3', '3', '3', '4', '4', '4',
	      '4', '4', '5', '5', '5', '5', '5', '6', '6', '\0'
};
static uint8_t   value_v2_2_value[] = {
	      '3', '3', '3', '3', '3', '4', '4', '4', '4', '4', '5', '5', '5',
	      '5', '5', '6', '6', '6', '6', '6', '7', '7', '7', '\0'
};
static uint8_t   value_v2_3_value[] = {
	      '1', '1', '1', '1', '1', '2', '2', '2', '2', '2', '3', '3', '3',
	      '3', '3', '4', '4', '4', '4', '4', '5', '5', '5', '5', '5', '6',
	      '6', '6', '6', '6', '7', '7', '7', '7', '7', '8', '8', '8', '8',
	      '8', '9', '9', '9', '\0'
};
static uint8_t   value_v2_4_value[] = {
	      '2', '2', '2', '2', '2', '3', '3', '3', '3', '3', '4', '4', '4',
	      '4', '4', '5', '5', '5', '5', '5', '6', '6', '6', '6', '6', '7',
	      '7', '7', '7', '7', '8', '8', '8', '8', '8', '9', '9', '9', '9',
	      '9', '0', '0', '0', '0', '\0'
};
static uint8_t   value_v2_5_value[] = {
	      '3', '3', '3', '3', '3', '4', '4', '4', '4', '4', '5', '5', '5',
	      '5', '5', '6', '6', '6', '6', '6', '7', '7', '7', '7', '7', '8',
	      '8', '8', '8', '8', '9', '9', '9', '9', '9', '0', '0', '0', '0',
	      '0', '1', '1', '1', '1', '1', '\0'
};

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

	if (offset >= sizeof(value_v2_2_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v2_2_value))
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
static ssize_t write_value_v2_3(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	char *value = attr->user_data;

	if (offset >= sizeof(value_v2_3_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v2_3_value))
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
static ssize_t write_value_v2_4(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	char *value = attr->user_data;

	if (offset >= sizeof(value_v2_4_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v2_4_value))
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
static ssize_t write_value_v2_5(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	char *value = attr->user_data;

	if (offset >= sizeof(value_v2_5_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v2_5_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr service_c_2_2_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_C_2, 0x10),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V10,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_value_v10, NULL, &value_v10_value, 0x11),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2, &value_v2_value, 0x13),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2_1, &value_v2_1_value, 0x15),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2_2, &value_v2_2_value, 0x17),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2_3, &value_v2_3_value, 0x19),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2_4, &value_v2_4_value, 0x1B),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V2,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_str_value, write_value_v2_5, &value_v2_5_value, 0x1D)
};

static struct bt_gatt_service service_c_2_2_svc =
		    BT_GATT_SERVICE(service_c_2_2_attrs);

/**
 * @brief Register the Service C.2 and all its Characteristics...
 */
void service_c_2_2_init(void)
{
	bt_gatt_service_register(&service_c_2_2_svc);
}

/**
 * @brief Un-Register the Service C.2 and all its Characteristics...
 */
void service_c_2_2_remove(void)
{
	bt_gatt_service_unregister(&service_c_2_2_svc);
}
