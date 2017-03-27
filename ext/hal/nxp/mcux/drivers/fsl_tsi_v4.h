/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
 * o Neither the name of the copyright holder nor the names of its
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
#ifndef _FSL_TSI_V4_H_
#define _FSL_TSI_V4_H_

#include "fsl_common.h"

/*!
 * @addtogroup tsi_v4_driver
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief TSI driver version */
#define FSL_TSI_DRIVER_VERSION (MAKE_VERSION(2, 1, 2))
/*@}*/

/*! @brief TSI status flags macro collection */
#define ALL_FLAGS_MASK  (TSI_GENCS_EOSF_MASK | TSI_GENCS_OUTRGF_MASK)

/*! @brief resistor bit shift in EXTCHRG bit-field */
#define TSI_V4_EXTCHRG_RESISTOR_BIT_SHIFT TSI_GENCS_EXTCHRG_SHIFT

/*! @brief filter bits shift in EXTCHRG bit-field  */
#define TSI_V4_EXTCHRG_FILTER_BITS_SHIFT (1U + TSI_GENCS_EXTCHRG_SHIFT)

/*! @brief macro of clearing the resistor bit in EXTCHRG bit-field */
#define TSI_V4_EXTCHRG_RESISTOR_BIT_CLEAR \
    ((uint32_t)((~(ALL_FLAGS_MASK | TSI_GENCS_EXTCHRG_MASK)) | (3U << TSI_V4_EXTCHRG_FILTER_BITS_SHIFT)))

/*! @brief macro of clearing the filter bits in EXTCHRG bit-field */
#define TSI_V4_EXTCHRG_FILTER_BITS_CLEAR \
    ((uint32_t)((~(ALL_FLAGS_MASK | TSI_GENCS_EXTCHRG_MASK)) | (1U << TSI_V4_EXTCHRG_RESISTOR_BIT_SHIFT)))

/*!
 * @brief TSI number of scan intervals for each electrode.
 *
 * These constants define the tsi number of consecutive scans in a TSI instance for each electrode.
 */
typedef enum _tsi_n_consecutive_scans
{
    kTSI_ConsecutiveScansNumber_1time = 0U,   /*!< Once per electrode */
    kTSI_ConsecutiveScansNumber_2time = 1U,   /*!< Twice per electrode */
    kTSI_ConsecutiveScansNumber_3time = 2U,   /*!< 3 times consecutive scan */
    kTSI_ConsecutiveScansNumber_4time = 3U,   /*!< 4 times consecutive scan */
    kTSI_ConsecutiveScansNumber_5time = 4U,   /*!< 5 times consecutive scan */
    kTSI_ConsecutiveScansNumber_6time = 5U,   /*!< 6 times consecutive scan */
    kTSI_ConsecutiveScansNumber_7time = 6U,   /*!< 7 times consecutive scan */
    kTSI_ConsecutiveScansNumber_8time = 7U,   /*!< 8 times consecutive scan */
    kTSI_ConsecutiveScansNumber_9time = 8U,   /*!< 9 times consecutive scan */
    kTSI_ConsecutiveScansNumber_10time = 9U,  /*!< 10 times consecutive scan */
    kTSI_ConsecutiveScansNumber_11time = 10U, /*!< 11 times consecutive scan */
    kTSI_ConsecutiveScansNumber_12time = 11U, /*!< 12 times consecutive scan */
    kTSI_ConsecutiveScansNumber_13time = 12U, /*!< 13 times consecutive scan */
    kTSI_ConsecutiveScansNumber_14time = 13U, /*!< 14 times consecutive scan */
    kTSI_ConsecutiveScansNumber_15time = 14U, /*!< 15 times consecutive scan */
    kTSI_ConsecutiveScansNumber_16time = 15U, /*!< 16 times consecutive scan */
    kTSI_ConsecutiveScansNumber_17time = 16U, /*!< 17 times consecutive scan */
    kTSI_ConsecutiveScansNumber_18time = 17U, /*!< 18 times consecutive scan */
    kTSI_ConsecutiveScansNumber_19time = 18U, /*!< 19 times consecutive scan */
    kTSI_ConsecutiveScansNumber_20time = 19U, /*!< 20 times consecutive scan */
    kTSI_ConsecutiveScansNumber_21time = 20U, /*!< 21 times consecutive scan */
    kTSI_ConsecutiveScansNumber_22time = 21U, /*!< 22 times consecutive scan */
    kTSI_ConsecutiveScansNumber_23time = 22U, /*!< 23 times consecutive scan */
    kTSI_ConsecutiveScansNumber_24time = 23U, /*!< 24 times consecutive scan */
    kTSI_ConsecutiveScansNumber_25time = 24U, /*!< 25 times consecutive scan */
    kTSI_ConsecutiveScansNumber_26time = 25U, /*!< 26 times consecutive scan */
    kTSI_ConsecutiveScansNumber_27time = 26U, /*!< 27 times consecutive scan */
    kTSI_ConsecutiveScansNumber_28time = 27U, /*!< 28 times consecutive scan */
    kTSI_ConsecutiveScansNumber_29time = 28U, /*!< 29 times consecutive scan */
    kTSI_ConsecutiveScansNumber_30time = 29U, /*!< 30 times consecutive scan */
    kTSI_ConsecutiveScansNumber_31time = 30U, /*!< 31 times consecutive scan */
    kTSI_ConsecutiveScansNumber_32time = 31U  /*!< 32 times consecutive scan */
} tsi_n_consecutive_scans_t;

/*!
 * @brief TSI electrode oscillator prescaler.
 *
 * These constants define the TSI electrode oscillator prescaler in a TSI instance.
 */
typedef enum _tsi_electrode_osc_prescaler
{
    kTSI_ElecOscPrescaler_1div = 0U,  /*!< Electrode oscillator frequency divided by 1 */
    kTSI_ElecOscPrescaler_2div = 1U,  /*!< Electrode oscillator frequency divided by 2 */
    kTSI_ElecOscPrescaler_4div = 2U,  /*!< Electrode oscillator frequency divided by 4 */
    kTSI_ElecOscPrescaler_8div = 3U,  /*!< Electrode oscillator frequency divided by 8 */
    kTSI_ElecOscPrescaler_16div = 4U, /*!< Electrode oscillator frequency divided by 16 */
    kTSI_ElecOscPrescaler_32div = 5U, /*!< Electrode oscillator frequency divided by 32 */
    kTSI_ElecOscPrescaler_64div = 6U, /*!< Electrode oscillator frequency divided by 64 */
    kTSI_ElecOscPrescaler_128div = 7U /*!< Electrode oscillator frequency divided by 128 */
} tsi_electrode_osc_prescaler_t;

/*!
 * @brief TSI analog mode select.
 *
 * Set up TSI analog modes in a TSI instance.
 */
typedef enum _tsi_analog_mode
{
    kTSI_AnalogModeSel_Capacitive = 0U,     /*!< Active TSI capacitive sensing mode */
    kTSI_AnalogModeSel_NoiseNoFreqLim = 4U, /*!< Single threshold noise detection mode with no freq. limitation. */
    kTSI_AnalogModeSel_NoiseFreqLim = 8U,   /*!< Single threshold noise detection mode with freq. limitation. */
    kTSI_AnalogModeSel_AutoNoise = 12U      /*!< Active TSI analog in automatic noise detection mode */
} tsi_analog_mode_t;

/*!
 * @brief TSI Reference oscillator charge and discharge current select.
 *
 * These constants define the TSI Reference oscillator charge current select in a TSI (REFCHRG) instance.
 */
typedef enum _tsi_reference_osc_charge_current
{
    kTSI_RefOscChargeCurrent_500nA = 0U, /*!< Reference oscillator charge current is 500 µA */
    kTSI_RefOscChargeCurrent_1uA = 1U,   /*!< Reference oscillator charge current is 1 µA */
    kTSI_RefOscChargeCurrent_2uA = 2U,   /*!< Reference oscillator charge current is 2 µA */
    kTSI_RefOscChargeCurrent_4uA = 3U,   /*!< Reference oscillator charge current is 4 µA */
    kTSI_RefOscChargeCurrent_8uA = 4U,   /*!< Reference oscillator charge current is 8 µA */
    kTSI_RefOscChargeCurrent_16uA = 5U,  /*!< Reference oscillator charge current is 16 µA */
    kTSI_RefOscChargeCurrent_32uA = 6U,  /*!< Reference oscillator charge current is 32 µA */
    kTSI_RefOscChargeCurrent_64uA = 7U   /*!< Reference oscillator charge current is 64 µA */
} tsi_reference_osc_charge_current_t;

/*!
 * @brief TSI oscilator's voltage rails.
 *
 * These bits indicate the oscillator's voltage rails.
 */
typedef enum _tsi_osc_voltage_rails
{
    kTSI_OscVolRailsOption_0 = 0U, /*!< DVOLT value option 0, the value may differ on different platforms */
    kTSI_OscVolRailsOption_1 = 1U, /*!< DVOLT value option 1, the value may differ on different platforms */
    kTSI_OscVolRailsOption_2 = 2U, /*!< DVOLT value option 2, the value may differ on different platforms */
    kTSI_OscVolRailsOption_3 = 3U  /*!< DVOLT value option 3, the value may differ on different platforms */
} tsi_osc_voltage_rails_t;

/*!
 * @brief TSI External oscillator charge and discharge current select.
 *
 * These bits indicate the electrode oscillator charge and discharge current value
 * in TSI (EXTCHRG) instance.
 */
typedef enum _tsi_external_osc_charge_current
{
    kTSI_ExtOscChargeCurrent_500nA = 0U, /*!< External oscillator charge current is 500 µA */
    kTSI_ExtOscChargeCurrent_1uA = 1U,   /*!< External oscillator charge current is 1 µA */
    kTSI_ExtOscChargeCurrent_2uA = 2U,   /*!< External oscillator charge current is 2 µA */
    kTSI_ExtOscChargeCurrent_4uA = 3U,   /*!< External oscillator charge current is 4 µA */
    kTSI_ExtOscChargeCurrent_8uA = 4U,   /*!< External oscillator charge current is 8 µA */
    kTSI_ExtOscChargeCurrent_16uA = 5U,  /*!< External oscillator charge current is 16 µA */
    kTSI_ExtOscChargeCurrent_32uA = 6U,  /*!< External oscillator charge current is 32 µA */
    kTSI_ExtOscChargeCurrent_64uA = 7U   /*!< External oscillator charge current is 64 µA */
} tsi_external_osc_charge_current_t;

/*!
 * @brief TSI series resistance RS value select.
 *
 * These bits indicate the electrode RS series resistance for the noise mode
 * in TSI (EXTCHRG) instance.
 */
typedef enum _tsi_series_resistance
{
    kTSI_SeriesResistance_32k = 0U, /*!< Series Resistance is 32 kilo ohms   */
    kTSI_SeriesResistance_187k = 1U /*!< Series Resistance is 18 7 kilo ohms  */
} tsi_series_resistor_t;

/*!
 * @brief TSI series filter bits select.
 *
 * These bits indicate the count of the filter bits
 * in TSI noise mode EXTCHRG[2:1] bits
 */
typedef enum _tsi_filter_bits
{
    kTSI_FilterBits_3 = 0U, /*!< 3 filter bits, 8 peaks increments the cnt+1 */
    kTSI_FilterBits_2 = 1U, /*!< 2 filter bits, 4 peaks increments the cnt+1 */
    kTSI_FilterBits_1 = 2U, /*!< 1 filter bits, 2 peaks increments the cnt+1 */
    kTSI_FilterBits_0 = 3U  /*!< no filter bits,1 peak  increments the cnt+1 */
} tsi_filter_bits_t;

/*! @brief TSI status flags. */
typedef enum _tsi_status_flags
{
    kTSI_EndOfScanFlag = TSI_GENCS_EOSF_MASK,   /*!< End-Of-Scan flag */
    kTSI_OutOfRangeFlag = TSI_GENCS_OUTRGF_MASK /*!< Out-Of-Range flag */
} tsi_status_flags_t;

/*! @brief TSI feature interrupt source.*/
typedef enum _tsi_interrupt_enable
{
    kTSI_GlobalInterruptEnable = 1U,     /*!< TSI module global interrupt */
    kTSI_OutOfRangeInterruptEnable = 2U, /*!< Out-Of-Range interrupt */
    kTSI_EndOfScanInterruptEnable = 4U   /*!< End-Of-Scan interrupt */
} tsi_interrupt_enable_t;

/*! @brief TSI calibration data storage. */
typedef struct _tsi_calibration_data
{
    uint16_t calibratedData[FSL_FEATURE_TSI_CHANNEL_COUNT]; /*!< TSI calibration data storage buffer */
} tsi_calibration_data_t;

/*!
 * @brief TSI configuration structure.
 *
 * This structure contains the settings for the most common TSI configurations including
 * the TSI module charge currents, number of scans, thresholds, and so on.
 */
typedef struct _tsi_config
{
    uint16_t thresh;                            /*!< High threshold. */
    uint16_t thresl;                            /*!< Low threshold. */
    tsi_electrode_osc_prescaler_t prescaler;    /*!< Prescaler */
    tsi_external_osc_charge_current_t extchrg;  /*!< Electrode charge current */
    tsi_reference_osc_charge_current_t refchrg; /*!< Reference charge current */
    tsi_n_consecutive_scans_t nscn;             /*!< Number of scans. */
    tsi_analog_mode_t mode;                     /*!< TSI mode of operation. */
    tsi_osc_voltage_rails_t dvolt;              /*!< Oscillator's voltage rails. */
    tsi_series_resistor_t resistor;             /*!< Series resistance value  */
    tsi_filter_bits_t filter;                   /*!< Noise mode filter bits   */
} tsi_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Initializes hardware.
 *
 * @details Initializes the peripheral to the targeted state specified by parameter configuration,
 *          such as sets prescalers, number of scans, clocks, delta voltage
 *          series resistor, filter bits, reference, and electrode charge current and threshold.
 * @param  base    TSI peripheral base address.
 * @param  config  Pointer to TSI module configuration structure.
 * @return none
 */
void TSI_Init(TSI_Type *base, const tsi_config_t *config);

/*!
 * @brief De-initializes hardware.
 *
 * @details De-initializes the peripheral to default state.
 *
 * @param  base  TSI peripheral base address.
 * @return none
 */
void TSI_Deinit(TSI_Type *base);

/*!
 * @brief Gets the TSI normal mode user configuration structure.
 * This interface sets userConfig structure to a default value. The configuration structure only
 * includes the settings for the whole TSI.
 * The user configure is set to these values:
 * @code
    userConfig->prescaler = kTSI_ElecOscPrescaler_2div;
    userConfig->extchrg = kTSI_ExtOscChargeCurrent_500nA;
    userConfig->refchrg = kTSI_RefOscChargeCurrent_4uA;
    userConfig->nscn = kTSI_ConsecutiveScansNumber_10time;
    userConfig->mode = kTSI_AnalogModeSel_Capacitive;
    userConfig->dvolt = kTSI_OscVolRailsOption_0;
    userConfig->thresh = 0U;
    userConfig->thresl = 0U;
   @endcode
 *
 * @param userConfig Pointer to the TSI user configuration structure.
 */
void TSI_GetNormalModeDefaultConfig(tsi_config_t *userConfig);

/*!
 * @brief Gets the TSI low power mode default user configuration structure.
 * This interface sets userConfig structure to a default value. The configuration structure only
 * includes the settings for the whole TSI.
 * The user configure is set to these values:
 * @code
    userConfig->prescaler = kTSI_ElecOscPrescaler_2div;
    userConfig->extchrg = kTSI_ExtOscChargeCurrent_500nA;
    userConfig->refchrg = kTSI_RefOscChargeCurrent_4uA;
    userConfig->nscn = kTSI_ConsecutiveScansNumber_10time;
    userConfig->mode = kTSI_AnalogModeSel_Capacitive;
    userConfig->dvolt = kTSI_OscVolRailsOption_0;
    userConfig->thresh = 400U;
    userConfig->thresl = 0U;
   @endcode
 *
 * @param userConfig Pointer to the TSI user configuration structure.
 */
void TSI_GetLowPowerModeDefaultConfig(tsi_config_t *userConfig);

/*!
 * @brief Hardware calibration.
 *
 * @details Calibrates the peripheral to fetch the initial counter value of
 *          the enabled electrodes.
 *          This API is mostly used at initial application setup. Call
 *          this function after the \ref TSI_Init API and use the calibrated
 *          counter values to set up applications (such as to determine
 *          under which counter value we can confirm a touch event occurs).
 *
 * @param   base    TSI peripheral base address.
 * @param   calBuff Data buffer that store the calibrated counter value.
 * @return none
 *
 */
void TSI_Calibrate(TSI_Type *base, tsi_calibration_data_t *calBuff);

/*!
 * @brief Enables the TSI interrupt requests.
 * @param base TSI peripheral base address.
 * @param mask interrupt source
 *     The parameter can be combination of the following source if defined:
 *     @arg kTSI_GlobalInterruptEnable
 *     @arg kTSI_EndOfScanInterruptEnable
 *     @arg kTSI_OutOfRangeInterruptEnable
 */
void TSI_EnableInterrupts(TSI_Type *base, uint32_t mask);

/*!
 * @brief Disables the TSI interrupt requests.
 * @param base TSI peripheral base address.
 * @param mask interrupt source
 *     The parameter can be combination of the following source if defined:
 *     @arg kTSI_GlobalInterruptEnable
 *     @arg kTSI_EndOfScanInterruptEnable
 *     @arg kTSI_OutOfRangeInterruptEnable
 */
void TSI_DisableInterrupts(TSI_Type *base, uint32_t mask);

/*!
* @brief Gets an interrupt flag.
* This function gets the TSI interrupt flags.
*
* @param    base  TSI peripheral base address.
* @return         The mask of these status flags combination.
*/
static inline uint32_t TSI_GetStatusFlags(TSI_Type *base)
{
    return (base->GENCS & (kTSI_EndOfScanFlag | kTSI_OutOfRangeFlag));
}

/*!
 * @brief Clears the interrupt flag.
 *
 * This function clears the TSI interrupt flag,
 * automatically cleared flags can't be cleared by this function.
 *
 * @param base TSI peripheral base address.
 * @param mask The status flags to clear.
 */
void TSI_ClearStatusFlags(TSI_Type *base, uint32_t mask);

/*!
* @brief Gets the TSI scan trigger mode.
*
* @param  base  TSI peripheral base address.
* @return       Scan trigger mode.
*/
static inline uint32_t TSI_GetScanTriggerMode(TSI_Type *base)
{
    return (base->GENCS & TSI_GENCS_STM_MASK);
}

/*!
* @brief Gets the scan in progress flag.
*
* @param    base TSI peripheral base address.
* @return   True  - scan is in progress.
*           False - scan is not in progress.
*/
static inline bool TSI_IsScanInProgress(TSI_Type *base)
{
    return (base->GENCS & TSI_GENCS_SCNIP_MASK);
}

/*!
* @brief Sets the prescaler.
*
* @param    base      TSI peripheral base address.
* @param    prescaler Prescaler value.
* @return   none.
*/
static inline void TSI_SetElectrodeOSCPrescaler(TSI_Type *base, tsi_electrode_osc_prescaler_t prescaler)
{
    base->GENCS = (base->GENCS & ~(TSI_GENCS_PS_MASK | ALL_FLAGS_MASK)) | (TSI_GENCS_PS(prescaler));
}

/*!
* @brief Sets the number of scans (NSCN).
*
* @param    base    TSI peripheral base address.
* @param    number  Number of scans.
* @return   none.
*/
static inline void TSI_SetNumberOfScans(TSI_Type *base, tsi_n_consecutive_scans_t number)
{
    base->GENCS = (base->GENCS & ~(TSI_GENCS_NSCN_MASK | ALL_FLAGS_MASK)) | (TSI_GENCS_NSCN(number));
}

/*!
* @brief Enables/disables the TSI module.
*
* @param   base   TSI peripheral base address.
* @param   enable Choose whether to enable or disable module;
*                 - true   Enable TSI module;
*                 - false  Disable TSI module;
* @return  none.
*/
static inline void TSI_EnableModule(TSI_Type *base, bool enable)
{
    if (enable)
    {
        base->GENCS = (base->GENCS & ~ALL_FLAGS_MASK) | TSI_GENCS_TSIEN_MASK;    /* Enable module */
    }
    else
    {
        base->GENCS = (base->GENCS & ~ALL_FLAGS_MASK) & (~TSI_GENCS_TSIEN_MASK); /* Disable module */
    }
}

/*!
* @brief Sets the TSI low power STOP mode as enabled or disabled.
*        This enables the TSI module function in low power modes.
*
* @param    base    TSI peripheral base address.
* @param    enable  Choose to enable or disable STOP mode.
*                   - true   Enable module in STOP mode;
*                   - false  Disable module in STOP mode;
* @return   none.
*/
static inline void TSI_EnableLowPower(TSI_Type *base, bool enable)
{
    if (enable)
    {
        base->GENCS = (base->GENCS & ~ALL_FLAGS_MASK) | TSI_GENCS_STPE_MASK;    /* Module enabled in low power stop modes */
    }
    else
    {
        base->GENCS = (base->GENCS & ~ALL_FLAGS_MASK) & (~TSI_GENCS_STPE_MASK); /* Module disabled in low power stop modes */
    }
}

/*!
* @brief Enables/disables the hardware trigger scan.
*
* @param    base TSI peripheral base address.
* @param    enable Choose to enable hardware trigger or software trigger scan.
*                  - true    Enable hardware trigger scan;
*                  - false   Enable software trigger scan;
* @return   none.
*/
static inline void TSI_EnableHardwareTriggerScan(TSI_Type *base, bool enable)
{
    if (enable)
    {
        base->GENCS = (base->GENCS & ~ALL_FLAGS_MASK) | TSI_GENCS_STM_MASK;    /* Enable hardware trigger scan */
    }
    else
    {
        base->GENCS = (base->GENCS & ~ALL_FLAGS_MASK) & (~TSI_GENCS_STM_MASK); /* Enable software trigger scan */
    }
}

/*!
* @brief Starts a software trigger measurement (triggers a new measurement).
*
* @param    base TSI peripheral base address.
* @return   none.
*/
static inline void TSI_StartSoftwareTrigger(TSI_Type *base)
{
    base->DATA |= TSI_DATA_SWTS_MASK;
}

/*!
* @brief Sets the the measured channel number.
*
* @param    base    TSI peripheral base address.
* @param    channel Channel number 0 ... 15.
* @return   none.
*/
static inline void TSI_SetMeasuredChannelNumber(TSI_Type *base, uint8_t channel)
{
    assert(channel < FSL_FEATURE_TSI_CHANNEL_COUNT);

    base->DATA = ((base->DATA) & ~TSI_DATA_TSICH_MASK) | (TSI_DATA_TSICH(channel));
}

/*!
* @brief Gets the current measured channel number.
*
* @param    base    TSI peripheral base address.
* @return   uint8_t    Channel number 0 ... 15.
*/
static inline uint8_t TSI_GetMeasuredChannelNumber(TSI_Type *base)
{
    return (uint8_t)((base->DATA & TSI_DATA_TSICH_MASK) >> TSI_DATA_TSICH_SHIFT);
}

/*!
* @brief Enables/disables the DMA transfer.
*
* @param   base   TSI peripheral base address.
* @param   enable Choose to enable DMA transfer or not.
*                 - true    Enable DMA transfer;
*                 - false   Disable DMA transfer;
* @return  none.
*/
static inline void TSI_EnableDmaTransfer(TSI_Type *base, bool enable)
{
    if (enable)
    {
        base->DATA |= TSI_DATA_DMAEN_MASK; /* Enable DMA transfer */
    }
    else
    {
        base->DATA &= ~TSI_DATA_DMAEN_MASK; /* Disable DMA transfer */
    }
}

#if defined(FSL_FEATURE_TSI_HAS_END_OF_SCAN_DMA_ENABLE) && (FSL_FEATURE_TSI_HAS_END_OF_SCAN_DMA_ENABLE == 1)
/*!
* @brief Decides whether to enable end of scan DMA transfer request only.
*
* @param    base TSI peripheral base address.
* @param    enable  Choose whether to enable End of Scan DMA transfer request only.
*                   - true  Enable End of Scan DMA transfer request only;
*                   - false Both End-of-Scan and Out-of-Range can generate DMA transfer request.
* @return   none.
*/
static inline void TSI_EnableEndOfScanDmaTransferOnly(TSI_Type *base, bool enable)
{
    if (enable)
    {
        base->GENCS = (base->GENCS & ~ALL_FLAGS_MASK) | TSI_GENCS_EOSDMEO_MASK; /* Enable End of Scan DMA transfer request only; */
    }
    else
    {
        base->GENCS =
            (base->GENCS & ~ALL_FLAGS_MASK) & (~TSI_GENCS_EOSDMEO_MASK); /* Both End-of-Scan and Out-of-Range can generate DMA transfer request. */
    }
}
#endif /* End of (FSL_FEATURE_TSI_HAS_END_OF_SCAN_DMA_ENABLE == 1)*/

/*!
* @brief Gets the conversion counter value.
*
* @param    base TSI peripheral base address.
* @return   Accumulated scan counter value ticked by the reference clock.
*/
static inline uint16_t TSI_GetCounter(TSI_Type *base)
{
    return (uint16_t)(base->DATA & TSI_DATA_TSICNT_MASK);
}

/*!
* @brief Sets the TSI wake-up channel low threshold.
*
* @param    base           TSI peripheral base address.
* @param    low_threshold  Low counter threshold.
* @return   none.
*/
static inline void TSI_SetLowThreshold(TSI_Type *base, uint16_t low_threshold)
{
    assert(low_threshold < 0xFFFFU);

    base->TSHD = ((base->TSHD) & ~TSI_TSHD_THRESL_MASK) | (TSI_TSHD_THRESL(low_threshold));
}

/*!
* @brief Sets the TSI wake-up channel high threshold.
*
* @param    base            TSI peripheral base address.
* @param    high_threshold  High counter threshold.
* @return   none.
*/
static inline void TSI_SetHighThreshold(TSI_Type *base, uint16_t high_threshold)
{
    assert(high_threshold < 0xFFFFU);

    base->TSHD = ((base->TSHD) & ~TSI_TSHD_THRESH_MASK) | (TSI_TSHD_THRESH(high_threshold));
}

/*!
* @brief Sets the analog mode of the TSI module.
*
* @param    base   TSI peripheral base address.
* @param    mode   Mode value.
* @return   none.
*/
static inline void TSI_SetAnalogMode(TSI_Type *base, tsi_analog_mode_t mode)
{
    base->GENCS = (base->GENCS & ~(TSI_GENCS_MODE_MASK | ALL_FLAGS_MASK)) | (TSI_GENCS_MODE(mode));
}

/*!
* @brief Gets the noise mode result of the TSI module.
*
* @param   base  TSI peripheral base address.
* @return        Value of the GENCS[MODE] bit-fields.
*/
static inline uint8_t TSI_GetNoiseModeResult(TSI_Type *base)
{
    return (base->GENCS & TSI_GENCS_MODE_MASK) >> TSI_GENCS_MODE_SHIFT;
}

/*!
* @brief Sets the reference oscillator charge current.
*
* @param   base    TSI peripheral base address.
* @param   current The reference oscillator charge current.
* @return  none.
*/
static inline void TSI_SetReferenceChargeCurrent(TSI_Type *base, tsi_reference_osc_charge_current_t current)
{
    base->GENCS = (base->GENCS & ~(TSI_GENCS_REFCHRG_MASK | ALL_FLAGS_MASK)) | (TSI_GENCS_REFCHRG(current));
}

/*!
* @brief Sets the external electrode charge current.
*
* @param    base    TSI peripheral base address.
* @param    current External electrode charge current.
* @return   none.
*/
static inline void TSI_SetElectrodeChargeCurrent(TSI_Type *base, tsi_external_osc_charge_current_t current)
{
    base->GENCS = (base->GENCS & ~(TSI_GENCS_EXTCHRG_MASK | ALL_FLAGS_MASK)) | (TSI_GENCS_EXTCHRG(current));
}

/*!
* @brief Sets the oscillator's voltage rails.
*
* @param    base    TSI peripheral base address.
* @param    dvolt   The voltage rails.
* @return   none.
*/
static inline void TSI_SetOscVoltageRails(TSI_Type *base, tsi_osc_voltage_rails_t dvolt)
{
    base->GENCS = (base->GENCS & ~(TSI_GENCS_DVOLT_MASK | ALL_FLAGS_MASK)) | (TSI_GENCS_DVOLT(dvolt));
}

/*!
* @brief Sets the electrode series resistance value in EXTCHRG[0] bit.
*
* @param    base     TSI peripheral base address.
* @param    resistor Series resistance.
* @return   none.
*/
static inline void TSI_SetElectrodeSeriesResistor(TSI_Type *base, tsi_series_resistor_t resistor)
{
    base->GENCS = (base->GENCS & TSI_V4_EXTCHRG_RESISTOR_BIT_CLEAR) | TSI_GENCS_EXTCHRG(resistor);
}

/*!
* @brief Sets the electrode filter bits value in EXTCHRG[2:1] bits.
*
* @param    base    TSI peripheral base address.
* @param    filter  Series resistance.
* @return   none.
*/
static inline void TSI_SetFilterBits(TSI_Type *base, tsi_filter_bits_t filter)
{
    base->GENCS = (base->GENCS & TSI_V4_EXTCHRG_FILTER_BITS_CLEAR) | (filter << TSI_V4_EXTCHRG_FILTER_BITS_SHIFT);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_TSI_V4_H_ */
