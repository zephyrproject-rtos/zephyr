/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_REGS_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_REGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_CONTROL_ADDR 0x00

#define SOFT_RESET_ADDR   0x01
#define SOFT_RESET_ASSERT 0x80

#define CODEC_DATAPATH_SETUP 0x07

#define ASI_CTRL_A             0x08
#define ASI_CTRL_A_HIZ_DOUT    0x20

#define ASI_CTRL_A_BCLK_MASTER 0x80
#define ASI_CTRL_A_WCLK_MASTER 0x40

#define ASI_CTRL_B                      0x09

#define ASI_CTRL_B_MODE_I2S             0x00
#define ASI_CTRL_B_MODE_DSP             0x40
#define ASI_CTRL_B_MODE_RIGHT_JUSTIFIED 0x80
#define ASI_CTRL_B_MODE_LEFT_JUSTIFIED  0xC0

#define ASI_CTRL_B_WORDLEN_16           0x00
#define ASI_CTRL_B_WORDLEN_20           0x10
#define ASI_CTRL_B_WORDLEN_24           0x20
#define ASI_CTRL_B_WORDLEN_32           0x30

#define ASI_CTRL_C               0x0A
#define CODEC_FILTER             0x0C
#define HEADSET_DETECT_B         0x0E
#define HEADSET_DETECT_B_DIFF_AC 0xC0

#define NDAC_NADC                       0x02
#define PLL_PROG_A                      0x03
#define PLL_PROG_B                      0x04
#define PLL_PROG_C                      0x05
#define PLL_PROG_D                      0x06

#define CODEC_DATAPATH_FSREF_FAMILY_BIT 0x80
#define AUDIO_CODEC_OVERFLOW_FLAG       0x0B

#define CLOCK_REG 0x65
#define R101_CODEC_CLKIN_PLLDIV_OUT 0x00
#define R101_CODEC_CLKIN_CLKDIV_OUT 0x01

#define R3_PLL_ENABLE_BIT 0x80

#define DAC_POWER_DRV 0x25
#define DAC_POWER_ON  0xC0

#define HP_OUTPUT_STAGE     0x28
#define HP_OUTPUT_1P5V_SOFT 0x40

#define OUTPUT_POP_REDUCTION 0x2A

#define OUTPUT_POP_800MS_BG  0x92

#define LEFT_DAC_VOL         0x2B
#define RIGHT_DAC_VOL        0x2C
#define DAC_L1_TO_HPLOUT_VOL 0x2F
#define HP_LEVEL_0DB_UNMUTED_POWERED 0x09
#define HPLOUT_LEVEL         0x33
#define DAC_R1_TO_HPROUT_VOL 0x40
#define HPROUT_LEVEL         0x41

#define DAC_VOL_0DB  0x00
#define DAC_VOL_MUTE 0x80

#define DAC_TO_OUT_ROUTED_0DB   0x80
#define DAC_TO_OUT_ROUTE_BIT    0x80
#define DAC_TO_OUT_ROUTED_MUTE  0xF6
#define DAC_L1_TO_LEFT_LOP_VOL  0x52
#define LOP_LEVEL_0DB_UNMUTED_POWERED 0x09
#define LEFT_LOP_LEVEL          0x56
#define DAC_R1_TO_RIGHT_LOP_VOL 0x5C
#define RIGHT_LOP_LEVEL         0x5D
#define DAC_QUIESCENT           0x6D

#define CODEC_DATAPATH_DAC_STEREO      0x0A
#define CODEC_DATAPATH_DAC_MONO        0x1E
#define CODEC_DATAPATH_MODE_FIELD_MASK 0x1E

#define LEFT_ADC_PGA_GAIN     0x0F
#define RIGHT_ADC_PGA_GAIN    0x10
#define ADC_PGA_MUTE          0x80

#define ADC_PGA_GAIN_MAX_CODE 0x77

#define MIC2_LINE2_TO_LADC      0x11
#define MIC2_LINE2_TO_RADC      0x12
#define LINE2_TO_ADC_STEREO_L   0x0F
#define LINE2_TO_ADC_STEREO_R   0xF0
#define LINE2_TO_ADC_MONO       0x44
#define LINE2_TO_ADC_DISCONNECT 0xFF

#define LINE1L_TO_LADC_CTRL              0x13
#define LINE1R_TO_RADC_CTRL              0x16
#define ADC_CHANNEL_POWER_BIT            0x04

#define LINE1_TO_ADC_FIELD_PRESERVE_MASK 0x87
#define LINE1_TO_ADC_CONNECT_0DB         0x00
#define LINE1_TO_ADC_DISCONNECT          0x78

#define MICBIAS_CTRL 0x19
#define MICBIAS_OFF  0x00
#define MICBIAS_2V   0x40
#define MICBIAS_2V5  0x80
#define MICBIAS_AVDD 0xC0

#define HP_OUTPUT_DRIVER_CTRL                 0x26
#define HP_OUTPUT_DRIVER_SHORT_CCT_PROTECT_EN 0x04
#define HP_OUTPUT_DRIVER_SHORT_CCT_AUTO_PWRDN 0x02

#define STICKY_INTERRUPT_FLAGS                0x60
#define STICKY_INTERRUPT_FLAGS_SHORT_CCT_MASK 0xF0

#ifdef __cplusplus
}
#endif

#endif
