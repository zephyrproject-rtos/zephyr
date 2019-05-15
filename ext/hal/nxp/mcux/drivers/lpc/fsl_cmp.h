/*
 * Copyright 2018 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef __FSL_CMP_H_
#define __FSL_CMP_H_

#include "fsl_common.h"

/*!
 * @addtogroup cmp_1
 * @{
 */

/******************************************************************************
 * Definitions.
 *****************************************************************************/
/*! @name Driver version */
/*@{*/
/*! @brief Driver version 2.0.0. */
#define FSL_CMP_DRIVER_VERSION (MAKE_VERSION(2U, 0U, 0U))
/*@}*/

/*! @brief VREF select */
enum _cmp_vref_select
{
    KCMP_VREFSelectVDDA = 1U,         /*!< Select VDDA as VREF*/
    KCMP_VREFSelectInternalVREF = 0U, /*!< select internal VREF as VREF*/
};

/*! @brief cmp interrupt type */
typedef enum _cmp_interrupt_type
{
    kCMP_EdgeDisable = 0U << SYSCON_COMP_INT_CTRL_INT_CTRL_SHIFT,       /*!< disable edge sensitive */
    kCMP_EdgeRising = 2U << SYSCON_COMP_INT_CTRL_INT_CTRL_SHIFT,        /*!< Edge sensitive, falling edge */
    kCMP_EdgeFalling = 4U << SYSCON_COMP_INT_CTRL_INT_CTRL_SHIFT,       /*!< Edge sensitive, rising edge */
    kCMP_EdgeRisingFalling = 6U << SYSCON_COMP_INT_CTRL_INT_CTRL_SHIFT, /*!< Edge sensitive, rising and falling edge */

    kCMP_LevelDisable = 1U << SYSCON_COMP_INT_CTRL_INT_CTRL_SHIFT,  /*!< disable level sensitive */
    kCMP_LevelHigh = 3U << SYSCON_COMP_INT_CTRL_INT_CTRL_SHIFT,     /*!< Level sensitive, high level */
    kCMP_LevelLow = 5U << SYSCON_COMP_INT_CTRL_INT_CTRL_SHIFT,      /*!< Level sensitive, low level */
    kCMP_LevelDisable1 = 7U << SYSCON_COMP_INT_CTRL_INT_CTRL_SHIFT, /*!< disable level sensitive */
} cmp_interrupt_type_t;

/*! @brief cmp Pmux input source */
typedef enum _cmp_pmux_input
{
    kCMP_PInputVREF = 0U << PMC_COMP_PMUX_SHIFT,  /*!< Cmp Pmux input from VREF */
    kCMP_PInputP0_0 = 1U << PMC_COMP_PMUX_SHIFT,  /*!< Cmp Pmux input from P0_0 */
    kCMP_PInputP0_9 = 2U << PMC_COMP_PMUX_SHIFT,  /*!< Cmp Pmux input from P0_9 */
    kCMP_PInputP0_18 = 3U << PMC_COMP_PMUX_SHIFT, /*!< Cmp Pmux input from P0_18 */
    kCMP_PInputP1_14 = 4U << PMC_COMP_PMUX_SHIFT, /*!< Cmp Pmux input from P1_14 */
    kCMP_PInputP2_23 = 5U << PMC_COMP_PMUX_SHIFT, /*!< Cmp Pmux input from P2_23 */
} cmp_pmux_input_t;

/*! @brief cmp Nmux input source */
typedef enum _cmp_nmux_input
{
    kCMP_NInputVREF = 0U << PMC_COMP_NMUX_SHIFT,  /*!< Cmp Nmux input from VREF */
    kCMP_NInputP0_0 = 1U << PMC_COMP_NMUX_SHIFT,  /*!< Cmp Nmux input from P0_0 */
    kCMP_NInputP0_9 = 2U << PMC_COMP_NMUX_SHIFT,  /*!< Cmp Nmux input from P0_9 */
    kCMP_NInputP0_18 = 3U << PMC_COMP_NMUX_SHIFT, /*!< Cmp Nmux input from P0_18 */
    kCMP_NInputP1_14 = 4U << PMC_COMP_NMUX_SHIFT, /*!< Cmp Nmux input from P1_14 */
    kCMP_NInputP2_23 = 5U << PMC_COMP_NMUX_SHIFT, /*!< Cmp Nmux input from P2_23 */
} cmp_nmux_input_t;

/*! @brief cmp configurataions */
typedef struct _cmp_config
{
    bool enHysteris;            /*!< low hysteresis */
    bool enLowPower;            /*!<low power mode*/
    cmp_nmux_input_t nmuxInput; /*!<Nmux input select*/
    cmp_pmux_input_t pmuxInput; /*!<Pmux input select*/
} cmp_config_t;
/*************************************************************************************************
 * API
 ************************************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Cmp Initialization and deinitialization
 * @{
 */

/*!
 * @brief CMP intialization.
 * Note: The cmp initial function not responsible for cmp power, application shall handle it.
 *
 * @param config init configurations.
 */
void CMP_Init(cmp_config_t *config);

/*!
 * @brief CMP deintialization.
 * Note: The cmp deinit function not responsible for cmp power, application shall handle it.
 *
 */
void CMP_Deinit(void);

/* @} */

/*!
 * @name cmp functionality
 * @{
 */

/*!
 * @brief select input source for pmux.
 *
 * @param pmux_select_source reference cmp_pmux_input_t above.
 */
static inline void CMP_PmuxSelect(cmp_pmux_input_t pmux_select_source)
{
    PMC->COMP &= ~PMC_COMP_PMUX_MASK;
    PMC->COMP |= pmux_select_source;
}

/*!
 * @brief select input source for nmux.
 *
 * @param nmux_select_source reference cmp_nmux_input_t above.
 */
static inline void CMP_NmuxSelect(cmp_nmux_input_t nmux_select_source)
{
    PMC->COMP &= ~PMC_COMP_NMUX_MASK;
    PMC->COMP |= nmux_select_source;
}

/*!
 * @brief switch cmp work mode.
 *
 * @param enable true is enter low power mode, false is enter fast mode
 */
static inline void CMP_EnableLowePowerMode(bool enable)
{
    if (enable)
    {
        PMC->COMP |= PMC_COMP_LOWPOWER_MASK;
    }
    else
    {
        PMC->COMP &= ~PMC_COMP_LOWPOWER_MASK;
    }
}

/*!
 * @brief Control reference voltage step, per steps of (VREFINPUT/31).
 *
 * @param step reference voltage step, per steps of (VREFINPUT/31).
 */
static inline void CMP_SetRefStep(uint32_t step)
{
    PMC->COMP |= step << PMC_COMP_VREF_SHIFT;
}

/*!
 * @brief cmp enable hysteresis.
 *
 */
static inline void CMP_EnableHysteresis(bool enable)
{
    if (enable)
    {
        PMC->COMP |= PMC_COMP_HYST_MASK;
    }
    else
    {
        PMC->COMP &= ~PMC_COMP_HYST_MASK;
    }
}

/*!
 * @brief VREF select between internal VREF and VDDA (for the resistive ladder).
 *
 * @param select 1 is Select VDDA, 0 is Select internal VREF.
 */
static inline void CMP_VREFSelect(uint32_t select)
{
    if (select)
    {
        PMC->COMP |= PMC_COMP_VREFINPUT_MASK;
    }
    else
    {
        PMC->COMP &= ~PMC_COMP_VREFINPUT_MASK;
    }
}

/*!
 * @brief comparator analog output.
 *
 * @return 1 indicates p is greater than n, 0 indicates n is greater than p.
 */
static inline uint32_t CMP_GetOutput(void)
{
    return (SYSCON->COMP_INT_STATUS & SYSCON_COMP_INT_STATUS_VAL_MASK) ? 1 : 0;
}

/* @} */

/*!
 * @name cmp interrupt
 * @{
 */

/*!
 * @brief cmp enable interrupt.
 *
 */
static inline void CMP_EnableInterrupt(void)
{
    SYSCON->COMP_INT_CTRL |= SYSCON_COMP_INT_CTRL_INT_ENABLE_MASK;
}

/*!
 * @brief cmp disable interrupt.
 *
 */
static inline void CMP_DisableInterrupt(void)
{
    SYSCON->COMP_INT_CTRL &= ~SYSCON_COMP_INT_CTRL_INT_ENABLE_MASK;
}

/*!
 * @brief Select which Analog comparator output (filtered or un-filtered) is used for interrupt detection.
 *
 * @param enable true is Select Analog Comparator raw output (unfiltered) as input for interrupt detection.
 *               false is Select Analog Comparator filtered output as input for interrupt detection.
 */
static inline void CMP_InterruptSourceSelect(bool enable)
{
    if (enable)
    {
        SYSCON->COMP_INT_CTRL |= SYSCON_COMP_INT_CTRL_INT_SOURCE_MASK;
    }
    else
    {
        SYSCON->COMP_INT_CTRL &= ~SYSCON_COMP_INT_CTRL_INT_SOURCE_MASK;
    }
}

/*!
 * @brief cmp get status.
 *
 * @return true is interrupt pending, false is no interrupt pending.
 */
static inline bool CMP_GetStatus(void)
{
    return (SYSCON->COMP_INT_STATUS & SYSCON_COMP_INT_STATUS_STATUS_MASK) ? true : false;
}

/*!
 * @brief cmp clear interrupt status.
 *
 */
static inline void CMP_ClearStatus(void)
{
    SYSCON->COMP_INT_CTRL |= SYSCON_COMP_INT_CTRL_INT_CLEAR_MASK;
}

/*!
 * @brief Comparator interrupt type select.
 *
 * @param type reference cmp_interrupt_type_t.
 */
static inline void CMP_InterruptTypeSelect(cmp_interrupt_type_t cmp_interrupt_type)
{
    SYSCON->COMP_INT_CTRL &= ~SYSCON_COMP_INT_CTRL_INT_CTRL_MASK;
    SYSCON->COMP_INT_CTRL |= cmp_interrupt_type;
}

/*!
 * @brief cmp get interrupt status.
 *
 * @return true is interrupt pending, false is no interrupt pending.
 */
static inline bool CMP_GetInterruptStatus(void)
{
    return (SYSCON->COMP_INT_STATUS & SYSCON_COMP_INT_STATUS_INT_STATUS_MASK) ? true : false;
}
/* @} */

#if defined(__cplusplus)
}
#endif

/*! @} */
#endif /* __FSL_CMP_H_ */
