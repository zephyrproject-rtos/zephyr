/*
 * Copyright (c) 2021 arithmetics.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BQ35100_BQ35100_H_
#define ZEPHYR_DRIVERS_SENSOR_BQ35100_BQ35100_H_

#include <drivers/sensor.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>

/*
 * BQ35100 command register definition
 */
#define BQ35100_CMD_CONTROL                            0x00     /* 2 Byte */
#define BQ35100_CMD_ACCUMULATED_CAPACITY               0x02     /* 4 Byte */
#define BQ35100_CMD_TEMPERATURE                        0x06     /* 2 Byte */
#define BQ35100_CMD_VOLTAGE                            0x08     /* 2 Byte */
#define BQ35100_CMD_BATTERY_STATUS                     0x0A     /* 1 Byte */
#define BQ35100_CMD_BATTERY_ALERT                      0x0B     /* 1 Byte */
#define BQ35100_CMD_CURRENT                            0x0C     /* 2 Byte */
#define BQ35100_CMD_SCALEDR                            0x16     /* 2 Byte */
#define BQ35100_CMD_MEASUREDZ                          0x22     /* 2 Byte */
#define BQ35100_CMD_INTERNAL_TEMP                      0x28     /* 2 Byte */
#define BQ35100_CMD_SOH                                0x2E     /* 1 Byte */
#define BQ35100_CMD_DESIGN_CAPACITY                    0x3C     /* 2 Byte */
#define BQ35100_CMD_MAC_CONTROL                        0x3E     /* 2 Byte */
#define BQ35100_CMD_MAC_DATA                           0x40     /* 31 Byte */
#define BQ35100_CMD_MAC_DATA_SUM                       0x60     /* 1 Byte */
#define BQ35100_CMD_MAC_DATA_LEN                       0x61     /* 1 Byte */
#define BQ35100_CMD_CAL_COUNT                          0x79     /* 1 Byte */
#define BQ35100_CMD_CAL_CURRENT                        0x7A     /* 2 Byte */
#define BQ35100_CMD_CAL_VOLTAGE                        0x7C     /* 2 Byte */
#define BQ35100_CMD_CAL_TEMPERATURE                    0x7E     /* 2 Byte */

/*
 * BQ35100 control subcommands definition
 */
#define BQ35100_CTRL_CONTROL_STATUS                    0x0000
#define BQ35100_CTRL_DEVICE_TYPE                       0x0001
#define BQ35100_CTRL_FW_VERSION                        0x0002
#define BQ35100_CTRL_HW_VERSION                        0x0002
#define BQ35100_CTRL_STATIC_CHEM_CHKSUM                0x0005
#define BQ35100_CTRL_CHEM_ID                           0x0006
#define BQ35100_CTRL_PREV_MACWRITE                     0x0007
#define BQ35100_CTRL_BOARD_OFFSET                      0x0009
#define BQ35100_CTRL_CC_OFFSET                         0x000A
#define BQ35100_CTRL_CC_OFFSET_SAVE                    0x000B
#define BQ35100_CTRL_GAUGE_START                       0x0011
#define BQ35100_CTRL_GAUGE_STOP                        0x0012
#define BQ35100_CTRL_SEALED                            0x0020
#define BQ35100_CTRL_CAL_ENABLE                        0x002D
#define BQ35100_CTRL_LT_ENABLE                         0x002E
#define BQ35100_CTRL_RESET                             0x0041
#define BQ35100_CTRL_EXIT_CAL                          0x0080
#define BQ35100_CTRL_ENTER_CAL                         0x0081
#define BQ35100_CTRL_NEW_BATTERY                       0xA631

/*
 * BQ35100 flash data address definition
 */
#define BQ35100_FLASH_CC_GAIN                          0x4000
#define BQ35100_FLASH_CC_DELTA                         0x4004
#define BQ35100_FLASH_CC_OFFSET                        0x4008
#define BQ35100_FLASH_AD_I_OFFSET                      0x400A
#define BQ35100_FLASH_BOARD_OFFSET                     0x400C
#define BQ35100_FLASH_INT_TEMP_OFFSET                  0x400D
#define BQ35100_FLASH_EXT_TEMP_OFFSET                  0x400E
#define BQ35100_FLASH_PACK_V_OFFSET                    0x400F
#define BQ35100_FLASH_VIN_GAIN                         0x4010
#define BQ35100_FLASH_INT_COEFF1                       0x4012
#define BQ35100_FLASH_INT_COEFF2                       0x4014
#define BQ35100_FLASH_INT_COEFF3                       0x4016
#define BQ35100_FLASH_INT_COEFF4                       0x4018
#define BQ35100_FLASH_INT_MIN_AD                       0x401A
#define BQ35100_FLASH_INT_MAX_TEMP                     0x401C
#define BQ35100_FLASH_EXT_COEFF1                       0x401E
#define BQ35100_FLASH_EXT_COEFF2                       0x4020
#define BQ35100_FLASH_EXT_COEFF3                       0x4022
#define BQ35100_FLASH_EXT_COEFF4                       0x4024
#define BQ35100_FLASH_EXT_RC0                          0x4026
#define BQ35100_FLASH_VCOMP_COEFF1                     0x4028
#define BQ35100_FLASH_VCOMP_COEFF2                     0x402A
#define BQ35100_FLASH_VCOMP_COEFF3                     0x402C
#define BQ35100_FLASH_VCOMP_COEFF4                     0x402E
#define BQ35100_FLASH_VCOMP_IN_MULTIPLIER              0x4030
#define BQ35100_FLASH_VCOMP_OUT_MULTIPLIER             0x4031
#define BQ35100_FLASH_FILTER                           0x4033
#define BQ35100_FLASH_OPERATION_CFG_A                  0x41B1
#define BQ35100_FLASH_ALERT_CFG                        0x41B2
#define BQ35100_FLASH_CLK_CTL_REG                      0x41B3
#define BQ35100_FLASH_BATTERY_ID                       0x4254
#define BQ35100_FLASH_UPDATE_OK_VOLTAGE                0x41B6
#define BQ35100_FLASH_OFFSET_CAL_INHIBIT_TEMP_LOW      0x41B8
#define BQ35100_FLASH_OFFSET_CAL_INHIBIT_TEMP_HIGH     0x41BA
#define BQ35100_FLASH_DEVICE_NAME                      0x4060
#define BQ35100_FLASH_DATA_FLASH_VERSION               0x4068
#define BQ35100_FLASH_DEFAULT_TEMP                     0x41D4
#define BQ35100_FLASH_OT_DSG                           0x41D6
#define BQ35100_FLASH_OT_DSG_TIME                      0x41B8
#define BQ35100_FLASH_OT_DSG_RECOVERY                  0x41D9
#define BQ35100_FLASH_BATLOW_VOLTAGE_SET_THRESH        0x41DB
#define BQ35100_FLASH_UNDER_TEMP_SET_TRESH             0x41E0
#define BQ35100_FLASH_UNDER_TEMP_SET_TIME              0x41E2
#define BQ35100_FLASH_UNDER_TEMP_CLEAR                 0x41E3
#define BQ35100_FLASH_SOH_LOW                          0x41E5
#define BQ35100_FLASH_STATIC_CHEM_CHECKSUM             0x4056
#define BQ35100_FLASH_IF_CHECKSUM                      0x405C
#define BQ35100_FLASH_RESET_COUNTER_WD                 0x4253
#define BQ35100_FLASH_PRIMARY_MAX                      0x4240
#define BQ35100_FLASH_PRIMARY_MIN                      0x4242
#define BQ35100_FLASH_MAX_DISCHARGE                    0x4244
#define BQ35100_FLASH_MIN_DISCHARGE                    0x4246
#define BQ35100_FLASH_MAX_CELL                         0x4248
#define BQ35100_FLASH_MIN_CELL                         0x424A
#define BQ35100_FLASH_MAX_GAUGE                        0x424C
#define BQ35100_FLASH_MIN_GAUGE                        0x424E
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A01      0x4036
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A02      0x4037
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A03      0x4038
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A04      0x4039
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A05      0x403A
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A06      0x403B
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A07      0x403C
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A08      0x403D
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A09      0x403E
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A10      0x403F
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A11      0x4040
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A12      0x4041
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A13      0x4042
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A14      0x4043
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A15      0x4044
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A16      0x4045
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A17      0x4046
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A18      0x4047
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A19      0x4048
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A20      0x4049
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A21      0x404A
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A22      0x404B
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A23      0x404C
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A24      0x404D
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A25      0x404E
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A26      0x404F
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A27      0x4050
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A28      0x4051
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A29      0x4052
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A30      0x4053
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A31      0x4054
#define BQ35100_FLASH_MANUFACTURER_INFO_BLOCK_A32      0x4055
#define BQ35100_FLASH_CELL_DESIGN_CAPACITY_MAH         0x41FE
#define BQ35100_FLASH_CELL_DESIGN_VOLTAGE              0x4202
#define BQ35100_FLASH_CELL_TERMINATE_VOLTAGE           0x4204
#define BQ35100_FLASH_SERIES_CELL_COUNT                0x4206
#define BQ35100_FLASH_MAX_LOAD                         0x4207
#define BQ35100_FLASH_SOH                              0x4209
#define BQ35100_FLASH_SOH_MAX_DELTA                    0x420A
#define BQ35100_FLASH_TABLE0_PAGE_ACTIVE               0x4280
#define BQ35100_FLASH_TABLE_0_LAST_ENTRY_CODE0         0x4281
#define BQ35100_FLASH_TABLE_0_LAST_ENTRY_CODE1         0x4282
#define BQ35100_FLASH_TABLE_0_LAST_ENTRY_CODE2         0x4283
#define BQ35100_FLASH_TABLE_0_LAST_ENTRY_CODE3         0x4284
#define BQ35100_FLASH_TABLE_0_LAST_ENTRY_CODE4         0x4285
#define BQ35100_FLASH_TABLE0_INT_PART0                 0x4286
#define BQ35100_FLASH_TABLE0_FRACT_PART0               0x4288
#define BQ35100_FLASH_TABLE0_INT_PART1                 0x428C
#define BQ35100_FLASH_TABLE0_FRACT_PART1               0x428E
#define BQ35100_FLASH_TABLE0_INT_PART2                 0x4292
#define BQ35100_FLASH_TABLE0_FRACT_PART2               0x4294
#define BQ35100_FLASH_TABLE0_INT_PART3                 0x4298
#define BQ35100_FLASH_TABLE0_FRACT_PART3               0x429A
#define BQ35100_FLASH_TABLE0_INT_PART4                 0x429E
#define BQ35100_FLASH_TABLE0_FRACT_PART4               0x42A0
#define BQ35100_FLASH_TABLE0_INT_PART5                 0x42A4
#define BQ35100_FLASH_TABLE0_FRACT_PART5               0x42A6
#define BQ35100_FLASH_TABLE0_INT_PART6                 0x42AA
#define BQ35100_FLASH_TABLE0_FRACT_PART6               0x42AC
#define BQ35100_FLASH_TABLE0_INT_PART7                 0x42B0
#define BQ35100_FLASH_TABLE0_FRACT_PART7               0x42B2
#define BQ35100_FLASH_TABLE0_INT_PART8                 0x42B6
#define BQ35100_FLASH_TABLE0_FRACT_PART8               0x42B8
#define BQ35100_FLASH_TABLE1_PAGE_ACTIVE               0x42C0
#define BQ35100_FLASH_TABLE1_LAST_ENTRY_CODE0          0x42C1
#define BQ35100_FLASH_TABLE1_LAST_ENTRY_CODE1          0x42C2
#define BQ35100_FLASH_TABLE1_LAST_ENTRY_CODE2          0x42C3
#define BQ35100_FLASH_TABLE1_LAST_ENTRY_CODE3          0x42C4
#define BQ35100_FLASH_TABLE1_LAST_ENTRY_CODE4          0x42C5
#define BQ35100_FLASH_TABLE1_INT_PART0                 0x42C6
#define BQ35100_FLASH_TABLE1_FRACT_PART0               0x42C8
#define BQ35100_FLASH_TABLE1_INT_PART1                 0x42CC
#define BQ35100_FLASH_TABLE1_FRACT_PART1               0x42CE
#define BQ35100_FLASH_TABLE1_INT_PART2                 0x42D2
#define BQ35100_FLASH_TABLE1_FRACT_PART2               0x42D4
#define BQ35100_FLASH_TABLE1_INT_PART3                 0x42D8
#define BQ35100_FLASH_TABLE1_FRACT_PART3               0x42DA
#define BQ35100_FLASH_TABLE1_INT_PART4                 0x42DE
#define BQ35100_FLASH_TABLE1_FRACT_PART4               0x42E0
#define BQ35100_FLASH_TABLE1_INT_PART5                 0x42E4
#define BQ35100_FLASH_TABLE1_FRACT_PART5               0x42E6
#define BQ35100_FLASH_TABLE1_INT_PART6                 0x42EA
#define BQ35100_FLASH_TABLE1_FRACT_PART6               0x42EC
#define BQ35100_FLASH_TABLE1_INT_PART7                 0x42F0
#define BQ35100_FLASH_TABLE1_FRACT_PART7               0x42F2
#define BQ35100_FLASH_TABLE1_INT_PART8                 0x42F6
#define BQ35100_FLASH_TABLE1_FRACT_PART8               0x42F8
#define BQ35100_FLASH_RA0                              0x4175
#define BQ35100_FLASH_RA1                              0x4177
#define BQ35100_FLASH_RA2                              0x4179
#define BQ35100_FLASH_RA3                              0x417B
#define BQ35100_FLASH_RA4                              0x417D
#define BQ35100_FLASH_RA5                              0x417F
#define BQ35100_FLASH_RA6                              0x4181
#define BQ35100_FLASH_RA7                              0x4183
#define BQ35100_FLASH_RA8                              0x4185
#define BQ35100_FLASH_RA9                              0x4187
#define BQ35100_FLASH_RA10                             0x4189
#define BQ35100_FLASH_RA11                             0x418B
#define BQ35100_FLASH_RA12                             0x418D
#define BQ35100_FLASH_RA13                             0x418F
#define BQ35100_FLASH_RA14                             0x4191
#define BQ35100_FLASH_R_DATA_SECONDS                   0x4255
#define BQ35100_FLASH_R_TABLE_SCALE                    0x4257
#define BQ35100_FLASH_NEW_BATT_R_SCALE_DELAY           0x4259
#define BQ35100_FLASH_R_TABLE_SCALE_UPDATE_FLAG        0x425A
#define BQ35100_FLASH_R_SHORT_TREND_FILTER             0x425B
#define BQ35100_FLASH_R_LONG_TREND_FILTER              0x425C
#define BQ35100_FLASH_EOS_TREND_DETECTION_PERCENT      0x425D
#define BQ35100_FLASH_EOS_DETEC_PULSE_CNT_THRESH       0x425E
#define BQ35100_FLASH_SHORT_TREND_AVERAGE              0x4260
#define BQ35100_FLASH_LONG_TREND_AVERAGE               0x4264
#define BQ35100_FLASH_EOS_TREND_DETEC_PULSE_CNT        0x4268
#define BQ35100_FLASH_EOS_NOT_DETECTED_FLAG            0x426A
#define BQ35100_FLASH_EOS_SOH_SMOOTH_START_VOLTAGE     0x426B
#define BQ35100_FLASH_EOS_SOH_SMOOTHING_MARGIN         0x426D
#define BQ35100_FLASH_EOS_RELAX_V_HI_MAX_CNT           0x426E
#define BQ35100_FLASH_AUTHEN_KEY3_MSB                  0x41BC
#define BQ35100_FLASH_AUTHEN_KEY3_LSB                  0x41BE
#define BQ35100_FLASH_AUTHEN_KEY2_MSB                  0x41C0
#define BQ35100_FLASH_AUTHEN_KEY2_LSB                  0x41C2
#define BQ35100_FLASH_AUTHEN_KEY1_MSB                  0x41C4
#define BQ35100_FLASH_AUTHEN_KEY1_LSB                  0x41C6
#define BQ35100_FLASH_AUTHEN_KEY0_MSB                  0x41C8
#define BQ35100_FLASH_AUTHEN_KEY0_LSB                  0x41CA
#define BQ35100_FLASH_UNSEAL_STEP1                     0x41CC
#define BQ35100_FLASH_UNSEAL_STEP2                     0x41CE
#define BQ35100_FLASH_FULL_UNSEAL_STEP1                0x41D0
#define BQ35100_FLASH_FULL_UNSEAL_STEP2                0x41D2

#define BQ35100_DEVICE_TYPE_ID                         0x100

#define BQ35100_DEFAULT_SEAL_CODES                     0x04143672

/*
 * I2C helper bit masks
 */
#define BQ35100_READ                    0x01u
#define BQ35100_REG_READ(x)             (((x & 0xFF) << 1) | BQ35100_READ)
#define BQ35100_REG_WRITE(x)            ((x & 0xFF) << 1)
#define BQ35100_TO_I2C_REG(x)           ((x) >> 1)

enum bq35100_gauge_mode{
    BQ35100_ACCUMULATOR_MODE = 0b00,
    BQ35100_SOH_MODE = 0b01, // for LiMnO2
    BQ35100_EOS_MODE = 0b10, // for LiSOCl2
    BQ35100_UNKNOWN_MODE = 0b11 // invalid
};

enum bq35100_security{
    BQ35100_SECURITY_UNKNOWN = 0x00,
    BQ35100_SECURITY_FULL_ACCESS = 0x01, // Allows writes to all of memory
    BQ35100_SECURITY_UNSEALED = 0x02, // Allows writes to all of memory apart from the security codes area
    BQ35100_SECURITY_SEALED = 0x03 // Normal operating mode, prevents accidental writes
};

enum bq35100_security bq35100_current_security_mode;

struct bq35100_data {
	uint16_t temperature;
	uint16_t voltage;
	uint16_t avg_current;
	uint16_t state_of_health;
	uint32_t acc_capacity;
};

struct bq35100_config {
	const struct device *bus;
	uint8_t i2c_addr;

	const struct device *ge_gpio;
	gpio_pin_t ge_pin;
	gpio_dt_flags_t ge_flags;
};

#endif  /* ZEPHYR_DRIVERS_SENSOR_BQ35100_BQ35100_H_ */