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
#ifndef _FSL_WDOG_H_
#define _FSL_WDOG_H_

#include "fsl_common.h"

/*!
 * @addtogroup wdog_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief Defines WDOG driver version 2.0.0. */
#define FSL_WDOG_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*! @name Unlock sequence */
/*@{*/
#define WDOG_FIRST_WORD_OF_UNLOCK (0xC520U)  /*!< First word of unlock sequence */
#define WDOG_SECOND_WORD_OF_UNLOCK (0xD928U) /*!< Second word of unlock sequence */
/*@}*/

/*! @name Refresh sequence */
/*@{*/
#define WDOG_FIRST_WORD_OF_REFRESH (0xA602U)  /*!< First word of refresh sequence */
#define WDOG_SECOND_WORD_OF_REFRESH (0xB480U) /*!< Second word of refresh sequence */
/*@}*/

/*! @brief Describes WDOG clock source. */
typedef enum _wdog_clock_source
{
    kWDOG_LpoClockSource = 0U,       /*!< WDOG clock sourced from LPO*/
    kWDOG_AlternateClockSource = 1U, /*!< WDOG clock sourced from alternate clock source*/
} wdog_clock_source_t;

/*! @brief Defines WDOG work mode. */
typedef struct _wdog_work_mode
{
#if defined(FSL_FEATURE_WDOG_HAS_WAITEN) && FSL_FEATURE_WDOG_HAS_WAITEN
    bool enableWait;  /*!< Enables or disables WDOG in wait mode  */
#endif                /* FSL_FEATURE_WDOG_HAS_WAITEN */
    bool enableStop;  /*!< Enables or disables WDOG in stop mode  */
    bool enableDebug; /*!< Enables or disables WDOG in debug mode */
} wdog_work_mode_t;

/*! @brief Describes the selection of the clock prescaler. */
typedef enum _wdog_clock_prescaler
{
    kWDOG_ClockPrescalerDivide1 = 0x0U, /*!< Divided by 1 */
    kWDOG_ClockPrescalerDivide2 = 0x1U, /*!< Divided by 2 */
    kWDOG_ClockPrescalerDivide3 = 0x2U, /*!< Divided by 3 */
    kWDOG_ClockPrescalerDivide4 = 0x3U, /*!< Divided by 4 */
    kWDOG_ClockPrescalerDivide5 = 0x4U, /*!< Divided by 5 */
    kWDOG_ClockPrescalerDivide6 = 0x5U, /*!< Divided by 6 */
    kWDOG_ClockPrescalerDivide7 = 0x6U, /*!< Divided by 7 */
    kWDOG_ClockPrescalerDivide8 = 0x7U, /*!< Divided by 8 */
} wdog_clock_prescaler_t;

/*! @brief Describes WDOG configuration structure. */
typedef struct _wdog_config
{
    bool enableWdog;                  /*!< Enables or disables WDOG */
    wdog_clock_source_t clockSource;  /*!< Clock source select */
    wdog_clock_prescaler_t prescaler; /*!< Clock prescaler value */
    wdog_work_mode_t workMode;        /*!< Configures WDOG work mode in debug stop and wait mode */
    bool enableUpdate;                /*!< Update write-once register enable */
    bool enableInterrupt;             /*!< Enables or disables WDOG interrupt */
    bool enableWindowMode;            /*!< Enables or disables WDOG window mode */
    uint32_t windowValue;             /*!< Window value */
    uint32_t timeoutValue;            /*!< Timeout value */
} wdog_config_t;

/*! @brief Describes WDOG test mode. */
typedef enum _wdog_test_mode
{
    kWDOG_QuickTest = 0U, /*!< Selects quick test */
    kWDOG_ByteTest = 1U,  /*!< Selects byte test */
} wdog_test_mode_t;

/*! @brief Describes WDOG tested byte selection in byte test mode. */
typedef enum _wdog_tested_byte
{
    kWDOG_TestByte0 = 0U, /*!< Byte 0 selected in byte test mode */
    kWDOG_TestByte1 = 1U, /*!< Byte 1 selected in byte test mode */
    kWDOG_TestByte2 = 2U, /*!< Byte 2 selected in byte test mode */
    kWDOG_TestByte3 = 3U, /*!< Byte 3 selected in byte test mode */
} wdog_tested_byte_t;

/*! @brief Describes WDOG test mode configuration structure. */
typedef struct _wdog_test_config
{
    wdog_test_mode_t testMode;     /*!< Selects test mode */
    wdog_tested_byte_t testedByte; /*!< Selects tested byte in byte test mode */
    uint32_t timeoutValue;         /*!< Timeout value */
} wdog_test_config_t;

/*!
 * @brief WDOG interrupt configuration structure, default settings all disabled.
 *
 * This structure contains the settings for all of the WDOG interrupt configurations.
 */
enum _wdog_interrupt_enable_t
{
    kWDOG_InterruptEnable = WDOG_STCTRLH_IRQRSTEN_MASK, /*!< WDOG timeout will generate interrupt before reset*/
};

/*!
 * @brief WDOG status flags.
 *
 * This structure contains the WDOG status flags for use in the WDOG functions.
 */
enum _wdog_status_flags_t
{
    kWDOG_RunningFlag = WDOG_STCTRLH_WDOGEN_MASK, /*!< Running flag, set when WDOG is enabled*/
    kWDOG_TimeoutFlag = WDOG_STCTRLL_INTFLG_MASK, /*!< Interrupt flag, set when an exception occurs*/
};

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name WDOG Initialization and De-initialization
 * @{
 */

/*!
 * @brief Initializes WDOG configure sturcture.
 *
 * This function initializes the WDOG configure structure to default value. The default
 * value are:
 * @code
 *   wdogConfig->enableWdog = true;
 *   wdogConfig->clockSource = kWDOG_LpoClockSource;
 *   wdogConfig->prescaler = kWDOG_ClockPrescalerDivide1;
 *   wdogConfig->workMode.enableWait = true;
 *   wdogConfig->workMode.enableStop = false;
 *   wdogConfig->workMode.enableDebug = false;
 *   wdogConfig->enableUpdate = true;
 *   wdogConfig->enableInterrupt = false;
 *   wdogConfig->enableWindowMode = false;
 *   wdogConfig->windowValue = 0;
 *   wdogConfig->timeoutValue = 0xFFFFU;
 * @endcode
 *
 * @param config Pointer to WDOG config structure.
 * @see wdog_config_t
 */
void WDOG_GetDefaultConfig(wdog_config_t *config);

/*!
 * @brief Initializes the WDOG.
 *
 * This function initializes the WDOG. When called, the WDOG runs according to the configuration.
 * If user wants to reconfigure WDOG without forcing a reset first, enableUpdate must be set to true
 * in configuration.
 *
 * Example:
 * @code
 *   wdog_config_t config;
 *   WDOG_GetDefaultConfig(&config);
 *   config.timeoutValue = 0x7ffU;
 *   config.enableUpdate = true;
 *   WDOG_Init(wdog_base,&config);
 * @endcode
 *
 * @param base   WDOG peripheral base address
 * @param config The configuration of WDOG
 */
void WDOG_Init(WDOG_Type *base, const wdog_config_t *config);

/*!
 * @brief Shuts down the WDOG.
 *
 * This function shuts down the WDOG.
 * Make sure that the WDOG_STCTRLH.ALLOWUPDATE is 1 which means that the register update is enabled.
 */
void WDOG_Deinit(WDOG_Type *base);

/*!
 * @brief Configures WDOG functional test.
 *
 * This function is used to configure the WDOG functional test. When called, the WDOG goes into test mode
 * and runs according to the configuration.
 * Make sure that the WDOG_STCTRLH.ALLOWUPDATE is 1 which means that the register update is enabled.
 *
 * Example:
 * @code
 *   wdog_test_config_t test_config;
 *   test_config.testMode = kWDOG_QuickTest;
 *   test_config.timeoutValue = 0xfffffu;
 *   WDOG_SetTestModeConfig(wdog_base, &test_config);
 * @endcode
 * @param base   WDOG peripheral base address
 * @param config The functional test configuration of WDOG
 */
void WDOG_SetTestModeConfig(WDOG_Type *base, wdog_test_config_t *config);

/* @} */

/*!
 * @name WDOG Functional Operation
 * @{
 */

/*!
 * @brief Enables the WDOG module.
 *
 * This function write value into WDOG_STCTRLH register to enable the WDOG, it is a write-once register,
 * make sure that the WCT window is still open and this register has not been written in this WCT
 * while this function is called.
 *
 * @param base WDOG peripheral base address
 */
static inline void WDOG_Enable(WDOG_Type *base)
{
    base->STCTRLH |= WDOG_STCTRLH_WDOGEN_MASK;
}

/*!
 * @brief Disables the WDOG module.
 *
 * This function write value into WDOG_STCTRLH register to disable the WDOG, it is a write-once register,
 * make sure that the WCT window is still open and this register has not been written in this WCT
 * while this function is called.
 *
 * @param base WDOG peripheral base address
 */
static inline void WDOG_Disable(WDOG_Type *base)
{
    base->STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK;
}

/*!
 * @brief Enable WDOG interrupt.
 *
 * This function write value into WDOG_STCTRLH register to enable WDOG interrupt, it is a write-once register,
 * make sure that the WCT window is still open and this register has not been written in this WCT
 * while this function is called.
 *
 * @param base WDOG peripheral base address
 * @param mask The interrupts to enable
 *        The parameter can be combination of the following source if defined:
 *        @arg kWDOG_InterruptEnable
 */
static inline void WDOG_EnableInterrupts(WDOG_Type *base, uint32_t mask)
{
    base->STCTRLH |= mask;
}

/*!
 * @brief Disable WDOG interrupt.
 *
 * This function write value into WDOG_STCTRLH register to disable WDOG interrupt, it is a write-once register,
 * make sure that the WCT window is still open and this register has not been written in this WCT
 * while this function is called.
 *
 * @param base WDOG peripheral base address
 * @param mask The interrupts to disable
 *        The parameter can be combination of the following source if defined:
 *        @arg kWDOG_InterruptEnable
 */
static inline void WDOG_DisableInterrupts(WDOG_Type *base, uint32_t mask)
{
    base->STCTRLH &= ~mask;
}

/*!
 * @brief Gets WDOG all status flags.
 *
 * This function gets all status flags.
 *
 * Example for getting Running Flag:
 * @code
 *   uint32_t status;
 *   status = WDOG_GetStatusFlags(wdog_base) & kWDOG_RunningFlag;
 * @endcode
 * @param base        WDOG peripheral base address
 * @return            State of the status flag: asserted (true) or not-asserted (false).@see _wdog_status_flags_t
 *                    - true: related status flag has been set.
 *                    - false: related status flag is not set.
 */
uint32_t WDOG_GetStatusFlags(WDOG_Type *base);

/*!
 * @brief Clear WDOG flag.
 *
 * This function clears WDOG status flag.
 *
 * Example for clearing timeout(interrupt) flag:
 * @code
 *   WDOG_ClearStatusFlags(wdog_base,kWDOG_TimeoutFlag);
 * @endcode
 * @param base        WDOG peripheral base address
 * @param mask        The status flags to clear.
 *                    The parameter could be any combination of the following values:
 *                    kWDOG_TimeoutFlag
 */
void WDOG_ClearStatusFlags(WDOG_Type *base, uint32_t mask);

/*!
 * @brief Set the WDOG timeout value.
 *
 * This function sets the timeout value.
 * It should be ensured that the time-out value for the WDOG is always greater than
 * 2xWCT time + 20 bus clock cycles.
 * This function write value into WDOG_TOVALH and WDOG_TOVALL registers which are wirte-once.
 * Make sure the WCT window is still open and these two registers have not been written in this WCT
 * while this function is called.
 *
 * @param base WDOG peripheral base address
 * @param timeoutCount WDOG timeout value, count of WDOG clock tick.
 */
static inline void WDOG_SetTimeoutValue(WDOG_Type *base, uint32_t timeoutCount)
{
    base->TOVALH = (uint16_t)((timeoutCount >> 16U) & 0xFFFFU);
    base->TOVALL = (uint16_t)((timeoutCount)&0xFFFFU);
}

/*!
 * @brief Sets the WDOG window value.
 *
 * This function sets the WDOG window value.
 * This function write value into WDOG_WINH and WDOG_WINL registers which are wirte-once.
 * Make sure the WCT window is still open and these two registers have not been written in this WCT
 * while this function is called.
 *
 * @param base WDOG peripheral base address
 * @param windowValue WDOG window value.
 */
static inline void WDOG_SetWindowValue(WDOG_Type *base, uint32_t windowValue)
{
    base->WINH = (uint16_t)((windowValue >> 16U) & 0xFFFFU);
    base->WINL = (uint16_t)((windowValue)&0xFFFFU);
}

/*!
 * @brief Unlocks the WDOG register written.
 *
 * This function unlocks the WDOG register written.
 * Before starting the unlock sequence and following congfiguration, disable the global interrupts.
 * Otherwise, an interrupt could effectively invalidate the unlock sequence and the WCT may expire,
 * After the configuration finishes, re-enable the global interrupts.
 *
 * @param base WDOG peripheral base address
 */
static inline void WDOG_Unlock(WDOG_Type *base)
{
    base->UNLOCK = WDOG_FIRST_WORD_OF_UNLOCK;
    base->UNLOCK = WDOG_SECOND_WORD_OF_UNLOCK;
}

/*!
 * @brief Refreshes the WDOG timer.
 *
 * This function feeds the WDOG.
 * This function should be called before WDOG timer is in timeout. Otherwise, a reset is asserted.
 *
 * @param base WDOG peripheral base address
 */
void WDOG_Refresh(WDOG_Type *base);

/*!
 * @brief Gets the WDOG reset count.
 *
 * This function gets the WDOG reset count value.
 *
 * @param base WDOG peripheral base address
 * @return     WDOG reset count value
 */
static inline uint16_t WDOG_GetResetCount(WDOG_Type *base)
{
    return base->RSTCNT;
}
/*!
 * @brief Clears the WDOG reset count.
 *
 * This function clears the WDOG reset count value.
 *
 * @param base WDOG peripheral base address
 */
static inline void WDOG_ClearResetCount(WDOG_Type *base)
{
    base->RSTCNT |= UINT16_MAX;
}

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_WDOG_H_ */
