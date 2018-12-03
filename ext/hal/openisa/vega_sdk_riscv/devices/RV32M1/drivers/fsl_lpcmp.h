/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_LPCMP_H_
#define _FSL_LPCMP_H_

#include "fsl_common.h"

/*!
 * @addtogroup lpcmp
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief LPCMP driver version 2.0.0. */
#define FSL_LPCMP_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*!
* @brief LPCMP status falgs mask.
*/
enum _lpcmp_status_flags
{
    kLPCMP_OutputRisingEventFlag = LPCMP_CSR_CFR_MASK,  /*!< Rising-edge on the comparison output has occurred. */
    kLPCMP_OutputFallingEventFlag = LPCMP_CSR_CFF_MASK, /*!< Falling-edge on the comparison output has occurred. */
    kLPCMP_OutputAssertEventFlag = LPCMP_CSR_COUT_MASK, /*!< Return the current value of the analog comparator output.
                                                             The flag does not support W1C. */
};

/*!
* @brief LPCMP interrupt enable/disable mask.
*/
enum _lpcmp_interrupt_enable
{
    kLPCMP_OutputRisingInterruptEnable = LPCMP_IER_CFR_IE_MASK,  /*!< Comparator interrupt enable rising. */
    kLPCMP_OutputFallingInterruptEnable = LPCMP_IER_CFF_IE_MASK, /*!< Comparator interrupt enable falling. */
};
/*!
* @brief LPCMP hysteresis mode. See chip data sheet to get the actual hystersis
*        value with each level
*/
typedef enum _lpcmp_hysteresis_mode
{
    kLPCMP_HysteresisLevel0 = 0U, /*!< The hard block output has level 0 hysteresis internally. */
    kLPCMP_HysteresisLevel1 = 1U, /*!< The hard block output has level 1 hysteresis internally. */
    kLPCMP_HysteresisLevel2 = 2U, /*!< The hard block output has level 2 hysteresis internally. */
    kLPCMP_HysteresisLevel3 = 3U, /*!< The hard block output has level 3 hysteresis internally. */
} lpcmp_hysteresis_mode_t;

/*!
* @brief LPCMP nano mode.
*/
typedef enum _lpcmp_power_mode
{
    kLPCMP_LowSpeedPowerMode = 0U,  /*!< Low speed comparison mode is selected. */
    kLPCMP_HighSpeedPowerMode = 1U, /*!< High speed comparison mode is selected. */
    kLPCMP_NanoPowerMode = 2U,      /*!< Nano power comparator is enabled. */
} lpcmp_power_mode_t;

/*!
* @brief Internal DAC reference voltage source.
*/
typedef enum _lpcmp_dac_reference_voltage_source
{
    kLPCMP_VrefSourceVin1 = 0U, /*!< vrefh_int is selected as resistor ladder network supply reference Vin. */
    kLPCMP_VrefSourceVin2 = 1U, /*!< vrefh_ext is selected as resistor ladder network supply reference Vin. */
} lpcmp_dac_reference_voltage_source_t;

/*!
* @brief Configure the filter.
*/
typedef struct _lpcmp_filter_config
{
    bool enableSample;          /*!< Decide whether to use the external SAMPLE as a sampling clock input. */
    uint8_t filterSampleCount;  /*!< Filter Sample Count. Available range is 1-7; 0 disables the filter. */
    uint8_t filterSamplePeriod; /*!< Filter Sample Period. The divider to the bus clock. Available range is 0-255. The
                                sampling clock must be at least 4 times slower than the system clock to the comparator.
                                So if enableSample is "false", filterSamplePeriod should be set greater than 4.*/
} lpcmp_filter_config_t;

/*!
* @brief configure the internal DAC.
*/
typedef struct _lpcmp_dac_config
{
    bool enableLowPowerMode;                                     /*!< Decide whether to enable DAC low power mode. */
    lpcmp_dac_reference_voltage_source_t referenceVoltageSource; /*!< Internal DAC supply voltage reference source. */
    uint8_t DACValue; /*!< Value for the DAC Output Voltage. Available range is 0-63.*/
} lpcmp_dac_config_t;

/*!
* @brief Configures the comparator.
*/
typedef struct _lpcmp_config
{
    bool enableStopMode;      /*!< Decide whether to enable the comparator when in STOP modes. */
    bool enableOutputPin;     /*!< Decide whether to enable the comparator is available in selected pin. */
    bool useUnfilteredOutput; /*!< Decide whether to use unfiltered output. */
    bool enableInvertOutput;  /*!< Decide whether to inverts the comparator output. */
    lpcmp_hysteresis_mode_t hysteresisMode; /*!< LPCMP hysteresis mode. */
    lpcmp_power_mode_t powerMode;           /*!< LPCMP power mode. */
} lpcmp_config_t;
/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @name Initialization
 * @{
 */

/*!
* @brief Initialize the LPCMP
*
* This function initializes the LPCMP module. The operations included are:
* - Enabling the clock for LPCMP module.
* - Configuring the comparator.
* - Enabling the LPCMP module.
* Note: For some devices, multiple LPCMP instance share the same clock gate. In this case, to enable the clock for
* any instance enables all the LPCMPs. Check the chip reference manual for the clock assignment of the LPCMP.
*
* @param base LPCMP peripheral base address.
* @param config Pointer to "lpcmp_config_t" structure.
*/
void LPCMP_Init(LPCMP_Type *base, const lpcmp_config_t *config);

/*!
 * @brief De-initializes the LPCMP module.
 *
 * This function de-initializes the LPCMP module. The operations included are:
 * - Disabling the LPCMP module.
 * - Disabling the clock for LPCMP module.
 *
 * This function disables the clock for the LPCMP.
 * Note: For some devices, multiple LPCMP instance shares the same clock gate. In this case, before disabling the
 * clock for the LPCMP, ensure that all the LPCMP instances are not used.
 *
 * @param base LPCMP peripheral base address.
 */
void LPCMP_Deinit(LPCMP_Type *base);

/*!
* @brief Gets an available pre-defined settings for the comparator's configuration.
*
* This function initializes the comparator configuration structure to these default values:
* @code
*   config->enableStopMode      = false;
*   config->enableOutputPin     = false;
*   config->useUnfilteredOutput = false;
*   config->enableInvertOutput  = false;
*   config->hysteresisMode      = kLPCMP_HysteresisLevel0;
*   config->powerMode           = kLPCMP_LowSpeedPowerMode;
* @endcode
* @param config Pointer to "lpcmp_config_t" structure.
*/
void LPCMP_GetDefaultConfig(lpcmp_config_t *config);

/*!
* @brief Enable/Disable LPCMP module.
*
* @param base LPCMP peripheral base address.
* @param enable "true" means enable the module, and "false" means disable the module.
*/
static inline void LPCMP_Enable(LPCMP_Type *base, bool enable)
{
    if (enable)
    {
        base->CCR0 |= LPCMP_CCR0_CMP_EN_MASK;
    }
    else
    {
        base->CCR0 &= ~LPCMP_CCR0_CMP_EN_MASK;
    }
}

/*!
* @brief Select the input channels for LPCMP. This function determines which input
*        is selected for the negative and positive mux.
*
* @param base LPCMP peripheral base address.
* @param positiveChannel Positive side input channel number. Available range is 0-7.
* @param negativeChannel Negative side input channel number. Available range is 0-7.
*/
void LPCMP_SetInputChannels(LPCMP_Type *base, uint32_t positiveChannel, uint32_t negativeChannel);

/*!
* @brief Enables/disables the DMA request for rising/falling events.
*        Normally, the LPCMP generates a CPU interrupt if there is a rising/falling event. When
*        DMA support is enabled and the rising/falling interrupt is enabled , the rising/falling
*        event forces a DMA transfer request rather than a CPU interrupt instead.
*
* @param base LPCMP peripheral base address.
* @param enable "true" means enable DMA support, and "false" means disable DMA support.
*/
static inline void LPCMP_EnableDMA(LPCMP_Type *base, bool enable)
{
    if (enable)
    {
        base->CCR1 |= LPCMP_CCR1_DMA_EN_MASK;
    }
    else
    {
        base->CCR1 &= ~LPCMP_CCR1_DMA_EN_MASK;
    }
}

/*!
* @brief Enable/Disable window mode.When any windowed mode is active, COUTA is clocked by
*        the bus clock whenever WINDOW = 1. The last latched value is held when WINDOW = 0.
*        The optionally inverted comparator output COUT_RAW is sampled on every bus clock
*        when WINDOW=1 to generate COUTA.
*
* @param base LPCMP peripheral base address.
* @param enable "true" means enable window mode, and "false" means disable window mode.
*/
static inline void LPCMP_EnableWindowMode(LPCMP_Type *base, bool enable)
{
    if (enable)
    {
        base->CCR1 |= LPCMP_CCR1_WINDOW_EN_MASK;
    }
    else
    {
        base->CCR1 &= ~LPCMP_CCR1_WINDOW_EN_MASK;
    }
}

/*!
* @brief Configures the filter.
*
* @param base LPCMP peripheral base address.
* @param config Pointer to "lpcmp_filter_config_t" structure.
*/
void LPCMP_SetFilterConfig(LPCMP_Type *base, const lpcmp_filter_config_t *config);

/*!
* @brief Configure the internal DAC module.
*
* @param base LPCMP peripheral base address.
* @param config Pointer to "lpcmp_dac_config_t" structure. If config is "NULL", disable internal DAC.
*/
void LPCMP_SetDACConfig(LPCMP_Type *base, const lpcmp_dac_config_t *config);

/*!
* @brief Enable the interrupts.
*
* @param base LPCMP peripheral base address.
* @param mask Mask value for interrupts. See "_lpcmp_interrupt_enable".
*/
static inline void LPCMP_EnableInterrupts(LPCMP_Type *base, uint32_t mask)
{
    base->IER |= mask;
}

/*!
* @brief Disable the interrupts.
*
* @param base LPCMP peripheral base address.
* @param mask Mask value for interrupts. See "_lpcmp_interrupt_enable".
*/
static inline void LPCMP_DisableInterrupts(LPCMP_Type *base, uint32_t mask)
{
    base->IER &= ~mask;
}

/*!
* @brief Get the LPCMP status flags.
*
* @param LPCMP peripheral base address.
*
* @return Mask value for the asserted flags. See "_lpcmp_status_flags".
*/
static inline uint32_t LPCMP_GetStatusFlags(LPCMP_Type *base)
{
    return base->CSR;
}

/*!
* @brief Clear the LPCMP status flags
*
* @param base LPCMP peripheral base address.
* @param mask Mask value for the flags. See "_lpcmp_status_flags".
*/
static inline void LPCMP_ClearStatusFlags(LPCMP_Type *base, uint32_t mask)
{
    base->CSR = mask;
}

#endif /* _FSL_LPCMP_H_ */
