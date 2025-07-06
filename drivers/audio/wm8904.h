/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CODEC_WM8904_H_
#define ZEPHYR_INCLUDE_CODEC_WM8904_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define WM8904_REG_RESET                    (0x00)
#define WM8904_REG_ANALOG_ADC_0             (0x0A)
#define WM8904_REG_POWER_MGMT_0             (0x0C)
#define WM8904_REG_POWER_MGMT_2             (0x0E)
#define WM8904_REG_POWER_MGMT_3             (0x0F)
#define WM8904_REG_POWER_MGMT_6             (0x12)
#define WM8904_REG_CLK_RATES_0              (0x14)
#define WM8904_REG_CLK_RATES_1              (0x15)
#define WM8904_REG_CLK_RATES_2              (0x16)
#define WM8904_REG_AUDIO_IF_0               (0x18)
#define WM8904_REG_AUDIO_IF_1               (0x19)
#define WM8904_REG_AUDIO_IF_2               (0x1A)
#define WM8904_REG_AUDIO_IF_3               (0x1B)
#define WM8904_REG_DAC_DIG_1                (0x21)
#define WM8904_REG_DAC_DIG_0                (0x27)
#define WM8904_REG_ANALOG_LEFT_IN_0         (0x2C)
#define WM8904_REG_ANALOG_RIGHT_IN_0        (0x2D)
#define WM8904_REG_ANALOG_LEFT_IN_1         (0x2E)
#define WM8904_REG_ANALOG_RIGHT_IN_1        (0x2F)
#define WM8904_REG_ANALOG_OUT1_LEFT         (0x39)
#define WM8904_REG_ANALOG_OUT1_RIGHT        (0x3A)
#define WM8904_REG_ANALOG_OUT12_ZC          (0x3D)
#define WM8904_REG_DC_SERVO_0               (0x43)
#define WM8904_REG_ANALOG_HP_0              (0x5A)
#define WM8904_REG_CHRG_PUMP_0              (0x62)
#define WM8904_REG_CLS_W_0                  (0x68)
#define WM8904_REG_WRT_SEQUENCER_0          (0x6C)
#define WM8904_REG_WRT_SEQUENCER_3          (0x6F)
#define WM8904_REG_WRT_SEQUENCER_4          (0x70)
#define WM8904_REG_DAC_DIGITAL_VOLUME_LEFT  (0x1E)
#define WM8904_REG_DAC_DIGITAL_VOLUME_RIGHT (0x1F)
#define WM8904_REG_ADC_DIGITAL_VOLUME_LEFT  (0x24)
#define WM8904_REG_ADC_DIGITAL_VOLUME_RIGHT (0x25)
#define WM8904_REG_ANALOG_OUT2_LEFT         (0x3B)
#define WM8904_REG_ANALOG_OUT2_RIGHT        (0x3C)
#define WM8904_REG_GPIO_CONTROL_4           (0x7C)
/* FLL control register */
#define WM8904_REG_FLL_CONTROL_1 (0x74)
#define WM8904_REG_FLL_CONTROL_2 (0x75)
#define WM8904_REG_FLL_CONTROL_3 (0x76)
#define WM8904_REG_FLL_CONTROL_4 (0x77)
#define WM8904_REG_FLL_CONTROL_5 (0x78)
/* GPIO control register */
#define WM8904_REG_GPIO_CONTROL_1 (0x79)
#define WM8904_REG_GPIO_CONTROL_2 (0x7A)
#define WM8904_REG_GPIO_CONTROL_3 (0x7B)
#define WM8904_REG_GPIO_CONTROL_4 (0x7C)
/* fll nco */
#define WM8904_REG_FLL_NCO_TEST_0 (0xF7U)
#define WM8904_REG_FLL_NCO_TEST_1 (0xF8U)

#define WM8904_OUTPUT_VOLUME_MIN (0b000000)
#define WM8904_OUTPUT_VOLUME_MAX (0b111111)
#define WM8904_OUTPUT_VOLUME_DEFAULT (0b101101)
#define WM8904_INPUT_VOLUME_MIN (0b00000)
#define WM8904_INPUT_VOLUME_MAX (0b11111)
#define WM8904_INPUT_VOLUME_DEFAULT (0b00101)

/**
 * WM8904_ANALOG_OUT1_LEFT, WM8904_ANALOG_OUT1_RIGHT (headphone outs),
 * WM8904_ANALOG_OUT2_LEFT, WM8904_ANALOG_OUT2_RIGHT (line outs):
 * [8]   - MUTE: Output mute
 * [7]   - VU: Volume update, works for entire channel pair
 * [6]   - ZC: Zero-crossing enable
 * [5:0] - VOL: 6-bit volume value
 */
#define WM8904_REGVAL_OUT_VOL(mute, vu, zc, vol) \
	(((mute & 0b1) << 8) | (vu & 0b1) << 7 | (zc & 0b1) << 6 | (vol & 0b000111111))
#define WM8904_REGMASK_OUT_MUTE	0b100000000
#define WM8904_REGMASK_OUT_VU	0b010000000
#define WM8904_REGMASK_OUT_ZC	0b001000000
#define WM8904_REGMASK_OUT_VOL	0b000111111

/**
 * WM8904_ANALOG_LEFT_IN_0, WM8904_ANALOG_RIGHT_IN_0:
 * [7]   - MUTE: Input mute
 * [4:0] - VOL: 5 bit volume value
 */
#define WM8904_REGVAL_IN_VOL(mute, vol) \
	((mute & 0b1) << 7 | (vol & 0b00011111))
#define WM8904_REGMASK_IN_MUTE		0b10000000
#define WM8904_REGMASK_IN_VOLUME	0b00011111

/**
 * WM8904_ANALOG_LEFT_IN_1, WM8904_ANALOG_RIGHT_IN_1:
 * [6]   - INx_CM_ENA: Common-mode rejection enable (N/A for single-mode)
 * [5:4] - x_IP_SEL_N: Inverting input selection
 * [3:2] - x_IP_SEL_P: Non-inverting input selection
 * [1:0] - x_MODE: Input mode
 */
#define WM8904_REGVAL_INSEL(cm, nin, pin, mode) \
	(((cm) & 0b1) << 6) | (((nin) & 0b11) << 4) | (((pin) & 0b11) << 2)
#define WM8904_REGMASK_INSEL_CMENA		0b01000000
#define WM8904_REGMASK_INSEL_IP_SEL_N	0b00110000
#define WM8904_REGMASK_INSEL_IP_SEL_P	0b00001100
#define WM8904_REGMASK_INSEL_MODE		0b00000011


/*!@brief WM8904 maximum volume */
#define WM8904_MAP_HEADPHONE_LINEOUT_MAX_VOLUME 0x3FU
#define WM8904_DAC_MAX_VOLUME                   0xC0U


/*! @brief The audio data transfer protocol. */
typedef enum _wm8904_protocol {
	kWM8904_ProtocolI2S            = 0x2,            /*!< I2S type */
	kWM8904_ProtocolLeftJustified  = 0x1,            /*!< Left justified mode */
	kWM8904_ProtocolRightJustified = 0x0,            /*!< Right justified mode */
	kWM8904_ProtocolPCMA           = 0x3,            /*!< PCM A mode */
	kWM8904_ProtocolPCMB           = 0x3 | (1 << 4), /*!< PCM B mode */
} wm8904_protocol_t;

/*! @brief The SYSCLK / fs ratio. */
typedef enum _wm8904_fs_ratio {
	kWM8904_FsRatio64X   = 0x0, /*!< SYSCLK is   64 * sample rate * frame width */
	kWM8904_FsRatio128X  = 0x1, /*!< SYSCLK is  128 * sample rate * frame width */
	kWM8904_FsRatio192X  = 0x2, /*!< SYSCLK is  192 * sample rate * frame width */
	kWM8904_FsRatio256X  = 0x3, /*!< SYSCLK is  256 * sample rate * frame width */
	kWM8904_FsRatio384X  = 0x4, /*!< SYSCLK is  384 * sample rate * frame width */
	kWM8904_FsRatio512X  = 0x5, /*!< SYSCLK is  512 * sample rate * frame width */
	kWM8904_FsRatio768X  = 0x6, /*!< SYSCLK is  768 * sample rate * frame width */
	kWM8904_FsRatio1024X = 0x7, /*!< SYSCLK is 1024 * sample rate * frame width */
	kWM8904_FsRatio1408X = 0x8, /*!< SYSCLK is 1408 * sample rate * frame width */
	kWM8904_FsRatio1536X = 0x9  /*!< SYSCLK is 1536 * sample rate * frame width */
} wm8904_fs_ratio_t;


/*! @brief Sample rate. */
typedef enum _wm8904_sample_rate {
	kWM8904_SampleRate8kHz    = 0x0, /*!< 8 kHz */
	kWM8904_SampleRate12kHz   = 0x1, /*!< 12kHz */
	kWM8904_SampleRate16kHz   = 0x2, /*!< 16kHz */
	kWM8904_SampleRate24kHz   = 0x3, /*!< 24kHz */
	kWM8904_SampleRate32kHz   = 0x4, /*!< 32kHz */
	kWM8904_SampleRate48kHz   = 0x5, /*!< 48kHz */
	kWM8904_SampleRate11025Hz = 0x6, /*!< 11.025kHz */
	kWM8904_SampleRate22050Hz = 0x7, /*!< 22.05kHz */
	kWM8904_SampleRate44100Hz = 0x8  /*!< 44.1kHz */
} wm8904_sample_rate_t;

#endif /* ZEPHYR_INCLUDE_CODEC_WM8904_H_ */
