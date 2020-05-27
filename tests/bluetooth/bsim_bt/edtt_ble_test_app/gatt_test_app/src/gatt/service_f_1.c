/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief Service F
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Test_Databases.xlsm' Sheet: 'Large Database 1'
 */
#include <sys/byteorder.h>
#include <sys/printk.h>

#include <bluetooth/gatt.h>

#include "gatt_macs.h"

/** @def BT_UUID_SERVICE_F
 *  @brief UUID for the Service F
 */
#define BT_UUID_SERVICE_F               BT_UUID_DECLARE_16(0xa00f)

/** @def BT_UUID_VALUE_V14
 *  @brief UUID for the Value V14 Characteristic
 */
#define BT_UUID_VALUE_V14               BT_UUID_DECLARE_16(0xb00e)

/** @def BT_UUID_VALUE_V15
 *  @brief UUID for the Value V15 Characteristic
 */
#define BT_UUID_VALUE_V15               BT_UUID_DECLARE_16(0xb00f)

/** @def BT_UUID_VALUE_V6
 *  @brief UUID for the Value V6 Characteristic
 */
#define BT_UUID_VALUE_V6                BT_UUID_DECLARE_16(0xb006)

/** @def BT_UUID_VALUE_V7
 *  @brief UUID for the Value V7 Characteristic
 */
#define BT_UUID_VALUE_V7                BT_UUID_DECLARE_16(0xb007)

/** @def BT_UUID_VALUE_V16
 *  @brief UUID for the Value V16 Characteristic
 */
#define BT_UUID_VALUE_V16               BT_UUID_DECLARE_16(0xb010)

/** @def BT_UUID_AGG_FORMAT
 *  @brief UUID for the Aggregate Format Characteristic
 */
#define BT_UUID_AGG_FORMAT              BT_UUID_DECLARE_16(0x2905)

/** @def BT_UUID_VALUE_V17
 *  @brief UUID for the Value V17 Characteristic
 */
#define BT_UUID_VALUE_V17               BT_UUID_DECLARE_16(0xb011)

static uint8_t   value_v14_value[] = {
	      'L', 'e', 'n', 'g', 't', 'h', ' ', 'i', 's', ' ', '\0'
};
static const struct bt_gatt_cpf cha_format_value = {
	     0x19, 0x00, 0x3000, 0x01, 0x0000
};
static uint8_t   value_v15_value = 0x65;
static const struct bt_gatt_cpf cha_format_1_value = {
	     0x04, 0x00, 0x2701, 0x01, 0x0001
};
static uint16_t  value_v6_value = 0x1234;
static const struct bt_gatt_cpf cha_format_2_value = {
	     0x06, 0x00, 0x2710, 0x01, 0x0002
};
static uint32_t  value_v7_value = 0x01020304;
static const struct bt_gatt_cpf cha_format_3_value = {
	     0x08, 0x00, 0x2717, 0x01, 0x0003
};
static struct __packed value_v16_t {
	uint8_t  field_a;
	uint16_t field_b;
	uint32_t field_c;
} value_v16_value = { 0x65, 0x1234, 0x01020304 };
static struct __packed agg_format_t {
	uint16_t field_a;
	uint16_t field_b;
	uint16_t field_c;
} agg_format_value = { 0x00A6, 0x00A9, 0x00AC };
static uint8_t   value_v17_value = 0x12;

/**
 * @brief Attribute read call back for the Value V14 string attribute
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
 * @brief Attribute read call back for the Value V15 attribute
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
static ssize_t read_value_v15(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v15_value));
}

/**
 * @brief Attribute write call back for the Value V15 attribute
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
static ssize_t write_value_v15(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset >= sizeof(value_v15_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v15_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy(value + offset, buf, len);

	return len;
}

/**
 * @brief Attribute read call back for the Value V6 attribute
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
static ssize_t read_value_v6(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const uint16_t *value = attr->user_data;
	uint16_t value_v6_conv = sys_cpu_to_le16(*value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value_v6_conv,
				 sizeof(value_v6_conv));
}

/**
 * @brief Attribute write call back for the Value V6 attribute
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
static ssize_t write_value_v6(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint16_t *value = attr->user_data;
	uint16_t value_v6_conv = sys_cpu_to_le16(*value);

	if (offset >= sizeof(value_v6_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v6_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy((uint8_t *)&value_v6_conv + offset, buf, len);

	*value = sys_le16_to_cpu(value_v6_conv);

	return len;
}

/**
 * @brief Attribute read call back for the Value V7 attribute
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
static ssize_t read_value_v7(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const uint32_t *value = attr->user_data;
	uint32_t value_v7_conv = sys_cpu_to_le32(*value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value_v7_conv,
				 sizeof(value_v7_conv));
}

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
static ssize_t write_value_v7(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint32_t *value = attr->user_data;
	uint32_t value_v7_conv = sys_cpu_to_le32(*value);

	if (offset >= sizeof(value_v7_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	if (offset + len > sizeof(value_v7_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);

	memcpy((uint8_t *)&value_v7_conv + offset, buf, len);

	*value = sys_le32_to_cpu(value_v7_conv);

	return len;
}

/**
 * @brief Attribute read call back for the Value V16 attribute
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
static ssize_t read_value_v16(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const struct value_v16_t *value = attr->user_data;
	struct value_v16_t value_v16_conv;

	value_v16_conv.field_a = value->field_a;
	value_v16_conv.field_b = sys_cpu_to_le16(value->field_b);
	value_v16_conv.field_c = sys_cpu_to_le32(value->field_c);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value_v16_conv,
				 sizeof(value_v16_conv));
}

/**
 * @brief Attribute read call back for the Aggregate Format attribute
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
static ssize_t read_agg_format(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	const struct agg_format_t *value = attr->user_data;
	struct agg_format_t agg_format_conv;

	agg_format_conv.field_a = sys_cpu_to_le16(value->field_a);
	agg_format_conv.field_b = sys_cpu_to_le16(value->field_b);
	agg_format_conv.field_c = sys_cpu_to_le16(value->field_c);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &agg_format_conv,
				 sizeof(agg_format_conv));
}

/**
 * @brief Attribute read call back for the Value V17 attribute
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
static ssize_t read_value_v17(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(value_v17_value));
}

static struct bt_gatt_attr service_f_1_attrs[] = {
	BT_GATT_H_PRIMARY_SERVICE(BT_UUID_SERVICE_F, 0xA0),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V14,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_str_value, NULL, &value_v14_value, 0xA1),
	BT_GATT_H_CPF(&cha_format_value, 0xA3),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V15,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v15, write_value_v15, &value_v15_value, 0xA4),
	BT_GATT_H_CPF(&cha_format_1_value, 0xA6),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V6,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v6, write_value_v6, &value_v6_value, 0xA7),
	BT_GATT_H_CPF(&cha_format_2_value, 0xA9),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V7,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v7, write_value_v7, &value_v7_value, 0xAA),
	BT_GATT_H_CPF(&cha_format_3_value, 0xAC),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V16,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		read_value_v16, NULL, &value_v16_value, 0xAD),
	BT_GATT_H_DESCRIPTOR(BT_UUID_AGG_FORMAT,
		BT_GATT_PERM_READ,
		read_agg_format, NULL, &agg_format_value, 0xAF),
	BT_GATT_H_CHARACTERISTIC(BT_UUID_VALUE_V17,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_AUTH,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_value_v17, NULL, &value_v17_value, 0xB0)
};

static struct bt_gatt_service service_f_1_svc =
		    BT_GATT_SERVICE(service_f_1_attrs);

/**
 * @brief Register the Service F and all its Characteristics...
 */
void service_f_1_init(void)
{
	bt_gatt_service_register(&service_f_1_svc);
}

/**
 * @brief Un-Register the Service F and all its Characteristics...
 */
void service_f_1_remove(void)
{
	bt_gatt_service_unregister(&service_f_1_svc);
}
