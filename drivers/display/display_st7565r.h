/*
 * Copyright (c) 2025 Giyora Haroon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ST7565R_H
#define ST7565R_H

/*
 * Display Control Commands
 */

#define ST7565R_DISPLAY_OFF                      0xAE
#define ST7565R_DISPLAY_ON                       0xAF

/* Sets the display RAM display start line address (0-63). Command format: 01xxxxxx */
#define ST7565R_SET_START_LINE_CMD               0x40
#define ST7565R_SET_START_LINE_VAL_MASK          0x3F

/* When RAM data is 1, the pixel is ON */
#define ST7565R_SET_NORMAL_DISPLAY               0xA6
/* When RAM data is 0, the pixel is ON */
#define ST7565R_SET_REVERSE_DISPLAY              0xA7

/* Forces the entire display ON, regardless of RAM content */
#define ST7565R_SET_ENTIRE_DISPLAY_ON            0xA5
/* Returns display to normal RAM content dependent mode */
#define ST7565R_SET_ENTIRE_DISPLAY_OFF           0xA4


/*
 * Addressing Setting Commands
 */

/* Sets the display RAM page address (PAGE0 ~ PAGE8). Command format: 1011xxxx */
#define ST7565R_SET_PAGE_START_ADDRESS_CMD       0xB0
#define ST7565R_SET_PAGE_START_ADDRESS_VAL_MASK  0x0F

/* Sets the display RAM column address upper 4 bits (A7-A4). Command format: 0001xxxx */
#define ST7565R_SET_HIGHER_COL_ADDRESS_CMD       0x10
#define ST7565R_SET_HIGHER_COL_ADDRESS_VAL_MASK  0x0F

/* Sets the display RAM column address lower 4 bits (A3-A0). Command format: 0000xxxx */
#define ST7565R_SET_LOWER_COL_ADDRESS_CMD        0x00
#define ST7565R_SET_LOWER_COL_ADDRESS_VAL_MASK   0x0F


/*
 * Hardware Configuration Commands
 */

/* Select Normal (ADC=0) or Reversed (ADC=1) segment driver direction */
#define ST7565R_SET_SEGMENT_MAP_NORMAL           0xA0 /* SEG0 -> SEG131 */
#define ST7565R_SET_SEGMENT_MAP_REVERSED         0xA1 /* SEG131 -> SEG0 */

/* Select the scan direction of the COM output terminal */
#define ST7565R_SET_COM_OUTPUT_SCAN_NORMAL       0xC0 /* COM0 -> COM63 */
#define ST7565R_SET_COM_OUTPUT_SCAN_REVERSED     0xC8 /* COM63 -> COM0 */

/* Select LCD Bias Ratio */
#define ST7565R_SET_LCD_BIAS_9                   0xA2 /* 1/9 Bias */
#define ST7565R_SET_LCD_BIAS_7                   0xA3 /* 1/7 Bias */


/*
 * Timing and Driving Scheme Setting Commands
 */

/* Control internal power supply circuits. Command format: 00101xxx */
#define ST7565R_POWER_CONTROL_CMD                0x28
#define ST7565R_POWER_CONTROL_VB_MASK            0x04 /* Bit D2: Voltage Booster */
#define ST7565R_POWER_CONTROL_VR_MASK            0x02 /* Bit D1: Voltage Regulator */
#define ST7565R_POWER_CONTROL_VF_MASK            0x01 /* Bit D0: Voltage Follower */
#define ST7565R_POWER_CONTROL_ALL_ON_MASK (ST7565R_POWER_CONTROL_VB_MASK | \
                                           ST7565R_POWER_CONTROL_VR_MASK | \
                                           ST7565R_POWER_CONTROL_VF_MASK) /* = 0x07 */

/* Set internal resistor ratio for Vo regulation. Command format: 00100xxx */
#define ST7565R_SET_RESISTOR_RATIO_CMD           0x20
#define ST7565R_SET_RESISTOR_RATIO_VAL_MASK      0x07

/* 
 * This command makes it possible to adjust the contrast of the liquid
 * crystal display by controlling the LCD drive voltage V0 through the 
 * output from the voltage regulator circuits of the internal liquid 
 * crystal power supply. This command is a double byte command used as
 * a pair with the electronic volume mode set command and the electronic
 * volume register set command, and both commands must be issued one
 * after the other. 
 */
#define ST7565R_SET_CONTRAST_CTRL_CMD            0x81
/* Second byte is 6-bit value 0-63 (0x00 - 0x3F) */
#define ST7565R_SET_CONTRAST_VALUE_MASK          0x3F

/* Set Booster Ratio. Double byte command. */
#define ST7565R_SET_BOOSTER_RATIO_SET_CMD        0xF8
/* Second byte sets booster ratio */
#define ST7565R_SET_BOOSTER_RATIO_2X_3X_4X       0x00 /* Value for 2x, 3x, 4x (default) */
#define ST7565R_SET_BOOSTER_RATIO_5X             0x01 /* Value for 5x */
#define ST7565R_SET_BOOSTER_RATIO_6X             0x03 /* Value for 6x */


/*
 * Sleep Mode Commands (Double Byte)
 */

/* Preceding command byte for Sleep Mode Set */
#define ST7565R_SLEEP_MODE_ENTER_CMD             0xAC /* Enter Sleep Mode */
#define ST7565R_SLEEP_MODE_EXIT_CMD              0xAD /* Exit Sleep Mode (Normal) */
/* Following command byte for Sleep Mode Set */
#define ST7565R_SLEEP_MODE_FOLLOW_BYTE           0x00


/*
 * Other Commands
 */

/* Software Reset */
#define ST7565R_RESET                            0xE2

/* NOP (No Operation) */
#define ST7565R_NOP                              0xE3

/* time constants in ms */

#define ST7565R_RESET_DELAY                      1

#endif /* ST7565R_H */
