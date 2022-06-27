/** @file
 * @brief Health faults
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_FAULTS_H__
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_FAULTS_H__

/**
 * @brief List of specification defined Health fault values.
 * @defgroup bt_mesh_health_faults Health faults
 * @ingroup bt_mesh
 * @{
 */

/** No fault has occurred. */
#define BT_MESH_HEALTH_FAULT_NO_FAULT                           0x00

#define BT_MESH_HEALTH_FAULT_BATTERY_LOW_WARNING                0x01
#define BT_MESH_HEALTH_FAULT_BATTERY_LOW_ERROR                  0x02
#define BT_MESH_HEALTH_FAULT_SUPPLY_VOLTAGE_TOO_LOW_WARNING     0x03
#define BT_MESH_HEALTH_FAULT_SUPPLY_VOLTAGE_TOO_LOW_ERROR       0x04
#define BT_MESH_HEALTH_FAULT_SUPPLY_VOLTAGE_TOO_HIGH_WARNING    0x05
#define BT_MESH_HEALTH_FAULT_SUPPLY_VOLTAGE_TOO_HIGH_ERROR      0x06
#define BT_MESH_HEALTH_FAULT_POWER_SUPPLY_INTERRUPTED_WARNING   0x07
#define BT_MESH_HEALTH_FAULT_POWER_SUPPLY_INTERRUPTED_ERROR     0x08
#define BT_MESH_HEALTH_FAULT_NO_LOAD_WARNING                    0x09
#define BT_MESH_HEALTH_FAULT_NO_LOAD_ERROR                      0x0A
#define BT_MESH_HEALTH_FAULT_OVERLOAD_WARNING                   0x0B
#define BT_MESH_HEALTH_FAULT_OVERLOAD_ERROR                     0x0C
#define BT_MESH_HEALTH_FAULT_OVERHEAT_WARNING                   0x0D
#define BT_MESH_HEALTH_FAULT_OVERHEAT_ERROR                     0x0E
#define BT_MESH_HEALTH_FAULT_CONDENSATION_WARNING               0x0F
#define BT_MESH_HEALTH_FAULT_CONDENSATION_ERROR                 0x10
#define BT_MESH_HEALTH_FAULT_VIBRATION_WARNING                  0x11
#define BT_MESH_HEALTH_FAULT_VIBRATION_ERROR                    0x12
#define BT_MESH_HEALTH_FAULT_CONFIGURATION_WARNING              0x13
#define BT_MESH_HEALTH_FAULT_CONFIGURATION_ERROR                0x14
#define BT_MESH_HEALTH_FAULT_ELEMENT_NOT_CALIBRATED_WARNING     0x15
#define BT_MESH_HEALTH_FAULT_ELEMENT_NOT_CALIBRATED_ERROR       0x16
#define BT_MESH_HEALTH_FAULT_MEMORY_WARNING                     0x17
#define BT_MESH_HEALTH_FAULT_MEMORY_ERROR                       0x18
#define BT_MESH_HEALTH_FAULT_SELF_TEST_WARNING                  0x19
#define BT_MESH_HEALTH_FAULT_SELF_TEST_ERROR                    0x1A
#define BT_MESH_HEALTH_FAULT_INPUT_TOO_LOW_WARNING              0x1B
#define BT_MESH_HEALTH_FAULT_INPUT_TOO_LOW_ERROR                0x1C
#define BT_MESH_HEALTH_FAULT_INPUT_TOO_HIGH_WARNING             0x1D
#define BT_MESH_HEALTH_FAULT_INPUT_TOO_HIGH_ERROR               0x1E
#define BT_MESH_HEALTH_FAULT_INPUT_NO_CHANGE_WARNING            0x1F
#define BT_MESH_HEALTH_FAULT_INPUT_NO_CHANGE_ERROR              0x20
#define BT_MESH_HEALTH_FAULT_ACTUATOR_BLOCKED_WARNING           0x21
#define BT_MESH_HEALTH_FAULT_ACTUATOR_BLOCKED_ERROR             0x22
#define BT_MESH_HEALTH_FAULT_HOUSING_OPENED_WARNING             0x23
#define BT_MESH_HEALTH_FAULT_HOUSING_OPENED_ERROR               0x24
#define BT_MESH_HEALTH_FAULT_TAMPER_WARNING                     0x25
#define BT_MESH_HEALTH_FAULT_TAMPER_ERROR                       0x26
#define BT_MESH_HEALTH_FAULT_DEVICE_MOVED_WARNING               0x27
#define BT_MESH_HEALTH_FAULT_DEVICE_MOVED_ERROR                 0x28
#define BT_MESH_HEALTH_FAULT_DEVICE_DROPPED_WARNING             0x29
#define BT_MESH_HEALTH_FAULT_DEVICE_DROPPED_ERROR               0x2A
#define BT_MESH_HEALTH_FAULT_OVERFLOW_WARNING                   0x2B
#define BT_MESH_HEALTH_FAULT_OVERFLOW_ERROR                     0x2C
#define BT_MESH_HEALTH_FAULT_EMPTY_WARNING                      0x2D
#define BT_MESH_HEALTH_FAULT_EMPTY_ERROR                        0x2E
#define BT_MESH_HEALTH_FAULT_INTERNAL_BUS_WARNING               0x2F
#define BT_MESH_HEALTH_FAULT_INTERNAL_BUS_ERROR                 0x30
#define BT_MESH_HEALTH_FAULT_MECHANISM_JAMMED_WARNING           0x31
#define BT_MESH_HEALTH_FAULT_MECHANISM_JAMMED_ERROR             0x32

/**
 * Start of the vendor specific fault values.
 *
 * All values below this are reserved for the Bluetooth Specification.
 */
#define BT_MESH_HEALTH_FAULT_VENDOR_SPECIFIC_START              0x80

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_HEALTH_FAULTS_H__ */
