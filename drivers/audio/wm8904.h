/*
 * Copyright (c) 2023 NXP Semiconductor INC.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CODEC_WM8904_H_
#define ZEPHYR_INCLUDE_CODEC_WM8904_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define WM8904_RESET                    (0x00)
#define WM8904_ANALOG_ADC_0             (0x0A)
#define WM8904_POWER_MGMT_0             (0x0C)
#define WM8904_POWER_MGMT_2             (0x0E)
#define WM8904_POWER_MGMT_3             (0x0F)
#define WM8904_POWER_MGMT_6             (0x12)
#define WM8904_CLK_RATES_0              (0x14)
#define WM8904_CLK_RATES_1              (0x15)
#define WM8904_CLK_RATES_2              (0x16)
#define WM8904_AUDIO_IF_0               (0x18)
#define WM8904_AUDIO_IF_1               (0x19)
#define WM8904_AUDIO_IF_2               (0x1A)
#define WM8904_AUDIO_IF_3               (0x1B)
#define WM8904_DAC_DIG_1                (0x21)
#define WM8904_DAC_DIG_0                (0x27)
#define WM8904_ANALOG_LEFT_IN_0         (0x2C)
#define WM8904_ANALOG_RIGHT_IN_0        (0x2D)
#define WM8904_ANALOG_LEFT_IN_1         (0x2E)
#define WM8904_ANALOG_RIGHT_IN_1        (0x2F)
#define WM8904_ANALOG_OUT1_LEFT         (0x39)
#define WM8904_ANALOG_OUT1_RIGHT        (0x3A)
#define WM8904_ANALOG_OUT12_ZC          (0x3D)
#define WM8904_DC_SERVO_0               (0x43)
#define WM8904_ANALOG_HP_0              (0x5A)
#define WM8904_CHRG_PUMP_0              (0x62)
#define WM8904_CLS_W_0                  (0x68)
#define WM8904_WRT_SEQUENCER_0          (0x6C)
#define WM8904_WRT_SEQUENCER_3          (0x6F)
#define WM8904_WRT_SEQUENCER_4          (0x70)
#define WM8904_DAC_DIGITAL_VOLUME_LEFT  (0x1E)
#define WM8904_DAC_DIGITAL_VOLUME_RIGHT (0x1F)
#define WM8904_ADC_DIGITAL_VOLUME_LEFT  (0x24)
#define WM8904_ADC_DIGITAL_VOLUME_RIGHT (0x25)
#define WM8904_ANALOG_OUT2_LEFT         (0x3B)
#define WM8904_ANALOG_OUT2_RIGHT        (0x3C)
#define WM8904_GPIO_CONTROL_4           (0x7C)
/* FLL control register */
#define WM8904_FLL_CONTROL_1 (0x74)
#define WM8904_FLL_CONTROL_2 (0x75)
#define WM8904_FLL_CONTROL_3 (0x76)
#define WM8904_FLL_CONTROL_4 (0x77)
#define WM8904_FLL_CONTROL_5 (0x78)
/* GPIO control register */
#define WM8904_GPIO_CONTROL_1 (0x79)
#define WM8904_GPIO_CONTROL_2 (0x7A)
#define WM8904_GPIO_CONTROL_3 (0x7B)
#define WM8904_GPIO_CONTROL_4 (0x7C)
/* fll nco */
#define WM8904_FLL_NCO_TEST_0 (0xF7U)
#define WM8904_FLL_NCO_TEST_1 (0xF8U)

/*! @brief WM8904 I2C address. */
#define WM8904_I2C_ADDRESS (0x1A)

/*! @brief WM8904 I2C bit rate. */
#define WM8904_I2C_BITRATE (400000U)

/*!@brief WM8904 maximum volume */
#define WM8904_MAP_HEADPHONE_LINEOUT_MAX_VOLUME 0x3FU
#define WM8904_DAC_MAX_VOLUME                   0xC0U


/*! @brief WM8904  lrc polarity.
 * @anchor _wm8904_lrc_polarity
 */
enum {
	kWM8904_LRCPolarityNormal   = 0U,       /*!< LRC polarity  normal */
	kWM8904_LRCPolarityInverted = 1U << 4U, /*!< LRC polarity inverted */
};

/*! @brief wm8904 module value*/
typedef enum _wm8904_module {
	kWM8904_ModuleADC       = 0, /*!< moduel ADC */
	kWM8904_ModuleDAC       = 1, /*!< module DAC */
	kWM8904_ModulePGA       = 2, /*!< module PGA */
	kWM8904_ModuleHeadphone = 3, /*!< module headphone */
	kWM8904_ModuleLineout   = 4, /*!< module line out */
} wm8904_module_t;


/*! @brief WM8904 time slot. */
typedef enum _wm8904_timeslot {
	kWM8904_TimeSlot0 = 0U, /*!< time slot0 */
	kWM8904_TimeSlot1 = 1U, /*!< time slot1 */
} wm8904_timeslot_t;

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

/*! @brief Bit width. */
typedef enum _wm8904_bit_width {
	kWM8904_BitWidth16 = 0x0, /*!< 16 bits */
	kWM8904_BitWidth20 = 0x1, /*!< 20 bits */
	kWM8904_BitWidth24 = 0x2, /*!< 24 bits */
	kWM8904_BitWidth32 = 0x3  /*!< 32 bits */
} wm8904_bit_width_t;

/*! @brief wm8904 record source
 * @anchor _wm8904_record_source
 */
enum {
	kWM8904_RecordSourceDifferentialLine = 1U, /*!< record source from differential line */
	kWM8904_RecordSourceLineInput        = 2U, /*!< record source from line input */
	kWM8904_RecordSourceDifferentialMic  = 4U, /*!< record source from differential mic */
	kWM8904_RecordSourceDigitalMic       = 8U, /*!< record source from digital microphone */
};

/*! @brief wm8904 record channel
 * @anchor _wm8904_record_channel
 */
enum {
	kWM8904_RecordChannelLeft1                 = 1U,  /*!< left record channel 1 */
	kWM8904_RecordChannelLeft2                 = 2U,  /*!< left record channel 2 */
	kWM8904_RecordChannelLeft3                 = 4U,  /*!< left record channel 3 */
	kWM8904_RecordChannelRight1                = 1U,  /*!< right record channel 1 */
	kWM8904_RecordChannelRight2                = 2U,  /*!< right record channel 2 */
	kWM8904_RecordChannelRight3                = 4U,  /*!< right record channel 3 */
	kWM8904_RecordChannelDifferentialPositive1 = 1U,  /*!< differential positive channel 1 */
	kWM8904_RecordChannelDifferentialPositive2 = 2U,  /*!< differential positive channel 2 */
	kWM8904_RecordChannelDifferentialPositive3 = 4U,  /*!< differential positive channel 3 */
	kWM8904_RecordChannelDifferentialNegative1 = 8U,  /*!< differential negative channel 1 */
	kWM8904_RecordChannelDifferentialNegative2 = 16U, /*!< differential negative channel 2 */
	kWM8904_RecordChannelDifferentialNegative3 = 32U, /*!< differential negative channel 3 */
};

/*! @brief wm8904 play source
 * @anchor _wm8904_play_source
 */
enum {
	kWM8904_PlaySourcePGA = 1U, /*!< play source PGA, bypass ADC */
	kWM8904_PlaySourceDAC = 4U, /*!< play source Input3 */
};

/*! @brief wm8904 system clock source */
typedef enum _wm8904_sys_clk_source {
	kWM8904_SysClkSourceMCLK = 0U,       /*!< wm8904 system clock soure from MCLK */
	kWM8904_SysClkSourceFLL  = 1U << 14, /*!< wm8904 system clock soure from FLL */
} wm8904_sys_clk_source_t;

/*! @brief wm8904 fll clock source */
typedef enum _wm8904_fll_clk_source {
	kWM8904_FLLClkSourceMCLK = 0U, /*!< wm8904 FLL clock source from MCLK */
} wm8904_fll_clk_source_t;

/*! @brief wm8904 fll configuration */
typedef struct _wm8904_fll_config {
	wm8904_fll_clk_source_t source; /*!< fll reference clock source */
	uint32_t refClock_HZ;           /*!< fll reference clock frequency */
	uint32_t outputClock_HZ;        /*!< fll output clock frequency  */
} wm8904_fll_config_t;


#endif /* ZEPHYR_INCLUDE_CODEC_WM8904_H_ */
