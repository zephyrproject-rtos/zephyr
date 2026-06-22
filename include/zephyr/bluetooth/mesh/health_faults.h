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

/** Battery Low Warning */
#define BT_MESH_HEALTH_FAULT_BATTERY_LOW_WARNING                0x01
/** Battery Low Error */
#define BT_MESH_HEALTH_FAULT_BATTERY_LOW_ERROR                  0x02
/** Supply Voltage Too Low Warning */
#define BT_MESH_HEALTH_FAULT_SUPPLY_VOLTAGE_TOO_LOW_WARNING     0x03
/** Supply Voltage Too Low Error */
#define BT_MESH_HEALTH_FAULT_SUPPLY_VOLTAGE_TOO_LOW_ERROR       0x04
/** Supply Voltage Too High Warning */
#define BT_MESH_HEALTH_FAULT_SUPPLY_VOLTAGE_TOO_HIGH_WARNING    0x05
/** Supply Voltage Too High Error */
#define BT_MESH_HEALTH_FAULT_SUPPLY_VOLTAGE_TOO_HIGH_ERROR      0x06
/** Power Supply Interrupted Warning */
#define BT_MESH_HEALTH_FAULT_POWER_SUPPLY_INTERRUPTED_WARNING   0x07
/** Power Supply Interrupted Error */
#define BT_MESH_HEALTH_FAULT_POWER_SUPPLY_INTERRUPTED_ERROR     0x08
/** No Load Warning */
#define BT_MESH_HEALTH_FAULT_NO_LOAD_WARNING                    0x09
/** No Load Error */
#define BT_MESH_HEALTH_FAULT_NO_LOAD_ERROR                      0x0A
/** Overload Warning */
#define BT_MESH_HEALTH_FAULT_OVERLOAD_WARNING                   0x0B
/** Overload Error */
#define BT_MESH_HEALTH_FAULT_OVERLOAD_ERROR                     0x0C
/** Overheat Warning */
#define BT_MESH_HEALTH_FAULT_OVERHEAT_WARNING                   0x0D
/** Overheat Error */
#define BT_MESH_HEALTH_FAULT_OVERHEAT_ERROR                     0x0E
/** Condensation Warning */
#define BT_MESH_HEALTH_FAULT_CONDENSATION_WARNING               0x0F
/** Condensation Error */
#define BT_MESH_HEALTH_FAULT_CONDENSATION_ERROR                 0x10
/** Vibration Warning */
#define BT_MESH_HEALTH_FAULT_VIBRATION_WARNING                  0x11
/** Vibration Error */
#define BT_MESH_HEALTH_FAULT_VIBRATION_ERROR                    0x12
/** Configuration Warning */
#define BT_MESH_HEALTH_FAULT_CONFIGURATION_WARNING              0x13
/** Configuration Error */
#define BT_MESH_HEALTH_FAULT_CONFIGURATION_ERROR                0x14
/** Element Not Calibrated Warning */
#define BT_MESH_HEALTH_FAULT_ELEMENT_NOT_CALIBRATED_WARNING     0x15
/** Element Not Calibrated Error */
#define BT_MESH_HEALTH_FAULT_ELEMENT_NOT_CALIBRATED_ERROR       0x16
/** Memory Warning */
#define BT_MESH_HEALTH_FAULT_MEMORY_WARNING                     0x17
/** Memory Error */
#define BT_MESH_HEALTH_FAULT_MEMORY_ERROR                       0x18
/** Self-Test Warning */
#define BT_MESH_HEALTH_FAULT_SELF_TEST_WARNING                  0x19
/** Self-Test Error */
#define BT_MESH_HEALTH_FAULT_SELF_TEST_ERROR                    0x1A
/** Input Too Low Warning */
#define BT_MESH_HEALTH_FAULT_INPUT_TOO_LOW_WARNING              0x1B
/** Input Too Low Error */
#define BT_MESH_HEALTH_FAULT_INPUT_TOO_LOW_ERROR                0x1C
/** Input Too High Warning */
#define BT_MESH_HEALTH_FAULT_INPUT_TOO_HIGH_WARNING             0x1D
/** Input Too High Error */
#define BT_MESH_HEALTH_FAULT_INPUT_TOO_HIGH_ERROR               0x1E
/** Input No Change Warning */
#define BT_MESH_HEALTH_FAULT_INPUT_NO_CHANGE_WARNING            0x1F
/** Input No Change Error */
#define BT_MESH_HEALTH_FAULT_INPUT_NO_CHANGE_ERROR              0x20
/** Actuator Blocked Warning */
#define BT_MESH_HEALTH_FAULT_ACTUATOR_BLOCKED_WARNING           0x21
/** Actuator Blocked Error */
#define BT_MESH_HEALTH_FAULT_ACTUATOR_BLOCKED_ERROR             0x22
/** Housing Opened Warning */
#define BT_MESH_HEALTH_FAULT_HOUSING_OPENED_WARNING             0x23
/** Housing Opened Error */
#define BT_MESH_HEALTH_FAULT_HOUSING_OPENED_ERROR               0x24
/** Tamper Warning */
#define BT_MESH_HEALTH_FAULT_TAMPER_WARNING                     0x25
/** Tamper Error */
#define BT_MESH_HEALTH_FAULT_TAMPER_ERROR                       0x26
/** Device Moved Warning */
#define BT_MESH_HEALTH_FAULT_DEVICE_MOVED_WARNING               0x27
/** Device Moved Error */
#define BT_MESH_HEALTH_FAULT_DEVICE_MOVED_ERROR                 0x28
/** Device Dropped Warning */
#define BT_MESH_HEALTH_FAULT_DEVICE_DROPPED_WARNING             0x29
/** Device Dropped Error */
#define BT_MESH_HEALTH_FAULT_DEVICE_DROPPED_ERROR               0x2A
/** Overflow Warning */
#define BT_MESH_HEALTH_FAULT_OVERFLOW_WARNING                   0x2B
/** Overflow Error */
#define BT_MESH_HEALTH_FAULT_OVERFLOW_ERROR                     0x2C
/** Empty Warning */
#define BT_MESH_HEALTH_FAULT_EMPTY_WARNING                      0x2D
/** Empty Error */
#define BT_MESH_HEALTH_FAULT_EMPTY_ERROR                        0x2E
/** Internal Bus Warning */
#define BT_MESH_HEALTH_FAULT_INTERNAL_BUS_WARNING               0x2F
/** Internal Bus Error */
#define BT_MESH_HEALTH_FAULT_INTERNAL_BUS_ERROR                 0x30
/** Mechanism Jammed Warning */
#define BT_MESH_HEALTH_FAULT_MECHANISM_JAMMED_WARNING           0x31
/** Mechanism Jammed Error */
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
