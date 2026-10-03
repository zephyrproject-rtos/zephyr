/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TAS2560_H_
#define ZEPHYR_DRIVERS_AUDIO_TAS2560_H_

#define TAS2560_PAGE              0x00
#define TAS2560_RESET             0x01
#define TAS2560_SPK_CTRL          0x04
#define TAS2560_PWR_CTRL_1        0x07
#define TAS2560_RAMP_CTRL         0x08
#define TAS2560_EDGE_ISNS_BOOST   0x09
#define TAS2560_DECIMATION        0x0D
#define TAS2560_INTERPOLATION     0x0E
#define TAS2560_PLL_CLKIN         0x0F
#define TAS2560_PLL_JVAL          0x10
#define TAS2560_PLL_DVAL_1        0x11
#define TAS2560_PLL_DVAL_2        0x12
#define TAS2560_ASI_FORMAT        0x14
#define TAS2560_ASI_CHANNEL       0x15
#define TAS2560_CLK_ERR_2         0x22
#define TAS2560_PCM_RATE          0x36
#define TAS2560_CLOCK_ERR_CFG_2   0x50
#define TAS2560_BOOK              0x7F

/* SPK_CTRL resets to this value. Used to tell the part from TAS2563. */
#define TAS2560_SPK_CTRL_RESET    0x5F
/* Class-H, 15 dB, matching the module vendor playback setup. */
#define TAS2560_SPK_CTRL_PLAY     0x4F
#define TAS2560_PWR_SHUTDOWN      0x00
#define TAS2560_PWR_ACTIVE        0x80
#define TAS2560_PLL_CLKIN_BCLK    0x01
/* PLL output used by the 16-bit stereo playback setup (49.152 MHz). */
#define TAS2560_PLL_OUT_HZ        49152000
#define TAS2560_EDGE_8OHM_3A      0x83
#define TAS2560_ASI_LEN_MASK      0x03

#endif
