/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief GATT Macros
 *
 *  This code is auto-generated from the Excel Workbook
 *  'GATT_Qualification_Test_Databases.xlsm' Sheet: 'Large Database 1'
 *
 *  Feel free to change it - but be aware that your changes might be
 *  overwritten at the next generation...
 */

#ifndef GATT_MACS_H
#define GATT_MACS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/bluetooth/gatt.h>

/**
 *  @brief Attribute Declaration Macro.
 *
 *  Helper macro to declare an attribute.
 *
 *  @param _uuid   Attribute uuid.
 *  @param _perm   Attribute access permissions.
 *  @param _read   Attribute read callback.
 *  @param _write  Attribute write callback.
 *  @param _value  Attribute value.
 *  @param _handle Attribute handle.
 */
#define BT_GATT_H_ATTRIBUTE(_uuid, _perm, _read, _write, _value, _handle) \
	{ \
	.uuid = _uuid, \
	.perm = _perm, \
	.read = _read, \
	.write = _write, \
	.user_data = _value, \
	.handle = _handle \
	}

/**
 *  @brief Characteristic and Value Declaration Macro.
 *
 *  Helper macro to declare a characteristic attribute along with its attribute
 *  value.
 *
 *  @param _uuid   Characteristic attribute uuid.
 *  @param _props  Characteristic attribute properties.
 *  @param _perm   Characteristic attribute access permissions.
 *  @param _read   Characteristic attribute read callback.
 *  @param _write  Characteristic attribute write callback.
 *  @param _value  Characteristic attribute value.
 *  @param _handle Characteristic attribute handle.
 */
#define BT_GATT_H_CHARACTERISTIC(_uuid, _props, _perm, _read, _write, _value,\
			       _handle) \
	BT_GATT_H_ATTRIBUTE(BT_UUID_GATT_CHRC, \
			  BT_GATT_PERM_READ, \
			  bt_gatt_attr_read_chrc, \
			  NULL, \
			  (&(struct bt_gatt_chrc) {.uuid = _uuid, \
						   .properties = _props \
						  }), \
			  _handle), \
	BT_GATT_H_ATTRIBUTE(_uuid, _perm, _read, _write, _value, _handle + 1)

/**
 *  @brief Primary Service Declaration Macro.
 *
 *  Helper macro to declare a primary service attribute.
 *
 *  @param _service Service attribute value.
 *  @param _handle  Service attribute handle.
 */
#define BT_GATT_H_PRIMARY_SERVICE(_service, _handle) \
		BT_GATT_H_ATTRIBUTE(BT_UUID_GATT_PRIMARY, \
				BT_GATT_PERM_READ, \
				bt_gatt_attr_read_service, \
				NULL, \
				_service, \
				_handle)

/**
 *  @brief Secondary Service Declaration Macro.
 *
 *  Helper macro to declare a secondary service attribute.
 *
 *  @param _service Service attribute value.
 *  @param _handle  Service attribute handle.
 */
#define BT_GATT_H_SECONDARY_SERVICE(_service, _handle) \
		BT_GATT_H_ATTRIBUTE(BT_UUID_GATT_SECONDARY, \
				BT_GATT_PERM_READ, \
				bt_gatt_attr_read_service, \
				NULL, \
				_service, \
				_handle)

/**
 *  @brief Include Service Declaration Macro.
 *
 *  Helper macro to declare database internal include service attribute.
 *
 *  @param _service_incl The first service attribute of service to include.
 *  @param _handle       Service attribute handle.
 */
#define BT_GATT_H_INCLUDE_SERVICE(_service_incl, _handle) \
		BT_GATT_H_ATTRIBUTE(BT_UUID_GATT_INCLUDE, \
				BT_GATT_PERM_READ, \
				bt_gatt_attr_read_included, \
				NULL, \
				_service_incl, \
				_handle)

/**
 *  @brief Descriptor Declaration Macro.
 *
 *  Helper macro to declare a descriptor attribute.
 *
 *  @param _uuid   Descriptor attribute uuid.
 *  @param _perm   Descriptor attribute access permissions.
 *  @param _read   Descriptor attribute read callback.
 *  @param _write  Descriptor attribute write callback.
 *  @param _value  Descriptor attribute value.
 *  @param _handle Descriptor attribute handle.
 */
#define BT_GATT_H_DESCRIPTOR(_uuid, _perm, _read, _write, _value, _handle) \
	       BT_GATT_H_ATTRIBUTE(_uuid, _perm, _read, _write, _value, _handle)

/**
 *  @brief Managed Client Characteristic Configuration Declaration Macro.
 *
 *  Helper macro to declare a Managed CCC attribute.
 *
 *  @param _ccc    CCC attribute user data, shall point to a _bt_gatt_ccc.
 *  @param _perm   CCC access permissions.
 *  @param _handle Descriptor attribute handle.
 */
#define BT_GATT_H_MANAGED(_ccc, _perm, _handle) \
		BT_GATT_H_ATTRIBUTE(BT_UUID_GATT_CCC, _perm,\
		bt_gatt_attr_read_ccc, bt_gatt_attr_write_ccc, _ccc, _handle)

/**
 *  @brief Client Characteristic Configuration Change Declaration Macro.
 *
 *  Helper macro to declare a CCC attribute.
 *
 *  @param _cfg         Initial configuration.
 *  @param _cfg_changed Configuration changed callback.
 *  @param _handle      Descriptor attribute handle.
 */
/* #undef  BT_GATT_H_CCC
 *  #define BT_GATT_H_CCC(_cfg, _cfg_changed, _handle) \
 *     BT_GATT_H_DESCRIPTOR(BT_UUID_GATT_CCC, \
 *         BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, \
 *         bt_gatt_attr_read_ccc, \
 *         bt_gatt_attr_write_ccc, \
 *         (&(struct _bt_gatt_ccc) { \
 *             .cfg = _cfg, \
 *             .cfg_len = ARRAY_SIZE(_cfg), \
 *             .cfg_changed = _cfg_changed \
 *         } ), \
 *         _handle)
 */
/**
 *  @brief Client Characteristic Configuration Change Declaration Macro.
 *
 *  Helper macro to declare a CCC attribute.
 *
 *  @param _cfg         Initial configuration.
 *  @param _cfg_changed Configuration changed callback.
 *  @param _handle      Descriptor attribute handle.
 */
#define BT_GATT_H_CCC(_cfg, _cfg_changed, _handle) \
		BT_GATT_H_MANAGED((&(struct _bt_gatt_ccc) \
			BT_GATT_CCC_INITIALIZER(_cfg_changed, NULL, NULL)),\
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, _handle)

/**
 *  @brief Characteristic Extended Properties Declaration Macro.
 *
 *  Helper macro to declare a CEP attribute.
 *
 *  @param _value  Descriptor attribute value.
 *  @param _handle Descriptor attribute handle.
 */
#define BT_GATT_H_CEP(_value, _handle) \
		BT_GATT_H_DESCRIPTOR(BT_UUID_GATT_CEP, \
				BT_GATT_PERM_READ, \
				bt_gatt_attr_read_cep, \
				NULL, \
				(void *)_value, \
				_handle)

/**
 *  @brief Characteristic User Format Descriptor Declaration Macro.
 *
 *  Helper macro to declare a CUD attribute.
 *
 *  @param _value  User description NULL-terminated C string.
 *  @param _perm   Descriptor attribute access permissions.
 *  @param _handle Descriptor attribute handle.
 */
#define BT_GATT_H_CUD(_value, _perm, _handle) \
		BT_GATT_H_DESCRIPTOR(BT_UUID_GATT_CUD, \
				_perm, \
				bt_gatt_attr_read_cud, \
				NULL, \
				(void *)_value, \
				_handle)

/**
 *  @brief Characteristic Presentation Format Descriptor Declaration Macro.
 *
 *  Helper macro to declare a CPF attribute.
 *
 *  @param _value  Descriptor attribute value.
 *  @param _handle Descriptor attribute handle.
 */
#define BT_GATT_H_CPF(_value, _handle) \
		BT_GATT_H_DESCRIPTOR(BT_UUID_GATT_CPF, \
				BT_GATT_PERM_READ, \
				bt_gatt_attr_read_cpf, \
				NULL, \
				(void *)_value, \
				_handle)

#ifdef __cplusplus
}
#endif

#endif /* GATT_MACS_H */
