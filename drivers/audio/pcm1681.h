/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_AUDIO_PCM1681_H_
#define ZEPHYR_DRIVERS_AUDIO_PCM1681_H_

#define PCM1681_N_CHANNELS 8

/* +0.5 for rounding */
#define VOL2ATT_FINE(vol) (127 * (vol) / 100 + 128 + 0.5)
#define VOL2ATT_WIDE(vol) (101 * (vol) / 100 + 154 + 0.5)

#define PCM1681_ATxx_MASK 0xFF
#define PCM1681_ATxx_POS  0
#define PCM1681_AT1x_REG  1
#define PCM1681_AT2x_REG  2
#define PCM1681_AT3x_REG  3
#define PCM1681_AT4x_REG  4
#define PCM1681_AT5x_REG  5
#define PCM1681_AT6x_REG  6
#define PCM1681_AT7x_REG  16
#define PCM1681_AT8x_REG  17

#define PCM1681_MUTx_MASK    0xFF
#define PCM1681_MUTx_POS     0
#define PCM1681_MUTx_REG     7
#define PCM1681_MUT_OR_MASK  0x3
#define PCM1681_MUT_OR_POS   0
#define PCM1681_MUT_OR_REG   18
#define PCM1681_MUT_DISABLED 0x00
#define PCM1681_MUT_ENABLED  0x01

#define PCM1681_DACx_MASK    0xFF
#define PCM1681_DACx_POS     0
#define PCM1681_DACx_REG     8
#define PCM1681_DAC_OR_MASK  0x3
#define PCM1681_DAC_OR_POS   0
#define PCM1681_DAC_OR_REG   19
#define PCM1681_DAC_ENABLED  0x00
#define PCM1681_DAC_DISABLED 0x01

#define PCM1681_FLT_MASK           0x20
#define PCM1681_FLT_POS            5
#define PCM1681_FLT_REG            9
#define PCM1681_FLTx_MASK          0x0F
#define PCM1681_FLTx_POS           0
#define PCM1681_FLTx_REG           12
#define PCM1681_FLT_SHARP_ROLL_OFF 0x00
#define PCM1681_FLT_SLOW_ROLL_OFF  0x01

#define PCM1681_FMTx_MASK                 0x0F
#define PCM1681_FMTx_POS                  0
#define PCM1681_FMTx_REG                  9
#define PCM1681_FMT_RIGHT_JUSTIFIED_24    0x00
#define PCM1681_FMT_RIGHT_JUSTIFIED_16    0x03
#define PCM1681_FMT_I2S_16_24             0x04
#define PCM1681_FMT_LEFT_JUSTIFIED_16_24  0x05
#define PCM1681_FMT_I2S_TDM_24            0x06
#define PCM1681_FMT_LEFT_JUSTIFIED_TDM_24 0x07
#define PCM1681_FMT_I2S_DSP_24            0x08
#define PCM1681_FMT_LEFT_JUSTIFIED_DSP_24 0x09

#define PCM1681_SRST_MASK     0x80
#define PCM1681_SRST_POS      7
#define PCM1681_SRST_REG      10
#define PCM1681_SRST_DISABLED 0x00
#define PCM1681_SRST_ENABLED  0x01

#define PCM1681_ZREV_MASK 0x40
#define PCM1681_ZREV_POS  6
#define PCM1681_ZREV_REG  10
#define PCM1681_ZREV_HIGH 0x00
#define PCM1681_ZREV_LOW  0x01

#define PCM1681_DREV_MASK     0x20
#define PCM1681_DREV_POS      5
#define PCM1681_DREV_REG      10
#define PCM1681_DREV_NORMAL   0x00
#define PCM1681_DREV_INVERTED 0x01

#define PCM1681_DMFx_MASK 0x18
#define PCM1681_DMFx_POS  3
#define PCM1681_DMFx_REG  10
#define PCM1681_DMF_44100 0x00
#define PCM1681_DMF_48000 0x01
#define PCM1681_DMF_32000 0x02

#define PCM1681_DMC_MASK     0x01
#define PCM1681_DMC_POS      0
#define PCM1681_DMC_REG      10
#define PCM1681_DMC_DISABLED 0x00
#define PCM1681_DMC_ENABLED  0x01

#define PCM1681_REVx_MASK    0xFF
#define PCM1681_REVx_POS     0
#define PCM1681_REVx_REG     11
#define PCM1681_REV_NORMAL   0x00
#define PCM1681_REV_INVERTED 0x01

#define PCM1681_OVER_MASK   0x80
#define PCM1681_OVER_POS    7
#define PCM1681_OVER_REG    12
#define PCM1681_OVER_NARROW 0x00
#define PCM1681_OVER_WIDE   0x01

#define PCM1681_DAMS_MASK 0x80
#define PCM1681_DAMS_POS  7
#define PCM1681_DAMS_REG  13
#define PCM1681_DAMS_FINE 0x00
#define PCM1681_DAMS_WIDE 0x01

#define PCM1681_AZROx_MASK 0x60
#define PCM1681_AZROx_POS  5
#define PCM1681_AZROx_REG  13
#define PCM1681_AZRO_A     0x00
#define PCM1681_AZRO_B     0x01
#define PCM1681_AZRO_C     0x02
#define PCM1681_AZRO_D     0x03

#define PCM1681_REG_0  0x00
#define PCM1681_REG_14 0x0E
#define PCM1681_REG_15 0x0F

#define PCM1681_N_REGISTERS 20
#define PCM1681_DEFAULT_REG_MAP                                                                    \
	{                                                                                          \
		0x00, /* register 0x00. Factory use only */                                        \
		0xFF, /* register 0x01 */                                                          \
		0xFF, /* register 0x02 */                                                          \
		0xFF, /* register 0x03 */                                                          \
		0xFF, /* register 0x04 */                                                          \
		0xFF, /* register 0x05 */                                                          \
		0xFF, /* register 0x06 */                                                          \
		0x00, /* register 0x07 */                                                          \
		0x00, /* register 0x08 */                                                          \
		0x05, /* register 0x09 */                                                          \
		0x00, /* register 0x0A */                                                          \
		0xFF, /* register 0x0B */                                                          \
		0x0F, /* register 0x0C */                                                          \
		0x00, /* register 0x0D */                                                          \
		0x00, /* register 0x0E. Read only */                                               \
		0x00, /* register 0x0F. Factory use only */                                        \
		0xFF, /* register 0x10 */                                                          \
		0xFF, /* register 0x11 */                                                          \
		0x00, /* register 0x12 */                                                          \
		0x00, /* register 0x13 */                                                          \
	}

#endif /* ZEPHYR_DRIVERS_AUDIO_PCM1681_H_ */
