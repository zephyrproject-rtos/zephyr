/*
 * Copyright (c) 2024 Shen Xuyang  <shenxuyang@shlinyuantech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef IST3931_DISPLAY_DRIVER_H__
#define IST3931_DISPLAY_DRIVER_H__

#define IST3931_CMD_NOP                    0xe3
#define IST3931_CMD_IST_COMMAND_ENTRY      0x88
#define IST3931_CMD_EXIT_ENTRY             0xe3
#define IST3931_CMD_IST_COM_MAPPING        0x60
#define IST3931_CMD_POWER_CONTROL          0x2c
#define IST3931_CMD_BIAS                   0x30
#define IST3931_CMD_CT                     0xb1
#define IST3931_CMD_FRAME_CONTROL          0xb2
#define IST3931_CMD_SET_AX_ADD             0xc0
#define IST3931_CMD_SET_AY_ADD_LSB         0x00
#define IST3931_CMD_SET_AY_ADD_MSB         0x10
#define IST3931_CMD_SET_START_LINE_LSB     0x40
#define IST3931_CMD_SET_START_LINE_MSB     0x50
#define IST3931_CMD_OSC_CONTROL            0x2a
#define IST3931_CMD_DRIVER_DISPLAY_CONTROL 0x60
#define IST3931_CMD_SW_RESET               0x76
#define IST3931_CMD_SET_DUTY_LSB           0x90
#define IST3931_CMD_SET_DUTY_MSB           0xa0
#define IST3931_CMD_DISPLAY_ON_OFF         0x3c
#define IST3931_CMD_SLEEP_MODE             0x38

#define IST3931_CMD_BYTE    0x80
#define IST3931_DATA_BYTE   0xc0
#define IST3931_RESET_DELAY 50
#define IST3931_CMD_DELAY   10
#define IST3931_RAM_WIDTH   144
#define IST3931_RAM_HEIGHT  65
#endif /* _ZEPHYR_DRIVERS_DISPLAY_IST3931_H_ */
