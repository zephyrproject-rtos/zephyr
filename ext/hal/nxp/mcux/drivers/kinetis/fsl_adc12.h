/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_ADC12_H_
#define _FSL_ADC12_H_

#include "fsl_common.h"

/*!
 * @addtogroup adc12
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief ADC12 driver version */
#define FSL_ADC12_DRIVER_VERSION (MAKE_VERSION(2, 0, 2)) /*!< Version 2.0.2. */

/*!
 * @brief Channel status flags' mask.
 */
enum _adc12_channel_status_flags
{
    kADC12_ChannelConversionCompletedFlag = ADC_SC1_COCO_MASK, /*!< Conversion done. */
};

/*!
 * @brief Converter status flags' mask.
 */
enum _adc12_status_flags
{
    kADC12_ActiveFlag = ADC_SC2_ADACT_MASK,                    /*!< Converter is active. */
    kADC12_CalibrationFailedFlag = (ADC_SC2_ADACT_MASK << 1U), /*!< Calibration is failed. */
};

/*!
 * @brief Clock divider for the converter.
 */
typedef enum _adc12_clock_divider
{
    kADC12_ClockDivider1 = 0U, /*!< For divider 1 from the input clock to the module. */
    kADC12_ClockDivider2 = 1U, /*!< For divider 2 from the input clock to the module. */
    kADC12_ClockDivider4 = 2U, /*!< For divider 4 from the input clock to the module. */
    kADC12_ClockDivider8 = 3U, /*!< For divider 8 from the input clock to the module. */
} adc12_clock_divider_t;

/*!
 * @brief Converter's resolution.
 */
typedef enum _adc12_resolution
{
    kADC12_Resolution8Bit = 0U,  /*!< 8 bit resolution. */
    kADC12_Resolution12Bit = 1U, /*!< 12 bit resolution. */
    kADC12_Resolution10Bit = 2U, /*!< 10 bit resolution. */
} adc12_resolution_t;

/*!
 * @brief Conversion clock source.
 */
typedef enum _adc12_clock_source
{
    kADC12_ClockSourceAlt0 = 0U, /*!< Alternate clock 1 (ADC_ALTCLK1). */
    kADC12_ClockSourceAlt1 = 1U, /*!< Alternate clock 2 (ADC_ALTCLK2). */
    kADC12_ClockSourceAlt2 = 2U, /*!< Alternate clock 3 (ADC_ALTCLK3). */
    kADC12_ClockSourceAlt3 = 3U, /*!< Alternate clock 4 (ADC_ALTCLK4). */
} adc12_clock_source_t;

/*!
 * @brief Reference voltage source.
 */
typedef enum _adc12_reference_voltage_source
{
    kADC12_ReferenceVoltageSourceVref = 0U, /*!< For external pins pair of VrefH and VrefL. */
    kADC12_ReferenceVoltageSourceValt = 1U, /*!< For alternate reference pair of ValtH and ValtL. */
} adc12_reference_voltage_source_t;

/*!
 * @brief Hardware average mode.
 */
typedef enum _adc12_hardware_average_mode
{
    kADC12_HardwareAverageCount4 = 0U,   /*!< For hardware average with 4 samples. */
    kADC12_HardwareAverageCount8 = 1U,   /*!< For hardware average with 8 samples. */
    kADC12_HardwareAverageCount16 = 2U,  /*!< For hardware average with 16 samples. */
    kADC12_HardwareAverageCount32 = 3U,  /*!< For hardware average with 32 samples. */
    kADC12_HardwareAverageDisabled = 4U, /*!< Disable the hardware average feature.*/
} adc12_hardware_average_mode_t;

/*!
 * @brief Hardware compare mode.
 */
typedef enum _adc12_hardware_compare_mode
{
    kADC12_HardwareCompareMode0 = 0U, /*!< x < value1. */
    kADC12_HardwareCompareMode1 = 1U, /*!< x > value1. */
    kADC12_HardwareCompareMode2 = 2U, /*!< if value1 <= value2, then x < value1 || x > value2;
                                           else, value1 > x > value2. */
    kADC12_HardwareCompareMode3 = 3U, /*!< if value1 <= value2, then value1 <= x <= value2;
                                           else x >= value1 || x <= value2. */
} adc12_hardware_compare_mode_t;

/*!
 * @brief Converter configuration.
 */
typedef struct _adc12_config
{
    adc12_reference_voltage_source_t referenceVoltageSource; /*!< Select the reference voltage source. */
    adc12_clock_source_t clockSource;                        /*!< Select the input clock source to converter. */
    adc12_clock_divider_t clockDivider;                      /*!< Select the divider of input clock source. */
    adc12_resolution_t resolution;                           /*!< Select the sample resolution mode. */
    uint32_t sampleClockCount;                               /*!< Select the sample clock count. Add its value may
                                                             improve the stability of the conversion result. */
    bool enableContinuousConversion;                         /*!< Enable continuous conversion mode. */
} adc12_config_t;

/*!
 * @brief Hardware compare configuration.
 */
typedef struct _adc12_hardware_compare_config
{
    adc12_hardware_compare_mode_t hardwareCompareMode; /*!< Select the hardware compare mode. */
    int16_t value1;                                    /*!< Setting value1 for hardware compare mode. */
    int16_t value2;                                    /*!< Setting value2 for hardware compare mode. */
} adc12_hardware_compare_config_t;

/*!
 * @brief Channel conversion configuration.
 */
typedef struct _adc12_channel_config
{
    uint32_t channelNumber;                    /*!< Setting the conversion channel number. The available range is 0-31.
                                                    See channel connection information for each chip in Reference Manual
                                                    document. */
    bool enableInterruptOnConversionCompleted; /*!< Generate a interrupt request once the conversion is completed. */
} adc12_channel_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization
 * @{
 */

/*!
 * @brief Initialize the ADC12 module.
 *
 * @param base ADC12 peripheral base address.
 * @param config Pointer to "adc12_config_t" structure.
 */
void ADC12_Init(ADC_Type *base, const adc12_config_t *config);

/*!
 * @brief De-initialize the ADC12 module.
 *
 * @param base ADC12 peripheral base address.
 */
void ADC12_Deinit(ADC_Type *base);

/*!
 * @brief Gets an available pre-defined settings for converter's configuration.
 *
 * This function initializes the converter configuration structure with an available settings. The default values are:
 *
 * Example:
   @code
   config->referenceVoltageSource = kADC12_ReferenceVoltageSourceVref;
   config->clockSource = kADC12_ClockSourceAlt0;
   config->clockDivider = kADC12_ClockDivider1;
   config->resolution = kADC12_Resolution8Bit;
   config->sampleClockCount = 12U;
   config->enableContinuousConversion = false;
   @endcode
 * @param config Pointer to "adc12_config_t" structure.
 */
void ADC12_GetDefaultConfig(adc12_config_t *config);
/* @} */

/*!
 * @name Basic Operations
 * @{
 */

/*!
 * @brief Configure the conversion channel.
 *
 * This operation triggers the conversion in software trigger mode. In hardware trigger mode, this API configures the
 * channel while the external trigger source helps to trigger the conversion.
 *
 * Note that the "Channel Group" has a detailed description.
 * To allow sequential conversions of the ADC to be triggered by internal peripherals, the ADC can have more than one
 * group of status and control register, one for each conversion. The channel group parameter indicates which group of
 * registers are used, channel group 0 is for Group A registers and channel group 1 is for Group B registers. The
 * channel groups are used in a "ping-pong" approach to control the ADC operation.  At any time, only one of the
 * channel groups is actively controlling ADC conversions. Channel group 0 is used for both software and hardware
 * trigger modes of operation. Channel groups 1 and greater indicate potentially multiple channel group registers for
 * use only in hardware trigger mode. See the chip configuration information in the MCU reference manual about the
 * number of SC1n registers (channel groups) specific to this device.  None of the channel groups 1 or greater are used
 * for software trigger operation and therefore writes to these channel groups do not initiate a new conversion.
 * Updating channel group 0 while a different channel group is actively controlling a conversion is allowed and
 * vice versa. Writing any of the channel group registers while that specific channel group is actively controlling a
 * conversion aborts the current conversion.
 *
 * @param base ADC12 peripheral base address.
 * @param channelGroup Channel group index.
 * @param config Pointer to "adc12_channel_config_t" structure.
 */
void ADC12_SetChannelConfig(ADC_Type *base, uint32_t channelGroup, const adc12_channel_config_t *config);

/*!
 * @brief Get the conversion value.
 *
 * @param base ADC12 peripheral base address.
 * @param channelGroup Channel group index.
 *
 * @return Conversion value.
 */
static inline uint32_t ADC12_GetChannelConversionValue(ADC_Type *base, uint32_t channelGroup)
{
    assert(channelGroup < ADC_R_COUNT);

    return base->R[channelGroup];
}

/*!
 * @brief Get the status flags of channel.
 *
 * @param base ADC12 peripheral base address.
 * @param channelGroup Channel group index.
 *
 * @return Flags' mask if indicated flags are asserted. See to "_adc12_channel_status_flags".
 */
uint32_t ADC12_GetChannelStatusFlags(ADC_Type *base, uint32_t channelGroup);

/* @} */

/*!
 * @name Advanced Operations
 * @{
 */

/*!
 * @brief Automate the hardware calibration.
 *
 * This auto calibration helps to adjust the gain automatically according to the converter's working environment.
 * Execute the calibration before conversion. Note that the software trigger should be used during calibration.
 *
 * @note The calibration function has bug in the SOC. The calibration failed flag may be set after calibration process
 * even if you configure the ADC12 as the reference manual correctly. It is a known issue now and may be fixed in the
 * future.
 *
 * @param base ADC12 peripheral base address.
 * @retval kStatus_Success Calibration is done successfully.
 * @retval kStatus_Fail Calibration is failed.
 */
status_t ADC12_DoAutoCalibration(ADC_Type *base);

/*!
 * @brief Set the offset value for the conversion result.
 *
 * This offset value takes effect on the conversion result. If the offset value is not zero, the conversion result is
 * substracted by it.
 *
 * @param base ADC12 peripheral base address.
 * @param value Offset value.
 */
static inline void ADC12_SetOffsetValue(ADC_Type *base, uint32_t value)
{
    base->USR_OFS = (value & ADC_USR_OFS_USR_OFS_MASK);
}

/*!
 * @brief Set the gain value for the conversion result.
 *
 * This gain value takes effect on the conversion result. If the gain value is not zero, the conversion result is
 * amplified as it.
 *
 * @param base ADC12 peripheral base address.
 * @param value Gain value.
 */
static inline void ADC12_SetGainValue(ADC_Type *base, uint32_t value)
{
    base->UG = (value & ADC_UG_UG_MASK);
}

#if defined(FSL_FEATURE_ADC12_HAS_DMA_SUPPORT) && (FSL_FEATURE_ADC12_HAS_DMA_SUPPORT == 1)
/*!
 * @brief Enable generating the DMA trigger when conversion is completed.
 *
 * @param base ADC12 peripheral base address.
 * @param enable Switcher of DMA feature. "true" means to enable, "false" means to disable.
 */
static inline void ADC12_EnableDMA(ADC_Type *base, bool enable)
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
#endif /* FSL_FEATURE_ADC12_HAS_DMA_SUPPORT */

/*!
 * @brief Enable of disable the hardware trigger mode.
 *
 * @param base ADC12 peripheral base address.
 * @param enable Switcher of hardware trigger feature. "true" means to enable, "false" means not.
 */
static inline void ADC12_EnableHardwareTrigger(ADC_Type *base, bool enable)
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

/*!
 * @brief Configure the hardware compare mode.
 *
 * The hardware compare mode provides a way to process the conversion result automatically by hardware. Only the result
 * in compare range is available. To compare the range, see "adc12_hardware_compare_mode_t", or the reference manual
 * document for more detailed information.
 *
 * @param base ADC12 peripheral base address.
 * @param config Pointer to "adc12_hardware_compare_config_t" structure. Pass "NULL" to disable the feature.
 */
void ADC12_SetHardwareCompareConfig(ADC_Type *base, const adc12_hardware_compare_config_t *config);

/*!
 * @brief Set the hardware average mode.
 *
 * Hardware average mode provides a way to process the conversion result automatically by hardware. The multiple
 * conversion results are accumulated and averaged internally. This aids to get more accurate conversion result.
 *
 * @param base ADC12 peripheral base address.
 * @param mode Setting hardware average mode. See to "adc12_hardware_average_mode_t".
 */
void ADC12_SetHardwareAverage(ADC_Type *base, adc12_hardware_average_mode_t mode);

/*!
 * @brief Get the status flags of the converter.
 *
 * @param base ADC12 peripheral base address.
 *
 * @return Flags' mask if indicated flags are asserted. See to "_adc12_status_flags".
 */
uint32_t ADC12_GetStatusFlags(ADC_Type *base);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_ADC12_H_ */
