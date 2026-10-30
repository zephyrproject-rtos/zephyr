/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TAS2560_H_
#define ZEPHYR_DRIVERS_AUDIO_TAS2560_H_

#include <zephyr/sys/util_macro.h>

#define TAS2560_PAGE            0x00
#define TAS2560_RESET           0x01
#define TAS2560_MODE            0x02
#define TAS2560_SPK_CTRL        0x04
#define TAS2560_PWR_CTRL_1      0x07
#define TAS2560_PLL_CLKIN       0x0F
#define TAS2560_PLL_JVAL        0x10
#define TAS2560_PLL_DVAL_1      0x11
#define TAS2560_PLL_DVAL_2      0x12
#define TAS2560_ASI_FORMAT      0x14
#define TAS2560_ASI_CHANNEL     0x15
#define TAS2560_PCM_RATE        0x36
#define TAS2560_CLOCK_ERR_CFG_2 0x50
#define TAS2560_BOOK            0x7F

/* DSP_MODE = 2: PCM playback with PCM IV sense. Reset is mode 1. */
#define TAS2560_DSP_MODE_PCM_IV 0x02

/* SPK_CTRL resets to this value. Used to tell the part from TAS2563. */
#define TAS2560_SPK_CTRL_RESET  0x5F
/* BST_MODE = 0 (Class-H), DAC_GAIN = 15 dB. */
#define TAS2560_SPK_CTRL_PLAY   0x4F

/*
 * PWR_DEV is bits 7:6. Section 9.6.1 unmutes with 0x40 (power up with
 * boost). 0x41 is that mode plus MUTE_AUDIO, used while the DSP patch loads.
 * 0x80 is power up without boost and is not used with Class-H.
 */
#define TAS2560_PWR_SHUTDOWN    0x00
#define TAS2560_PWR_ACTIVE      0x40
#define TAS2560_PWR_MUTE        0x41
#define TAS2560_MUTE_AUDIO      BIT(0)

/* Playback script wait. The reset bit is self-clearing. */
#define TAS2560_RESET_WAIT_MS   16

/* PLL_CLK_SRC = BCLK, P divider = 1. Reset is 0x41 (MCLK). */
#define TAS2560_PLL_CLKIN_BCLK  0x01
#define TAS2560_PLL_LOWF        BIT(7)

/* ASI_MODE = I2S, ASI_LENGTH = 16 bits. Reset length is 24 bits. */
#define TAS2560_ASI_I2S_16      0x00

/* PCM_RATE[1:0]. Reserved bits 5:2 reset to 0x3 and are left unchanged. */
#define TAS2560_PCM_RATE_MASK   0x03
#define TAS2560_PCM_RATE_8KHZ   0x00
#define TAS2560_PCM_RATE_16KHZ  0x01
#define TAS2560_PCM_RATE_48KHZ  0x02
#define TAS2560_PCM_RATE_96KHZ  0x03

/* Reset is 0x11. 0x31 selects a 1.36 ms CLK_ERR1 timeout. */
#define TAS2560_CLOCK_ERR_PLAY  0x31

#endif
