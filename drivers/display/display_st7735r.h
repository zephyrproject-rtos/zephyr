/*
 * Copyright (c) 2020 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ST7735R_DISPLAY_DRIVER_H__
#define ST7735R_DISPLAY_DRIVER_H__

#include <zephyr/zephyr.h>

#define ST7735R_CMD_SW_RESET            0x01
#define ST7735R_CMD_RDDID               0x04
#define ST7735R_CMD_RDDST               0x09
#define ST7735R_CMD_RDDPM               0x0A
#define ST7735R_CMD_RDD_MADCTL          0x0B
#define ST7735R_CMD_RDD_COLMOD          0x0C
#define ST7735R_CMD_RDDIM               0x0D
#define ST7735R_CMD_RDDSM               0x0E

#define ST7735R_CMD_SLEEP_IN            0x10
#define ST7735R_CMD_SLEEP_OUT           0x11
#define ST7735R_CMD_PTLON               0x12
#define ST7735R_CMD_NORON               0x13

#define ST7735R_CMD_INV_OFF             0x20
#define ST7735R_CMD_INV_ON              0x21
#define ST7735R_CMD_GAMSET              0x26
#define ST7735R_CMD_DISP_OFF            0x28
#define ST7735R_CMD_DISP_ON             0x29
#define ST7735R_CMD_CASET               0x2a
#define ST7735R_CMD_RASET               0x2b
#define ST7735R_CMD_RAMWR               0x2c
#define ST7735R_CMD_RGBSET              0x2D
#define ST7735R_CMD_RAMRD               0x2E

#define ST7735R_CMD_PTLAR               0x30
#define ST7735R_CMD_TEOFF               0x34
#define ST7735R_CMD_TEON                0x35
#define ST7735R_CMD_MADCTL              0x36
#define ST7735R_CMD_IDMOFF              0x38
#define ST7735R_CMD_IDMON               0x39
#define ST7735R_CMD_COLMOD              0x3a

#define ST7735R_CMD_FRMCTR1             0xB1
#define ST7735R_CMD_FRMCTR2             0xB2
#define ST7735R_CMD_FRMCTR3             0xB3
#define ST7735R_CMD_INVCTR              0xB4

#define ST7735R_CMD_PWCTR1              0xC0
#define ST7735R_CMD_PWCTR2              0xC1
#define ST7735R_CMD_PWCTR3              0xC2
#define ST7735R_CMD_PWCTR4              0xC3
#define ST7735R_CMD_PWCTR5              0xC4
#define ST7735R_CMD_VMCTR1              0xC5
#define ST7735R_CMD_VMOFCTR             0xC7

#define ST7735R_CMD_WRID2               0xD1
#define ST7735R_CMD_WRID3               0xD2
#define ST7735R_CMD_NVCTR1              0xD9
#define ST7735R_CMD_NVCTR2              0xDE
#define ST7735R_CMD_NVCTR3              0xDF
#define ST7735R_CMD_RDID1               0xDA
#define ST7735R_CMD_RDID2               0xDB
#define ST7735R_CMD_RDID3               0xDC
#define ST7735R_CMD_NVCTR2              0xDE
#define ST7735R_CMD_NVCTR3              0xDF

#define ST7735R_CMD_GAMCTRP1            0xE0
#define ST7735R_CMD_GAMCTRN1            0xE1

/* CMD_MADCTL bits */
#define ST7735R_MADCTL_RBG                      0x00
#define ST7735R_MADCTL_BGR                      0x08


#endif  /* ST7735R_DISPLAY_DRIVER_H__ */
