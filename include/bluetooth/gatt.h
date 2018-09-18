/** @file
 *  @brief Generic Attribute Profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_GATT_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_GATT_H_

/**
 * @brief Generic Attribute Profile (GATT)
 * @defgroup bt_gatt Generic Attribute Profile (GATT)
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <sys/types.h>
#include <misc/util.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/att.h>

/* GATT attribute permission bit field values */
enum {
	/** No operations supported, e.g. for notify-only */
	BT_GATT_PERM_NONE = 0,

	/** Attribute read permission. */
	BT_GATT_PERM_READ = BIT(0),

	/** Attribute write permission. */
	BT_GATT_PERM_WRITE = BIT(1),

	/** Attribute read permission with encryption.
	 *
	 *  If set, requires encryption for read access.
	 */
	BT_GATT_PERM_READ_ENCRYPT = BIT(2),

	/** Attribute write permission with encryption.
	 *
	 *  If set, requires encryption for write access.
	 */
	BT_GATT_PERM_WRITE_ENCRYPT = BIT(3),

	/** Attribute read permission with authentication.
	 *
	 *  If set, requires encryption using authenticated link-key for read
	 *  access.
	 */
	BT_GATT_PERM_READ_AUTHEN = BIT(4),

	/** Attribute write permission with authentication.
	 *
	 *  If set, requires encryption using authenticated link-key for write
	 *  access.
	 */
	BT_GATT_PERM_WRITE_AUTHEN = BIT(5),

	/** Attribute prepare write permission.
	 *
	 *  If set, allows prepare writes with use of BT_GATT_WRITE_FLAG_PREPARE
	 *  passed to write callback.
	 */
	BT_GATT_PERM_PREPARE_WRITE = BIT(6),
};

/**  @def BT_GATT_ERR
  *  @brief Construct error return value for attribute read and write callbacks.
  *
  *  @param _att_err ATT error code
  *
  *  @return Appropriate error code for the attribute callbacks.
  *
  */
#define BT_GATT_ERR(_att_err)                  (-(_att_err))

/* GATT attribute write flags */
enum {
	/** Attribute prepare write flag
	 *
	 * If set, write callback should only check if the device is
	 * authorized but no data shall be written.
	 */
	BT_GATT_WRITE_FLAG_PREPARE = BIT(0),
};

/** @brief GATT Attribute structure. */
struct bt_gatt_attr {
	/** Attribute UUID */
	const struct bt_uuid	*uuid;

	/** Attribute read callback
	 *
	 *  @param conn   The connection that is requesting to read
	 *  @param attr   The attribute that's being read
	 *  @param buf    Buffer to place the read result in
	 *  @param len    Length of data to read
	 *  @param offset Offset to start reading from
	 *
	 *  @return Number fo bytes read, or in case of an error
	 *          BT_GATT_ERR() with a specific ATT error code.
	 */
	ssize_t			(*read)(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, u16_t len,
					u16_t offset);

	/** Attribute write callback
	 *
	 *  @param conn   The connection that is requesting to write
	 *  @param attr   The attribute that's being written
	 *  @param buf    Buffer with the data to write
	 *  @param len    Number of bytes in the buffer
	 *  @param offset Offset to start writing from
	 *  @param flags  Flags (BT_GATT_WRITE_*)
	 *
	 *  @return Number of bytes written, or in case of an error
	 *          BT_GATT_ERR() with a specific ATT error code.
	 */
	ssize_t			(*write)(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 const void *buf, u16_t len,
					 u16_t offset, u8_t flags);

	/** Attribute user data */
	void			*user_data;
	/** Attribute handle */
	u16_t			handle;
	/** Attribute permissions */
	u8_t			perm;
};

/** @brief GATT Service structure */
struct bt_gatt_service {
	/** Service Attributes */
	struct bt_gatt_attr	*attrs;
	/** Service Attribute count */
	size_t			attr_count;
	sys_snode_t		node;
};

/** @brief Service Attribute Value. */
struct bt_gatt_service_val {
	/** Service UUID. */
	const struct bt_uuid	*uuid;
	/** Service end handle. */
	u16_t			end_handle;
};

/** @brief Include Attribute Value. */
struct bt_gatt_include {
	/** Service UUID. */
	const struct bt_uuid	*uuid;
	/** Service start handle. */
	u16_t			start_handle;
	/** Service end handle. */
	u16_t			end_handle;
};

/* Characteristic Properties Bit field values */

/** @def BT_GATT_CHRC_BROADCAST
 *  @brief Characteristic broadcast property.
 *
 *  If set, permits broadcasts of the Characteristic Value using Server
 *  Characteristic Configuration Descriptor.
 */
#define BT_GATT_CHRC_BROADCAST			0x01
/** @def BT_GATT_CHRC_READ
 *  @brief Characteristic read property.
 *
 *  If set, permits reads of the Characteristic Value.
 */
#define BT_GATT_CHRC_READ			0x02
/** @def BT_GATT_CHRC_WRITE_WITHOUT_RESP
 *  @brief Characteristic write without response property.
 *
 *  If set, permits write of the Characteristic Value without response.
 */
#define BT_GATT_CHRC_WRITE_WITHOUT_RESP		0x04
/** @def BT_GATT_CHRC_WRITE
 *  @brief Characteristic write with response property.
 *
 *  If set, permits write of the Characteristic Value with response.
 */
#define BT_GATT_CHRC_WRITE			0x08
/** @def BT_GATT_CHRC_NOTIFY
 *  @brief Characteristic notify property.
 *
 *  If set, permits notifications of a Characteristic Value without
 *  acknowledgment.
 */
#define BT_GATT_CHRC_NOTIFY			0x10
/** @def BT_GATT_CHRC_INDICATE
 *  @brief Characteristic indicate property.
 *
 * If set, permits indications of a Characteristic Value with acknowledgment.
 */
#define BT_GATT_CHRC_INDICATE			0x20
/** @def BT_GATT_CHRC_AUTH
 *  @brief Characteristic Authenticated Signed Writes property.
 *
 *  If set, permits signed writes to the Characteristic Value.
 */
#define BT_GATT_CHRC_AUTH			0x40
/** @def BT_GATT_CHRC_EXT_PROP
 *  @brief Characteristic Extended Properties property.
 *
 * If set, additional characteristic properties are defined in the
 * Characteristic Extended Properties Descriptor.
 */
#define BT_GATT_CHRC_EXT_PROP			0x80

/** @brief Characteristic Attribute Value. */
struct bt_gatt_chrc {
	/** Characteristic UUID. */
	const struct bt_uuid	*uuid;
	/** Characteristic properties. */
	u8_t			properties;
};

/* Characteristic Extended Properties Bit field values */
#define BT_GATT_CEP_RELIABLE_WRITE		0x0001
#define BT_GATT_CEP_WRITABLE_AUX		0x0002

/** @brief Characteristic Extended Properties Attribute Value. */
struct bt_gatt_cep {
	/** Characteristic Extended properties */
	u16_t		properties;
};

/* Client Characteristic Configuration Values */

/** @def BT_GATT_CCC_NOTIFY
 *  @brief Client Characteristic Configuration Notification.
 *
 *  If set, changes to Characteristic Value shall be notified.
 */
#define BT_GATT_CCC_NOTIFY			0x0001
/** @def BT_GATT_CCC_INDICATE
 *  @brief Client Characteristic Configuration Indication.
 *
 *  If set, changes to Characteristic Value shall be indicated.
 */
#define BT_GATT_CCC_INDICATE			0x0002

/* Client Characteristic Configuration Attribute Value */
struct bt_gatt_ccc {
	/** Client Characteristic Configuration flags */
	u16_t		flags;
};

/** @brief GATT Characteristic Presentation Format Attribute Value. */
struct bt_gatt_cpf {
	/** Format of the value of the characteristic */
	u8_t format;
	/** Exponent field to determine how the value of this characteristic is further formatted */
	s8_t exponent;
	/** Unit of the characteristic */
	u16_t unit;
	/** Name space of the description */
	u8_t name_space;
	/** Description of the characteristic as defined in a higher layer profile */
	u16_t description;
} __packed;

/* Server API */

/** @brief Register GATT service.
 *
 *  Register GATT service. Applications can make use of
 *  macros such as BT_GATT_PRIMARY_SERVICE, BT_GATT_CHARACTERISTIC,
 *  BT_GATT_DESCRIPTOR, etc.
 *
 *  @param svc Service containing the available attributes
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_service_register(struct bt_gatt_service *svc);

/** @brief Unregister GATT service.
 * *
 *  @param svc Service to be unregistered.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_service_unregister(struct bt_gatt_service *svc);

enum {
	BT_GATT_ITER_STOP = 0,
	BT_GATT_ITER_CONTINUE,
};

/** @typedef bt_gatt_attr_func_t
 *  @brief Attribute iterator callback.
 *
 *  @param attr Attribute found.
 *  @param user_data Data given.
 *
 *  @return BT_GATT_ITER_CONTINUE if should continue to the next attribute
 *  or BT_GATT_ITER_STOP to stop.
 */
typedef u8_t (*bt_gatt_attr_func_t)(const struct bt_gatt_attr *attr,
				       void *user_data);

/** @brief Attribute iterator.
 *
 *  Iterate attributes in the given range.
 *
 *  @param start_handle Start handle.
 *  @param end_handle End handle.
 *  @param func Callback function.
 *  @param user_data Data to pass to the callback.
 */
void bt_gatt_foreach_attr(u16_t start_handle, u16_t end_handle,
			  bt_gatt_attr_func_t func, void *user_data);

/** @brief Iterate to the next attribute
 *
 *  Iterate to the next attribute following a given attribute.
 *
 *  @param attr Current Attribute.
 *
 *  @return The next attribute or NULL if it cannot be found.
 */
struct bt_gatt_attr *bt_gatt_attr_next(const struct bt_gatt_attr *attr);

/** @brief Generic Read Attribute value helper.
 *
 *  Read attribute value storing the result into buffer.
 *
 *  @param conn Connection object.
 *  @param attr Attribute to read.
 *  @param buf Buffer to store the value.
 *  @param buf_len Buffer length.
 *  @param offset Start offset.
 *  @param value Attribute value.
 *  @param value_len Length of the attribute value.
 *
 *  @return int number of bytes read in case of success or negative values in
 *  case of error.
 */
ssize_t bt_gatt_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, u16_t buf_len, u16_t offset,
			  const void *value, u16_t value_len);

/** @brief Read Service Attribute helper.
 *
 *  Read service attribute value storing the result into buffer after
 *  encoding it.
 *  NOTE: Only use this with attributes which user_data is a bt_uuid.
 *
 *  @param conn Connection object.
 *  @param attr Attribute to read.
 *  @param buf Buffer to store the value read.
 *  @param len Buffer length.
 *  @param offset Start offset.
 *
 *  @return int number of bytes read in case of success or negative values in
 *  case of error.
 */
ssize_t bt_gatt_attr_read_service(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, u16_t len, u16_t offset);

/** @def BT_GATT_SERVICE
 *  @brief Service Structure Declaration Macro.
 *
 *  Helper macro to declare a service structure.
 *
 *  @param _attrs Service attributes.
 */
#define BT_GATT_SERVICE(_attrs)						\
{									\
	.attrs = _attrs,						\
	.attr_count = ARRAY_SIZE(_attrs),				\
}

/** @def BT_GATT_PRIMARY_SERVICE
 *  @brief Primary Service Declaration Macro.
 *
 *  Helper macro to declare a primary service attribute.
 *
 *  @param _service Service attribute value.
 */
#define BT_GATT_PRIMARY_SERVICE(_service)				\
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_PRIMARY, BT_GATT_PERM_READ,	\
			 bt_gatt_attr_read_service, NULL, _service)

/** @def BT_GATT_SECONDARY_SERVICE
 *  @brief Secondary Service Declaration Macro.
 *
 *  Helper macro to declare a secondary service attribute.
 *
 *  @param _service Service attribute value.
 */
#define BT_GATT_SECONDARY_SERVICE(_service)				\
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_SECONDARY, BT_GATT_PERM_READ,	\
			 bt_gatt_attr_read_service, NULL, _service)

/** @brief Read Include Attribute helper.
 *
 *  Read include service attribute value storing the result into buffer after
 *  encoding it.
 *  NOTE: Only use this with attributes which user_data is a bt_gatt_include.
 *
 *  @param conn Connection object.
 *  @param attr Attribute to read.
 *  @param buf Buffer to store the value read.
 *  @param len Buffer length.
 *  @param offset Start offset.
 *
 *  @return int number of bytes read in case of success or negative values in
 *  case of error.
 */
ssize_t bt_gatt_attr_read_included(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   void *buf, u16_t len, u16_t offset);

/** @def BT_GATT_INCLUDE_SERVICE
 *  @brief Include Service Declaration Macro.
 *
 *  Helper macro to declare database internal include service attribute.
 *
 *  @param _service_incl the first service attribute of service to include
 */
#define BT_GATT_INCLUDE_SERVICE(_service_incl)				\
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_INCLUDE, BT_GATT_PERM_READ,	\
			  bt_gatt_attr_read_included, NULL, _service_incl)

/** @brief Read Characteristic Attribute helper.
 *
 *  Read characteristic attribute value storing the result into buffer after
 *  encoding it.
 *  NOTE: Only use this with attributes which user_data is a bt_gatt_chrc.
 *
 *  @param conn Connection object.
 *  @param attr Attribute to read.
 *  @param buf Buffer to store the value read.
 *  @param len Buffer length.
 *  @param offset Start offset.
 *
 *  @return number of bytes read in case of success or negative values in
 *  case of error.
 */
ssize_t bt_gatt_attr_read_chrc(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       u16_t len, u16_t offset);

/** @def BT_GATT_CHARACTERISTIC
 *  @brief Characteristic and Value Declaration Macro.
 *
 *  Helper macro to declare a characteristic attribute along with its
 *  attribute value.
 *
 *  @param _uuid Characteristic attribute uuid.
 *  @param _props Characteristic attribute properties.
 *  @param _perm Characteristic Attribute access permissions.
 *  @param _read Characteristic Attribute read callback.
 *  @param _write Characteristic Attribute write callback.
 *  @param _value Characteristic Attribute value.
 */
#define BT_GATT_CHARACTERISTIC(_uuid, _props, _perm, _read, _write, _value) \
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC, BT_GATT_PERM_READ,		\
			  bt_gatt_attr_read_chrc, NULL,			\
			  (&(struct bt_gatt_chrc) { .uuid = _uuid,	\
						   .properties = _props, })), \
	BT_GATT_ATTRIBUTE(_uuid, _perm, _read, _write, _value)

#define BT_GATT_CCC_MAX (CONFIG_BT_MAX_PAIRED + CONFIG_BT_MAX_CONN)

/** @brief GATT CCC configuration entry.
 *  @param id   Local identity, BT_ID_DEFAULT in most cases.
 *  @param peer Remote peer address
 *  @param value Configuration value.
 *  @param data Configuration pointer data.
 */
struct bt_gatt_ccc_cfg {
	u8_t                    id;
	bt_addr_le_t		peer;
	u16_t			value;
	u8_t			data[4] __aligned(4);
};

/* Internal representation of CCC value */
struct _bt_gatt_ccc {
	struct bt_gatt_ccc_cfg	*cfg;
	size_t			cfg_len;
	u16_t			value;
	void			(*cfg_changed)(const struct bt_gatt_attr *attr,
					       u16_t value);
};

/** @brief Read Client Characteristic Configuration Attribute helper.
 *
 *  Read CCC attribute value storing the result into buffer after
 *  encoding it.
 *  NOTE: Only use this with attributes which user_data is a _bt_gatt_ccc.
 *
 *  @param conn Connection object.
 *  @param attr Attribute to read.
 *  @param buf Buffer to store the value read.
 *  @param len Buffer length.
 *  @param offset Start offset.
 *
 *  @return number of bytes read in case of success or negative values in
 *  case of error.
 */
ssize_t bt_gatt_attr_read_ccc(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      u16_t len, u16_t offset);

/** @brief Write Client Characteristic Configuration Attribute helper.
 *
 *  Write value in the buffer into CCC attribute.
 *  NOTE: Only use this with attributes which user_data is a _bt_gatt_ccc.
 *
 *  @param conn Connection object.
 *  @param attr Attribute to read.
 *  @param buf Buffer to store the value read.
 *  @param len Buffer length.
 *  @param offset Start offset.
 *  @param flags Write flags.
 *
 *  @return number of bytes written in case of success or negative values in
 *  case of error.
 */
ssize_t bt_gatt_attr_write_ccc(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       u16_t len, u16_t offset, u8_t flags);

/** @def BT_GATT_CCC
 *  @brief Client Characteristic Configuration Declaration Macro.
 *
 *  Helper macro to declare a CCC attribute.
 *
 *  @param _cfg Initial configuration.
 *  @param _cfg_changed Configuration changed callback.
 */
#define BT_GATT_CCC(_cfg, _cfg_changed)					\
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_CCC,				\
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,		\
			bt_gatt_attr_read_ccc, bt_gatt_attr_write_ccc,	\
			(&(struct _bt_gatt_ccc) { .cfg = _cfg,		\
					       .cfg_len = ARRAY_SIZE(_cfg), \
					       .cfg_changed = _cfg_changed, }))

/** @brief Read Characteristic Extended Properties Attribute helper
 *
 *  Read CEP attribute value storing the result into buffer after
 *  encoding it.
 *  NOTE: Only use this with attributes which user_data is a bt_gatt_cep.
 *
 *  @param conn Connection object
 *  @param attr Attribute to read
 *  @param buf Buffer to store the value read
 *  @param len Buffer length
 *  @param offset Start offset
 *
 *  @return number of bytes read in case of success or negative values in
 *  case of error.
 */
ssize_t bt_gatt_attr_read_cep(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      u16_t len, u16_t offset);

/** @def BT_GATT_CEP
 *  @brief Characteristic Extended Properties Declaration Macro.
 *
 *  Helper macro to declare a CEP attribute.
 *
 *  @param _value Descriptor attribute value.
 */
#define BT_GATT_CEP(_value)						\
	BT_GATT_DESCRIPTOR(BT_UUID_GATT_CEP, BT_GATT_PERM_READ,		\
			  bt_gatt_attr_read_cep, NULL, (void *)_value)

/** @brief Read Characteristic User Description Descriptor Attribute helper
 *
 *  Read CUD attribute value storing the result into buffer after
 *  encoding it.
 *  NOTE: Only use this with attributes which user_data is a NULL-terminated C string.
 *
 *  @param conn Connection object
 *  @param attr Attribute to read
 *  @param buf Buffer to store the value read
 *  @param len Buffer length
 *  @param offset Start offset
 *
 *  @return number of bytes read in case of success or negative values in
 *  case of error.
 */
ssize_t bt_gatt_attr_read_cud(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      u16_t len, u16_t offset);

/** @def BT_GATT_CUD
 *  @brief Characteristic User Format Descriptor Declaration Macro.
 *
 *  Helper macro to declare a CUD attribute.
 *
 *  @param _value User description NULL-terminated C string.
 *  @param _perm Descriptor attribute access permissions.
 */
#define BT_GATT_CUD(_value, _perm)					\
	BT_GATT_DESCRIPTOR(BT_UUID_GATT_CUD, _perm, bt_gatt_attr_read_cud, \
			   NULL, (void *)_value)

/** @brief Read Characteristic Presentation format Descriptor Attribute helper
 *
 *  Read CPF attribute value storing the result into buffer after
 *  encoding it.
 *  NOTE: Only use this with attributes which user_data is a bt_gatt_pf.
 *
 *  @param conn Connection object
 *  @param attr Attribute to read
 *  @param buf Buffer to store the value read
 *  @param len Buffer length
 *  @param offset Start offset
 *
 *  @return number of bytes read in case of success or negative values in
 *  case of error.
 */
ssize_t bt_gatt_attr_read_cpf(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      u16_t len, u16_t offset);

/** @def BT_GATT_CPF
 *  @brief Characteristic Presentation Format Descriptor Declaration Macro.
 *
 *  Helper macro to declare a CPF attribute.
 *
 *  @param _value Descriptor attribute value.
 */
#define BT_GATT_CPF(_value)						\
	BT_GATT_DESCRIPTOR(BT_UUID_GATT_CPF, BT_GATT_PERM_READ,		\
			  bt_gatt_attr_read_cpf, NULL, (void *)_value)

/** @def BT_GATT_DESCRIPTOR
 *  @brief Descriptor Declaration Macro.
 *
 *  Helper macro to declare a descriptor attribute.
 *
 *  @param _uuid Descriptor attribute uuid.
 *  @param _perm Descriptor attribute access permissions.
 *  @param _read Descriptor attribute read callback.
 *  @param _write Descriptor attribute write callback.
 *  @param _value Descriptor attribute value.
 */
#define BT_GATT_DESCRIPTOR(_uuid, _perm, _read, _write, _value)		\
	BT_GATT_ATTRIBUTE(_uuid, _perm, _read, _write, _value)

/** @def BT_GATT_ATTRIBUTE
 *  @brief Attribute Declaration Macro.
 *
 *  Helper macro to declare an attribute.
 *
 *  @param _uuid Attribute uuid.
 *  @param _perm Attribute access permissions.
 *  @param _read Attribute read callback.
 *  @param _write Attribute write callback.
 *  @param _value Attribute value.
 */
#define BT_GATT_ATTRIBUTE(_uuid, _perm, _read, _write, _value)		\
{									\
	.uuid = _uuid,							\
	.perm = _perm,							\
	.read = _read,							\
	.write = _write,						\
	.user_data = _value,						\
}

/** @brief Notification complete result callback.
 *
 *  @param conn Connection object.
 */
typedef void (*bt_gatt_notify_complete_func_t) (struct bt_conn *conn);

/** @brief Notify attribute value change with callback.
 *
 *  This function works in the same way as @ref bt_gatt_notify.
 *  With the addition that after sending the notification the
 *  callback function will be called.
 *
 *  @param conn Connection object.
 *  @param attr Characteristic or Characteristic Value attribute.
 *  @param data Pointer to Attribute data.
 *  @param len Attribute value length.
 *  @param func Notification value callback.
 */
int bt_gatt_notify_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		      const void *data, u16_t len,
		      bt_gatt_notify_complete_func_t func);

/** @brief Notify attribute value change.
 *
 *  Send notification of attribute value change, if connection is NULL notify
 *  all peer that have notification enabled via CCC otherwise do a direct
 *  notification only the given connection.
 *
 *  The attribute object can be the so called Characteristic Declaration,
 *  which is usually declared with BT_GATT_CHARACTERISTIC followed by
 *  BT_GATT_CCC, or the Characteristic Value Declaration which is automatically
 *  created after the Characteristic Declaration when using
 *  BT_GATT_CHARACTERISTIC.
 *
 *  @param conn Connection object.
 *  @param attr Characteristic or Characteristic Value attribute.
 *  @param data Pointer to Attribute data.
 *  @param len Attribute value length.
 */
static inline int bt_gatt_notify(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *data, u16_t len)
{
	return bt_gatt_notify_cb(conn, attr, data, len, NULL);
}

/** @typedef bt_gatt_indicate_func_t
 *  @brief Indication complete result callback.
 *
 *  @param conn Connection object.
 *  @param attr Attribute object.
 *  @param err ATT error code
 *
 *  @return 0 in case of success or negative value in case of error.
 */
typedef void (*bt_gatt_indicate_func_t)(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					u8_t err);

/** @brief GATT Indicate Value parameters */
struct bt_gatt_indicate_params {
	struct bt_att_req _req;
	/** Indicate Attribute object*/
	const struct bt_gatt_attr *attr;
	/** Indicate Value callback */
	bt_gatt_indicate_func_t func;
	/** Indicate Value data*/
	const void *data;
	/** Indicate Value length*/
	u16_t len;
};

/** @brief Indicate attribute value change.
 *
 *  Send an indication of attribute value change.
 *  Note: This function should only be called if CCC is declared with
 *  BT_GATT_CCC otherwise it cannot find a valid peer configuration.
 *
 *  Note: This procedure is asynchronous therefore the parameters need to
 *  remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Indicate parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_indicate(struct bt_conn *conn,
		     struct bt_gatt_indicate_params *params);

/** @brief Get ATT MTU for a connection
 *
 *  Get negotiated ATT connection MTU, note that this does not equal the largest
 *  amount of attribute data that can be transferred within a single packet.
 *
 *  @param conn Connection object.
 *
 *  @return MTU in bytes
 */
u16_t bt_gatt_get_mtu(struct bt_conn *conn);

/* Client API */

/** @brief GATT Exchange MTU parameters */
struct bt_gatt_exchange_params {
	struct bt_att_req _req;
	/** Response callback */
	void (*func)(struct bt_conn *conn, u8_t err,
		     struct bt_gatt_exchange_params *params);
};

/** @brief Exchange MTU
 *
 *  This client procedure can be used to set the MTU to the maximum possible
 *  size the buffers can hold.
 *
 *  NOTE: Shall only be used once per connection.
 *
 *  @param conn Connection object.
 *  @param params Exchange MTU parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_exchange_mtu(struct bt_conn *conn,
			 struct bt_gatt_exchange_params *params);

struct bt_gatt_discover_params;

/** @typedef bt_gatt_discover_func_t
 *  @brief Discover attribute callback function.
 *
 *  @param conn Connection object.
 *  @param attr Attribute found.
 *  @param params Discovery parameters given.
 *
 *  If discovery procedure has completed this callback will be called with
 *  attr set to NULL. This will not happen if procedure was stopped by returning
 *  BT_GATT_ITER_STOP. The attribute is read-only and cannot be cached without
 *  copying its contents.
 *
 *  @return BT_GATT_ITER_CONTINUE if should continue attribute discovery
 *  or BT_GATT_ITER_STOP to stop discovery procedure.
 */
typedef u8_t (*bt_gatt_discover_func_t)(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					struct bt_gatt_discover_params *params);

enum {
	BT_GATT_DISCOVER_PRIMARY,
	BT_GATT_DISCOVER_SECONDARY,
	BT_GATT_DISCOVER_INCLUDE,
	BT_GATT_DISCOVER_CHARACTERISTIC,
	BT_GATT_DISCOVER_DESCRIPTOR,
};

/** @brief GATT Discover Attributes parameters */
struct bt_gatt_discover_params {
	struct bt_att_req _req;
	/** Discover UUID type */
	struct bt_uuid *uuid;
	/** Discover attribute callback */
	bt_gatt_discover_func_t func;
	union {
		struct {
			/** Include service attribute declaration handle */
			u16_t attr_handle;
			/** Included service start handle */
			u16_t start_handle;
			/** Included service end handle */
			u16_t end_handle;
		} _included;
		/** Discover start handle */
		u16_t start_handle;
	};
	/** Discover end handle */
	u16_t end_handle;
	/** Discover type */
	u8_t type;
};

/** @brief GATT Discover function
 *
 *  This procedure is used by a client to discover attributes on a server.
 *
 *  Primary Service Discovery: Procedure allows to discover specific Primary
 *                             Service based on UUID.
 *  Include Service Discovery: Procedure allows to discover all Include Services
 *                             within specified range.
 *  Characteristic Discovery:  Procedure allows to discover all characteristics
 *                             within specified handle range as well as
 *                             discover characteristics with specified UUID.
 *  Descriptors Discovery:     Procedure allows to discover all characteristic
 *                             descriptors within specified range.
 *
 *  For each attribute found the callback is called which can then decide
 *  whether to continue discovering or stop.
 *
 *  Note: This procedure is asynchronous therefore the parameters need to
 *  remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Discover parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_discover(struct bt_conn *conn,
		     struct bt_gatt_discover_params *params);

struct bt_gatt_read_params;

/** @typedef bt_gatt_read_func_t
 *  @brief Read callback function
 *
 *  @param conn Connection object.
 *  @param err ATT error code.
 *  @param params Read parameters used.
 *  @param data Attribute value data. NULL means read has completed.
 *  @param length Attribute value length.
 */
typedef u8_t (*bt_gatt_read_func_t)(struct bt_conn *conn, u8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, u16_t length);

/** @brief GATT Read parameters
 *  @param func Read attribute callback
 *  @param handle_count If equals to 1 single.handle and single.offset
 *                      are used.  If >1 Read Multiple Characteristic
 *                      Values is performed and handles are used.
 *  @param handle Attribute handle
 *  @param offset Attribute data offset
 *  @param handles Handles to read in Read Multiple Characteristic Values
 */
struct bt_gatt_read_params {
	struct bt_att_req _req;
	bt_gatt_read_func_t func;
	size_t handle_count;
	union {
		struct {
			u16_t handle;
			u16_t offset;
		} single;
		u16_t *handles;
	};
};

/** @brief Read Attribute Value by handle
 *
 *  This procedure read the attribute value and return it to the callback.
 *
 *  Note: This procedure is asynchronous therefore the parameters need to
 *  remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Read parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_read(struct bt_conn *conn, struct bt_gatt_read_params *params);

struct bt_gatt_write_params;

/** @typedef bt_gatt_write_func_t
 *  @brief Write callback function
 *
 *  @param conn Connection object.
 *  @param err ATT error code.
 *  @param params Write parameters used.
 */
typedef void (*bt_gatt_write_func_t)(struct bt_conn *conn, u8_t err,
				     struct bt_gatt_write_params *params);

/** @brief GATT Write parameters */
struct bt_gatt_write_params {
	struct bt_att_req _req;
	/** Response callback */
	bt_gatt_write_func_t func;
	/** Attribute handle */
	u16_t handle;
	/** Attribute data offset */
	u16_t offset;
	/** Data to be written */
	const void *data;
	/** Length of the data */
	u16_t length;
};

/** @brief Write Attribute Value by handle
 *
 * This procedure write the attribute value and return the result in the
 * callback.
 *
 * Note: This procedure is asynchronous therefore the parameters need to
 *  remains valid while it is active.
 *
 * @param conn Connection object.
 * @param params Write parameters.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_write(struct bt_conn *conn, struct bt_gatt_write_params *params);

/** @brief Write Attribute Value by handle without response
 *
 * This procedure write the attribute value without requiring an
 * acknowledgment that the write was successfully performed
 *
 * @param conn Connection object.
 * @param handle Attribute handle.
 * @param data Data to be written.
 * @param length Data length.
 * @param sign Whether to sign data
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_write_without_response(struct bt_conn *conn, u16_t handle,
				   const void *data, u16_t length,
				   bool sign);

struct bt_gatt_subscribe_params;

/** @typedef bt_gatt_notify_func_t
 *  @brief Notification callback function
 *
 *  @param conn Connection object.
 *  @param params Subscription parameters.
 *  @param data Attribute value data. If NULL then subscription was removed.
 *  @param length Attribute value length.
 */
typedef u8_t (*bt_gatt_notify_func_t)(struct bt_conn *conn,
				      struct bt_gatt_subscribe_params *params,
				      const void *data, u16_t length);

/* Subscription flags */
enum {
	/** Persistence flag
	 *
	 * If set, indicates that the subscription is not saved
	 * on the GATT server side. Therefore, upon disconnection,
	 * the subscription will be automatically removed
	 * from the client's subscriptions list and
	 * when the client reconnects, it will have to
	 * issue a new subscription.
	 */
	BT_GATT_SUBSCRIBE_FLAG_VOLATILE = BIT(0),
};

/** @brief GATT Subscribe parameters */
struct bt_gatt_subscribe_params {
	struct bt_att_req _req;
	bt_addr_le_t _peer;
	/** Notification value callback */
	bt_gatt_notify_func_t notify;
	/** Subscribe value handle */
	u16_t value_handle;
	/** Subscribe CCC handle */
	u16_t ccc_handle;
	/** Subscribe value */
	u16_t value;
	/** Subscription flags */
	u8_t flags;
	sys_snode_t node;
};

/** @brief Subscribe Attribute Value Notification
 *
 *  This procedure subscribe to value notification using the Client
 *  Characteristic Configuration handle.
 *  If notification received subscribe value callback is called to return
 *  notified value. One may then decide whether to unsubscribe directly from
 *  this callback. Notification callback with NULL data will not be called if
 *  subscription was removed by this method.
 *
 *  Note: This procedure is asynchronous therefore the parameters need to
 *  remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Subscribe parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_subscribe(struct bt_conn *conn,
		      struct bt_gatt_subscribe_params *params);

/** @brief Unsubscribe Attribute Value Notification
 *
 * This procedure unsubscribe to value notification using the Client
 * Characteristic Configuration handle. Notification callback with NULL data
 * will be called if subscription was removed by this call, until then the
 * parameters cannot be reused.
 *
 * @param conn Connection object.
 * @param params Subscribe parameters.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_gatt_unsubscribe(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params);

/** @brief Cancel GATT pending request
 *
 *  @param conn Connection object.
 *  @param params Requested params address.
 */
void bt_gatt_cancel(struct bt_conn *conn, void *params);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_GATT_H_ */
