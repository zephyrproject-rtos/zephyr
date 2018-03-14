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

#ifndef __WDOG_IMX_H__
#define __WDOG_IMX_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup wdog_imx_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief The reset source of latest reset. */
enum _wdog_reset_source
{
    wdogResetSourcePor     = WDOG_WRSR_POR_MASK,     /*!< Indicates the reset is the result of a power on reset.*/
    wdogResetSourceTimeout = WDOG_WRSR_TOUT_MASK,    /*!< Indicates the reset is the result of a WDOG timeout.*/
    wdogResetSourceSwRst   = WDOG_WRSR_SFTW_MASK,    /*!< Indicates the reset is the result of a software reset.*/
};

/*! @brief Structure to configure the running mode. */
typedef struct _wdog_init_config
{
    bool wdw;   /*!< true: suspend in low power wait, false: not suspend */
    bool wdt;   /*!< true: assert WDOG_B when timeout, false: not assert WDOG_B */
    bool wdbg;  /*!< true: suspend in debug mode, false: not suspend */
    bool wdzst; /*!< true: suspend in doze and stop mode, false: not suspend */
} wdog_init_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name WDOG State Control
 * @{
 */

/*!
 * @brief Configure WDOG functions, call once only
 *
 * @param base WDOG base pointer.
 * @param initConfig WDOG mode configuration
 */
static inline void WDOG_Init(WDOG_Type *base, const wdog_init_config_t *initConfig)
{
    base->WCR |= (initConfig->wdw   ? WDOG_WCR_WDW_MASK   : 0) |
                 (initConfig->wdt   ? WDOG_WCR_WDT_MASK   : 0) |
                 (initConfig->wdbg  ? WDOG_WCR_WDBG_MASK  : 0) |
                 (initConfig->wdzst ? WDOG_WCR_WDZST_MASK : 0);
}

/*!
 * @brief Enable WDOG with timeout, call once only
 *
 * @param base WDOG base pointer.
 * @param timeout WDOG timeout ((n+1)/2 second)
 */
void WDOG_Enable(WDOG_Type *base, uint8_t timeout);

/*!
 * @brief Assert WDOG software reset signal
 *
 * @param base WDOG base pointer.
 * @param wda WDOG reset.
 *            - true: Assert WDOG_B.
 *            - false: No impact on WDOG_B.
 * @param srs System reset.
 *            - true: Assert system reset WDOG_RESET_B_DEB.
 *            - false: No impact on system reset.
 */
void WDOG_Reset(WDOG_Type *base, bool wda, bool srs);

/*!
 * @brief Get the latest reset source generated due to
 * WatchDog Timer.
 *
 * @param base WDOG base pointer.
 * @return The latest reset source (see @ref _wdog_reset_source enumeration).
 */
static inline uint32_t WDOG_GetResetSource(WDOG_Type *base)
{
    return base->WRSR;
}

/*!
 * @brief Refresh the WDOG to prevent timeout
 *
 * @param base WDOG base pointer.
 */
void WDOG_Refresh(WDOG_Type *base);

/*!
 * @brief Disable WDOG power down counter
 *
 * @param base WDOG base pointer.
 */
static inline void WDOG_DisablePowerdown(WDOG_Type *base)
{
    base->WMCR &= ~WDOG_WMCR_PDE_MASK;
}

/*@}*/

/*!
 * @name WDOG Interrupt Control
 * @{
 */

/*!
 * @brief Enable WDOG interrupt
 *
 * @param base WDOG base pointer.
 * @param time how long before the timeout must the interrupt occur (n/2 seconds).
 */
static inline void WDOG_EnableInt(WDOG_Type *base, uint8_t time)
{
    base->WICR = WDOG_WICR_WIE_MASK | WDOG_WICR_WICT(time);
}

/*!
 * @brief Check whether WDOG interrupt is pending
 *
 * @param base WDOG base pointer.
 * @return WDOG interrupt status.
 *              - true: Pending.
 *              - false: Not pending.
 */
static inline bool WDOG_IsIntPending(WDOG_Type *base)
{
    return (bool)(base->WICR & WDOG_WICR_WTIS_MASK);
}

/*!
 * @brief Clear WDOG interrupt status
 *
 * @param base WDOG base pointer.
 */
static inline void WDOG_ClearStatusFlag(WDOG_Type *base)
{
    base->WICR |= WDOG_WICR_WTIS_MASK;
}

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __WDOG_IMX_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
