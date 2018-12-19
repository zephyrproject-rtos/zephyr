/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_ADC16_H_
#define _FSL_ADC16_H_

#include "fsl_common.h"

/*!
 * @addtogroup adc16
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief ADC16 driver version 2.0.0. */
#define FSL_ADC16_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*!
 * @brief Channel status flags.
 */
enum _adc16_channel_status_flags
{
    kADC16_ChannelConversionDoneFlag = ADC_SC1_COCO_MASK, /*!< Conversion done. */
};

/*!
 * @brief Converter status flags.
 */
enum _adc16_status_flags
{
    kADC16_ActiveFlag = ADC_SC2_ADACT_MASK, /*!< Converter is active. */
#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
    kADC16_CalibrationFailedFlag = ADC_SC3_CALF_MASK, /*!< Calibration is failed. */
#endif                                                /* FSL_FEATURE_ADC16_HAS_CALIBRATION */
};

#if defined(FSL_FEATURE_ADC16_HAS_MUX_SELECT) && FSL_FEATURE_ADC16_HAS_MUX_SELECT
/*!
 * @brief Channel multiplexer mode for each channel.
 *
 * For some ADC16 channels, there are two pin selections in channel multiplexer. For example, ADC0_SE4a and ADC0_SE4b
 * are the different channels that share the same channel number.
 */
typedef enum _adc_channel_mux_mode
{
    kADC16_ChannelMuxA = 0U, /*!< For channel with channel mux a. */
    kADC16_ChannelMuxB = 1U, /*!< For channel with channel mux b. */
} adc16_channel_mux_mode_t;
#endif /* FSL_FEATURE_ADC16_HAS_MUX_SELECT */

/*!
 * @brief Clock divider for the converter.
 */
typedef enum _adc16_clock_divider
{
    kADC16_ClockDivider1 = 0U, /*!< For divider 1 from the input clock to the module. */
    kADC16_ClockDivider2 = 1U, /*!< For divider 2 from the input clock to the module. */
    kADC16_ClockDivider4 = 2U, /*!< For divider 4 from the input clock to the module. */
    kADC16_ClockDivider8 = 3U, /*!< For divider 8 from the input clock to the module. */
} adc16_clock_divider_t;

/*!
 *@brief Converter's resolution.
 */
typedef enum _adc16_resolution
{
    /* This group of enumeration is for internal use which is related to register setting. */
    kADC16_Resolution8or9Bit = 0U,   /*!< Single End 8-bit or Differential Sample 9-bit. */
    kADC16_Resolution12or13Bit = 1U, /*!< Single End 12-bit or Differential Sample 13-bit. */
    kADC16_Resolution10or11Bit = 2U, /*!< Single End 10-bit or Differential Sample 11-bit. */

    /* This group of enumeration is for a public user. */
    kADC16_ResolutionSE8Bit = kADC16_Resolution8or9Bit,    /*!< Single End 8-bit. */
    kADC16_ResolutionSE12Bit = kADC16_Resolution12or13Bit, /*!< Single End 12-bit. */
    kADC16_ResolutionSE10Bit = kADC16_Resolution10or11Bit, /*!< Single End 10-bit. */
#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
    kADC16_ResolutionDF9Bit = kADC16_Resolution8or9Bit,    /*!< Differential Sample 9-bit. */
    kADC16_ResolutionDF13Bit = kADC16_Resolution12or13Bit, /*!< Differential Sample 13-bit. */
    kADC16_ResolutionDF11Bit = kADC16_Resolution10or11Bit, /*!< Differential Sample 11-bit. */
#endif                                                     /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */

#if defined(FSL_FEATURE_ADC16_MAX_RESOLUTION) && (FSL_FEATURE_ADC16_MAX_RESOLUTION >= 16U)
    /* 16-bit is supported by default. */
    kADC16_Resolution16Bit = 3U,                       /*!< Single End 16-bit or Differential Sample 16-bit. */
    kADC16_ResolutionSE16Bit = kADC16_Resolution16Bit, /*!< Single End 16-bit. */
#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
    kADC16_ResolutionDF16Bit = kADC16_Resolution16Bit, /*!< Differential Sample 16-bit. */
#endif                                                 /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */
#endif                                                 /* FSL_FEATURE_ADC16_MAX_RESOLUTION >= 16U */
} adc16_resolution_t;

/*!
 * @brief Clock source.
 */
typedef enum _adc16_clock_source
{
    kADC16_ClockSourceAlt0 = 0U, /*!< Selection 0 of the clock source. */
    kADC16_ClockSourceAlt1 = 1U, /*!< Selection 1 of the clock source. */
    kADC16_ClockSourceAlt2 = 2U, /*!< Selection 2 of the clock source. */
    kADC16_ClockSourceAlt3 = 3U, /*!< Selection 3 of the clock source. */

    /* Chip defined clock source */
    kADC16_ClockSourceAsynchronousClock = kADC16_ClockSourceAlt3, /*!< Using internal asynchronous clock. */
} adc16_clock_source_t;

/*!
 * @brief Long sample mode.
 */
typedef enum _adc16_long_sample_mode
{
    kADC16_LongSampleCycle24 = 0U,  /*!< 20 extra ADCK cycles, 24 ADCK cycles total. */
    kADC16_LongSampleCycle16 = 1U,  /*!< 12 extra ADCK cycles, 16 ADCK cycles total. */
    kADC16_LongSampleCycle10 = 2U,  /*!< 6 extra ADCK cycles, 10 ADCK cycles total. */
    kADC16_LongSampleCycle6 = 3U,   /*!< 2 extra ADCK cycles, 6 ADCK cycles total. */
    kADC16_LongSampleDisabled = 4U, /*!< Disable the long sample feature. */
} adc16_long_sample_mode_t;

/*!
 * @brief Reference voltage source.
 */
typedef enum _adc16_reference_voltage_source
{
    kADC16_ReferenceVoltageSourceVref = 0U, /*!< For external pins pair of VrefH and VrefL. */
    kADC16_ReferenceVoltageSourceValt = 1U, /*!< For alternate reference pair of ValtH and ValtL. */
} adc16_reference_voltage_source_t;

#if defined(FSL_FEATURE_ADC16_HAS_HW_AVERAGE) && FSL_FEATURE_ADC16_HAS_HW_AVERAGE
/*!
 * @brief Hardware average mode.
 */
typedef enum _adc16_hardware_average_mode
{
    kADC16_HardwareAverageCount4 = 0U,   /*!< For hardware average with 4 samples. */
    kADC16_HardwareAverageCount8 = 1U,   /*!< For hardware average with 8 samples. */
    kADC16_HardwareAverageCount16 = 2U,  /*!< For hardware average with 16 samples. */
    kADC16_HardwareAverageCount32 = 3U,  /*!< For hardware average with 32 samples. */
    kADC16_HardwareAverageDisabled = 4U, /*!< Disable the hardware average feature.*/
} adc16_hardware_average_mode_t;
#endif /* FSL_FEATURE_ADC16_HAS_HW_AVERAGE */

/*!
 * @brief Hardware compare mode.
 */
typedef enum _adc16_hardware_compare_mode
{
    kADC16_HardwareCompareMode0 = 0U, /*!< x < value1. */
    kADC16_HardwareCompareMode1 = 1U, /*!< x > value1. */
    kADC16_HardwareCompareMode2 = 2U, /*!< if value1 <= value2, then x < value1 || x > value2;
                                           else, value1 > x > value2. */
    kADC16_HardwareCompareMode3 = 3U, /*!< if value1 <= value2, then value1 <= x <= value2;
                                           else x >= value1 || x <= value2. */
} adc16_hardware_compare_mode_t;

#if defined(FSL_FEATURE_ADC16_HAS_PGA) && FSL_FEATURE_ADC16_HAS_PGA
/*!
 * @brief PGA's Gain mode.
 */
typedef enum _adc16_pga_gain
{
    kADC16_PGAGainValueOf1 = 0U,  /*!< For amplifier gain of 1.  */
    kADC16_PGAGainValueOf2 = 1U,  /*!< For amplifier gain of 2.  */
    kADC16_PGAGainValueOf4 = 2U,  /*!< For amplifier gain of 4.  */
    kADC16_PGAGainValueOf8 = 3U,  /*!< For amplifier gain of 8.  */
    kADC16_PGAGainValueOf16 = 4U, /*!< For amplifier gain of 16. */
    kADC16_PGAGainValueOf32 = 5U, /*!< For amplifier gain of 32. */
    kADC16_PGAGainValueOf64 = 6U, /*!< For amplifier gain of 64. */
} adc16_pga_gain_t;
#endif /* FSL_FEATURE_ADC16_HAS_PGA */

/*!
 * @brief ADC16 converter configuration.
 */
typedef struct _adc16_config
{
    adc16_reference_voltage_source_t referenceVoltageSource; /*!< Select the reference voltage source. */
    adc16_clock_source_t clockSource;                        /*!< Select the input clock source to converter. */
    bool enableAsynchronousClock;                            /*!< Enable the asynchronous clock output. */
    adc16_clock_divider_t clockDivider;                      /*!< Select the divider of input clock source. */
    adc16_resolution_t resolution;                           /*!< Select the sample resolution mode. */
    adc16_long_sample_mode_t longSampleMode;                 /*!< Select the long sample mode. */
    bool enableHighSpeed;                                    /*!< Enable the high-speed mode. */
    bool enableLowPower;                                     /*!< Enable low power. */
    bool enableContinuousConversion;                         /*!< Enable continuous conversion mode. */
} adc16_config_t;

/*!
 * @brief ADC16 Hardware comparison configuration.
 */
typedef struct _adc16_hardware_compare_config
{
    adc16_hardware_compare_mode_t hardwareCompareMode; /*!< Select the hardware compare mode.
                                                            See "adc16_hardware_compare_mode_t". */
    int16_t value1;                                    /*!< Setting value1 for hardware compare mode. */
    int16_t value2;                                    /*!< Setting value2 for hardware compare mode. */
} adc16_hardware_compare_config_t;

/*!
 * @brief ADC16 channel conversion configuration.
 */
typedef struct _adc16_channel_config
{
    uint32_t channelNumber;                    /*!< Setting the conversion channel number. The available range is 0-31.
                                                    See channel connection information for each chip in Reference
                                                    Manual document. */
    bool enableInterruptOnConversionCompleted; /*!< Generate an interrupt request once the conversion is completed. */
#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
    bool enableDifferentialConversion; /*!< Using Differential sample mode. */
#endif                                 /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */
} adc16_channel_config_t;

#if defined(FSL_FEATURE_ADC16_HAS_PGA) && FSL_FEATURE_ADC16_HAS_PGA
/*!
 * @brief ADC16 programmable gain amplifier configuration.
 */
typedef struct _adc16_pga_config
{
    adc16_pga_gain_t pgaGain;   /*!< Setting PGA gain. */
    bool enableRunInNormalMode; /*!< Enable PGA working in normal mode, or low power mode by default. */
#if defined(FSL_FEATURE_ADC16_HAS_PGA_CHOPPING) && FSL_FEATURE_ADC16_HAS_PGA_CHOPPING
    bool disablePgaChopping; /*!< Disable the PGA chopping function.
                                  The PGA employs chopping to remove/reduce offset and 1/f noise and offers
                                  an offset measurement configuration that aids the offset calibration. */
#endif                       /* FSL_FEATURE_ADC16_HAS_PGA_CHOPPING */
#if defined(FSL_FEATURE_ADC16_HAS_PGA_OFFSET_MEASUREMENT) && FSL_FEATURE_ADC16_HAS_PGA_OFFSET_MEASUREMENT
    bool enableRunInOffsetMeasurement; /*!< Enable the PGA working in offset measurement mode.
                                            When this feature is enabled, the PGA disconnects itself from the external
                                            inputs and auto-configures into offset measurement mode. With this field
                                            set, run the ADC in the recommended settings and enable the maximum hardware
                                            averaging to get the PGA offset number. The output is the
                                            (PGA offset * (64+1)) for the given PGA setting. */
#endif                                 /* FSL_FEATURE_ADC16_HAS_PGA_OFFSET_MEASUREMENT */
} adc16_pga_config_t;
#endif /* FSL_FEATURE_ADC16_HAS_PGA */

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @name Initialization
 * @{
 */

/*!
 * @brief Initializes the ADC16 module.
 *
 * @param base   ADC16 peripheral base address.
 * @param config Pointer to configuration structure. See "adc16_config_t".
 */
void ADC16_Init(ADC_Type *base, const adc16_config_t *config);

/*!
 * @brief De-initializes the ADC16 module.
 *
 * @param base ADC16 peripheral base address.
 */
void ADC16_Deinit(ADC_Type *base);

/*!
 * @brief Gets an available pre-defined settings for the converter's configuration.
 *
 * This function initializes the converter configuration structure with available settings. The default values are as follows.
 * @code
 *   config->referenceVoltageSource     = kADC16_ReferenceVoltageSourceVref;
 *   config->clockSource                = kADC16_ClockSourceAsynchronousClock;
 *   config->enableAsynchronousClock    = true;
 *   config->clockDivider               = kADC16_ClockDivider8;
 *   config->resolution                 = kADC16_ResolutionSE12Bit;
 *   config->longSampleMode             = kADC16_LongSampleDisabled;
 *   config->enableHighSpeed            = false;
 *   config->enableLowPower             = false;
 *   config->enableContinuousConversion = false;
 * @endcode
 * @param config Pointer to the configuration structure.
 */
void ADC16_GetDefaultConfig(adc16_config_t *config);

#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
/*!
 * @brief  Automates the hardware calibration.
 *
 * This auto calibration helps to adjust the plus/minus side gain automatically.
 * Execute the calibration before using the converter. Note that the hardware trigger should be used
 * during the calibration.
 *
 * @param  base ADC16 peripheral base address.
 *
 * @return                 Execution status.
 * @retval kStatus_Success Calibration is done successfully.
 * @retval kStatus_Fail    Calibration has failed.
 */
status_t ADC16_DoAutoCalibration(ADC_Type *base);
#endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */

#if defined(FSL_FEATURE_ADC16_HAS_OFFSET_CORRECTION) && FSL_FEATURE_ADC16_HAS_OFFSET_CORRECTION
/*!
 * @brief Sets the offset value for the conversion result.
 *
 * This offset value takes effect on the conversion result. If the offset value is not zero, the reading result
 * is subtracted by it. Note, the hardware calibration fills the offset value automatically.
 *
 * @param base  ADC16 peripheral base address.
 * @param value Setting offset value.
 */
static inline void ADC16_SetOffsetValue(ADC_Type *base, int16_t value)
{
    base->OFS = (uint32_t)(value);
}
#endif /* FSL_FEATURE_ADC16_HAS_OFFSET_CORRECTION */

/* @} */

/*!
 * @name Advanced Features
 * @{
 */

#if defined(FSL_FEATURE_ADC16_HAS_DMA) && FSL_FEATURE_ADC16_HAS_DMA
/*!
 * @brief Enables generating the DMA trigger when the conversion is complete.
 *
 * @param base   ADC16 peripheral base address.
 * @param enable Switcher of the DMA feature. "true" means enabled, "false" means not enabled.
 */
static inline void ADC16_EnableDMA(ADC_Type *base, bool enable)
{
    if (enable)
    {
        base->SC2 |= ADC_SC2_DMAEN_MASK;
    }
    else
    {
        base->SC2 &= ~ADC_SC2_DMAEN_MASK;
    }
}
#endif /* FSL_FEATURE_ADC16_HAS_DMA */

/*!
 * @brief Enables the hardware trigger mode.
 *
 * @param base   ADC16 peripheral base address.
 * @param enable Switcher of the hardware trigger feature. "true" means enabled, "false" means not enabled.
 */
static inline void ADC16_EnableHardwareTrigger(ADC_Type *base, bool enable)
{
    if (enable)
    {
        base->SC2 |= ADC_SC2_ADTRG_MASK;
    }
    else
    {
        base->SC2 &= ~ADC_SC2_ADTRG_MASK;
    }
}

#if defined(FSL_FEATURE_ADC16_HAS_MUX_SELECT) && FSL_FEATURE_ADC16_HAS_MUX_SELECT
/*!
 * @brief Sets the channel mux mode.
 *
 * Some sample pins share the same channel index. The channel mux mode decides which pin is used for an
 * indicated channel.
 *
 * @param base ADC16 peripheral base address.
 * @param mode Setting channel mux mode. See "adc16_channel_mux_mode_t".
 */
void ADC16_SetChannelMuxMode(ADC_Type *base, adc16_channel_mux_mode_t mode);
#endif /* FSL_FEATURE_ADC16_HAS_MUX_SELECT */

/*!
 * @brief Configures the hardware compare mode.
 *
 * The hardware compare mode provides a way to process the conversion result automatically by using hardware. Only the result
 * in the compare range is available. To compare the range, see "adc16_hardware_compare_mode_t" or the appopriate reference
 * manual for more information.
 *
 * @param base     ADC16 peripheral base address.
 * @param config   Pointer to the "adc16_hardware_compare_config_t" structure. Passing "NULL" disables the feature.
 */
void ADC16_SetHardwareCompareConfig(ADC_Type *base, const adc16_hardware_compare_config_t *config);

#if defined(FSL_FEATURE_ADC16_HAS_HW_AVERAGE) && FSL_FEATURE_ADC16_HAS_HW_AVERAGE
/*!
 * @brief Sets the hardware average mode.
 *
 * The hardware average mode provides a way to process the conversion result automatically by using hardware. The multiple
 * conversion results are accumulated and averaged internally making them easier to read.
 *
 * @param base  ADC16 peripheral base address.
 * @param mode  Setting the hardware average mode. See "adc16_hardware_average_mode_t".
 */
void ADC16_SetHardwareAverage(ADC_Type *base, adc16_hardware_average_mode_t mode);
#endif /* FSL_FEATURE_ADC16_HAS_HW_AVERAGE */

#if defined(FSL_FEATURE_ADC16_HAS_PGA) && FSL_FEATURE_ADC16_HAS_PGA
/*!
 * @brief Configures the PGA for the converter's front end.
 *
 * @param base    ADC16 peripheral base address.
 * @param config  Pointer to the "adc16_pga_config_t" structure. Passing "NULL" disables the feature.
 */
void ADC16_SetPGAConfig(ADC_Type *base, const adc16_pga_config_t *config);
#endif /* FSL_FEATURE_ADC16_HAS_PGA */

/*!
 * @brief  Gets the status flags of the converter.
 *
 * @param  base ADC16 peripheral base address.
 *
 * @return      Flags' mask if indicated flags are asserted. See "_adc16_status_flags".
 */
uint32_t ADC16_GetStatusFlags(ADC_Type *base);

/*!
 * @brief  Clears the status flags of the converter.
 *
 * @param  base ADC16 peripheral base address.
 * @param  mask Mask value for the cleared flags. See "_adc16_status_flags".
 */
void ADC16_ClearStatusFlags(ADC_Type *base, uint32_t mask);

/* @} */

/*!
 * @name Conversion Channel
 * @{
 */

/*!
 * @brief Configures the conversion channel.
 *
 * This operation triggers the conversion when in software trigger mode. When in hardware trigger mode, this API
 * configures the channel while the external trigger source helps to trigger the conversion.
 *
 * Note that the "Channel Group" has a detailed description.
 * To allow sequential conversions of the ADC to be triggered by internal peripherals, the ADC has more than one
 * group of status and control registers, one for each conversion. The channel group parameter indicates which group of
 * registers are used, for example, channel group 0 is for Group A registers and channel group 1 is for Group B registers. The
 * channel groups are used in a "ping-pong" approach to control the ADC operation.  At any point, only one of
 * the channel groups is actively controlling ADC conversions. The channel group 0 is used for both software and hardware
 * trigger modes. Channel group 1 and greater indicates multiple channel group registers for
 * use only in hardware trigger mode. See the chip configuration information in the appropriate MCU reference manual for the
 * number of SC1n registers (channel groups) specific to this device.  Channel group 1 or greater are not used
 * for software trigger operation. Therefore, writing to these channel groups does not initiate a new conversion.
 * Updating the channel group 0 while a different channel group is actively controlling a conversion is allowed and
 * vice versa. Writing any of the channel group registers while that specific channel group is actively controlling a
 * conversion aborts the current conversion.
 *
 * @param base          ADC16 peripheral base address.
 * @param channelGroup  Channel group index.
 * @param config        Pointer to the "adc16_channel_config_t" structure for the conversion channel.
 */
void ADC16_SetChannelConfig(ADC_Type *base, uint32_t channelGroup, const adc16_channel_config_t *config);

/*!
 * @brief  Gets the conversion value.
 *
 * @param  base         ADC16 peripheral base address.
 * @param  channelGroup Channel group index.
 *
 * @return              Conversion value.
 */
static inline uint32_t ADC16_GetChannelConversionValue(ADC_Type *base, uint32_t channelGroup)
{
    assert(channelGroup < ADC_R_COUNT);

    return base->R[channelGroup];
}

/*!
 * @brief  Gets the status flags of channel.
 *
 * @param  base         ADC16 peripheral base address.
 * @param  channelGroup Channel group index.
 *
 * @return              Flags' mask if indicated flags are asserted. See "_adc16_channel_status_flags".
 */
uint32_t ADC16_GetChannelStatusFlags(ADC_Type *base, uint32_t channelGroup);

/* @} */

#if defined(__cplusplus)
}
#endif
/*!
 * @}
 */
#endif /* _FSL_ADC16_H_ */
