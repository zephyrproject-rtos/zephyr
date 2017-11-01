/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
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
#ifndef _FSL_WDOG_H_
#define _FSL_WDOG_H_

#include "fsl_common.h"

/*!
 * @addtogroup wdog
 * @{
 */

/*******************************************************************************
 * Definitions
 *******************************************************************************/
/*! @name Driver version */
/*@{*/
/*! @brief Defines WDOG driver version */
#define FSL_WDOG_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/
/*! @name Refresh sequence */
/*@{*/
#define WDOG_REFRESH_KEY                         (0xAAAA5555U)
/*@}*/

/*! @brief Defines WDOG work mode. */
typedef struct _wdog_work_mode
{
    bool enableWait;  /*!< continue or suspend WDOG in wait mode  */
    bool enableStop;  /*!< continue or suspend WDOG in stop mode  */
    bool enableDebug; /*!< continue or suspend WDOG in debug mode */
} wdog_work_mode_t;

/*! @brief Describes WDOG configuration structure. */
typedef struct _wdog_config
{
    bool enableWdog;             /*!< Enables or disables WDOG */
    wdog_work_mode_t workMode;   /*!< Configures WDOG work mode in debug stop and wait mode */
    bool enableInterrupt;        /*!< Enables or disables WDOG interrupt */
    uint16_t timeoutValue;       /*!< Timeout value */
    uint16_t interruptTimeValue; /*!< Interrupt count timeout value */
    bool softwareResetExtension; /*!< software reset extension */
    bool enablePowerDown;        /*!< power down enable bit */
    bool softwareAssertion;      /*!< software assertion bit*/
    bool softwareResetSignal;    /*!< software reset signalbit*/
} wdog_config_t;

/*!
 * @brief WDOG interrupt configuration structure, default settings all disabled.
 *
 * This structure contains the settings for all of the WDOG interrupt configurations.
 */
enum _wdog_interrupt_enable
{
    kWDOG_InterruptEnable = WDOG_WICR_WIE_MASK /*!< WDOG timeout generates an interrupt before reset*/
};

/*!
 * @brief WDOG status flags.
 *
 * This structure contains the WDOG status flags for use in the WDOG functions.
 */
enum _wdog_status_flags
{
    kWDOG_RunningFlag = WDOG_WCR_WDE_MASK,         /*!< Running flag, set when WDOG is enabled*/
    kWDOG_PowerOnResetFlag = WDOG_WRSR_POR_MASK,   /*!< Power On flag, set when reset is the result of a powerOnReset*/
    kWDOG_TimeoutResetFlag = WDOG_WRSR_TOUT_MASK,  /*!< Timeout flag, set when reset is the result of a timeout*/
    kWDOG_SoftwareResetFlag = WDOG_WRSR_SFTW_MASK, /*!< Software flag, set when reset is the result of a software*/
    kWDOG_InterruptFlag = WDOG_WICR_WTIS_MASK      /*!< interrupt flag,whether interrupt has occurred or not*/
};

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name WDOG Initialization and De-initialization.
 * @{
 */

/*!
 * @brief Initializes the WDOG configuration sturcture.
 *
 * This function initializes the WDOG configuration structure to default values. The default
 * values are as follows.
 * @code
 *   wdogConfig->enableWdog = true;
 *   wdogConfig->workMode.enableWait = true;
 *   wdogConfig->workMode.enableStop = false;
 *   wdogConfig->workMode.enableDebug = false;
 *   wdogConfig->enableInterrupt = false;
 *   wdogConfig->enablePowerdown = false;
 *   wdogConfig->resetExtension = flase;
 *   wdogConfig->timeoutValue = 0xFFU;
 *   wdogConfig->interruptTimeValue = 0x04u;
 * @endcode
 *
 * @param config Pointer to the WDOG configuration structure.
 * @see wdog_config_t
 */
void WDOG_GetDefaultConfig(wdog_config_t *config);

/*!
 * @brief Initializes the WDOG.
 *
 * This function initializes the WDOG. When called, the WDOG runs according to the configuration.
 *
 * This is an example.
 * @code
 *   wdog_config_t config;
 *   WDOG_GetDefaultConfig(&config);
 *   config.timeoutValue = 0xffU;
 *   config->interruptTimeValue = 0x04u;
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
 * Watchdog Enable bit is a write one once only bit. It is not
 * possible to clear this bit by a software write, once the bit is set.
 * This bit(WDE) can be set/reset only in debug mode(exception).
 */
void WDOG_Deinit(WDOG_Type *base);

/*!
 * @brief Enables the WDOG module.
 *
 * This function writes a value into the WDOG_WCR register to enable the WDOG.
 * This is a write one once only bit. It is not possible to clear this bit by a software write,
 * once the bit is set. only debug mode exception.
 * @param base WDOG peripheral base address
 */
static inline void WDOG_Enable(WDOG_Type *base)
{
    base->WCR |= WDOG_WCR_WDE_MASK;
}

/*!
 * @brief Disables the WDOG module.
 *
 * This function writes a value into the WDOG_WCR register to disable the WDOG.
 * This is a write one once only bit. It is not possible to clear this bit by a software write,once the bit is set.
 * only debug mode exception
 * @param base WDOG peripheral base address
 */
static inline void WDOG_Disable(WDOG_Type *base)
{
    base->WCR &= ~WDOG_WCR_WDE_MASK;
}

/*!
 * @brief Enables the WDOG interrupt.
 *
 *This bit is a write once only bit. Once the software does a write access to this bit, it will get
 *locked and cannot be reprogrammed until the next system reset assertion
 *
 * @param base WDOG peripheral base address
 * @param mask The interrupts to enable
 * The parameter can be combination of the following source if defined.
 * @arg kWDOG_InterruptEnable
 */
static inline void WDOG_EnableInterrupts(WDOG_Type *base, uint16_t mask)
{
    base->WICR |= mask;
}

/*!
 * @brief Gets the WDOG all reset status flags.
 *
 * This function gets all reset status flags.
 *
 * @code
 * uint16_t status;
 * status = WDOG_GetStatusFlags (wdog_base);
 * @endcode
 * @param base        WDOG peripheral base address
 * @return            State of the status flag: asserted (true) or not-asserted (false).@see _wdog_status_flags
 *                    - true: a related status flag has been set.
 *                    - false: a related status flag is not set.
 */
uint16_t WDOG_GetStatusFlags(WDOG_Type *base);

/*!
 * @brief Clears the WDOG flag.
 *
 * This function clears the WDOG status flag.
 *
 * This is an example for clearing the interrupt flag.
 * @code
 *   WDOG_ClearStatusFlags(wdog_base,KWDOG_InterruptFlag);
 * @endcode
 * @param base        WDOG peripheral base address
 * @param mask        The status flags to clear.
 *                    The parameter could be any combination of the following values.
 *                    kWDOG_TimeoutFlag
 */
void WDOG_ClearInterruptStatus(WDOG_Type *base, uint16_t mask);

/*!
 * @brief Sets the WDOG timeout value.
 *
 * This function sets the timeout value.
 * This function writes a value into WCR registers.
 * The time-out value can be written at any point of time but it is loaded to the counter at the time
 * when WDOG is enabled or after the service routine has been performed.
 *
 * @param base WDOG peripheral base address
 * @param timeoutCount WDOG timeout value; count of WDOG clock tick.
 */
static inline void WDOG_SetTimeoutValue(WDOG_Type *base, uint16_t timeoutCount)
{
    base->WCR = (base->WCR & ~WDOG_WCR_WT_MASK) | WDOG_WCR_WT(timeoutCount);
}

/*!
 * @brief Sets the WDOG interrupt count timeout value.
 *
 * This function sets the interrupt count timeout value.
 * This function writes a value into WIC registers which are wirte-once.
 * This field is write once only. Once the software does a write access to this field, it will get locked
 * and cannot be reprogrammed until the next system reset assertion.
 * @param base WDOG peripheral base address
 * @param timeoutCount WDOG timeout value; count of WDOG clock tick.
 */
static inline void WDOG_SetInterrputTimeoutValue(WDOG_Type *base, uint16_t timeoutCount)
{
    base->WICR = (base->WICR & ~WDOG_WICR_WICT_MASK) | WDOG_WICR_WICT(timeoutCount);
}

/*!
 * @brief Disable the WDOG power down enable bit.
 *
 * This function disable the WDOG power down enable(PDE).
 * This function writes a value into WMCR registers which are wirte-once.
 * This field is write once only. Once software sets this bit it cannot be reset until the next system reset.
 * @param base WDOG peripheral base address
 */
static inline void WDOG_DisablePowerDownEnable(WDOG_Type *base)
{
    base->WMCR &= ~WDOG_WMCR_PDE_MASK;
}

/*!
 * @brief Refreshes the WDOG timer.
 *
 * This function feeds the WDOG.
 * This function should be called before the WDOG timer is in timeout. Otherwise, a reset is asserted.
 *
 * @param base WDOG peripheral base address
 */
void WDOG_Refresh(WDOG_Type *base);

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_WDOG_H_ */
