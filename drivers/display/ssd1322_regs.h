/*
 * Copyright (c) 2024 Lukasz Hawrylko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SSD1322_REGS_H__
#define __SSD1322_REGS_H__

#define SSD1322_SET_COLUMN_ADDR      0x15
#define SSD1322_ENABLE_RAM_WRITE     0x5C
#define SSD1322_SET_ROW_ADDR         0x75
#define SSD1322_SET_REMAP            0xA0
#define SSD1322_BLANKING_ON          0xA4
#define SSD1322_BLANKING_OFF         0xA6
#define SSD1322_EXIT_PARTIAL         0xA9
#define SSD1322_DISPLAY_OFF          0xAE
#define SSD1322_DISPLAY_ON           0xAF
#define SSD1322_SET_PHASE_LENGTH     0xB1
#define SSD1322_SET_CLOCK_DIV        0xB3
#define SSD1322_SET_GPIO             0xB5
#define SSD1322_SET_SECOND_PRECHARGE 0xB6
#define SSD1322_DEFAULT_GREYSCALE    0xB9
#define SSD1322_SET_PRECHARGE        0xBB
#define SSD1322_SET_VCOMH            0xBE
#define SSD1322_SET_CONTRAST         0xC1
#define SSD1322_SET_MUX_RATIO        0xCA
#define SSD1322_COMMAND_LOCK         0xFD

#endif /* end of include guard: __SSD1322_REGS_H__ */
