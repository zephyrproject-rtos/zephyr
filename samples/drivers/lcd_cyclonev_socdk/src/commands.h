/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2022, Intel Corporation
 * Description:
 * Example to use LCD disply in Cyclone V SoC DevKit
 * Reference: https://datasheetspdf.com/pdf-file/746090/Newhaven/NHD-0216K3Z-NSW-BBW/1
 */

/* Insert Prefix 0xFE before executing command */
#define DISPLAY_ON	0x41
#define DISPLAY_OFF	0x42
#define SET_CURSOR	0x45 /*1 byte param in range (0x00 - 0x4F) 2x16 display */
#define CURSOR_HOME	0x46
#define UNDERLINE_ON	0x47
#define UNDERLINE_OFF	0x48
#define MOVE_CUR_LEFT	0x49
#define MOVE_CUR_RIGHT	0x4a
#define BLINK_ON	0x4b
#define BLINK_OFF	0x4c
#define BACKSPACE	0x4e
#define CLEAR_SCREEN	0x51
#define SET_CONTRAST	0x52 /* 1 byte param in range (1 - 50) 40 default */
#define SET_BACKLIGHT	0x53 /* 1 byte param in range (1 - 8) 1 default */

#define LD_CUSTOM_CHAR	0x54 /* 9 byte param 1st param: 1-byte */

/* Syntax   prefix LD_CUSTOM_CHAR [addr] [d0 …d7]
 *
 * Parameter Length   Description
 * [addr]    1 byte   Custom character address, 0 – 7
 * [D0...D7] 8 bytes  Custom character pattern bit map
 *
 * Example: ¿ Character
 * Bit	 7 6 5 4 3 2 1 0 Hex
 * Byte1	 0 0 0 0 0 1 0 0 0x04
 * Byte2	 0 0 0 0 0 0 0 0 0x00
 * Byte3	 0 0 0 0 0 1 0 0 0x04
 * Byte4	 0 0 0 0 1 0 0 0 0x08
 * Byte5	 0 0 0 1 0 0 0 0 0x10
 * Byte6	 0 0 0 1 0 0 0 1 0x11
 * Byte7	 0 0 0 0 1 1 1 0 0x0E
 * Byte8	 0 0 0 0 0 0 0 0 0x00
 */

#define MOVE_DISP_LEFT	0x55
#define MOVE_DISP_RIGHT	0x56
#define CHGE_RS232_BAUD 0x61 /* 1 byte param in range (1 - 8) */

/* Syntax   prefix CHGE_RS232_BAUD [param]
 *
 * Param	 BAUD
 * 1	 300
 * 2	 1200
 * 3	 2400
 * 4	 9600
 * 5	 14400
 * 6	 19.2K
 * 7	 57.6K
 * 8	 115.2K
 */

#define CHGE_I2C_ADDR	0X62 /* 1 byte param in range (0x00 - 0xFE), LSB = 0 */
#define DISP_FIRMW_VER	0x70
#define DISP_RS232_BAUD	0x71
#define DISP_I2C_ADDR	0x72

/* Functions definitions */

void send_ascii(char c);

void send_string(uint8_t *str_ptr, int len);

void send_command_no_param(uint8_t command_id);

void send_command_one_param(uint8_t command_id, uint8_t param);

void clear(void);

void next_line(void);
