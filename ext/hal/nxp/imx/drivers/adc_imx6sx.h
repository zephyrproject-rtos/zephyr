/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ADC_IMX6SX_H__
#define __ADC_IMX6SX_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup adc_imx6sx_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief ADC module initialize structure. */
typedef struct _adc_init_config
{
    uint8_t clockSource;    /*!< Select input clock source to generate the internal conversion clock.*/
    uint8_t divideRatio;    /*!< Selects divide ratio used to generate the internal conversion clock.*/
    uint8_t averageNumber;  /*!< The average number for hardware average function.*/
    uint8_t resolutionMode; /*!< Set ADC resolution mode.*/
} adc_init_config_t;

/*! @brief ADC hardware average number. */
enum _adc_average_number
{
    adcAvgNum4    = 0U, /*!< ADC Hardware Average Number is set to 4.*/
    adcAvgNum8    = 1U, /*!< ADC Hardware Average Number is set to 8.*/
    adcAvgNum16   = 2U, /*!< ADC Hardware Average Number is set to 16.*/
    adcAvgNum32   = 3U, /*!< ADC Hardware Average Number is set to 32.*/
    adcAvgNumNone = 4U, /*!< Disable ADC Hardware Average.*/
};

/*! @brief ADC conversion trigger select. */
enum _adc_convert_trigger_mode
{
    adcSoftwareTrigger = 0U, /*!< ADC software trigger a conversion.*/
    adcHardwareTrigger = 1U, /*!< ADC hardware trigger a conversion.*/
};

/*! @brief ADC conversion speed configure. */
enum _adc_convert_speed_config
{
    adcNormalSpeed = 0U, /*!< ADC set as normal conversion speed.*/
    adcHighSpeed   = 1U, /*!< ADC set as high conversion speed.*/
};

/*! @brief ADC sample time duration. */
enum _adc_sample_time_duration
{
    adcSamplePeriodClock2,  /*!< The sample time duration is set as 2 ADC clocks.*/
    adcSamplePeriodClock4,  /*!< The sample time duration is set as 4 ADC clocks.*/
    adcSamplePeriodClock6,  /*!< The sample time duration is set as 6 ADC clocks.*/
    adcSamplePeriodClock8,  /*!< The sample time duration is set as 8 ADC clocks.*/
    adcSamplePeriodClock12, /*!< The sample time duration is set as 12 ADC clocks.*/
    adcSamplePeriodClock16, /*!< The sample time duration is set as 16 ADC clocks.*/
    adcSamplePeriodClock20, /*!< The sample time duration is set as 20 ADC clocks.*/
    adcSamplePeriodClock24, /*!< The sample time duration is set as 24 ADC clocks.*/
};

/*! @brief ADC low power configure. */
enum _adc_power_mode
{
    adcNormalPowerMode = 0U, /*!< ADC hard block set as normal power mode.*/
    adcLowPowerMode    = 1U, /*!< ADC hard block set as low power mode.*/
};

/*! @brief ADC conversion resolution mode. */
enum _adc_resolution_mode
{
    adcResolutionBit8  = 0U, /*!< ADC resolution set as 8 bit conversion mode.*/
    adcResolutionBit10 = 1U, /*!< ADC resolution set as 10 bit conversion mode.*/
    adcResolutionBit12 = 2U, /*!< ADC resolution set as 12 bit conversion mode.*/
};

/*! @brief ADC input clock divide. */
enum _adc_clock_divide
{
    adcInputClockDiv1 = 0U, /*!< Input clock divide 1 to generate internal clock.*/
    adcInputClockDiv2 = 1U, /*!< Input clock divide 2 to generate internal clock.*/
    adcInputClockDiv4 = 2U, /*!< Input clock divide 4 to generate internal clock.*/
    adcInputClockDiv8 = 3U, /*!< Input clock divide 8 to generate internal clock.*/
};

/*! @brief ADC clock source. */
enum _adc_clock_source
{
    adcIpgClock        = 0U, /*!< Select ipg clock as input clock source.*/
    adcIpgClockDivide2 = 1U, /*!< Select ipg clock divide 2 as input clock source.*/
    adcAsynClock       = 3U, /*!< Select asynchronous clock as input clock source.*/
};

/*! @brief ADC comparer work mode configuration. */
enum _adc_compare_mode
{
    adcCmpModeLessThanCmpVal1,     /*!< Compare true if the result is less than compare value 1.*/
    adcCmpModeGreaterThanCmpVal1,  /*!< Compare true if the result is greater than or equal to compare value 1.*/
    adcCmpModeOutRangNotInclusive, /*!< Compare true if the result is less than compare value 1 or the result is Greater than compare value 2.*/
    adcCmpModeInRangNotInclusive,  /*!< Compare true if the result is less than compare value 1 and the result is greater than compare value 2.*/
    adcCmpModeInRangInclusive,     /*!< Compare true if the result is greater than or equal to compare value 1 and the result is less than or equal to compare value 2.*/
    adcCmpModeOutRangInclusive,    /*!< Compare true if the result is greater than or equal to compare value 1 or the result is less than or equal to compare value 2.*/
    adcCmpModeDisable,             /*!< ADC compare function disable.*/
};

/*! @brief ADC general status flag. */
enum _adc_general_status_flag
{
    adcFlagAsynWakeUpInt   = 1U << 0, /*!< Indicate asynchronous wake up interrupt occurred in stop mode.*/
    adcFlagCalibrateFailed = 1U << 1, /*!< Indicate the result of the calibration sequence.*/
    adcFlagConvertActive   = 1U << 2, /*!< Indicate a conversion is in the process.*/
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name ADC Module Initialization and Configuration Functions.
 * @{
 */

/*!
 * @brief Initialize ADC to reset state and initialize with initialize structure.
 *
 * @param base ADC base pointer.
 * @param initConfig ADC initialize structure.
 */
void ADC_Init(ADC_Type* base, const adc_init_config_t* initConfig);

/*!
 * @brief This function reset ADC module register content to its default value.
 *
 * @param base ADC base pointer.
 */
void ADC_Deinit(ADC_Type* base);

/*!
 * @brief Enable or disable ADC module overwrite conversion result.
 *
 * @param base ADC base pointer.
 * @param enable Enable/Disable conversion result overwire function.
 *               - true: Enable conversion result overwire.
 *               - false: Disable conversion result overwrite.
 */
void ADC_SetConvertResultOverwrite(ADC_Type* base, bool enable);

/*!
 * @brief This function set ADC module conversion trigger mode.
 *
 * @param base ADC base pointer.
 * @param mode Conversion trigger (see @ref _adc_convert_trigger_mode enumeration).
 */
void ADC_SetConvertTrigMode(ADC_Type* base, uint8_t mode);

/*!
 * @brief This function is used to get conversion trigger mode.
 *
 * @param base ADC base pointer.
 * @return Conversion trigger mode (see @ref _adc_convert_trigger_mode enumeration).
 */
static inline uint8_t ADC_GetConvertTrigMode(ADC_Type* base)
{
    return (uint8_t)((ADC_CFG_REG(base) & ADC_CFG_ADTRG_MASK) >> ADC_CFG_ADTRG_SHIFT);
}

/*!
 * @brief This function set ADC module conversion speed mode.
 *
 * @param base ADC base pointer.
 * @param mode Conversion speed mode (see @ref _adc_convert_speed_config enumeration).
 */
void ADC_SetConvertSpeed(ADC_Type* base, uint8_t mode);

/*!
 * @brief This function get ADC module conversion speed mode.
 *
 * @param base ADC base pointer.
 * @return Conversion speed mode.
 */
static inline uint8_t ADC_GetConvertSpeed(ADC_Type* base)
{
    return (uint8_t)((ADC_CFG_REG(base) & ADC_CFG_ADHSC_MASK) >> ADC_CFG_ADHSC_SHIFT);
}

/*!
 * @brief This function set ADC module sample time duration.
 *
 * @param base ADC base pointer.
 * @param duration Sample time duration (see @ref _adc_sample_time_duration enumeration).
 */
void ADC_SetSampleTimeDuration(ADC_Type* base, uint8_t duration);

/*!
 * @brief This function set ADC module power mode.
 *
 * @param base ADC base pointer.
 * @param powerMode power mode (see @ref _adc_power_mode enumeration).
 */
void ADC_SetPowerMode(ADC_Type* base, uint8_t powerMode);

/*!
 * @brief This function get ADC module power mode.
 *
 * @param base ADC base pointer.
 * @return Power mode.
 */
static inline uint8_t ADC_GetPowerMode(ADC_Type* base)
{
    return (uint8_t)((ADC_CFG_REG(base) & ADC_CFG_ADLPC_MASK) >> ADC_CFG_ADLPC_SHIFT);
}

/*!
 * @brief This function set ADC module clock source.
 *
 * @param base ADC base pointer.
 * @param source Conversion clock source (see @ref _adc_clock_source enumeration).
 * @param div Input clock divide ratio (see @ref _adc_clock_divide enumeration).
 */
void ADC_SetClockSource(ADC_Type* base, uint8_t source, uint8_t div);

/*!
 * @brief This function enable asynchronous clock source output regardless of the
 *        state of ADC and input clock select of ADC module. Setting this bit
 *        allows the clock to be used even while the ADC is idle or operating from
 *        a different clock source.
 *
 * @param base ADC base pointer.
 * @param enable Asynchronous clock output enable.
 *               - true: Enable asynchronous clock output regardless of the state of ADC;
 *               - false: Only enable if selected as ADC input clock source and a
 *                        ADC conversion is active.
 */
void ADC_SetAsynClockOutput(ADC_Type* base, bool enable);

/*@}*/

/*!
 * @name ADC Calibration Control Functions.
 * @{
 */

/*!
 * @brief This function is used to enable or disable calibration function.
 *
 * @param base ADC base pointer.
 * @param enable Enable/Disable calibration function.
 *               - true: Enable calibration function.
 *               - false: Disable calibration function.
 */
void ADC_SetCalibration(ADC_Type* base, bool enable);

/*!
 * @brief This function is used to get calibrate result value.
 *
 * @param base ADC base pointer.
 * @return Calibration result value.
 */
static inline uint8_t ADC_GetCalibrationResult(ADC_Type* base)
{
    return (uint8_t)((ADC_CAL_REG(base) & ADC_CAL_CAL_CODE_MASK) >> ADC_CAL_CAL_CODE_SHIFT);
}

/*@}*/

/*!
 * @name ADC Module Conversion Control Functions.
 * @{
 */

/*!
 * @brief Enable continuous conversion and start a conversion on target channel.
 *        This function is only used for software trigger mode. If configured as
 *        hardware trigger mode, this function just enable continuous conversion mode
 *        and not start the conversion.
 *
 * @param base ADC base pointer.
 * @param channel Input channel selection.
 * @param enable Enable/Disable continuous conversion.
 *               - true: Enable and start continuous conversion.
 *               - false: Disable continuous conversion.
 */
void ADC_SetConvertCmd(ADC_Type* base, uint8_t channel, bool enable);

/*!
 * @brief Enable single conversion and trigger single time conversion
 *        on target input channel.If configured as hardware trigger
 *        mode, this function just set input channel and not start a
 *        conversion.
 *
 * @param base ADC base pointer.
 * @param channel Input channel selection.
 */
void ADC_TriggerSingleConvert(ADC_Type* base, uint8_t channel);

/*!
 * @brief Enable hardware average function and set hardware average number.
 *
 * @param base ADC base pointer.
 * @param avgNum Hardware average number (see @ref _adc_average_number enumeration).
 *        If avgNum is equal to adcAvgNumNone, it means disable hardware
 *        average function.
 */
void ADC_SetAverageNum(ADC_Type* base, uint8_t avgNum);

/*!
 * @brief Set conversion resolution mode.
 *
 * @param base ADC base pointer.
 * @param mode resolution mode (see @ref _adc_resolution_mode enumeration).
 */
static inline void ADC_SetResolutionMode(ADC_Type* base, uint8_t mode)
{
    assert(mode <= adcResolutionBit12);

    ADC_CFG_REG(base) = (ADC_CFG_REG(base) & (~ADC_CFG_MODE_MASK)) |
                        ADC_CFG_MODE(mode);
}

/*!
 * @brief Set conversion resolution mode.
 *
 * @param base ADC base pointer.
 * @return Resolution mode (see @ref _adc_resolution_mode enumeration).
 */
static inline uint8_t ADC_GetResolutionMode(ADC_Type* base)
{
    return (uint8_t)((ADC_CFG_REG(base) & ADC_CFG_MODE_MASK) >> ADC_CFG_MODE_SHIFT);
}

/*!
 * @brief Set conversion disabled.
 *
 * @param base ADC base pointer.
 */
void ADC_StopConvert(ADC_Type* base);

/*!
 * @brief Get right aligned conversion result.
 *
 * @param base ADC base pointer.
 * @return Conversion result.
 */
uint16_t ADC_GetConvertResult(ADC_Type* base);

/*@}*/

/*!
 * @name ADC Comparer Control Functions.
 * @{
 */

/*!
 * @brief Enable compare function and set the compare work mode of ADC module.
 *        If cmpMode is equal to adcCmpModeDisable, it means to disable the compare function.
 * @param base ADC base pointer.
 * @param cmpMode Comparer work mode selected (see @ref _adc_compare_mode enumeration).
 *                - adcCmpModeLessThanCmpVal1: only set compare value 1;
 *                - adcCmpModeGreaterThanCmpVal1: only set compare value 1;
 *                - adcCmpModeOutRangNotInclusive: set compare value 1 less than or equal to compare value 2;
 *                - adcCmpModeInRangNotInclusive: set compare value 1 greater than compare value 2;
 *                - adcCmpModeInRangInclusive: set compare value 1 less than or equal to compare value 2;
 *                - adcCmpModeOutRangInclusive: set compare value 1 greater than compare value 2;
 *                - adcCmpModeDisable: unnecessary to set compare value 1 and compare value 2.
 * @param cmpVal1 Compare threshold 1.
 * @param cmpVal2 Compare threshold 2.
 */
void ADC_SetCmpMode(ADC_Type* base, uint8_t cmpMode, uint16_t cmpVal1, uint16_t cmpVal2);

/*@}*/

/*!
 * @name Offset Correction Control Functions.
 * @{
 */

/*!
 * @brief Set ADC module offset correct mode.
 *
 * @param base ADC base pointer.
 * @param correctMode Offset correct mode.
 *                    - true: The offset value is subtracted from the raw converted value;
 *                    - false: The offset value is added with the raw result.
 */
void ADC_SetCorrectionMode(ADC_Type* base, bool correctMode);

/*!
 * @brief Set ADC module offset value.
 *
 * @param base ADC base pointer.
 * @param val Offset value.
 */
static inline void ADC_SetOffsetVal(ADC_Type* base, uint16_t val)
{
    ADC_OFS_REG(base) = (ADC_OFS_REG(base) & (~ADC_OFS_OFS_MASK)) |
                       ADC_OFS_OFS(val);
}

/*@}*/

/*!
 * @name Interrupt and Flag Control Functions.
 * @{
 */

/*!
 * @brief Enables or disables ADC conversion complete interrupt request.
 *
 * @param base ADC base pointer.
 * @param enable Enable/Disable ADC conversion complete interrupt.
 *               - true: Enable conversion complete interrupt.
 *               - false: Disable conversion complete interrupt.
 */
void ADC_SetIntCmd(ADC_Type* base, bool enable);

/*!
 * @brief Gets the ADC module conversion complete status flag state.
 *
 * @param base ADC base pointer.
 * @retval true: A conversion is completed.
 * @retval false: A conversion is not completed.
 */
bool ADC_IsConvertComplete(ADC_Type* base);

/*!
 * @brief Gets the ADC module general status flag state.
 *
 * @param base ADC base pointer.
 * @param flags ADC status flag mask (see @ref _adc_general_status_flag enumeration).
 * @return ADC status, each bit represents one status flag.
 */
static inline uint32_t ADC_GetStatusFlag(ADC_Type* base, uint32_t flags)
{
    return (uint32_t)(ADC_GS_REG(base) & flags);
}

/*!
 * @brief Clear one or more ADC status flag state.
 *
 * @param base ADC base pointer.
 * @param flags ADC status flag mask (see @ref _adc_general_status_flag enumeration).
 */
static inline void ADC_ClearStatusFlag(ADC_Type* base, uint32_t flags)
{
    assert(flags < adcFlagConvertActive);
    ADC_GS_REG(base) = flags;
}

/*@}*/

/*!
 * @name DMA Control Functions.
 * @{
 */

/*!
 * @brief Enable or Disable DMA request.
 *
 * @param base ADC base pointer.
 * @param enable Enable/Disable ADC DMA request.
 *               - true: Enable DMA request.
 *               - false: Disable DMA request.
 */
void ADC_SetDmaCmd(ADC_Type* base, bool enable);

/*@}*/

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* __ADC_IMX6SX_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
