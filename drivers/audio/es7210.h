/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_ES7210_H_
#define ZEPHYR_DRIVERS_AUDIO_ES7210_H_

#include <zephyr/sys/util.h>

#define ES7210_RESET          0x00U
#define ES7210_CLOCK_OFF      0x01U
#define ES7210_MAINCLK        0x02U
#define ES7210_MASTER_CLK     0x03U
#define ES7210_LRCK_HIGH      0x04U
#define ES7210_LRCK_LOW       0x05U
#define ES7210_POWER_DOWN     0x06U
#define ES7210_OSR            0x07U
#define ES7210_MODE           0x08U
#define ES7210_TIME_CONTROL0  0x09U
#define ES7210_TIME_CONTROL1  0x0aU
#define ES7210_STATUS         0x0bU
#define ES7210_DMIC           0x10U
#define ES7210_SDP1           0x11U
#define ES7210_SDP2           0x12U
#define ES7210_MUTE34         0x14U
#define ES7210_MUTE12         0x15U
#define ES7210_ALC_SELECT     0x16U
#define ES7210_ALC_RAMP       0x17U
#define ES7210_ALC_LEVEL34    0x18U
#define ES7210_ALC_LEVEL12    0x19U
#define ES7210_GAIN_RAMP      0x1aU
#define ES7210_ADC1_GAIN      0x1eU
#define ES7210_HPF34_2        0x20U
#define ES7210_HPF34_1        0x21U
#define ES7210_HPF12_2        0x22U
#define ES7210_HPF12_1        0x23U
#define ES7210_REVISION       0x3fU
#define ES7210_ANALOG         0x40U
#define ES7210_BIAS12         0x41U
#define ES7210_BIAS34         0x42U
#define ES7210_MIC1_GAIN      0x43U
#define ES7210_MIC1_LOW_POWER 0x47U
#define ES7210_POWER12        0x4bU
#define ES7210_POWER34        0x4cU

#define ES7210_MODE_CONTROLLER        BIT(0)
#define ES7210_MODE_DOUBLE_SPEED      BIT(1)
#define ES7210_MODE_EQ_BYPASS         BIT(2)
#define ES7210_MODE_BCLK_INVERT       BIT(3)
#define ES7210_MODE_LRCK_RATE_DEFAULT BIT(4)

#define ES7210_SDP1_LRCK_INVERT     BIT(4)
#define ES7210_SDP1_WORD_SIZE_MASK  GENMASK(7, 5)
#define ES7210_SDP2_SDOUT1_TRISTATE BIT(4)
#define ES7210_SDP2_SDOUT2_TRISTATE BIT(5)
#define ES7210_SDP2_TDM_DSP         1U
#define ES7210_SDP2_TDM_I2S         2U

#define ES7210_MIC_SELECT        BIT(4)
#define ES7210_HPF_FREEZE_OFFSET BIT(5)
#define ES7210_HPF_INVERT_FIRST  BIT(6)
#define ES7210_HPF_INVERT_SECOND BIT(7)
#define ES7210_HPF_ROUTE_MASK    GENMASK(7, 6)

#define ES7210_ANALOG_POWER_DOWN BIT(7)
#define ES7210_ANALOG_VX1_SELECT BIT(5)
#define ES7210_ANALOG_VX2_OFF    BIT(6)
#define ES7210_BIAS_VOLTAGE_MASK GENMASK(6, 4)

#endif /* ZEPHYR_DRIVERS_AUDIO_ES7210_H_ */
