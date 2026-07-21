/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DISPLAY_ST7567_REGS_H__
#define __DISPLAY_ST7567_REGS_H__

/* All following bytes will contain commands */
#define ST7567_CONTROL_ALL_BYTES_CMD     0x00
/* All following bytes will contain data */
#define ST7567_CONTROL_ALL_BYTES_DATA    0x40
/* The next byte will contain a command, then expect another control byte */
#define ST7567_CONTROL_CONTINUE_BYTE_CMD 0x80

#define ST7567_DISPLAY_OFF              0xAE
#define ST7567_DISPLAY_ON               0xAF
#define ST7567_DISPLAY_ALL_PIXEL_ON     0xA5
#define ST7567_DISPLAY_ALL_PIXEL_NORMAL 0xA4
#define ST7567_SET_CONTRAST_CTRL        0x81
#define ST7567_SET_BIAS                 0xA2
#define ST7567_SET_REGULATION_RATIO     0x20

/* Inversion controls */
#define ST7567_SET_NORMAL_DISPLAY  0xA6
#define ST7567_SET_REVERSE_DISPLAY 0xA7

/* COM invdir */
#define ST7567_SET_COM_OUTPUT_SCAN_FLIPPED 0xC8
#define ST7567_SET_COM_OUTPUT_SCAN_NORMAL  0xC0

/* SEG invdir */
#define ST7567_SET_SEGMENT_MAP_FLIPPED 0xA1
#define ST7567_SET_SEGMENT_MAP_NORMAL  0xA0

/* Power Control */
#define ST7567_POWER_CONTROL    0x28
#define ST7567_POWER_CONTROL_VF 0x1
#define ST7567_POWER_CONTROL_VR 0x2
#define ST7567_POWER_CONTROL_VB 0x4

/* Offsets */
#define ST7567_COLUMN_MSB  0x10
#define ST7567_COLUMN_LSB  0x0
#define ST7567_PAGE        0xB0
#define ST7567_LINE_SCROLL 0x40

#define ST7567_RESET_DELAY 1
#define ST7567_RESET       0xE2

#endif /* __DISPLAY_ST7567_REGS_H__ */
