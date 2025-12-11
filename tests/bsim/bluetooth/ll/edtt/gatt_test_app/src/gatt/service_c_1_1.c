/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service C.1
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 1'
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/gatt.h>

#include "gatt_macs.h"

extern struct bt_gatt_attr service_d_1_attrs[];

/**
 *  @brief UUID for the Service C.1
 */
#define BT_UUID_SERVICE_C_1             BT_UUID_DECLARE_128( \
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, \
		0x00, 0x00, 0x00, 0x00, 0x0c, 0xa0, 0x00, 0x00)

/**
 *  @brief UUID for the Value V9 (128-bit UUID) Characteristic
 */
#define BT_UUID_VALUE_V9__128_BIT_UUID  BT_UUID_DECLARE_128( \
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, \
		0x00, 0x00, 0x00, 0x00, 0x09, 0xb0, 0x00, 0x00)

/**
 *  @brief UUID for the Descriptor V9D2 (128-bit UUID) Characteristic
 */
#define BT_UUID_DES_V9D2__128_BIT_UUID  BT_UUID_DECLARE_128( \
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, \
		0x00, 0x00, 0x00, 0x00, 0xd2, 0xd9, 0x00, 0x00)

/**
 *  @brief UUID for the Descriptor V9D3 (128-bit UUID) Characteristic
 */
#define BT_UUID_DES_V9D3__128_BIT_UUID  BT_UUID_DECLARE_128( \
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, \
		0x00, 0x00, 0x00, 0x00, 0xd3, 0xd9, 0x00, 0x00)

static uint8_t   value_v9__128_bit_uuid_value = 0x09;
static struct bt_gatt_cep cha_ext_pro_value = { 0x0001 };
static uint8_t   des_v9d2__128_bit_uuid_value = 0x22;
static uint8_t   des_v9d3__128_bit_uuid_value = 0x33;

/**
 * @brief Attribute read call back for the Value V9 (128-bit UUID) attribute
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
static ssize_t read_value_v9__128_bit_uuid(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   void *buf, uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v9__128_bit_uuid_value));
}

/**
 * @brief Attribute write call back for the Value V9 (128-bit UUID) attribute
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
static ssize_t write_value_v9__128_bit_uuid(struct bt_conn *conn,
					    const struct bt_gatt_attr *attr,
					    const void *buf, uint16_t len,
					    uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v9__128_bit_uuid_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(value_v9__128_bit_uuid_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Descriptor V9D2 (128-bit UUID)
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
static ssize_t read_des_v9d2__128_bit_uuid(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   void *buf, uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(des_v9d2__128_bit_uuid_value));
}

/**
 * @brief Attribute write call back for the Descriptor V9D2 (128-bit UUID)
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
static ssize_t write_des_v9d2__128_bit_uuid(struct bt_conn *conn,
					    const struct bt_gatt_attr *attr,
					    const void *buf, uint16_t len,
					    uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(des_v9d2__128_bit_uuid_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(des_v9d2__128_bit_uuid_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute write call back for the Descriptor V9D3 (128-bit UUID)
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
static ssize_t write_des_v9d3__128_bit_uuid(struct bt_conn *conn,
					    const struct bt_gatt_attr *attr,
					    const void *buf, uint16_t len,
					    uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(des_v9d3__128_bit_uuid_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (offset + len > sizeof(des_v9d3__128_bit_uuid_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr service_c_1_1_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_C_1, 0x90),
	BT_GATT_H_INCLUDE_SERVICE(service_d_1_attrs, 0x91),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V9__128_BIT_UUID,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v9__128_bit_uuid, write_value_v9__128_bit_uuid,
		&value_v9__128_bit_uuid_value, 0x92),
	BT_GATT_H_CEP(&cha_ext_pro_value, 0x94),
	BT_GATT_H_DESCRIPTOR(BT_UUID_DES_V9D2__128_BIT_UUID,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_des_v9d2__128_bit_uuid, write_des_v9d2__128_bit_uuid,
		&des_v9d2__128_bit_uuid_value, 0x95),
	BT_GATT_H_DESCRIPTOR(BT_UUID_DES_V9D3__128_BIT_UUID,
		BT_GATT_PERM_WRITE,
		NULL, write_des_v9d3__128_bit_uuid,
		&des_v9d3__128_bit_uuid_value, 0x96)
};

static struct bt_gatt_service service_c_1_1_svc =
		    BT_GATT_SERVICE(service_c_1_1_attrs);

/**
 * @brief Register the Service C.1 and all its Characteristics...
 */
void service_c_1_1_init(void)
{
	bt_gatt_service_register(&service_c_1_1_svc);
}

/**
 * @brief Un-Register the Service C.1 and all its Characteristics...
 */
void service_c_1_1_remove(void)
{
	bt_gatt_service_unregister(&service_c_1_1_svc);
}
