/*
 * Copyright 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_DISPLAY_ST7796S_H_
#define _ZEPHYR_DRIVERS_DISPLAY_ST7796S_H_

#define ST7796S_CMD_SLPIN	0x10 /* Sleep in */
#define ST7796S_CMD_SLPOUT	0x11 /* Sleep out */
#define ST7796S_CMD_INVOFF	0x20 /* Display inversion off */
#define ST7796S_CMD_INVON	0x21 /* Display inversion on */
#define ST7796S_CMD_CASET	0x2A /* Column address set */
#define ST7796S_CMD_RASET	0x2B /* Row address set */
#define ST7796S_CMD_RAMWR	0x2C /* Memory write */
#define ST7796S_CMD_DISPOFF	0x28 /* Display off */
#define ST7796S_CMD_DISPON	0x29 /* Display on */
#define ST7796S_CMD_TEON	0x35 /* Tearing effect on */
#define ST7796S_CMD_MADCTL	0x36 /* Memory data access control */
#define ST7796S_CMD_COLMOD	0x3A /* Interface pixel format */
#define ST7796S_CMD_FRMCTR1	0xB1 /* Frame rate control 1 (normal mode) */
#define ST7796S_CMD_FRMCTR2	0xB2 /* Frame rate control 2 (idle mode) */
#define ST7796S_CMD_FRMCTR3	0xB3 /* Frame rate control 3 (partial mode) */
#define ST7796S_CMD_DIC		0xB4 /* Display inversion control */
#define ST7796S_CMD_BPC		0xB5 /* Blanking porch control */
#define ST7796S_CMD_DFC		0xB6 /* Display function control */
#define ST7796S_CMD_PWR1	0xC0 /* Power control 1 */
#define ST7796S_CMD_PWR2	0xC1 /* Power control 1 */
#define ST7796S_CMD_PWR3	0xC2 /* Power control 1 */
#define ST7796S_CMD_VCMPCTL	0xC5 /* VCOM control */
#define ST7796S_CMD_PGC		0xE0 /* Positive gamma control */
#define ST7796S_CMD_NGC		0xE1 /* Negative gamma control */
#define ST7796S_CMD_DOCA	0xE8 /* Display output control adjust */
#define ST7796S_CMD_CSCON	0xF0 /* Command set control */

#define ST7796S_CONTROL_16BIT 0x5 /* Sets control interface to 16 bit mode */
#define ST7796S_MADCTL_BGR BIT(3) /* Sets BGR color mode */


#endif /* _ZEPHYR_DRIVERS_DISPLAY_ST7796S_H_ */
