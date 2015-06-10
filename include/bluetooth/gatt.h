/*! @file
 *  @bried Generic Attribute Profile handling
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __BT_GATT_H
#define __BT_GATT_H

#include <misc/util.h>

/* GATT attribute permission bitfield values */
/*! @def BT_GATT_PERM_READ
 *  @brief Attribute read permission.
 */
#define BT_GATT_PERM_READ			0x01
/*! @def BT_GATT_PERM_WRITE
 *  @brief Attribute write permission.
 */
#define BT_GATT_PERM_WRITE			0x02
/*! @def BT_GATT_PERM_READ_ENCRYPT
 *  @brief Attribute read permission with encryption.
 *
 *  If set, requires encryption for read access.
 */
#define BT_GATT_PERM_READ_ENCRYPT		0x04
/*! @def BT_GATT_PERM_WRITE_ENCRYPT
 *  @brief Attribute write permission with encryption.
 *
 *  If set, requires encryption for write access.
 */
#define BT_GATT_PERM_WRITE_ENCRYPT		0x08
/*! @def BT_GATT_PERM_READ_AUTHEN
 *  @brief Attribute read permission with authentication.
 *
 *  If set, requires encryption using authenticated link-key for read access.
 */
#define BT_GATT_PERM_READ_AUTHEN		0x10
/*! @def BT_GATT_PERM_WRITE_AUTHEN
 *  @brief Attribute write permission with authentication.
 *
 *  If set, requires encryption using authenticated link-key for write access.
 */
#define BT_GATT_PERM_WRITE_AUTHEN		0x20
/*! @def BT_GATT_PERM_AUTHOR
 *  @brief Attribute authorization permission.
 */
#define BT_GATT_PERM_AUTHOR			0x40

/*! @brief GATT Attribute structure. */
struct bt_gatt_attr {
	/*! Attribute handle */
	uint16_t		handle;
	/*! Attribute UUID */
	const struct bt_uuid	*uuid;
	/*! Attribute permissions */
	uint8_t			perm;
	/*! Attribute read callback */
	int			(*read)(const bt_addr_le_t *peer,
					const struct bt_gatt_attr *attr,
					void *buf, uint8_t len,
					uint16_t offset);
	/*! Attribute write callback */
	int			(*write)(const bt_addr_le_t *peer,
					 const struct bt_gatt_attr *attr,
					 const void *buf, uint8_t len,
					 uint16_t offset);
	/*! Attribute user data */
	void			*user_data;
};

/*! @brief Service Attribute Value. */
struct bt_gatt_service {
	/*! Service UUID. */
	const struct bt_uuid	*uuid;
};

/*! @brief Include Attribute Value. */
struct bt_gatt_include {
	/*! Service start handle. */
	uint16_t		start_handle;
	/*! Service end handle. */
	uint16_t		end_handle;
	/*! Service UUID. */
	const struct bt_uuid	*uuid;
};

/* Characteristic Properties Bitfield values */
/*! @def BT_GATT_CHRC_BROADCAST
 *  @brief Characteristic broadcast property.
 *
 *  If set, permits broadcasts of the Characteristic Value using Server
 *  Characteristic Configuration Descriptor.
 */
#define BT_GATT_CHRC_BROADCAST			0x01
/*! @def BT_GATT_CHRC_READ
 *  @brief Characteristic read property.
 *
 *  If set, permits reads of the Characteristic Value.
 */
#define BT_GATT_CHRC_READ			0x02
/*! @def BT_GATT_CHRC_WRITE_WITHOUT_RESP
 *  @brief Characteristic write without response property.
 *
 *  If set, permits write of the Characteristic Value without response.
 */
#define BT_GATT_CHRC_WRITE_WITHOUT_RESP		0x04
/*! @def BT_GATT_CHRC_WRITE
 *  @brief Characteristic write with response property.
 *
 *  If set, permits write of the Characteristic Value with response.
 */
#define BT_GATT_CHRC_WRITE			0x08
/*! @def BT_GATT_CHRC_NOTIFY
 *  @brief Characteristic notify property.
 *
 *  If set, permits notifications of a Characteristic Value without
 *  acknowledgement.
 */
#define BT_GATT_CHRC_NOTIFY			0x10
/*! @def BT_GATT_CHRC_INDICATE
 *  @brief Characteristic indicate property.
 *
 * If set, permits indications of a Characteristic Value with acknowledgement.
 */
#define BT_GATT_CHRC_INDICATE			0x20
/*! @def BT_GATT_CHRC_AUTH
 *  @brief Characteristic Authenticated Signed Writes property.
 *
 *  If set, permits signed writes to the Characteristic Value
 */
#define BT_GATT_CHRC_AUTH			0x40
/*! @def BT_GATT_CHRC_EXT_PROP
 *  @brief Characteristic Extended Properties property.
 *
 * If set, additional characteristic properties are defined in the
 * Characteristic Extended Properties Descriptor.
 */
#define BT_GATT_CHRC_EXT_PROP			0x80

/*! @brief Characteristic Attribute Value. */
struct bt_gatt_chrc {
	/*! Characteristic properties. */
	uint8_t			properties;
	/*! Characteristic value handle. */
	uint16_t		value_handle;
	/*! Characteristic UUID. */
	const struct bt_uuid	*uuid;
};

/* Characteristic Extended Properties Bitfield values */
#define BT_GATT_CEP_RELIABLE_WRITE		0x01
#define BT_GATT_CEP_WRITABLE_AUX		0x02

/*! @brief Characteristic Extended Properties Attribute Value. */
struct bt_gatt_cep {
	/*! Characteristic Extended properties. */
	uint8_t			properties;
};

/* @brief Characteristic User Description Attribute Value. */
struct bt_gatt_cud {
	/*! Characteristic User Description string. */
	char			*string;
};

/* Client Characteristic Configuration Values*/
/* @def BT_GATT_CCC_NOTIFY
 * @brief Client Characteristic Configuration Notification.
 *
 * If set, changes to Characteristic Value shall be notified.
 */
#define BT_GATT_CCC_NOTIFY			0x0001
/* @def BT_GATT_CCC_INDICATE
 * @brief Client Characteristic Configuration Indication.
 *
 * If set, changes to Characteristic Value shall be indicated.
 */
#define BT_GATT_CCC_INDICATE			0x0002

/* Client Characteristic Configuration Attribute Value */
struct bt_gatt_ccc {
	/*! Client Characteristic Configuration flags */
	uint16_t		flags;
};

/* Server API */

/*! @brief Register attribute database.
 *
 *  Register GATT attribute database table. Applications can make use of
 *  macros such as BT_GATT_PRIMARY_SERVICE, BT_GATT_CHARACTERISTIC,
 *  BT_GATT_DESCRIPTOR, etc.
 *
 *  @param attrs dabase table containing the available attributes.
 *  @param count size of database table.
 */
void bt_gatt_register(const struct bt_gatt_attr *attrs, size_t count);

enum {
	BT_GATT_ITER_STOP = 0,
	BT_GATT_ITER_CONTINUE,
};

/*! @brief Attribute iterator callback.
 *
 *  @param attr attribute found.
 *  @param user_data data given.
 *
 *  @return BT_GATT_ITER_CONTINUE if should continue to the next attribute
 *   or BT_GATT_ITER_STOP to stop.
 */
typedef uint8_t (*bt_gatt_attr_func_t)(const struct bt_gatt_attr *attr,
				       void *user_data);

/*! @brief Attribute iterator.
 *
 *  Iterate attributes in the given range.
 *
 *  @param start_handle start handle.
 *  @param end_handle end handle.
 *  @param func callback function.
 *  @param user_data data to pass to the callback.
 */
void bt_gatt_foreach_attr(uint16_t start_handle, uint16_t end_handle,
			  bt_gatt_attr_func_t func, void *user_data);

/*! @brief Generic Read Attribute value helper.
 *
 *  Read attribute value storing the result into buffer.
 *
 *  @param peer remote address.
 *  @param attr attribute to read.
 *  @param buf buffer to store the value.
 *  @param buf_len buffer length.
 *  @param offset start offset.
 *  @param value attribute value.
 *  @param value_len length of the attribute value.
 *
 *  @return int number of bytes read in case of success or negative values in
 *  case of error.
 */
int bt_gatt_attr_read(const bt_addr_le_t *peer, const struct bt_gatt_attr *attr,
		      void *buf, uint8_t buf_len, uint16_t offset,
		      const void *value, uint8_t value_len);

/*! @brief Read Service Attribute helper.
 *
 *  Read service attribute value storing the result into buffer after
 *  enconding it.
 *  NOTE: Only use this with attributes which user_data is a bt_uuid.
 *
 *  @param peer remote address.
 *  @param attr attribute to read.
 *  @param buf buffer to store the value read.
 *  @param len buffer length.
 *  @param offset start offset.
 *
 *  @return int number of bytes read in case of success or negative values in
 *  case of error.
 */
int bt_gatt_attr_read_service(const bt_addr_le_t *peer,
			      const struct bt_gatt_attr *attr,
			      void *buf, uint8_t len, uint16_t offset);

/*! @def BT_GATT_SERVICE
 *  @brief Generic Service Declaration Macro.
 *
 *  Helper macro to declare a service attribute.
 *
 *  @param _handle service attribute handle.
 *  @param _uuid service attribute type.
 *  @param _data service attribute value.
 */
#define BT_GATT_SERVICE(_handle, _uuid, _service)			\
{									\
	.handle = _handle,						\
	.uuid = _uuid,							\
	.perm = BT_GATT_PERM_READ,					\
	.read = bt_gatt_attr_read_service,				\
	.user_data = _service,						\
}

/*! @def BT_GATT_PRIMARY_SERVICE
 *  @brief Primary Service Declaration Macro.
 *
 *  Helper macro to declare a primary service attribute.
 *
 *  @param _handle service attribute handle.
 *  @param _service service attribute value.
 */
#define BT_GATT_PRIMARY_SERVICE(_handle, _service)			\
{									\
	.handle = _handle,						\
	.uuid = (&(struct bt_uuid) { .type = BT_UUID_16,		\
				     .u16 = BT_UUID_GATT_PRIMARY }),	\
	.perm = BT_GATT_PERM_READ,					\
	.read = bt_gatt_attr_read_service,				\
	.user_data = _service,						\
}

/*! @def BT_GATT_SECONDARY_SERVICE
 *  @brief Secondary Service Declaration Macro.
 *
 *  Helper macro to declare a secondary service attribute.
 *
 *  @param _handle service attribute handle.
 *  @param _service service attribute value.
 */
#define BT_GATT_SECONDARY_SERVICE(_handle, _service)			\
{									\
	.handle = _handle,						\
	.uuid = (&(struct bt_uuid) { .type = BT_UUID_16,		\
				     .u16 = BT_UUID_GATT_SECONDARY }),	\
	.perm = BT_GATT_PERM_READ,					\
	.read = bt_gatt_attr_read_service,				\
	.user_data = _service,						\
}

/*! @brief Read Include Attribute helper.
 *
 *  Read include service attribute value storing the result into buffer after
 *  enconding it.
 *  NOTE: Only use this with attributes which user_data is a bt_gatt_include.
 *
 *  @param peer remote address.
 *  @param attr attribute to read.
 *  @param buf buffer to store the value read.
 *  @param len buffer length.
 *  @param offset start offset.
 *
 *  @return int number of bytes read in case of success or negative values in
 *  case of error.
 */
int bt_gatt_attr_read_included(const bt_addr_le_t *peer,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint8_t len, uint16_t offset);

/*! @def BT_GATT_INCLUDE_SERVICE
 *  @brief Include Service Declaration Macro.
 *
 *  Helper macro to declare a include service attribute.
 *
 *  @param _handle service attribute handle.
 *  @param _service service attribute value.
 */
#define BT_GATT_INCLUDE_SERVICE(_handle, _service)			\
{									\
	.handle = _handle,						\
	.uuid = (&(struct bt_uuid) { .type = BT_UUID_16,		\
				     .u16 = BT_UUID_GATT_INCLUDE }),	\
	.perm = BT_GATT_PERM_READ,					\
	.read = bt_gatt_attr_read_included,				\
	.user_data = _service,						\
}

/*! @brief Read Characteristic Attribute helper.
 *
 *  Read characteristic attribute value storing the result into buffer after
 *  enconding it.
 *  NOTE: Only use this with attributes which user_data is a bt_gatt_chrc.
 *
 *  @param peer remote address.
 *  @param attr attribute to read.
 *  @param buf buffer to store the value read.
 *  @param len buffer length.
 *  @param offset start offset.
 *
 *  @return number of bytes read in case of success or negative values in
 *  case of error.
 */
int bt_gatt_attr_read_chrc(const bt_addr_le_t *peer,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint8_t len, uint16_t offset);

/*! @def BT_GATT_CHARACTERISTIC
 *  @brief Characteristic Declaration Macro.
 *
 *  Helper macro to declare a characteristic attribute.
 *
 *  @param _handle characteristic attribute handle.
 *  @param _value characteristic attribute value.
 */
#define BT_GATT_CHARACTERISTIC(_handle, _value)				\
{									\
	.handle = _handle,						\
	.uuid = (&(struct bt_uuid) { .type = BT_UUID_16,		\
				     .u16 = BT_UUID_GATT_CHRC }),	\
	.perm = BT_GATT_PERM_READ,					\
	.read = bt_gatt_attr_read_chrc,					\
	.user_data = _value,						\
}

/*! @brief GATT CCC configuration entry. */
struct bt_gatt_ccc_cfg {
	/*! Config peer address. */
	bt_addr_le_t		peer;
	/*! Config peer value. */
	uint16_t		value;
	/*! Config valid flag. */
	uint8_t			valid;
};

/* Internal representation of CCC value */
struct _bt_gatt_ccc {
	struct bt_gatt_ccc_cfg	*cfg;
	size_t			cfg_len;
	uint16_t		value;
	uint16_t		value_handle;
	void			(*cfg_changed)(uint16_t value);
};

/*! @brief Read Client Characteristic Configuration Attribute helper.
 *
 *  Read CCC attribute value storing the result into buffer after
 *  enconding it.
 *  NOTE: Only use this with attributes which user_data is a _bt_gatt_ccc.
 *
 *  @param peer remote address.
 *  @param attr attribute to read.
 *  @param buf buffer to store the value read.
 *  @param len buffer length.
 *  @param offset start offset.
 *
 *  @return number of bytes read in case of success or negative values in
 *  case of error.
 */
int bt_gatt_attr_read_ccc(const bt_addr_le_t *peer,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint8_t len, uint16_t offset);

/*! @brief Write Client Characteristic Configuration Attribute helper.
 *
 *  Write value in the buffer into CCC attribute.
 *  NOTE: Only use this with attributes which user_data is a _bt_gatt_ccc.
 *
 *  @param peer remote address.
 *  @param attr attribute to read.
 *  @param buf buffer to store the value read.
 *  @param len buffer length.
 *  @param offset start offset.
 *
 *  @return number of bytes written in case of success or negative values in
 *  case of error.
 */
int bt_gatt_attr_write_ccc(const bt_addr_le_t *peer,
			   const struct bt_gatt_attr *attr, const void *buf,
			   uint8_t len, uint16_t offset);

/*! @def BT_GATT_CCC
 *  @brief Client Characteristic Configuration Declaration Macro.
 *
 *  Helper macro to declare a CCC attribute.
 *
 *  @param _handle descriptor attribute handle.
 *  @param _value_handle characteristic attribute value handle.
 *  @param _cfg initial configuration.
 *  @param _cfg_changed configuration changed callback.
 */
#define BT_GATT_CCC(_handle, _value_handle, _cfg, _cfg_changed)		\
{									\
	.handle = _handle,						\
	.uuid = (&(struct bt_uuid) { .type = BT_UUID_16,		\
				     .u16 = BT_UUID_GATT_CCC }),	\
	.perm = BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,			\
	.read = bt_gatt_attr_read_ccc,					\
	.write = bt_gatt_attr_write_ccc,				\
	.user_data = (&(struct _bt_gatt_ccc) { .cfg = _cfg,		\
					       .cfg_len = ARRAY_SIZE(_cfg), \
					       .value_handle = _value_handle, \
					       .cfg_changed = _cfg_changed, }),\
}

/*! @def BT_GATT_DESCRIPTOR
 *  @brief Descriptor Declaration Macro.
 *
 *  Helper macro to declare a descriptor attribute.
 *
 *  @param _handle descriptor attribute handle.
 *  @param _value descriptor attribute value.
 *  @param _perm descriptor attribute access permissions.
 *  @param _read descriptor attribute read callback.
 *  @param _write descriptor attribute write callback.
 *  @param _value descriptor attribute value.
 */
#define BT_GATT_DESCRIPTOR(_handle, _uuid, _perm, _read, _write, _value) \
{									\
	.handle = _handle,						\
	.uuid = _uuid,							\
	.perm = _perm,							\
	.read = _read,							\
	.write = _write,						\
	.user_data = _value,						\
}

/*! @brief Notify attribute value change.
 *
 *  Send notification of attribute value change.
 *  Note: This function should only be called if CCC is declared with
 *  BT_GATT_CCC otherwise it cannot find a valid peer configuration.
 *
 *  @param handle attribute handle.
 *  @param value attribute value.
 *  @param len attribute value length.
 */
void bt_gatt_notify(uint16_t handle, const void *data, size_t len);

#endif /* __BT_GATT_H */
