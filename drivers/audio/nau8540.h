/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_NAU8540_H_
#define ZEPHYR_DRIVERS_AUDIO_NAU8540_H_

#define NAU8540_REG_SW_RESET           0x00
#define NAU8540_REG_POWER_MANAGEMENT   0x01
#define NAU8540_REG_CLOCK_CTRL         0x02
#define NAU8540_REG_CLOCK_SRC          0x03
#define NAU8540_REG_FLL1               0x04
#define NAU8540_REG_FLL2               0x05
#define NAU8540_REG_FLL3               0x06
#define NAU8540_REG_FLL4               0x07
#define NAU8540_REG_FLL5               0x08
#define NAU8540_REG_FLL6               0x09
#define NAU8540_REG_FLL_VCO_RSV        0x0A
#define NAU8540_REG_PCM_CTRL0          0x10
#define NAU8540_REG_PCM_CTRL1          0x11
#define NAU8540_REG_PCM_CTRL2          0x12
#define NAU8540_REG_PCM_CTRL3          0x13
#define NAU8540_REG_PCM_CTRL4          0x14
#define NAU8540_REG_ALC_CONTROL_1      0x20
#define NAU8540_REG_ALC_CONTROL_2      0x21
#define NAU8540_REG_ALC_CONTROL_3      0x22
#define NAU8540_REG_ALC_CONTROL_4      0x23
#define NAU8540_REG_ALC_CONTROL_5      0x24
#define NAU8540_REG_HPF_FILTER_CH12    0x38
#define NAU8540_REG_HPF_FILTER_CH34    0x39
#define NAU8540_REG_ADC_SAMPLE_RATE    0x3A
#define NAU8540_REG_DIGITAL_GAIN_CH1   0x40
#define NAU8540_REG_DIGITAL_GAIN_CH2   0x41
#define NAU8540_REG_DIGITAL_GAIN_CH3   0x42
#define NAU8540_REG_DIGITAL_GAIN_CH4   0x43
#define NAU8540_REG_DIGITAL_MUX        0x44
#define NAU8540_REG_PEAK_CH1           0x4C
#define NAU8540_REG_PEAK_CH2           0x4D
#define NAU8540_REG_PEAK_CH3           0x4E
#define NAU8540_REG_PEAK_CH4           0x4F
#define NAU8540_REG_I2C_CTRL           0x52
#define NAU8540_REG_I2C_DEVICE_ID      0x58
#define NAU8540_REG_RST                0x5A
#define NAU8540_REG_VMID_CTRL          0x60
#define NAU8540_REG_MUTE               0x61
#define NAU8540_REG_ANALOG_ADC1        0x64
#define NAU8540_REG_ANALOG_ADC2        0x65
#define NAU8540_REG_ANALOG_PWR         0x66
#define NAU8540_REG_MIC_BIAS           0x67
#define NAU8540_REG_REFERENCE          0x68
#define NAU8540_REG_FEPGA1             0x69
#define NAU8540_REG_FEPGA2             0x6A
#define NAU8540_REG_FEPGA3             0x6B
#define NAU8540_REG_FEPGA4             0x6C
#define NAU8540_REG_PWR                0x6D

#define NAU8540_ADC1_EN                BIT(0)
#define NAU8540_ADC2_EN                BIT(1)
#define NAU8540_ADC3_EN                BIT(2)
#define NAU8540_ADC4_EN                BIT(3)
#define NAU8540_ADC_ALL_EN             0x000f

#define NAU8540_CLK_ADC_EN             BIT(15)
#define NAU8540_CLK_AGC_EN             BIT(3)
#define NAU8540_CLK_I2S_EN             BIT(1)

#define NAU8540_CLK_SRC_VCO            BIT(15)
#define NAU8540_CLK_ADC_SRC_SFT        6
#define NAU8540_CLK_ADC_SRC_DIV2       (0x1 << NAU8540_CLK_ADC_SRC_SFT)
#define NAU8540_CLK_MCLK_SRC_MASK      0x000f
#define NAU8540_MCLK_SRC_DIV4          0x0003
#define NAU8540_I2S_LRC_DIV_SFT        12
#define NAU8540_I2S_LRC_DIV_32         (0x3 << NAU8540_I2S_LRC_DIV_SFT)
#define NAU8540_I2S_BCLK_DIV_8         0x0003
#define NAU8540_ALC_CONTROL_3_DEFAULT  0x0022

#define NAU8540_FLL_RATIO_MASK         0x007f
#define NAU8540_ICTRL_LATCH_SFT        10
#define NAU8540_GAIN_ERR_SFT           12
#define NAU8540_FLL_CLK_SRC_FS         (0x3 << 10)
#define NAU8540_FLL_INTEGER_MASK       0x03ff
#define NAU8540_FLL_REF_DIV_SFT        10
#define NAU8540_DCO_EN                 BIT(15)
#define NAU8540_SDM_EN                 BIT(14)
#define NAU8540_FLL_VCO_RSV_DCO        0xF13C

#define NAU8540_ALC_CH1_EN             BIT(12)
#define NAU8540_ALC_CH2_EN             BIT(13)
#define NAU8540_ALC_CH_ALL_EN          (0xF << 12)

#define NAU8540_I2S_DL_SFT             2
#define NAU8540_I2S_DL_MASK            (0x3 << NAU8540_I2S_DL_SFT)
#define NAU8540_I2S_DL_16              (0x0 << NAU8540_I2S_DL_SFT)
#define NAU8540_I2S_DL_20              (0x1 << NAU8540_I2S_DL_SFT)
#define NAU8540_I2S_DL_24              (0x2 << NAU8540_I2S_DL_SFT)
#define NAU8540_I2S_DL_32              (0x3 << NAU8540_I2S_DL_SFT)
#define NAU8540_I2S_DF_MASK            0x0003
#define NAU8540_I2S_DF_RIGHT           0x0000
#define NAU8540_I2S_DF_LEFT            0x0001
#define NAU8540_I2S_DF_I2S             0x0002
#define NAU8540_I2S_DF_PCM_AB          0x0003
#define NAU8540_I2S_DO12_TRI           BIT(15)
#define NAU8540_I2S_DO12_OE            BIT(4)
#define NAU8540_I2S_MS_MASTER          BIT(3)
#define NAU8540_I2S_DO34_TRI           BIT(15)

#define NAU8540_CH_SYNC                BIT(14)
#define NAU8540_ADC_OSR_MASK           0x0003
#define NAU8540_ADC_OSR_64             0x0002

#define NAU8540_DIGITAL_GAIN_0DB       0x0400
#define NAU8540_DIGITAL_GAIN_CH1_0DB   0x0400
#define NAU8540_DIGITAL_GAIN_CH2_0DB   0x1400
#define NAU8540_DIGITAL_GAIN_CH3_0DB   0x2400
#define NAU8540_DIGITAL_GAIN_CH4_0DB   0x3400
#define NAU8540_DIGITAL_GAIN_MAX_STEPS 288

#define NAU8540_VMID_EN                BIT(6)
#define NAU8540_VMID_SEL_SFT           4
#define NAU8540_PGA_CH_ALL_MUTE        0x000f
#define NAU8540_PU_PRE                 BIT(8)
#define NAU8540_PRECHARGE_DIS          BIT(13)
#define NAU8540_GLOBAL_BIAS_EN         BIT(12)
#define NAU8540_DISCHRG_EN             BIT(14)
#define NAU8540_GAIN_ERR_FS            (0xf << NAU8540_GAIN_ERR_SFT)
#define NAU8540_ICTRL_LATCH_6          (0x6 << NAU8540_ICTRL_LATCH_SFT)
#define NAU8540_FEPGA_AA_16KHZ         0x0011

#define NAU8540_CLK_SRC_FLL_FS         0
#define NAU8540_CLK_SRC_INTERNAL       1

#endif /* ZEPHYR_DRIVERS_AUDIO_NAU8540_H_ */
