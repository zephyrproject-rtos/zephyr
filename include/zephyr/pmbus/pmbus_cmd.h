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
#define PMBUS_PAGE                      0x00
#define PMBUS_OPERATION                 0x01
#define PMBUS_ON_OFF_CONFIG             0x02
#define PMBUS_CLEAR_FAULTS              0x03
#define PMBUS_PHASE                     0x04
#define PMBUS_PAGE_PLUS_WRITE           0x05
#define PMBUS_PAGE_PLUS_READ            0x06
#define PMBUS_ZONE_CONFIG               0x07
#define PMBUS_ZONE_ACTIVE               0x08
#define PMBUS_WRITE_PROTECT             0x10
#define PMBUS_STORE_DEFAULT_ALL         0x11
#define PMBUS_RESTORE_DEFAULT_ALL       0x12
#define PMBUS_STORE_DEFAULT_CODE        0x13
#define PMBUS_RESTORE_DEFAULT_CODE      0x14
#define PMBUS_STORE_USER_ALL            0x15
#define PMBUS_RESTORE_USER_ALL          0x16
#define PMBUS_STORE_USER_CODE           0x17
#define PMBUS_RESTORE_USER_CODE         0x18
#define PMBUS_CAPABILITY                0x19
#define PMBUS_QUERY                     0x1A
#define PMBUS_SMBALERT_MASK             0x1B
#define PMBUS_VOUT_MODE                 0x20
#define PMBUS_VOUT_COMMAND              0x21
#define PMBUS_VOUT_TRIM                 0x22
#define PMBUS_VOUT_CAL_OFFSET           0x23
#define PMBUS_VOUT_MAX                  0x24
#define PMBUS_VOUT_MARGIN_HIGH          0x25
#define PMBUS_VOUT_MARGIN_LOW           0x26
#define PMBUS_VOUT_TRANSITION_RATE      0x27
#define PMBUS_VOUT_DROOP                0x28
#define PMBUS_VOUT_SCALE_LOOP           0x29
#define PMBUS_VOUT_SCALE_MONITOR        0x2A
#define PMBUS_VOUT_MIN                  0x2B
#define PMBUS_COEFFICIENTS              0x30
#define PMBUS_POUT_MAX                  0x31
#define PMBUS_MAX_DUTY                  0x32
#define PMBUS_FREQUENCY_SWITCH          0x33
#define PMBUS_POWER_MODE                0x34
#define PMBUS_VIN_ON                    0x35
#define PMBUS_VIN_OFF                   0x36
#define PMBUS_INTERLEAVE                0x37
#define PMBUS_IOUT_CAL_GAIN             0x38
#define PMBUS_IOUT_CAL_OFFSET           0x39
#define PMBUS_FAN_CONFIG_1_2            0x3A
#define PMBUS_FAN_COMMAND_1             0x3B
#define PMBUS_FAN_COMMAND_2             0x3C
#define PMBUS_FAN_CONFIG_3_4            0x3D
#define PMBUS_FAN_COMMAND_3             0x3E
#define PMBUS_FAN_COMMAND_4             0x3F
#define PMBUS_VOUT_OV_FAULT_LIMIT       0x40
#define PMBUS_VOUT_OV_FAULT_RESPONSE    0x41
#define PMBUS_VOUT_OV_WARN_LIMIT        0x42
#define PMBUS_VOUT_UV_WARN_LIMIT        0x43
#define PMBUS_VOUT_UV_FAULT_LIMIT       0x44
#define PMBUS_VOUT_UV_FAULT_RESPONSE    0x45
#define PMBUS_IOUT_OC_FAULT_LIMIT       0x46
#define PMBUS_IOUT_OC_FAULT_RESPONSE    0x47
#define PMBUS_IOUT_OC_LV_FAULT_LIMIT    0x48
#define PMBUS_IOUT_OC_LV_FAULT_RESPONSE 0x49
#define PMBUS_IOUT_OC_WARN_LIMIT        0x4A
#define PMBUS_IOUT_UC_FAULT_LIMIT       0x4B
#define PMBUS_IOUT_UC_FAULT_RESPONSE    0x4C
#define PMBUS_OT_FAULT_LIMIT            0x4F
#define PMBUS_OT_FAULT_RESPONSE         0x50
#define PMBUS_OT_WARN_LIMIT             0x51
#define PMBUS_UT_WARN_LIMIT             0x52
#define PMBUS_UT_FAULT_LIMIT            0x53
#define PMBUS_UT_FAULT_RESPONSE         0x54
#define PMBUS_VIN_OV_FAULT_LIMIT        0x55
#define PMBUS_VIN_OV_FAULT_RESPONSE     0x56
#define PMBUS_VIN_OV_WARN_LIMIT         0x57
#define PMBUS_VIN_UV_WARN_LIMIT         0x58
#define PMBUS_VIN_UV_FAULT_LIMIT        0x59
#define PMBUS_VIN_UV_FAULT_RESPONSE     0x5A
#define PMBUS_IIN_OC_FAULT_LIMIT        0x5B
#define PMBUS_IIN_OC_FAULT_RESPONSE     0x5C
#define PMBUS_IIN_OC_WARN_LIMIT         0x5D
#define PMBUS_POWER_GOOD_ON             0x5E
#define PMBUS_POWER_GOOD_OFF            0x5F
#define PMBUS_TON_DELAY                 0x60
#define PMBUS_TON_RISE                  0x61
#define PMBUS_TON_MAX_FAULT_LIMIT       0x62
#define PMBUS_TON_MAX_FAULT_RESPONSE    0x63
#define PMBUS_TOFF_DELAY                0x64
#define PMBUS_TOFF_FALL                 0x65
#define PMBUS_TOFF_MAX_WARN_LIMIT       0x66
#define PMBUS_POUT_OP_FAULT_LIMIT       0x68
#define PMBUS_POUT_OP_FAULT_RESPONSE    0x69
#define PMBUS_POUT_OP_WARN_LIMIT        0x6A
#define PMBUS_PIN_OP_WARN_LIMIT         0x6B
#define PMBUS_STATUS_BYTE               0x78
#define PMBUS_STATUS_WORD               0x79
#define PMBUS_STATUS_VOUT               0x7A
#define PMBUS_STATUS_IOUT               0x7B
#define PMBUS_STATUS_INPUT              0x7C
#define PMBUS_STATUS_TEMPERATURE        0x7D
#define PMBUS_STATUS_CML                0x7E
#define PMBUS_STATUS_OTHER              0x7F
#define PMBUS_STATUS_MFR_SPECIFIC       0x80
#define PMBUS_STATUS_FANS_1_2           0x81
#define PMBUS_STATUS_FANS_3_4           0x82
#define PMBUS_READ_KWH_IN               0x83
#define PMBUS_READ_KWH_OUT              0x84
#define PMBUS_READ_KWH_CONFIG           0x85
#define PMBUS_READ_EIN                  0x86
#define PMBUS_READ_EOUT                 0x87
#define PMBUS_READ_VIN                  0x88
#define PMBUS_READ_IIN                  0x89
#define PMBUS_READ_VCAP                 0x8A
#define PMBUS_READ_VOUT                 0x8B
#define PMBUS_READ_IOUT                 0x8C
#define PMBUS_READ_TEMPERATURE_1        0x8D
#define PMBUS_READ_TEMPERATURE_2        0x8E
#define PMBUS_READ_TEMPERATURE_3        0x8F
#define PMBUS_READ_FAN_SPEED_1          0x90
#define PMBUS_READ_FAN_SPEED_2          0x91
#define PMBUS_READ_FAN_SPEED_3          0x92
#define PMBUS_READ_FAN_SPEED_4          0x93
#define PMBUS_READ_DUTY_CYCLE           0x94
#define PMBUS_READ_FREQUENCY            0x95
#define PMBUS_READ_POUT                 0x96
#define PMBUS_READ_PIN                  0x97
#define PMBUS_REVISION                  0x98
#define PMBUS_MFR_ID                    0x99
#define PMBUS_MFR_MODEL                 0x9A
#define PMBUS_MFR_REVISION              0x9B
#define PMBUS_MFR_LOCATION              0x9C
#define PMBUS_MFR_DATE                  0x9D
#define PMBUS_MFR_SERIAL                0x9E
#define PMBUS_APP_PROFILE_SUPPORT       0x9F
#define PMBUS_MFR_VIN_MIN               0xA0
#define PMBUS_MFR_VIN_MAX               0xA1
#define PMBUS_MFR_IIN_MAX               0xA2
#define PMBUS_MFR_PIN_MAX               0xA3
#define PMBUS_MFR_VOUT_MIN              0xA4
#define PMBUS_MFR_VOUT_MAX              0xA5
#define PMBUS_MFR_IOUT_MAX              0xA6
#define PMBUS_MFR_POUT_MAX              0xA7
#define PMBUS_MFR_TAMBIENT_MAX          0xA8
#define PMBUS_MFR_TAMBIENT_MIN          0xA9
#define PMBUS_MFR_EFFICIENCY_LL         0xAA
#define PMBUS_MFR_EFFICIENCY_HL         0xAB
#define PMBUS_MFR_PIN_ACCURACY          0xAC
#define PMBUS_IC_DEVICE_ID              0xAD
#define PMBUS_IC_DEVICE_REV             0xAE
#define PMBUS_USER_DATA_00              0xB0
#define PMBUS_MFR_MAX_TEMP_1            0xC0
#define PMBUS_MFR_MAX_TEMP_2            0xC1
#define PMBUS_MFR_MAX_TEMP_3            0xC2
#define PMBUS_MFR_SPECIFIC_COMMAND_EXT  0xFE
#define PMBUS_COMMAND_EXT               0xFF
/** @} */

/** PAGE value that selects all pages, valid only for writes */
#define PMBUS_PAGE_ALL 0xFF

/**
 * @name CAPABILITY bits
 * @{
 */
#define PMBUS_CAPABILITY_PEC        BIT(7)
#define PMBUS_CAPABILITY_SPEED_MASK GENMASK(6, 5)
#define PMBUS_CAPABILITY_SMBALERT   BIT(4)
#define PMBUS_CAPABILITY_IEEE_HALF  BIT(3)
#define PMBUS_CAPABILITY_AVSBUS     BIT(2)
/** @} */

/**
 * @name VOUT_MODE fields
 * @{
 */
#define PMBUS_VOUT_MODE_RELATIVE   BIT(7)
#define PMBUS_VOUT_MODE_MODE_MASK  GENMASK(6, 5)
#define PMBUS_VOUT_MODE_PARAM_MASK GENMASK(4, 0)
#define PMBUS_VOUT_MODE_ULINEAR16  0
#define PMBUS_VOUT_MODE_VID        1
#define PMBUS_VOUT_MODE_DIRECT     2
#define PMBUS_VOUT_MODE_IEEE_HALF  3
/** @} */

/**
 * @name STATUS_BYTE bits, also the low byte of STATUS_WORD
 * @{
 */
#define PMBUS_STATUS_BYTE_BUSY              BIT(7)
#define PMBUS_STATUS_BYTE_OFF               BIT(6)
#define PMBUS_STATUS_BYTE_VOUT_OV_FAULT     BIT(5)
#define PMBUS_STATUS_BYTE_IOUT_OC_FAULT     BIT(4)
#define PMBUS_STATUS_BYTE_VIN_UV_FAULT      BIT(3)
#define PMBUS_STATUS_BYTE_TEMPERATURE       BIT(2)
#define PMBUS_STATUS_BYTE_CML               BIT(1)
#define PMBUS_STATUS_BYTE_NONE_OF_THE_ABOVE BIT(0)
/** @} */

/**
 * @name STATUS_WORD high byte bits
 * @{
 */
#define PMBUS_STATUS_WORD_VOUT      BIT(15)
#define PMBUS_STATUS_WORD_IOUT_POUT BIT(14)
#define PMBUS_STATUS_WORD_INPUT     BIT(13)
#define PMBUS_STATUS_WORD_MFR       BIT(12)
#define PMBUS_STATUS_WORD_PG_STATUS BIT(11)
#define PMBUS_STATUS_WORD_FANS      BIT(10)
#define PMBUS_STATUS_WORD_OTHER     BIT(9)
#define PMBUS_STATUS_WORD_UNKNOWN   BIT(8)
/** @} */

/**
 * @name STATUS_VOUT bits
 * @{
 */
#define PMBUS_STATUS_VOUT_OV_FAULT   BIT(7)
#define PMBUS_STATUS_VOUT_OV_WARNING BIT(6)
#define PMBUS_STATUS_VOUT_UV_WARNING BIT(5)
#define PMBUS_STATUS_VOUT_UV_FAULT   BIT(4)
#define PMBUS_STATUS_VOUT_MAX_MIN    BIT(3)
#define PMBUS_STATUS_VOUT_TON_MAX    BIT(2)
#define PMBUS_STATUS_VOUT_TOFF_MAX   BIT(1)
#define PMBUS_STATUS_VOUT_TRACKING   BIT(0)
/** @} */

/**
 * @name STATUS_IOUT bits
 * @{
 */
#define PMBUS_STATUS_IOUT_OC_FAULT      BIT(7)
#define PMBUS_STATUS_IOUT_OC_LV_FAULT   BIT(6)
#define PMBUS_STATUS_IOUT_OC_WARNING    BIT(5)
#define PMBUS_STATUS_IOUT_UC_FAULT      BIT(4)
#define PMBUS_STATUS_IOUT_SHARE_FAULT   BIT(3)
#define PMBUS_STATUS_IOUT_POWER_LIMIT   BIT(2)
#define PMBUS_STATUS_IOUT_POUT_OP_FAULT BIT(1)
#define PMBUS_STATUS_IOUT_POUT_OP_WARN  BIT(0)
/** @} */

/**
 * @name STATUS_INPUT bits
 * @{
 */
#define PMBUS_STATUS_INPUT_VIN_OV_FAULT   BIT(7)
#define PMBUS_STATUS_INPUT_VIN_OV_WARNING BIT(6)
#define PMBUS_STATUS_INPUT_VIN_UV_WARNING BIT(5)
#define PMBUS_STATUS_INPUT_VIN_UV_FAULT   BIT(4)
#define PMBUS_STATUS_INPUT_OFF_LOW_VIN    BIT(3)
#define PMBUS_STATUS_INPUT_IIN_OC_FAULT   BIT(2)
#define PMBUS_STATUS_INPUT_IIN_OC_WARNING BIT(1)
#define PMBUS_STATUS_INPUT_PIN_OP_WARNING BIT(0)
/** @} */

/**
 * @name STATUS_TEMPERATURE bits
 * @{
 */
#define PMBUS_STATUS_TEMP_OT_FAULT   BIT(7)
#define PMBUS_STATUS_TEMP_OT_WARNING BIT(6)
#define PMBUS_STATUS_TEMP_UT_WARNING BIT(5)
#define PMBUS_STATUS_TEMP_UT_FAULT   BIT(4)
/** @} */

/**
 * @name STATUS_CML bits
 * @{
 */
#define PMBUS_STATUS_CML_INVALID_CMD     BIT(7)
#define PMBUS_STATUS_CML_INVALID_DATA    BIT(6)
#define PMBUS_STATUS_CML_PEC_FAILED      BIT(5)
#define PMBUS_STATUS_CML_MEMORY_FAULT    BIT(4)
#define PMBUS_STATUS_CML_PROCESSOR_FAULT BIT(3)
#define PMBUS_STATUS_CML_OTHER_COMM      BIT(1)
#define PMBUS_STATUS_CML_OTHER_MEMORY    BIT(0)
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PMBUS_PMBUS_CMD_H_ */
