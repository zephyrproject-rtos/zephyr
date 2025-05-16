/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_WM8962_H_
#define ZEPHYR_DRIVERS_AUDIO_WM8962_H_

#ifdef __cplusplus
extern "C" {
#endif

#define WM8962_SWAP_UINT16_BYTE_SEQUENCE(x) ((((x) & 0x00ffU) << 8U) | (((x) & 0xff00U) >> 8U))
/*! @brief WM8962 max clock */
#define WM8962_MAX_DSP_CLOCK                (24576000U)
#define WM8962_MAX_SYS_CLOCK                (12288000U)
/*! @brief WM8962 f2 better performance range */
#define WM8962_FLL_VCO_MIN_FREQ             90000000U
#define WM8962_FLL_VCO_MAX_FREQ             100000000U
#define WM8962_FLL_LOCK_TIMEOUT             10000000U
/*! @brief WM8962 FLLN range */
#define WM8962_FLL_N_MIN_VALUE              6U
#define WM8962_FLL_N_MAX_VALUE              12U
#define WM8962_FLL_MAX_REFERENCE_CLOCK      (13500000U)

/*! @brief Define the register address of WM8962. */
#define WM8962_REG_LINVOL  0x0U
#define WM8962_REG_RINVOL  0x1U
#define WM8962_REG_LOUT1   0x2U
#define WM8962_REG_ROUT1   0x3U
#define WM8962_REG_CLOCK1  0x4U
#define WM8962_REG_DACCTL1 0x5U
#define WM8962_REG_DACCTL2 0x6U
#define WM8962_REG_IFACE0  0x7U
#define WM8962_REG_IFACE1  0x9U
#define WM8962_REG_CLOCK2  0x8U
#define WM8962_REG_IFACE2  0xEU
#define WM8962_REG_LDAC    0xaU
#define WM8962_REG_RDAC    0xbU

#define WM8962_REG_RESET         0xfU
#define WM8962_REG_3D            0x10U
#define WM8962_REG_ALC1          0x11U
#define WM8962_REG_ALC2          0x12U
#define WM8962_REG_ALC3          0x13U
#define WM8962_REG_NOISEG        0x14U
#define WM8962_REG_LADC          0x15U
#define WM8962_REG_RADC          0x16U
#define WM8962_REG_ADDCTL1       0x17U
#define WM8962_REG_ADDCTL2       0x18U
#define WM8962_REG_POWER1        0x19U
#define WM8962_REG_POWER2        0x1aU
#define WM8962_REG_ADDCTL3       0x1bU
#define WM8962_REG_APOP1         0x1cU
#define WM8962_REG_APOP2         0x1dU
#define WM8962_REG_INPUT_MIXER_1 0x1FU

#define WM8962_REG_LINPATH  0x20U
#define WM8962_REG_RINPATH  0x21U
#define WM8962_REG_INPUTMIX 0x22U

#define WM8962_REG_LEFT_INPUT_PGA  0x25U
#define WM8962_REG_RIGHT_INPUT_PGA 0x26U
#define WM8962_REG_MONOMIX2        0x27U
#define WM8962_REG_LOUT2           0x28U
#define WM8962_REG_ROUT2           0x29U
#define WM8962_REG_TEMP            0x2FU
#define WM8962_REG_ADDCTL4         0x30U
#define WM8962_REG_CLASSD1         0x31U

#define WM8962_REG_CLASSD3       0x33U
#define WM8962_REG_CLK4          0x38U
#define WM8962_REG_DC_SERVO_0    0x3CU
#define WM8962_REG_DC_SERVO_1    0x3DU
#define WM8962_REG_ANALOG_HP_0   0x45U
#define WM8962_REG_CHARGE_PUMP_1 0x48U

#define WM8962_REG_WRITE_SEQ_CTRL_1 0x57U
#define WM8962_REG_WRITE_SEQ_CTRL_2 0x5AU
#define WM8962_REG_WRITE_SEQ_CTRL_3 0x5DU

#define WM8962_WSEQ_ENA 0x20U

#define WM8962_REG_LEFT_HEADPHONE_MIXER         0x64U
#define WM8962_REG_RIGHT_HEADPHONE_MIXER        0x65U
#define WM8962_REG_LEFT_HEADPHONE_MIXER_VOLUME  0x66U
#define WM8962_REG_RIGHT_HEADPHONE_MIXER_VOLUME 0x67U

#define WM8962_REG_LEFT_SPEAKER_MIXER         0x69U
#define WM8962_REG_RIGHT_SPEAKER_MIXER        0x6AU
#define WM8962_REG_LEFT_SPEAKER_MIXER_VOLUME  0x6BU
#define WM8962_REG_RIGHT_SPEAKER_MIXER_VOLUME 0x6CU

#define WM8962_REG_PLL2 0x81U

#define WM8962_REG_FLL_CTRL_1   0x9BU
#define WM8962_REG_FLL_CTRL_2   0x9CU
#define WM8962_REG_FLL_CTRL_3   0x9DU
#define WM8962_REG_FLL_CTRL_6   0xA0U
#define WM8962_REG_FLL_CTRL_7   0xA1U
#define WM8962_REG_FLL_CTRL_8   0xA2U
#define WM8962_REG_INT_STATUS_2 0x231U

#define WSEQ_DONE_EINT_MASK 0x80U

#define WM8962_L_CH_MUTE_MASK 2U
#define WM8962_R_CH_MUTE_MASK 1U

/*! @brief WM8962 CLOCK2 bits */
#define WM8962_CLOCK2_BCLK_DIV_MASK 0xFU

/*! @brief WM8962_IFACE0 FORMAT bits */
#define WM8962_IFACE0_FORMAT_MASK  0x13U
#define WM8962_IFACE0_FORMAT_SHIFT 0x00U
#define WM8962_IFACE0_FORMAT_RJ    0x00U
#define WM8962_IFACE0_FORMAT_LJ    0x01U
#define WM8962_IFACE0_FORMAT_I2S   0x02U
#define WM8962_IFACE0_FORMAT_DSP   0x03U
#define WM8962_IFACE0_FORMAT(x)    (((x) << WM8962_IFACE1_FORMAT_SHIFT) & WM8962_IFACE1_FORMAT_MASK)

/*! @brief WM8962_IFACE0 WL bits */
#define WM8962_IFACE0_WL_MASK   0x0CU
#define WM8962_IFACE0_WL_SHIFT  0x02U
#define WM8962_IFACE0_WL_16BITS 0x00U
#define WM8962_IFACE0_WL_20BITS 0x01U
#define WM8962_IFACE0_WL_24BITS 0x02U
#define WM8962_IFACE0_WL_32BITS 0x03U
#define WM8962_IFACE0_WL(x)     (((x) << WM8962_IFACE0_WL_SHIFT) & WM8962_IFACE0_WL_MASK)

/*! @brief WM8962_IFACE1 LRP bit */
#define WM8962_IFACE1_LRP_MASK         0x10U
#define WM8962_IFACE1_LRP_SHIFT        0x04U
#define WM8962_IFACE1_LRCLK_NORMAL_POL 0x00U
#define WM8962_IFACE1_LRCLK_INVERT_POL 0x01U
#define WM8962_IFACE1_DSP_MODEA        0x00U
#define WM8962_IFACE1_DSP_MODEB        0x01U
#define WM8962_IFACE1_LRP(x)           (((x) << WM8962_IFACE1_LRP_SHIFT) & WM8962_IFACE1_LRP_MASK)

/*! @brief WM8962_IFACE1 DLRSWAP bit */
#define WM8962_IFACE1_DLRSWAP_MASK  0x20U
#define WM8962_IFACE1_DLRSWAP_SHIFT 0x05U
#define WM8962_IFACE1_DACCH_NORMAL  0x00U
#define WM8962_IFACE1_DACCH_SWAP    0x01U

#define WM8962_IFACE1_DLRSWAP(x) (((x) << WM8962_IFACE1_DLRSWAP_SHIFT) & WM8962_IFACE1_DLRSWAP_MASK)

/*! @brief WM8962_IFACE1 MS bit */
#define WM8962_IFACE1_MS_MASK  0x40U
#define WM8962_IFACE1_MS_SHIFT 0x06U
#define WM8962_IFACE1_SLAVE    0x00U
#define WM8962_IFACE1_MASTER   0x01U
#define WM8962_IFACE1_MS(x)    (((x) << WM8962_IFACE1_MS_SHIFT) & WM8962_IFACE1_MS_MASK)

/*! @brief WM8962_IFACE1 BCLKINV bit */
#define WM8962_IFACE1_BCLKINV_MASK   0x80U
#define WM8962_IFACE1_BCLKINV_SHIFT  0x07U
#define WM8962_IFACE1_BCLK_NONINVERT 0x00U
#define WM8962_IFACE1_BCLK_INVERT    0x01U

#define WM8962_IFACE1_BCLKINV(x) (((x) << WM8962_IFACE1_BCLKINV_SHIFT) & WM8962_IFACE1_BCLKINV_MASK)

/*! @brief WM8962_IFACE1 ALRSWAP bit */
#define WM8962_IFACE1_ALRSWAP_MASK  0x100U
#define WM8962_IFACE1_ALRSWAP_SHIFT 0x08U
#define WM8962_IFACE1_ADCCH_NORMAL  0x00U
#define WM8962_IFACE1_ADCCH_SWAP    0x01U

#define WM8962_IFACE1_ALRSWAP(x) (((x) << WM8962_IFACE1_ALRSWAP_SHIFT) & WM8962_IFACE1_ALRSWAP_MASK)

/*! @brief WM8962_POWER1 */
#define WM8962_POWER1_VREF_MASK  0x40U
#define WM8962_POWER1_VREF_SHIFT 0x06U

#define WM8962_POWER1_AINL_MASK  0x20U
#define WM8962_POWER1_AINL_SHIFT 0x05U

#define WM8962_POWER1_AINR_MASK  0x10U
#define WM8962_POWER1_AINR_SHIFT 0x04U

#define WM8962_POWER1_ADCL_MASK  0x08U
#define WM8962_POWER1_ADCL_SHIFT 0x03U

#define WM8962_POWER1_ADCR_MASK  0x4U
#define WM8962_POWER1_ADCR_SHIFT 0x02U

#define WM8962_POWER1_MICB_MASK  0x02U
#define WM8962_POWER1_MICB_SHIFT 0x01U

#define WM8962_POWER1_DIGENB_MASK  0x01U
#define WM8962_POWER1_DIGENB_SHIFT 0x00U

/*! @brief WM8962_POWER2 */
#define WM8962_POWER2_DACL_MASK  0x100U
#define WM8962_POWER2_DACL_SHIFT 0x08U

#define WM8962_POWER2_DACR_MASK  0x80U
#define WM8962_POWER2_DACR_SHIFT 0x07U

#define WM8962_POWER2_LOUT1_MASK  0x40U
#define WM8962_POWER2_LOUT1_SHIFT 0x06U

#define WM8962_POWER2_ROUT1_MASK  0x20U
#define WM8962_POWER2_ROUT1_SHIFT 0x05U

#define WM8962_POWER2_SPKL_MASK  0x10U
#define WM8962_POWER2_SPKL_SHIFT 0x04U

#define WM8962_POWER2_SPKR_MASK  0x08U
#define WM8962_POWER2_SPKR_SHIFT 0x03U

#define WM8962_POWER3_LMIC_MASK           0x20U
#define WM8962_POWER3_LMIC_SHIFT          0x05U
#define WM8962_POWER3_RMIC_MASK           0x10U
#define WM8962_POWER3_RMIC_SHIFT          0x04U
#define WM8962_POWER3_LOMIX_MASK          0x08U
#define WM8962_POWER3_LOMIX_SHIFT         0x03U
#define WM8962_POWER3_ROMIX_MASK          0x04U
#define WM8962_POWER3_ROMIX_SHIFT         0x02U
/*! @brief WM8962 I2C address. */
#define WM8962_I2C_ADDR                   (0x34 >> 1U)
/*! @brief WM8962 I2C baudrate */
#define WM8962_I2C_BAUDRATE               (100000U)
/*! @brief WM8962 maximum volume value */
#define WM8962_ADC_MAX_VOLUME_VALUE       0xFFU
#define WM8962_DAC_MAX_VOLUME_VALUE       0xFFU
#define WM8962_HEADPHONE_MAX_VOLUME_VALUE 0x7FU
#define WM8962_HEADPHONE_MIN_VOLUME_VALUE 0x2FU
#define WM8962_LINEIN_MAX_VOLUME_VALUE    0x3FU
#define WM8962_SPEAKER_MAX_VOLUME_VALUE   0x7FU
#define WM8962_SPEAKER_MIN_VOLUME_VALUE   0x2FU

#define WM8962_ADC_DEFAULT_VOLUME_VALUE       0x1C0U
#define WM8962_DAC_DEFAULT_VOLUME_VALUE       0x1C0U
#define WM8962_HEADPHONE_DEFAULT_VOLUME_VALUE 0x179U
#define WM8962_LINEIN_DEFAULT_VOLUME_VALUE    0x12DU
#define WM8962_SPEAKER_DEFAULT_VOLUME_VALUE   0x179U

/**
 * WM8962_REG_LOUT1, WM8962_REG_ROUT1 (headphone outs),
 * WM8962_REG_LOUT2, WM8962_REG_ROUT2 (line outs):
 * [8]   - VU: Volume update, works for entire channel pair
 * [7]   - ZC: Zero-crossing enable
 * [6:0] - VOL: 7-bit volume value
 */
#define WM8962_REGVAL_OUT_VOL(vu, zc, vol)                                                         \
	(((vu & 0b1) << 8) | (zc & 0b1) << 7 | (vol & 0b001111111))
#define WM8962_REGMASK_OUT_VU  0b100000000
#define WM8962_REGMASK_OUT_ZC  0b010000000
#define WM8962_REGMASK_OUT_VOL 0b001111111

#define WM8962_OUT_MUTE(x) ((x << 8U) & (1U << 8U))

/**
 * WM8962_REG_LINVOL, WM8962_REG_RINVOL:
 * [8]   - VU: Volume update, works for entire channel pair
 * [7]   - MUTE: Input mute
 * [6]   - ZC: Zero-crossing enable
 * [5:0] - VOL: 6-bit volume value
 */
#define WM8962_REGVAL_IN_VOL(vu, mute, zc, vol)                                                    \
	((vu & 0b1) << 8 | (mute & 0b1) << 7 | (zc & 0b1) << 6 | (vol & 0b000111111))
#define WM8962_REGMASK_IN_VU     0b100000000
#define WM8962_REGMASK_IN_MUTE   0b010000000
#define WM8962_REGMASK_IN_ZC     0b001000000
#define WM8962_REGMASK_IN_VOLUME 0b000111111

/*! @brief wm8962 input mixer source.
 * @anchor wm8962_input_mixer_source_t
 */
typedef enum _wm8962_input_mixer_source {
	kWM8962_InputMixerSourceInput2 = 4U,   /*!< input mixer source input 2 */
	kWM8962_InputMixerSourceInput3 = 2U,   /*!< input mixer source input 3 */
	kWM8962_InputMixerSourceInputPGA = 1U, /*!< input mixer source input PGA */
} wm8962_input_mixer_source_t;

/*! @brief wm8962 output mixer source.
 * @anchor wm8962_output_mixer_source_t
 */
typedef enum _wm8962_output_mixer_source {
	kWM8962_OutputMixerDisabled = 0U,              /*!< output mixer disabled */
	kWM8962_OutputMixerSourceInput4Right = 1U,     /*!< output mixer source input 4 left */
	kWM8962_OutputMixerSourceInput4Left = 2U,      /*!< output mixer source input 4 right */
	kWM8962_OutputMixerSourceRightInputMixer = 4U, /*!< output mixer source left input mixer */
	kWM8962_OutputMixerSourceLeftInputMixer = 8U,  /*!< output mixer source right input mixer*/
	kWM8962_OutputMixerSourceRightDAC = 0x10U,     /*!< output mixer source left DAC */
	kWM8962_OutputMixerSourceLeftDAC = 0x20U,      /*!< output mixer source Right DAC */
} wm8962_output_mixer_source_t;

/*! @brief Modules in WM8962 board. */
typedef enum _wm8962_module {
	kWM8962_ModuleADC = 0,           /*!< ADC module in WM8962 */
	kWM8962_ModuleDAC = 1,           /*!< DAC module in WM8962 */
	kWM8962_ModuleMICB = 4,          /*!< Mic bias */
	kWM8962_ModuleMIC = 5,           /*!< Input Mic */
	kWM8962_ModuleLineIn = 6,        /*!< Analog in PGA  */
	kWM8962_ModuleHeadphone = 7,     /*!< Line out module */
	kWM8962_ModuleSpeaker = 8,       /*!< Speaker module */
	kWM8962_ModuleHeaphoneMixer = 9, /*!< Output mixer */
	kWM8962_ModuleSpeakerMixer = 10, /*!< Output mixer */
} wm8962_module_t;

/*! @brief wm8962 play channel
 * @anchor _wm8962_play_channel
 */
typedef enum _wm8962_play_channel {
	kWM8962_HeadphoneLeft = 1,  /*!< wm8962 headphone left channel */
	kWM8962_HeadphoneRight = 2, /*!< wm8962 headphone right channel */
	kWM8962_SpeakerLeft = 4,    /*!< wm8962 speaker left channel */
	kWM8962_SpeakerRight = 8,   /*!< wm8962 speaker right channel */
} wm8962_play_channel_t;

/*!
 * @brief The audio data transfer protocol choice.
 * WM8962 only supports I2S format and PCM format.
 */
typedef enum _wm8962_protocol {
	kWM8962_BusPCMA = 4,           /*!< PCMA mode */
	kWM8962_BusPCMB = 3,           /*!< PCMB mode */
	kWM8962_BusI2S = 2,            /*!< I2S type */
	kWM8962_BusLeftJustified = 1,  /*!< Left justified mode */
	kWM8962_BusRightJustified = 0, /*!< Right justified mode */
} wm8962_protocol_t;

/*! @brief wm8962 input source */
typedef enum _wm8962_input_pga_source {
	kWM8962_InputPGASourceInput1 = 8, /*!< Input PGA source input1 */
	kWM8962_InputPGASourceInput2 = 4, /*!< Input PGA source input2 */
	kWM8962_InputPGASourceInput3 = 2, /*!< Input PGA source input3 */
	kWM8962_InputPGASourceInput4 = 1, /*!< Input PGA source input4 */
} wm8962_input_pga_source_t;

/*! @brief wm8962 input source */
typedef enum _wm8962_output_pga_source {
	kWM8962_OutputPGASourceMixer = 0, /*!< Output PGA source mixer */
	kWM8962_OutputPGASourceDAC = 1,   /*!< Output PGA source DAC */
} wm8962_output_pga_source_t;

/*! @brief audio sample rate definition
 * @anchor _wm8962_sample_rate
 */
typedef enum _wm8962_sample_rate {
	kWM8962_AudioSampleRate8kHz = 8000U,     /*!< Sample rate 8000 Hz */
	kWM8962_AudioSampleRate11025Hz = 11025U, /*!< Sample rate 11025 Hz */
	kWM8962_AudioSampleRate12kHz = 12000U,   /*!< Sample rate 12000 Hz */
	kWM8962_AudioSampleRate16kHz = 16000U,   /*!< Sample rate 16000 Hz */
	kWM8962_AudioSampleRate22050Hz = 22050U, /*!< Sample rate 22050 Hz */
	kWM8962_AudioSampleRate24kHz = 24000U,   /*!< Sample rate 24000 Hz */
	kWM8962_AudioSampleRate32kHz = 32000U,   /*!< Sample rate 32000 Hz */
	kWM8962_AudioSampleRate44100Hz = 44100U, /*!< Sample rate 44100 Hz */
	kWM8962_AudioSampleRate48kHz = 48000U,   /*!< Sample rate 48000 Hz */
	kWM8962_AudioSampleRate88200Hz = 88200U, /*!< Sample rate 88200 Hz */
	kWM8962_AudioSampleRate96kHz = 96000U,   /*!< Sample rate 96000 Hz */
} wm8962_sample_rate_t;

/*! @brief audio bit width
 * @anchor _wm8962_audio_bit_width
 */
typedef enum _wm8962_audio_bit_width {
	kWM8962_AudioBitWidth16bit = 16U, /*!< audio bit width 16 */
	kWM8962_AudioBitWidth20bit = 20U, /*!< audio bit width 20 */
	kWM8962_AudioBitWidth24bit = 24U, /*!< audio bit width 24 */
	kWM8962_AudioBitWidth32bit = 32U, /*!< audio bit width 32 */
} wm8962_audio_bit_width_t;

/*! @brief wm8962 fll clock source */
typedef enum _wm8962_fllclk_source {
	kWM8962_FLLClkSourceMCLK = 0U, /*!< FLL clock source from MCLK */
	kWM8962_FLLClkSourceBCLK = 1U, /*!< FLL clock source from BCLK */
} wm8962_fllclk_source_t;

/*! @brief wm8962 sysclk source */
typedef enum _wm8962_sysclk_source {
	kWM8962_SysClkSourceMclk = 0U, /*!< sysclk source from external MCLK */
	kWM8962_SysClkSourceFLL = 1U,  /*!< sysclk source from internal FLL */
} wm8962_sysclk_source_t;

/*! @brief WM8962 default sequence */
typedef enum _wm8962_sequence_id {
	kWM8962_SequenceDACToHeadphonePowerUp = 0x80U, /*!< dac to headphone power up sequence */
	kWM8962_SequenceAnalogueInputPowerUp = 0x92U,  /*!< Analogue input power up sequence */
	kWM8962_SequenceChipPowerDown = 0x9BU,         /*!< Chip power down sequence */
	kWM8962_SequenceSpeakerSleep = 0xE4U,          /*!< Speaker sleep sequence */
	kWM8962_SequenceSpeakerWake = 0xE8U,           /*!< speaker wake sequence */
} wm8962_sequence_id_t;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_WM8962_H_ */
