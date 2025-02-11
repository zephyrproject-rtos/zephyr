/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service B.2
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 3'
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/gatt.h>

#include "gatt_macs.h"

/**
 *  @brief UUID for the Service B.2
 */
#define BT_UUID_SERVICE_B_2             BT_UUID_DECLARE_16(0xa00b)

/**
 *  @brief UUID for the Value V5 Characteristic
 */
#define BT_UUID_VALUE_V5                BT_UUID_DECLARE_16(0xb005)

/**
 *  @brief UUID for the Descriptor V5D4 (128-bit UUID) Characteristic
 */
#define BT_UUID_DES_V5D4__128_BIT_UUID  BT_UUID_DECLARE_128( \
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01, \
		0x00, 0x00, 0x00, 0x00, 0xd4, 0xd5, 0x00, 0x00)

static uint8_t   value_v5_value = 0x05;
static uint8_t   cha_user_des_value[] = {
	      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	      '\0'
};
static struct bt_gatt_cep cha_ext_pro_value = { 0x0003 };
static uint8_t   des_v5d4__128_bit_uuid_value = 0x44;
static const struct bt_gatt_cpf cha_format_value = {
	     0x04, 0x00, 0x3001, 0x01, 0x3111
};

/**
 * @brief Attribute read call back for the Value V5 attribute
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
static ssize_t read_value_v5(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v5_value));
}

/**
 * @brief Attribute write call back for the Value V5 attribute
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
static ssize_t write_value_v5(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v5_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v5_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Descriptor V5D4 (128-bit UUID)
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
static ssize_t read_des_v5d4__128_bit_uuid(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   void *buf, uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(des_v5d4__128_bit_uuid_value));
}

static struct bt_gatt_attr service_b_2_3_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_B_2, 0x90),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V5,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v5, write_value_v5, &value_v5_value, 0x91),
	BT_GATT_H_CUD(cha_user_des_value, BT_GATT_PERM_READ, 0x93),
	BT_GATT_H_CEP(&cha_ext_pro_value, 0x94),
	BT_GATT_H_DESCRIPTOR(BT_UUID_DES_V5D4__128_BIT_UUID,
		BT_GATT_PERM_READ,
		read_des_v5d4__128_bit_uuid, NULL,
		&des_v5d4__128_bit_uuid_value, 0x95),
	BT_GATT_H_CPF(&cha_format_value, 0x96)
};

static struct bt_gatt_service service_b_2_3_svc =
		    BT_GATT_SERVICE(service_b_2_3_attrs);

/**
 * @brief Register the Service B.2 and all its Characteristics...
 */
void service_b_2_3_init(void)
{
	bt_gatt_service_register(&service_b_2_3_svc);
}

/**
 * @brief Un-Register the Service B.2 and all its Characteristics...
 */
void service_b_2_3_remove(void)
{
	bt_gatt_service_unregister(&service_b_2_3_svc);
}
