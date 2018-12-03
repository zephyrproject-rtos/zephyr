/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_WDOG32_H_
#define _FSL_WDOG32_H_

#include "fsl_common.h"

/*!
 * @addtogroup wdog32
 * @{
 */


/*******************************************************************************
 * Definitions
 *******************************************************************************/
/*! @name Unlock sequence */
/*@{*/
#define WDOG_FIRST_WORD_OF_UNLOCK (WDOG_UPDATE_KEY & 0xFFFFU)  /*!< First word of unlock sequence */
#define WDOG_SECOND_WORD_OF_UNLOCK ((WDOG_UPDATE_KEY >> 16U)& 0xFFFFU) /*!< Second word of unlock sequence */
/*@}*/

/*! @name Refresh sequence */
/*@{*/
#define WDOG_FIRST_WORD_OF_REFRESH (WDOG_REFRESH_KEY & 0xFFFFU)  /*!< First word of refresh sequence */
#define WDOG_SECOND_WORD_OF_REFRESH ((WDOG_REFRESH_KEY >> 16U)& 0xFFFFU) /*!< Second word of refresh sequence */
/*@}*/
/*! @name Driver version */
/*@{*/
/*! @brief WDOG32 driver version 2.0.0. */
#define FSL_WDOG32_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*! @brief Describes WDOG32 clock source. */
typedef enum _wdog32_clock_source
{
    kWDOG32_ClockSource0 = 0U, /*!< Clock source 0 */
    kWDOG32_ClockSource1 = 1U, /*!< Clock source 1 */
    kWDOG32_ClockSource2 = 2U, /*!< Clock source 2 */
    kWDOG32_ClockSource3 = 3U, /*!< Clock source 3 */
} wdog32_clock_source_t;

/*! @brief Describes the selection of the clock prescaler. */
typedef enum _wdog32_clock_prescaler
{
    kWDOG32_ClockPrescalerDivide1 = 0x0U,   /*!< Divided by 1 */
    kWDOG32_ClockPrescalerDivide256 = 0x1U, /*!< Divided by 256 */
} wdog32_clock_prescaler_t;

/*! @brief Defines WDOG32 work mode. */
typedef struct _wdog32_work_mode
{
    bool enableWait;  /*!< Enables or disables WDOG32 in wait mode */
    bool enableStop;  /*!< Enables or disables WDOG32 in stop mode */
    bool enableDebug; /*!< Enables or disables WDOG32 in debug mode */
} wdog32_work_mode_t;

/*! @brief Describes WDOG32 test mode. */
typedef enum _wdog32_test_mode
{
    kWDOG32_TestModeDisabled = 0U, /*!< Test Mode disabled */
    kWDOG32_UserModeEnabled = 1U,  /*!< User Mode enabled */
    kWDOG32_LowByteTest = 2U,      /*!< Test Mode enabled, only low byte is used */
    kWDOG32_HighByteTest = 3U,     /*!< Test Mode enabled, only high byte is used */
} wdog32_test_mode_t;

/*! @brief Describes WDOG32 configuration structure. */
typedef struct _wdog32_config
{
    bool enableWdog32;                  /*!< Enables or disables WDOG32 */
    wdog32_clock_source_t clockSource;  /*!< Clock source select */
    wdog32_clock_prescaler_t prescaler; /*!< Clock prescaler value */
    wdog32_work_mode_t workMode;        /*!< Configures WDOG32 work mode in debug stop and wait mode */
    wdog32_test_mode_t testMode;        /*!< Configures WDOG32 test mode */
    bool enableUpdate;                  /*!< Update write-once register enable */
    bool enableInterrupt;               /*!< Enables or disables WDOG32 interrupt */
    bool enableWindowMode;              /*!< Enables or disables WDOG32 window mode */
    uint16_t windowValue;               /*!< Window value */
    uint16_t timeoutValue;              /*!< Timeout value */
} wdog32_config_t;

/*!
 * @brief WDOG32 interrupt configuration structure.
 *
 * This structure contains the settings for all of the WDOG32 interrupt configurations.
 */
enum _wdog32_interrupt_enable_t
{
    kWDOG32_InterruptEnable = WDOG_CS_INT_MASK, /*!< Interrupt is generated before forcing a reset */
};

/*!
 * @brief WDOG32 status flags.
 *
 * This structure contains the WDOG32 status flags for use in the WDOG32 functions.
 */
enum _wdog32_status_flags_t
{
    kWDOG32_RunningFlag = WDOG_CS_EN_MASK,    /*!< Running flag, set when WDOG32 is enabled */
    kWDOG32_InterruptFlag = WDOG_CS_FLG_MASK, /*!< Interrupt flag, set when interrupt occurs */
};

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name WDOG32 Initialization and De-initialization
 * @{
 */

/*!
 * @brief Initializes the WDOG32 configuration structure.
 *
 * This function initializes the WDOG32 configuration structure to default values. The default
 * values are:
 * @code
 *   wdog32Config->enableWdog32 = true;
 *   wdog32Config->clockSource = kWDOG32_ClockSource1;
 *   wdog32Config->prescaler = kWDOG32_ClockPrescalerDivide1;
 *   wdog32Config->workMode.enableWait = true;
 *   wdog32Config->workMode.enableStop = false;
 *   wdog32Config->workMode.enableDebug = false;
 *   wdog32Config->testMode = kWDOG32_TestModeDisabled;
 *   wdog32Config->enableUpdate = true;
 *   wdog32Config->enableInterrupt = false;
 *   wdog32Config->enableWindowMode = false;
 *   wdog32Config->windowValue = 0U;
 *   wdog32Config->timeoutValue = 0xFFFFU;
 * @endcode
 *
 * @param config Pointer to the WDOG32 configuration structure.
 * @see wdog32_config_t
 */
void WDOG32_GetDefaultConfig(wdog32_config_t *config);

/*!
 * @brief Initializes the WDOG32 module.
 *
 * This function initializes the WDOG32.
 * To reconfigure the WDOG32 without forcing a reset first, enableUpdate must be set to true
 * in the configuration.
 *
 * Example:
 * @code
 *   wdog32_config_t config;
 *   WDOG32_GetDefaultConfig(&config);
 *   config.timeoutValue = 0x7ffU;
 *   config.enableUpdate = true;
 *   WDOG32_Init(wdog_base,&config);
 * @endcode
 *
 * @param base   WDOG32 peripheral base address.
 * @param config The configuration of the WDOG32.
 */
void WDOG32_Init(WDOG_Type *base, const wdog32_config_t *config);

/*!
 * @brief De-initializes the WDOG32 module.
 *
 * This function shuts down the WDOG32.
 * Ensure that the WDOG_CS.UPDATE is 1, which means that the register update is enabled.
 *
 * @param base   WDOG32 peripheral base address.
 */
void WDOG32_Deinit(WDOG_Type *base);

/* @} */

/*!
 * @name WDOG32 functional Operation
 * @{
 */

/*!
 * @brief Enables the WDOG32 module.
 *
 * This function writes a value into the WDOG_CS register to enable the WDOG32.
 * The WDOG_CS register is a write-once register. Ensure that the WCT window is still open and
 * this register has not been written in this WCT while the function is called.
 *
 * @param base WDOG32 peripheral base address.
 */
static inline void WDOG32_Enable(WDOG_Type *base)
{
    base->CS |= WDOG_CS_EN_MASK;
}

/*!
 * @brief Disables the WDOG32 module.
 *
 * This function writes a value into the WDOG_CS register to disable the WDOG32.
 * The WDOG_CS register is a write-once register. Ensure that the WCT window is still open and
 * this register has not been written in this WCT while the function is called.
 *
 * @param base WDOG32 peripheral base address
 */
static inline void WDOG32_Disable(WDOG_Type *base)
{
    base->CS &= ~WDOG_CS_EN_MASK;
}

/*!
 * @brief Enables the WDOG32 interrupt.
 *
 * This function writes a value into the WDOG_CS register to enable the WDOG32 interrupt.
 * The WDOG_CS register is a write-once register. Ensure that the WCT window is still open and
 * this register has not been written in this WCT while the function is called.
 *
 * @param base WDOG32 peripheral base address.
 * @param mask The interrupts to enable.
 *        The parameter can be a combination of the following source if defined:
 *        @arg kWDOG32_InterruptEnable
 */
static inline void WDOG32_EnableInterrupts(WDOG_Type *base, uint32_t mask)
{
    base->CS |= mask;
}

/*!
 * @brief Disables the WDOG32 interrupt.
 *
 * This function writes a value into the WDOG_CS register to disable the WDOG32 interrupt.
 * The WDOG_CS register is a write-once register. Ensure that the WCT window is still open and
 * this register has not been written in this WCT while the function is called.
 *
 * @param base WDOG32 peripheral base address.
 * @param mask The interrupts to disabled.
 *        The parameter can be a combination of the following source if defined:
 *        @arg kWDOG32_InterruptEnable
 */
static inline void WDOG32_DisableInterrupts(WDOG_Type *base, uint32_t mask)
{
    base->CS &= ~mask;
}

/*!
 * @brief Gets the WDOG32 all status flags.
 *
 * This function gets all status flags.
 *
 * Example to get the running flag:
 * @code
 *   uint32_t status;
 *   status = WDOG32_GetStatusFlags(wdog_base) & kWDOG32_RunningFlag;
 * @endcode
 * @param base        WDOG32 peripheral base address
 * @return            State of the status flag: asserted (true) or not-asserted (false). @see _wdog32_status_flags_t
 *                    - true: related status flag has been set.
 *                    - false: related status flag is not set.
 */
static inline uint32_t WDOG32_GetStatusFlags(WDOG_Type *base)
{
    return (base->CS & (WDOG_CS_EN_MASK | WDOG_CS_FLG_MASK));
}

/*!
 * @brief Clears the WDOG32 flag.
 *
 * This function clears the WDOG32 status flag.
 *
 * Example to clear an interrupt flag:
 * @code
 *   WDOG32_ClearStatusFlags(wdog_base,kWDOG32_InterruptFlag);
 * @endcode
 * @param base        WDOG32 peripheral base address.
 * @param mask        The status flags to clear.
 *                    The parameter can be any combination of the following values:
 *                    @arg kWDOG32_InterruptFlag
 */
void WDOG32_ClearStatusFlags(WDOG_Type *base, uint32_t mask);

/*!
 * @brief Sets the WDOG32 timeout value.
 *
 * This function writes a timeout value into the WDOG_TOVAL register.
 * The WDOG_TOVAL register is a write-once register. Ensure that the WCT window is still open and
 * this register has not been written in this WCT while the function is called.
 *
 * @param base WDOG32 peripheral base address
 * @param timeoutCount WDOG32 timeout value, count of WDOG32 clock ticks.
 */
static inline void WDOG32_SetTimeoutValue(WDOG_Type *base, uint16_t timeoutCount)
{
    base->TOVAL = timeoutCount;
}

/*!
 * @brief Sets the WDOG32 window value.
 *
 * This function writes a window value into the WDOG_WIN register.
 * The WDOG_WIN register is a write-once register. Ensure that the WCT window is still open and
 * this register has not been written in this WCT while the function is called.
 *
 * @param base WDOG32 peripheral base address.
 * @param windowValue WDOG32 window value.
 */
static inline void WDOG32_SetWindowValue(WDOG_Type *base, uint16_t windowValue)
{
    base->WIN = windowValue;
}

/*!
 * @brief Unlocks the WDOG32 register written.
 *
 * This function unlocks the WDOG32 register written.
 *
 * Before starting the unlock sequence and following the configuration, disable the global interrupts.
 * Otherwise, an interrupt could effectively invalidate the unlock sequence and the WCT may expire.
 * After the configuration finishes, re-enable the global interrupts.
 *
 * @param base WDOG32 peripheral base address
 */
static inline void WDOG32_Unlock(WDOG_Type *base)
{
    if ((base->CS) & WDOG_CS_CMD32EN_MASK)
    {
        base->CNT = WDOG_UPDATE_KEY;
    }
    else
    {
        base->CNT = WDOG_FIRST_WORD_OF_UNLOCK;
        base->CNT = WDOG_SECOND_WORD_OF_UNLOCK;
    }
}

/*!
 * @brief Refreshes the WDOG32 timer.
 *
 * This function feeds the WDOG32.
 * This function should be called before the Watchdog timer is in timeout. Otherwise, a reset is asserted.
 *
 * @param base WDOG32 peripheral base address
 */
static inline void WDOG32_Refresh(WDOG_Type *base)
{
    if ((base->CS) & WDOG_CS_CMD32EN_MASK)
    {
        base->CNT = WDOG_REFRESH_KEY;
    }
    else
    {
        base->CNT = WDOG_FIRST_WORD_OF_REFRESH;
        base->CNT = WDOG_SECOND_WORD_OF_REFRESH;
    }
}

/*!
 * @brief Gets the WDOG32 counter value.
 *
 * This function gets the WDOG32 counter value.
 *
 * @param base WDOG32 peripheral base address.
 * @return     Current WDOG32 counter value.
 */
static inline uint16_t WDOG32_GetCounterValue(WDOG_Type *base)
{
    return base->CNT;
}

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_WDOG32_H_ */
