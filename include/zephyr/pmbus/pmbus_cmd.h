/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup pmbus
 * @brief PMBus command codes and status bits, from PMBus 1.3.1 Part II.
 */

#ifndef ZEPHYR_INCLUDE_PMBUS_PMBUS_CMD_H_
#define ZEPHYR_INCLUDE_PMBUS_PMBUS_CMD_H_

#include <zephyr/sys/util_macro.h>

/**
 * @addtogroup pmbus
 * @{
 */

/**
 * @name PMBus command codes
 * @{
 */
#define PMBUS_PAGE                      0x00 /**< Select the page of the next commands */
#define PMBUS_OPERATION                 0x01 /**< Turn the output on or off and set margining */
#define PMBUS_ON_OFF_CONFIG             0x02 /**< Configure how the output is turned on and off */
#define PMBUS_CLEAR_FAULTS              0x03 /**< Clear all latched faults and warnings */
#define PMBUS_PHASE                     0x04 /**< Select the phase of the next commands */
#define PMBUS_PAGE_PLUS_WRITE           0x05 /**< Write a command to a page in one transaction */
#define PMBUS_PAGE_PLUS_READ            0x06 /**< Read a command from a page in one transaction */
#define PMBUS_ZONE_CONFIG               0x07 /**< Set the zones the device belongs to */
#define PMBUS_ZONE_ACTIVE               0x08 /**< Set the active zones for zone commands */
#define PMBUS_WRITE_PROTECT             0x10 /**< Protect the device against writes */
#define PMBUS_STORE_DEFAULT_ALL         0x11 /**< Store all settings to the default store */
#define PMBUS_RESTORE_DEFAULT_ALL       0x12 /**< Restore all settings from the default store */
#define PMBUS_STORE_DEFAULT_CODE        0x13 /**< Store one setting to the default store */
#define PMBUS_RESTORE_DEFAULT_CODE      0x14 /**< Restore one setting from the default store */
#define PMBUS_STORE_USER_ALL            0x15 /**< Store all settings to the user store */
#define PMBUS_RESTORE_USER_ALL          0x16 /**< Restore all settings from the user store */
#define PMBUS_STORE_USER_CODE           0x17 /**< Store one setting to the user store */
#define PMBUS_RESTORE_USER_CODE         0x18 /**< Restore one setting from the user store */
#define PMBUS_CAPABILITY                0x19 /**< PMBus features supported by the device */
#define PMBUS_QUERY                     0x1A /**< Query the support and format of a command */
#define PMBUS_SMBALERT_MASK             0x1B /**< Mask status bits from asserting SMBALERT# */
#define PMBUS_VOUT_MODE                 0x20 /**< Output voltage data format and exponent */
#define PMBUS_VOUT_COMMAND              0x21 /**< Output voltage set point */
#define PMBUS_VOUT_TRIM                 0x22 /**< Output voltage trim offset */
#define PMBUS_VOUT_CAL_OFFSET           0x23 /**< Output voltage calibration offset */
#define PMBUS_VOUT_MAX                  0x24 /**< Maximum output voltage that can be set */
#define PMBUS_VOUT_MARGIN_HIGH          0x25 /**< Output voltage for margin high */
#define PMBUS_VOUT_MARGIN_LOW           0x26 /**< Output voltage for margin low */
#define PMBUS_VOUT_TRANSITION_RATE      0x27 /**< Rate of output voltage changes */
#define PMBUS_VOUT_DROOP                0x28 /**< Output voltage drop per output current */
#define PMBUS_VOUT_SCALE_LOOP           0x29 /**< Output voltage scale of the control loop */
#define PMBUS_VOUT_SCALE_MONITOR        0x2A /**< Output voltage scale of the monitor */
#define PMBUS_VOUT_MIN                  0x2B /**< Minimum output voltage that can be set */
#define PMBUS_COEFFICIENTS              0x30 /**< DIRECT format coefficients of a command */
#define PMBUS_POUT_MAX                  0x31 /**< Maximum output power */
#define PMBUS_MAX_DUTY                  0x32 /**< Maximum duty cycle */
#define PMBUS_FREQUENCY_SWITCH          0x33 /**< Switching frequency */
#define PMBUS_POWER_MODE                0x34 /**< Power mode of the device */
#define PMBUS_VIN_ON                    0x35 /**< Input voltage to start power conversion */
#define PMBUS_VIN_OFF                   0x36 /**< Input voltage to stop power conversion */
#define PMBUS_INTERLEAVE                0x37 /**< Phase offset between interleaved devices */
#define PMBUS_IOUT_CAL_GAIN             0x38 /**< Output current sense gain */
#define PMBUS_IOUT_CAL_OFFSET           0x39 /**< Output current sense offset */
#define PMBUS_FAN_CONFIG_1_2            0x3A /**< Configuration of fans 1 and 2 */
#define PMBUS_FAN_COMMAND_1             0x3B /**< Speed command of fan 1 */
#define PMBUS_FAN_COMMAND_2             0x3C /**< Speed command of fan 2 */
#define PMBUS_FAN_CONFIG_3_4            0x3D /**< Configuration of fans 3 and 4 */
#define PMBUS_FAN_COMMAND_3             0x3E /**< Speed command of fan 3 */
#define PMBUS_FAN_COMMAND_4             0x3F /**< Speed command of fan 4 */
#define PMBUS_VOUT_OV_FAULT_LIMIT       0x40 /**< Output overvoltage fault limit */
#define PMBUS_VOUT_OV_FAULT_RESPONSE    0x41 /**< Response to an output overvoltage fault */
#define PMBUS_VOUT_OV_WARN_LIMIT        0x42 /**< Output overvoltage warning limit */
#define PMBUS_VOUT_UV_WARN_LIMIT        0x43 /**< Output undervoltage warning limit */
#define PMBUS_VOUT_UV_FAULT_LIMIT       0x44 /**< Output undervoltage fault limit */
#define PMBUS_VOUT_UV_FAULT_RESPONSE    0x45 /**< Response to an output undervoltage fault */
#define PMBUS_IOUT_OC_FAULT_LIMIT       0x46 /**< Output overcurrent fault limit */
#define PMBUS_IOUT_OC_FAULT_RESPONSE    0x47 /**< Response to an output overcurrent fault */
#define PMBUS_IOUT_OC_LV_FAULT_LIMIT    0x48 /**< Output voltage limit in overcurrent */
#define PMBUS_IOUT_OC_LV_FAULT_RESPONSE 0x49 /**< Response to a low voltage in overcurrent */
#define PMBUS_IOUT_OC_WARN_LIMIT        0x4A /**< Output overcurrent warning limit */
#define PMBUS_IOUT_UC_FAULT_LIMIT       0x4B /**< Output undercurrent fault limit */
#define PMBUS_IOUT_UC_FAULT_RESPONSE    0x4C /**< Response to an output undercurrent fault */
#define PMBUS_OT_FAULT_LIMIT            0x4F /**< Overtemperature fault limit */
#define PMBUS_OT_FAULT_RESPONSE         0x50 /**< Response to an overtemperature fault */
#define PMBUS_OT_WARN_LIMIT             0x51 /**< Overtemperature warning limit */
#define PMBUS_UT_WARN_LIMIT             0x52 /**< Undertemperature warning limit */
#define PMBUS_UT_FAULT_LIMIT            0x53 /**< Undertemperature fault limit */
#define PMBUS_UT_FAULT_RESPONSE         0x54 /**< Response to an undertemperature fault */
#define PMBUS_VIN_OV_FAULT_LIMIT        0x55 /**< Input overvoltage fault limit */
#define PMBUS_VIN_OV_FAULT_RESPONSE     0x56 /**< Response to an input overvoltage fault */
#define PMBUS_VIN_OV_WARN_LIMIT         0x57 /**< Input overvoltage warning limit */
#define PMBUS_VIN_UV_WARN_LIMIT         0x58 /**< Input undervoltage warning limit */
#define PMBUS_VIN_UV_FAULT_LIMIT        0x59 /**< Input undervoltage fault limit */
#define PMBUS_VIN_UV_FAULT_RESPONSE     0x5A /**< Response to an input undervoltage fault */
#define PMBUS_IIN_OC_FAULT_LIMIT        0x5B /**< Input overcurrent fault limit */
#define PMBUS_IIN_OC_FAULT_RESPONSE     0x5C /**< Response to an input overcurrent fault */
#define PMBUS_IIN_OC_WARN_LIMIT         0x5D /**< Input overcurrent warning limit */
#define PMBUS_POWER_GOOD_ON             0x5E /**< Output voltage to assert power good */
#define PMBUS_POWER_GOOD_OFF            0x5F /**< Output voltage to deassert power good */
#define PMBUS_TON_DELAY                 0x60 /**< Delay from turn on to output rise */
#define PMBUS_TON_RISE                  0x61 /**< Output rise time at turn on */
#define PMBUS_TON_MAX_FAULT_LIMIT       0x62 /**< Maximum output rise time at turn on */
#define PMBUS_TON_MAX_FAULT_RESPONSE    0x63 /**< Response to a turn on time fault */
#define PMBUS_TOFF_DELAY                0x64 /**< Delay from turn off to output fall */
#define PMBUS_TOFF_FALL                 0x65 /**< Output fall time at turn off */
#define PMBUS_TOFF_MAX_WARN_LIMIT       0x66 /**< Maximum output fall time at turn off */
#define PMBUS_POUT_OP_FAULT_LIMIT       0x68 /**< Output overpower fault limit */
#define PMBUS_POUT_OP_FAULT_RESPONSE    0x69 /**< Response to an output overpower fault */
#define PMBUS_POUT_OP_WARN_LIMIT        0x6A /**< Output overpower warning limit */
#define PMBUS_PIN_OP_WARN_LIMIT         0x6B /**< Input overpower warning limit */
#define PMBUS_STATUS_BYTE               0x78 /**< Summary status byte */
#define PMBUS_STATUS_WORD               0x79 /**< Summary status word */
#define PMBUS_STATUS_VOUT               0x7A /**< Output voltage status */
#define PMBUS_STATUS_IOUT               0x7B /**< Output current status */
#define PMBUS_STATUS_INPUT              0x7C /**< Input status */
#define PMBUS_STATUS_TEMPERATURE        0x7D /**< Temperature status */
#define PMBUS_STATUS_CML                0x7E /**< Communication, memory and logic status */
#define PMBUS_STATUS_OTHER              0x7F /**< Other status */
#define PMBUS_STATUS_MFR_SPECIFIC       0x80 /**< Manufacturer specific status */
#define PMBUS_STATUS_FANS_1_2           0x81 /**< Status of fans 1 and 2 */
#define PMBUS_STATUS_FANS_3_4           0x82 /**< Status of fans 3 and 4 */
#define PMBUS_READ_KWH_IN               0x83 /**< Input energy in kWh */
#define PMBUS_READ_KWH_OUT              0x84 /**< Output energy in kWh */
#define PMBUS_READ_KWH_CONFIG           0x85 /**< Configuration of the kWh counters */
#define PMBUS_READ_EIN                  0x86 /**< Input energy accumulator */
#define PMBUS_READ_EOUT                 0x87 /**< Output energy accumulator */
#define PMBUS_READ_VIN                  0x88 /**< Input voltage */
#define PMBUS_READ_IIN                  0x89 /**< Input current */
#define PMBUS_READ_VCAP                 0x8A /**< Energy storage capacitor voltage */
#define PMBUS_READ_VOUT                 0x8B /**< Output voltage */
#define PMBUS_READ_IOUT                 0x8C /**< Output current */
#define PMBUS_READ_TEMPERATURE_1        0x8D /**< Temperature sensor 1 */
#define PMBUS_READ_TEMPERATURE_2        0x8E /**< Temperature sensor 2 */
#define PMBUS_READ_TEMPERATURE_3        0x8F /**< Temperature sensor 3 */
#define PMBUS_READ_FAN_SPEED_1          0x90 /**< Speed of fan 1 */
#define PMBUS_READ_FAN_SPEED_2          0x91 /**< Speed of fan 2 */
#define PMBUS_READ_FAN_SPEED_3          0x92 /**< Speed of fan 3 */
#define PMBUS_READ_FAN_SPEED_4          0x93 /**< Speed of fan 4 */
#define PMBUS_READ_DUTY_CYCLE           0x94 /**< Duty cycle */
#define PMBUS_READ_FREQUENCY            0x95 /**< Switching frequency */
#define PMBUS_READ_POUT                 0x96 /**< Output power */
#define PMBUS_READ_PIN                  0x97 /**< Input power */
#define PMBUS_REVISION                  0x98 /**< PMBus revision of the device */
#define PMBUS_MFR_ID                    0x99 /**< Manufacturer name */
#define PMBUS_MFR_MODEL                 0x9A /**< Manufacturer model number */
#define PMBUS_MFR_REVISION              0x9B /**< Manufacturer revision */
#define PMBUS_MFR_LOCATION              0x9C /**< Manufacturing location */
#define PMBUS_MFR_DATE                  0x9D /**< Manufacturing date */
#define PMBUS_MFR_SERIAL                0x9E /**< Serial number */
#define PMBUS_APP_PROFILE_SUPPORT       0x9F /**< Supported application profiles */
#define PMBUS_MFR_VIN_MIN               0xA0 /**< Rated minimum input voltage */
#define PMBUS_MFR_VIN_MAX               0xA1 /**< Rated maximum input voltage */
#define PMBUS_MFR_IIN_MAX               0xA2 /**< Rated maximum input current */
#define PMBUS_MFR_PIN_MAX               0xA3 /**< Rated maximum input power */
#define PMBUS_MFR_VOUT_MIN              0xA4 /**< Rated minimum output voltage */
#define PMBUS_MFR_VOUT_MAX              0xA5 /**< Rated maximum output voltage */
#define PMBUS_MFR_IOUT_MAX              0xA6 /**< Rated maximum output current */
#define PMBUS_MFR_POUT_MAX              0xA7 /**< Rated maximum output power */
#define PMBUS_MFR_TAMBIENT_MAX          0xA8 /**< Rated maximum ambient temperature */
#define PMBUS_MFR_TAMBIENT_MIN          0xA9 /**< Rated minimum ambient temperature */
#define PMBUS_MFR_EFFICIENCY_LL         0xAA /**< Efficiency data at low line input */
#define PMBUS_MFR_EFFICIENCY_HL         0xAB /**< Efficiency data at high line input */
#define PMBUS_MFR_PIN_ACCURACY          0xAC /**< Accuracy of READ_PIN */
#define PMBUS_IC_DEVICE_ID              0xAD /**< Identifier of the controller IC */
#define PMBUS_IC_DEVICE_REV             0xAE /**< Revision of the controller IC */
#define PMBUS_USER_DATA_00              0xB0 /**< First user data command */
#define PMBUS_MFR_MAX_TEMP_1            0xC0 /**< Rated maximum of temperature 1 */
#define PMBUS_MFR_MAX_TEMP_2            0xC1 /**< Rated maximum of temperature 2 */
#define PMBUS_MFR_MAX_TEMP_3            0xC2 /**< Rated maximum of temperature 3 */
#define PMBUS_MFR_SPECIFIC_COMMAND_EXT  0xFE /**< Prefix of manufacturer extended commands */
#define PMBUS_COMMAND_EXT               0xFF /**< Prefix of PMBus extended commands */
/** @} */

/** PAGE value that selects all pages, valid only for writes */
#define PMBUS_PAGE_ALL 0xFF

/**
 * @name CAPABILITY bits
 * @{
 */
#define PMBUS_CAPABILITY_PEC        BIT(7)        /**< Packet Error Checking supported */
#define PMBUS_CAPABILITY_SPEED_MASK GENMASK(6, 5) /**< Maximum bus speed */
#define PMBUS_CAPABILITY_SMBALERT   BIT(4)        /**< SMBALERT# supported */
#define PMBUS_CAPABILITY_IEEE_HALF  BIT(3)        /**< Numeric format is IEEE 754 half precision */
#define PMBUS_CAPABILITY_AVSBUS     BIT(2)        /**< AVSBus supported */
/** @} */

/**
 * @name VOUT_MODE fields
 * @{
 */
#define PMBUS_VOUT_MODE_RELATIVE   BIT(7)        /**< Output voltage commands are relative */
#define PMBUS_VOUT_MODE_MODE_MASK  GENMASK(6, 5) /**< Data format of the output voltage */
#define PMBUS_VOUT_MODE_PARAM_MASK GENMASK(4, 0) /**< Parameter of the data format */
#define PMBUS_VOUT_MODE_ULINEAR16  0             /**< ULINEAR16 format, parameter is the exponent */
#define PMBUS_VOUT_MODE_VID        1             /**< VID format, parameter is the VID code type */
#define PMBUS_VOUT_MODE_DIRECT     2             /**< DIRECT format */
#define PMBUS_VOUT_MODE_IEEE_HALF  3             /**< IEEE 754 half precision format */
/** @} */

/**
 * @name STATUS_BYTE bits, also the low byte of STATUS_WORD
 * @{
 */
#define PMBUS_STATUS_BYTE_BUSY              BIT(7) /**< Device busy and unable to respond */
#define PMBUS_STATUS_BYTE_OFF               BIT(6) /**< Output is off */
#define PMBUS_STATUS_BYTE_VOUT_OV_FAULT     BIT(5) /**< Output overvoltage fault */
#define PMBUS_STATUS_BYTE_IOUT_OC_FAULT     BIT(4) /**< Output overcurrent fault */
#define PMBUS_STATUS_BYTE_VIN_UV_FAULT      BIT(3) /**< Input undervoltage fault */
#define PMBUS_STATUS_BYTE_TEMPERATURE       BIT(2) /**< Temperature fault or warning */
#define PMBUS_STATUS_BYTE_CML               BIT(1) /**< Communication, memory or logic fault */
#define PMBUS_STATUS_BYTE_NONE_OF_THE_ABOVE BIT(0) /**< Fault or warning not listed above */
/** @} */

/**
 * @name STATUS_WORD high byte bits
 * @{
 */
#define PMBUS_STATUS_WORD_VOUT      BIT(15) /**< Output voltage fault or warning */
#define PMBUS_STATUS_WORD_IOUT_POUT BIT(14) /**< Output current or power fault or warning */
#define PMBUS_STATUS_WORD_INPUT     BIT(13) /**< Input fault or warning */
#define PMBUS_STATUS_WORD_MFR       BIT(12) /**< Manufacturer specific fault or warning */
#define PMBUS_STATUS_WORD_PG_STATUS BIT(11) /**< Power good is negated */
#define PMBUS_STATUS_WORD_FANS      BIT(10) /**< Fan fault or warning */
#define PMBUS_STATUS_WORD_OTHER     BIT(9)  /**< Other fault or warning */
#define PMBUS_STATUS_WORD_UNKNOWN   BIT(8)  /**< Fault type not known */
/** @} */

/**
 * @name STATUS_VOUT bits
 * @{
 */
#define PMBUS_STATUS_VOUT_OV_FAULT   BIT(7) /**< Output overvoltage fault */
#define PMBUS_STATUS_VOUT_OV_WARNING BIT(6) /**< Output overvoltage warning */
#define PMBUS_STATUS_VOUT_UV_WARNING BIT(5) /**< Output undervoltage warning */
#define PMBUS_STATUS_VOUT_UV_FAULT   BIT(4) /**< Output undervoltage fault */
#define PMBUS_STATUS_VOUT_MAX_MIN    BIT(3) /**< Output voltage set beyond VOUT_MAX or VOUT_MIN */
#define PMBUS_STATUS_VOUT_TON_MAX    BIT(2) /**< Turn on time fault */
#define PMBUS_STATUS_VOUT_TOFF_MAX   BIT(1) /**< Turn off time warning */
#define PMBUS_STATUS_VOUT_TRACKING   BIT(0) /**< Output voltage tracking error */
/** @} */

/**
 * @name STATUS_IOUT bits
 * @{
 */
#define PMBUS_STATUS_IOUT_OC_FAULT      BIT(7) /**< Output overcurrent fault */
#define PMBUS_STATUS_IOUT_OC_LV_FAULT   BIT(6) /**< Output overcurrent and low voltage fault */
#define PMBUS_STATUS_IOUT_OC_WARNING    BIT(5) /**< Output overcurrent warning */
#define PMBUS_STATUS_IOUT_UC_FAULT      BIT(4) /**< Output undercurrent fault */
#define PMBUS_STATUS_IOUT_SHARE_FAULT   BIT(3) /**< Current share fault */
#define PMBUS_STATUS_IOUT_POWER_LIMIT   BIT(2) /**< Device in power limiting mode */
#define PMBUS_STATUS_IOUT_POUT_OP_FAULT BIT(1) /**< Output overpower fault */
#define PMBUS_STATUS_IOUT_POUT_OP_WARN  BIT(0) /**< Output overpower warning */
/** @} */

/**
 * @name STATUS_INPUT bits
 * @{
 */
#define PMBUS_STATUS_INPUT_VIN_OV_FAULT   BIT(7) /**< Input overvoltage fault */
#define PMBUS_STATUS_INPUT_VIN_OV_WARNING BIT(6) /**< Input overvoltage warning */
#define PMBUS_STATUS_INPUT_VIN_UV_WARNING BIT(5) /**< Input undervoltage warning */
#define PMBUS_STATUS_INPUT_VIN_UV_FAULT   BIT(4) /**< Input undervoltage fault */
#define PMBUS_STATUS_INPUT_OFF_LOW_VIN    BIT(3) /**< Output off for an input voltage too low */
#define PMBUS_STATUS_INPUT_IIN_OC_FAULT   BIT(2) /**< Input overcurrent fault */
#define PMBUS_STATUS_INPUT_IIN_OC_WARNING BIT(1) /**< Input overcurrent warning */
#define PMBUS_STATUS_INPUT_PIN_OP_WARNING BIT(0) /**< Input overpower warning */
/** @} */

/**
 * @name STATUS_TEMPERATURE bits
 * @{
 */
#define PMBUS_STATUS_TEMP_OT_FAULT   BIT(7) /**< Overtemperature fault */
#define PMBUS_STATUS_TEMP_OT_WARNING BIT(6) /**< Overtemperature warning */
#define PMBUS_STATUS_TEMP_UT_WARNING BIT(5) /**< Undertemperature warning */
#define PMBUS_STATUS_TEMP_UT_FAULT   BIT(4) /**< Undertemperature fault */
/** @} */

/**
 * @name STATUS_CML bits
 * @{
 */
#define PMBUS_STATUS_CML_INVALID_CMD     BIT(7) /**< Invalid or unsupported command received */
#define PMBUS_STATUS_CML_INVALID_DATA    BIT(6) /**< Invalid or unsupported data received */
#define PMBUS_STATUS_CML_PEC_FAILED      BIT(5) /**< Packet Error Check failed */
#define PMBUS_STATUS_CML_MEMORY_FAULT    BIT(4) /**< Memory fault */
#define PMBUS_STATUS_CML_PROCESSOR_FAULT BIT(3) /**< Processor fault */
#define PMBUS_STATUS_CML_OTHER_COMM      BIT(1) /**< Other communication fault */
#define PMBUS_STATUS_CML_OTHER_MEMORY    BIT(0) /**< Other memory or logic fault */
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PMBUS_PMBUS_CMD_H_ */
