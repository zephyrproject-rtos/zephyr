/**
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @brief GATT Macros
 *
 *  This code is auto-generated from the Excel Workbook 'GATT_Qualification_Test_Databases.xlsm' Sheet: 'Large Database 1'
 *  Feel free to change it - but be aware that your changes might be overwritten at the next generation...
 */

#ifndef GATT_MACS_H
#define GATT_MACS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt.h>

/** @def BT_GATT_ATTRIBUTE
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
#undef  BT_GATT_ATTRIBUTE
#define BT_GATT_ATTRIBUTE(_uuid, _perm, _read, _write, _value, _handle) \
{ \
    .uuid = _uuid, \
    .perm = _perm, \
    .read = _read, \
    .write = _write, \
    .user_data = _value, \
    .handle = _handle \
}

/** @def BT_GATT_CHARACTERISTIC
 *  @brief Characteristic and Value Declaration Macro.
 *
 *  Helper macro to declare a characteristic attribute along with its attribute value.
 *
 *  @param _uuid   Characteristic attribute uuid.
 *  @param _props  Characteristic attribute properties.
 *  @param _perm   Characteristic attribute access permissions.
 *  @param _read   Characteristic attribute read callback.
 *  @param _write  Characteristic attribute write callback.
 *  @param _value  Characteristic attribute value.
 *  @param _handle Characteristic attribute handle.
 */
#undef  BT_GATT_CHARACTERISTIC
#define BT_GATT_CHARACTERISTIC(_uuid, _props, _perm, _read, _write, _value, _handle) \
    BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC, \
        BT_GATT_PERM_READ, \
        bt_gatt_attr_read_chrc, \
        NULL, \
        (&(struct bt_gatt_chrc) { \
            .uuid = _uuid, \
            .properties = _props \
        } ), \
        _handle), \
    BT_GATT_ATTRIBUTE(_uuid, _perm, _read, _write, _value, _handle+1)

/** @def BT_GATT_PRIMARY_SERVICE
 *  @brief Primary Service Declaration Macro.
 *
 *  Helper macro to declare a primary service attribute.
 *
 *  @param _service Service attribute value.
 *  @param _handle  Service attribute handle.
 */
#undef  BT_GATT_PRIMARY_SERVICE
#define BT_GATT_PRIMARY_SERVICE(_service, _handle) \
    BT_GATT_ATTRIBUTE(BT_UUID_GATT_PRIMARY, \
        BT_GATT_PERM_READ, \
        bt_gatt_attr_read_service, \
        NULL, \
        _service, \
        _handle)

/** @def BT_GATT_SECONDARY_SERVICE
 *  @brief Secondary Service Declaration Macro.
 *
 *  Helper macro to declare a secondary service attribute.
 *
 *  @param _service Service attribute value.
 *  @param _handle  Service attribute handle.
 */
#undef  BT_GATT_SECONDARY_SERVICE
#define BT_GATT_SECONDARY_SERVICE(_service, _handle) \
    BT_GATT_ATTRIBUTE(BT_UUID_GATT_SECONDARY, \
        BT_GATT_PERM_READ, \
        bt_gatt_attr_read_service, \
        NULL, \
        _service, \
        _handle)

/** @def BT_GATT_INCLUDE_SERVICE
 *  @brief Include Service Declaration Macro.
 *
 *  Helper macro to declare database internal include service attribute.
 *
 *  @param _service_incl The first service attribute of service to include.
 *  @param _handle       Service attribute handle.
 */
#undef  BT_GATT_INCLUDE_SERVICE
#define BT_GATT_INCLUDE_SERVICE(_service_incl, _handle) \
    BT_GATT_ATTRIBUTE(BT_UUID_GATT_INCLUDE, \
        BT_GATT_PERM_READ, \
        bt_gatt_attr_read_included, \
        NULL, \
        _service_incl, \
        _handle)

/** @def BT_GATT_DESCRIPTOR
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
#undef  BT_GATT_DESCRIPTOR
#define BT_GATT_DESCRIPTOR(_uuid, _perm, _read, _write, _value, _handle) \
    BT_GATT_ATTRIBUTE(_uuid, _perm, _read, _write, _value, _handle)

/** @def BT_GATT_CCC
 *  @brief Client Characteristic Configuration Change Declaration Macro.
 *
 *  Helper macro to declare a CCC attribute.
 *
 *  @param _cfg         Initial configuration.
 *  @param _cfg_changed Configuration changed callback.
 *  @param _handle      Descriptor attribute handle.
 */
#undef  BT_GATT_CCC
#define BT_GATT_CCC(_cfg, _cfg_changed, _handle) \
    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CCC, \
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, \
        bt_gatt_attr_read_ccc, \
        bt_gatt_attr_write_ccc, \
        (&(struct _bt_gatt_ccc) { \
            .cfg = _cfg, \
            .cfg_len = ARRAY_SIZE(_cfg), \
            .cfg_changed = _cfg_changed \
        } ), \
        _handle)

/** @def BT_GATT_CEP
 *  @brief Characteristic Extended Properties Declaration Macro.
 *
 *  Helper macro to declare a CEP attribute.
 *
 *  @param _value  Descriptor attribute value.
 *  @param _handle Descriptor attribute handle.
 */
#undef  BT_GATT_CEP
#define BT_GATT_CEP(_value, _handle) \
    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CEP, \
        BT_GATT_PERM_READ, \
        bt_gatt_attr_read_cep, \
        NULL, \
        (void *)_value, \
        _handle)

/** @def BT_GATT_CUD
 *  @brief Characteristic User Format Descriptor Declaration Macro.
 *
 *  Helper macro to declare a CUD attribute.
 *
 *  @param _value  User description NULL-terminated C string.
 *  @param _perm   Descriptor attribute access permissions.
 *  @param _handle Descriptor attribute handle.
 */
#undef  BT_GATT_CUD
#define BT_GATT_CUD(_value, _perm, _handle) \
    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CUD, \
        _perm, \
        bt_gatt_attr_read_cud, \
        NULL, \
        (void *)_value, \
        _handle)

/** @def BT_GATT_CPF
 *  @brief Characteristic Presentation Format Descriptor Declaration Macro.
 *
 *  Helper macro to declare a CPF attribute.
 *
 *  @param _value  Descriptor attribute value.
 *  @param _handle Descriptor attribute handle.
 */
#undef  BT_GATT_CPF
#define BT_GATT_CPF(_value, _handle) \
    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CPF, \
        BT_GATT_PERM_READ, \
        bt_gatt_attr_read_cpf, \
        NULL, \
        (void *)_value, \
        _handle)

#ifdef __cplusplus
}
#endif

#endif /* GATT_MACS_H */
