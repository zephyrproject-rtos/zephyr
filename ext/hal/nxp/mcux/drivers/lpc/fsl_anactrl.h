/*
 * Copyright (c) 2018, NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __FSL_ANACTRL_H__
#define __FSL_ANACTRL_H__

#include "fsl_common.h"

/*!
 * @addtogroup anactrl
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief ANACTRL driver version. */
#define FSL_ANACTRL_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0. */`

/*!
 * @brief ANACTRL interrupt flags
 */
enum _anactrl_interrupt_flags
{
    kANACTRL_BodVbatFlag =
        ANACTRL_BOD_DCDC_INT_STATUS_BODVBAT_STATUS_MASK, /*!< BOD VBAT Interrupt status before Interrupt Enable. */
    kANACTRL_BodVbatInterruptFlag =
        ANACTRL_BOD_DCDC_INT_STATUS_BODVBAT_INT_STATUS_MASK, /*!< BOD VBAT Interrupt status after Interrupt Enable. */
    kANACTRL_BodVbatPowerFlag =
        ANACTRL_BOD_DCDC_INT_STATUS_BODVBAT_VAL_MASK, /*!< Current value of BOD VBAT power status output. */
    kANACTRL_BodCoreFlag =
        ANACTRL_BOD_DCDC_INT_STATUS_BODCORE_STATUS_MASK, /*!< BOD CORE Interrupt status before Interrupt Enable. */
    kANACTRL_BodCoreInterruptFlag =
        ANACTRL_BOD_DCDC_INT_STATUS_BODCORE_INT_STATUS_MASK, /*!< BOD CORE Interrupt status after Interrupt Enable. */
    kANACTRL_BodCorePowerFlag =
        ANACTRL_BOD_DCDC_INT_STATUS_BODCORE_VAL_MASK, /*!< Current value of BOD CORE power status output. */
    kANACTRL_DcdcFlag =
        ANACTRL_BOD_DCDC_INT_STATUS_DCDC_STATUS_MASK, /*!< DCDC Interrupt status before Interrupt Enable. */
    kANACTRL_DcdcInterruptFlag =
        ANACTRL_BOD_DCDC_INT_STATUS_DCDC_INT_STATUS_MASK, /*!< DCDC Interrupt status after Interrupt Enable. */
    kANACTRL_DcdcPowerFlag =
        ANACTRL_BOD_DCDC_INT_STATUS_DCDC_VAL_MASK, /*!< Current value of DCDC power status output. */
};

/*!
 * @brief ANACTRL interrupt control
 */
enum _anactrl_interrupt
{
    kANACTRL_BodVbatInterruptEnable =
        ANACTRL_BOD_DCDC_INT_CTRL_BODVBAT_INT_ENABLE_MASK, /*!< BOD VBAT interrupt control. */
    kANACTRL_BodCoreInterruptEnable =
        ANACTRL_BOD_DCDC_INT_CTRL_BODCORE_INT_ENABLE_MASK,                         /*!< BOD CORE interrupt control. */
    kANACTRL_DcdcInterruptEnable = ANACTRL_BOD_DCDC_INT_CTRL_DCDC_INT_ENABLE_MASK, /*!< DCDC interrupt control. */
    kANACTRL_BodVbatInterruptClear =
        ANACTRL_BOD_DCDC_INT_CTRL_BODVBAT_INT_CLEAR_MASK, /*!< BOD VBAT interrupt clear.1: Clear the interrupt.
                                                             Self-cleared bit. */
    kANACTRL_BodCoreInterruptClear =
        ANACTRL_BOD_DCDC_INT_CTRL_BODCORE_INT_CLEAR_MASK, /*!< BOD CORE interrupt clear.1: Clear the interrupt.
                                                             Self-cleared bit. */
    kANACTRL_DcdcInterruptClear = ANACTRL_BOD_DCDC_INT_CTRL_DCDC_INT_CLEAR_MASK, /*!< DCDC interrupt clear.1: Clear the
                                                                                    interrupt. Self-cleared bit. */
};

/*!
 * @brief ANACTRL status flags
 */
enum _anactrl_flags
{
    kANACTRL_PMUId = ANACTRL_ANALOG_CTRL_STATUS_PMU_ID_MASK, /*!< Power Management Unit (PMU) analog macro-bloc
                                                                identification number. */
    kANACTRL_OSCId =
        ANACTRL_ANALOG_CTRL_STATUS_OSC_ID_MASK, /*!< Oscillators analog macro-bloc identification number. */
    kANACTRL_FlashPowerDownFlag = ANACTRL_ANALOG_CTRL_STATUS_FLASH_PWRDWN_MASK, /*!< Flash power-down status. */
    kANACTRL_FlashInitErrorFlag =
        ANACTRL_ANALOG_CTRL_STATUS_FLASH_INIT_ERROR_MASK, /*!< Flash initialization error status. */
    kANACTRL_FinalTestFlag =
        ANACTRL_ANALOG_CTRL_STATUS_FINAL_TEST_DONE_VECT_MASK, /*!< Indicates current status of final test. */
};

/*!
 * @brief ANACTRL FRO192M and XO32M status flags
 */
enum _anactrl_osc_flags
{
    kANACTRL_OutputClkValidFlag = ANACTRL_FRO192M_STATUS_CLK_VALID_MASK, /*!< Output clock valid signal. */
    kANACTRL_CCOThresholdVoltageFlag =
        ANACTRL_FRO192M_STATUS_ATB_VCTRL_MASK, /*!< CCO threshold voltage detector output (signal vcco_ok). */
    kANACTRL_XO32MOutputReadyFlag = ANACTRL_XO32M_STATUS_XO_READY_MASK
                                    << 16U, /*!< Indicates XO out frequency statibilty. */
};

/*!
 * @brief LDO output mode
 */
typedef enum _anactrl_ldo_output_mode
{
    kANACTRL_LDOOutputHighNormalMode = 0U,    /*!< Output in High normal state. */
    kANACTRL_LDOOutputHighImpedanceMode = 1U, /*!< Output in High Impedance state. */
} anactrl_ldo_output_mode_t;

/*!
 * @brief LDO output level
 */
typedef enum _anactrl_ldo_output_level
{
    kANACTRL_LDOOutputLevel0 = 0U, /*!< Output level 0.750 V. */
    kANACTRL_LDOOutputLevel1,      /*!< Output level 0.775 V. */
    kANACTRL_LDOOutputLevel2,      /*!< Output level 0.800 V. */
    kANACTRL_LDOOutputLevel3,      /*!< Output level 0.825 V. */
    kANACTRL_LDOOutputLevel4,      /*!< Output level 0.850 V. */
    kANACTRL_LDOOutputLevel5,      /*!< Output level 0.875 V. */
    kANACTRL_LDOOutputLevel6,      /*!< Output level 0.900 V. */
    kANACTRL_LDOOutputLevel7,      /*!< Output level 0.925 V. */
} anactrl_ldo_output_level_t;

/*!
 * @brief Select short or long ring osc
 */
typedef enum _anactrl_ring_osc_selector
{
    kANACTRL_ShortRingOsc = 0U, /*!< Select short ring osc (few elements). */
    kANACTRL_LongRingOsc = 1U,  /*!< Select long ring osc (many elements). */
} anactrl_ring_osc_selector_t;

/*!
 * @brief Ring osc frequency output divider
 */
typedef enum _anactrl_ring_osc_freq_output_divider
{
    kANACTRL_HighFreqOutput = 0U, /*!< High frequency output (frequency lower than 100 MHz). */
    kANACTRL_LowFreqOutput = 1U,  /*!< Low frequency output (frequency lower than 10 MHz). */
} anactrl_ring_osc_freq_output_divider_t;

/*!
 * @brief PN-Ring osc (P-Transistor and N-Transistor processing) control.
 */
typedef enum _anactrl_pn_ring_osc_mode
{
    kANACTRL_NormalMode = 0U,              /*!< Normal mode. */
    kANACTRL_PMonitorPTransistorMode = 1U, /*!< P-Monitor mode. Measure with weak P transistor. */
    kANACTRL_PMonitorNTransistorMode = 2U, /*!< P-Monitor mode. Measure with weak N transistor. */
    kANACTRL_NotUse = 3U,                  /*!< Do not use. */
} anactrl_pn_ring_osc_mode_t;

/*!
 * @breif Configuration for FRO192M
 *
 * This structure holds the configuration settings for the on-chip high-speed Free Running Oscillator. To initialize
 * this structure to reasonable defaults, call the ANACTRL_GetDefaultFro192MConfig() function and pass a
 * pointer to your config structure instance.
 */
typedef struct _anactrl_fro192M_config
{
    uint8_t biasTrim;         /*!< Set bias trimming value (course frequency trimming). */
    uint8_t tempTrim;         /*!< Set temperature coefficient trimming value. */
    uint8_t dacTrim;          /*!< Set curdac trimming value (fine frequency trimming) This trim is used to
        adjust the frequency, given that the bias and temperature trim are set. */
    bool enable12MHzClk;      /*!< Enable 12MHz clock. */
    bool enable48MhzClk;      /*!< Enable 48MHz clock. */
    bool enable96MHzClk;      /*!< Enable 96MHz clock. */
    bool enableAnalogTestBus; /*!< Enable analog test bus. */
} anactrl_fro192M_config_t;

/*!
 * @breif Configuration for XO32M
 *
 * This structure holds the configuration settings for the 32 MHz crystal oscillator. To initialize this
 * structure to reasonable defaults, call the ANACTRL_GetDefaultXo32MConfig() function and pass a
 * pointer to your config structure instance.
 */
typedef struct _anactrl_xo32M_config
{
    bool enableACBufferBypass;                 /*!< Enable XO AC buffer bypass in pll and top level. */
    bool enablePllUsbOutput;                   /*!< Enable XO 32 MHz output to USB HS PLL. */
    bool enableSysCLkOutput;                   /*!< Enable XO 32 MHz output to CPU system, SCT, and CLKOUT */
    bool enableLDOBypass;                      /*!< Activate LDO bypass. */
    anactrl_ldo_output_mode_t LDOOutputMode;   /*!< Set LDO output mode. */
    anactrl_ldo_output_level_t LDOOutputLevel; /*!< Set LDO output level. */
    uint8_t bias;                              /*!< Adjust the biasing current. */
    uint8_t stability;                         /*!< Stability configuration. */
} anactrl_xo32M_config_t;

/*!
 * @breif Configuration for ring oscillator
 *
 * This structure holds the configuration settings for the three ring oscillators. To initialize this
 * structure to reasonable defaults, call the ANACTRL_GetDefaultRingOscConfig() function and pass a
 * pointer to your config structure instance.
 */
typedef struct _anactrl_ring_osc_config
{
    anactrl_ring_osc_selector_t ringOscSel;
    anactrl_ring_osc_freq_output_divider_t ringOscFreqOutputDiv;
    anactrl_pn_ring_osc_mode_t pnRingOscMode;
    uint8_t ringOscOutClkDiv;
} anactrl_ring_osc_config_t;
/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Enable the access to ANACTRL registers and initialize ANACTRL module.
 *
 * @param base ANACTRL peripheral base address.
 */
void ANACTRL_Init(ANACTRL_Type *base);

/*!
 * @brief De-initialize ANACTRL module.
 *
 * @param base ANACTRL peripheral base address.
 */
void ANACTRL_Deinit(ANACTRL_Type *base);
/* @} */

/*!
 * @name Set oscillators
 * @{
 */

/*!
 * @brief Set the on-chip high-speed Free Running Oscillator.
 *
 * @param base ANACTRL peripheral base address.
 * @param config Pointer to FRO192M configuration structure. Refer to "anactrl_fro192M_config_t" structure.
 */
void ANACTRL_SetFro192M(ANACTRL_Type *base, anactrl_fro192M_config_t *config);

/*!
 * @brief Get the default configuration of FRO192M.
 * The default values are:
 * code
 *   config->biasTrim = 0x1AU;
 *   config->tempTrim = 0x20U;
 *   config->enable12MHzClk = true;
 *   config->enable48MhzClk = true;
 *   config->dacTrim = 0x80U;
 *   config->enableAnalogTestBus = false;
 *   config->enable96MHzClk = false;
 * encode
 * @param config Pointer to FRO192M configuration structure. Refer to "anactrl_fro192M_config_t" structure.
 */
void ANACTRL_GetDefaultFro192MConfig(anactrl_fro192M_config_t *config);

/*!
 * @brief Set the 32 MHz Crystal oscillator.
 *
 * @param base ANACTRL peripheral base address.
 * @param config Pointer to XO32M configuration structure. Refer to "anactrl_xo32M_config_t" structure.
 */
void ANACTRL_SetXo32M(ANACTRL_Type *base, anactrl_xo32M_config_t *config);

/*!
 * @brief Get the default configuration of XO32M.
 * The default values are:
 * code
 *   config->enableACBufferBypass = false;
 *   config->enablePllUsbOutput = false;
 *   config->enableSysCLkOutput = false;
 *   config->enableLDOBypass = false;
 *   config->LDOOutputMode = kANACTRL_LDOOutputHighNormalMode;
 *   config->LDOOutputLevel = kANACTRL_LDOOutputLevel4;
 *   config->bias = 2U;
 *   config->stability = 3U;
 * encode
 * @param config Pointer to XO32M configuration structure. Refer to "anactrl_xo32M_config_t" structure.
 */
void ANACTRL_GetDefaultXo32MConfig(anactrl_xo32M_config_t *config);

/*!
 * @brief Set the ring oscillators.
 *
 * @param base ANACTRL peripheral base address.
 * @param config Pointer to ring osc configuration structure. Refer to "anactrl_ring_osc_config_t" structure.
 */
void ANACTRL_SetRingOsc(ANACTRL_Type *base, anactrl_ring_osc_config_t *config);

/*!
 * @brief Get the default configuration of ring oscillators.
 * The default values are:
 * code
 *   config->ringOscSel = kANACTRL_ShortRingOsc;
 *   config->ringOscFreqOutputDiv = kANACTRL_HighFreqOutput;
 *   config->pnRingOscMode = kANACTRL_NormalMode;
 *   config->ringOscOutClkDiv = 0U;
 * encode
 * @param config Pointer to ring oscillator configuration structure. Refer to "anactrl_ring_osc_config_t" structure.
 */
void ANACTRL_GetDefaultRingOscConfig(anactrl_ring_osc_config_t *config);
/* @} */

/*!
 * @name ADC control
 * @{
 */

/*!
 * @brief Enable VBAT divider branch.
 *
 * @param base ANACTRL peripheral base address.
 * @param enable switcher to the function.
 */
static inline void ANACTRL_EnableAdcVBATDivider(ANACTRL_Type *base, bool enable)
{
    if (enable)
    {
        base->ADC_CTRL |= ANACTRL_ADC_CTRL_VBATDIVENABLE_MASK;
    }
    else
    {
        base->ADC_CTRL &= ~ANACTRL_ADC_CTRL_VBATDIVENABLE_MASK;
    }
}
/* @} */

/*!
 * @name Measure Frequency
 * @{
 */

/*!
 * @brief Measure Frequency
 *
 * This function measures target frequency according to a accurate reference frequency.The formula is:
 * Ftarget = (CAPVAL * Freference) / ((1<<SCALE)-1)
 *
 * @param base ANACTRL peripheral base address.
 * @scale Define the power of 2 count that ref counter counts to during measurement.
 * @refClkFreq frequency of the reference clock.
 * @return frequency of the target clock.
 *
 * @Note the minimum count (scale) is 2.
 */
uint32_t ANACTRL_MeasureFrequency(ANACTRL_Type *base, uint8_t scale, uint32_t refClkFreq);
/* @} */

/*!
 * @name Interrupt Interface
 * @{
 */

/*!
 * @brief Enable the ANACTRL interrupts.
 *
 * @param bas ANACTRL peripheral base address.
 * @param mask The interrupt mask. Refer to "_anactrl_interrupt" enumeration.
 */
static inline void ANACTRL_EnableInterrupt(ANACTRL_Type *base, uint32_t mask)
{
    base->BOD_DCDC_INT_CTRL |= (0x15U & mask);
}

/*!
 * @brief Disable the ANACTRL interrupts.
 *
 * @param bas ANACTRL peripheral base address.
 * @param mask The interrupt mask. Refer to "_anactrl_interrupt" enumeration.
 */
static inline void ANACTRL_DisableInterrupt(ANACTRL_Type *base, uint32_t mask)
{
    base->BOD_DCDC_INT_CTRL = (base->BOD_DCDC_INT_CTRL & ~0x2AU) | (mask & 0x2AU);
}
/* @} */

/*!
 * @name Status Interface
 * @{
 */

/*!
 * @brief Get ANACTRL status flags.
 *
 * This function gets Analog control status flags. The flags are returned as the logical
 * OR value of the enumerators @ref _anactrl_flags. To check for a specific status,
 * compare the return value with enumerators in the @ref _anactrl_flags.
 * For example, to check whether the flash is in power down mode:
 * @code
 *     if (kANACTRL_FlashPowerDownFlag & ANACTRL_ANACTRL_GetStatusFlags(ANACTRL))
 *     {
 *         ...
 *     }
 * @endcode
 *
 * @param base ANACTRL peripheral base address.
 * @return ANACTRL status flags which are given in the enumerators in the @ref _anactrl_flags.
 */
static inline uint32_t ANACTRL_GetStatusFlags(ANACTRL_Type *base)
{
    return base->ANALOG_CTRL_STATUS;
}

/*!
 * @brief Get ANACTRL oscillators status flags.
 *
 * This function gets Anactrl oscillators status flags. The flags are returned as the logical
 * OR value of the enumerators @ref _anactrl_osc_flags. To check for a specific status,
 * compare the return value with enumerators in the @ref _anactrl_osc_flags.
 * For example, to check whether the FRO192M clock output is valid:
 * @code
 *     if (kANACTRL_OutputClkValidFlag & ANACTRL_ANACTRL_GetOscStatusFlags(ANACTRL))
 *     {
 *         ...
 *     }
 * @endcode
 *
 * @param base ANACTRL peripheral base address.
 * @return ANACTRL oscillators status flags which are given in the enumerators in the @ref _anactrl_osc_flags.
 */
static inline uint32_t ANACTRL_GetOscStatusFlags(ANACTRL_Type *base)
{
    return (base->FRO192M_STATUS & 0xFFU) | ((base->XO32M_STATUS & 0xFFU) << 16U);
}

/*!
 * @brief Get ANACTRL interrupt status flags.
 *
 * This function gets Anactrl interrupt status flags. The flags are returned as the logical
 * OR value of the enumerators @ref _anactrl_interrupt_flags. To check for a specific status,
 * compare the return value with enumerators in the @ref _anactrl_interrupt_flags.
 * For example, to check whether the VBAT voltage level is above the threshold:
 * @code
 *     if (kANACTRL_BodVbatPowerFlag & ANACTRL_ANACTRL_GetInterruptStatusFlags(ANACTRL))
 *     {
 *         ...
 *     }
 * @endcode
 *
 * @param base ANACTRL peripheral base address.
 * @return ANACTRL oscillators status flags which are given in the enumerators in the @ref _anactrl_osc_flags.
 */
static inline uint32_t ANACTRL_GetInterruptStatusFlags(ANACTRL_Type *base)
{
    return base->BOD_DCDC_INT_STATUS & 0x1FFU;
}
/* @} */

#if defined(__cplusplus)
}
#endif

/* @}*/

#endif /* __FSL_ANACTRL_H__ */
