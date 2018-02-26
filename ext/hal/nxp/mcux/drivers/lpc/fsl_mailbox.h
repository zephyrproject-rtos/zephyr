/*
 * Copyright(C) NXP Semiconductors, 2014
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

#ifndef _FSL_MAILBOX_H_
#define _FSL_MAILBOX_H_

#include "fsl_common.h"

/*!
 * @addtogroup mailbox
 * @{
 */

/*! @file */

/******************************************************************************
 * Definitions
 *****************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief MAILBOX driver version 2.0.0. */
#define FSL_MAILBOX_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*!
 * @brief CPU ID.
 */
typedef enum _mailbox_cpu_id
{
    kMAILBOX_CM0Plus = 0,
    kMAILBOX_CM4
} mailbox_cpu_id_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @name MAILBOX initialization
 * @{
 */

/*!
 * @brief Initializes the MAILBOX module.
 *
 * This function enables the MAILBOX clock only.
 *
 * @param base MAILBOX peripheral base address.
 */
static inline void MAILBOX_Init(MAILBOX_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_Mailbox);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * @brief De-initializes the MAILBOX module.
 *
 * This function disables the MAILBOX clock only.
 *
 * @param base MAILBOX peripheral base address.
 */
static inline void MAILBOX_Deinit(MAILBOX_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(kCLOCK_Mailbox);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/* @} */

/*!
 * @brief Set data value in the mailbox based on the CPU ID.
 *
 * @param base MAILBOX peripheral base address.
 * @param cpu_id CPU id, kMAILBOX_CM0Plus is M0+ or kMAILBOX_CM4 is M4.
 * @param mboxData Data to send in the mailbox.
 *
 * @note Sets a data value to send via the MAILBOX to the other core.
 */
static inline void MAILBOX_SetValue(MAILBOX_Type *base, mailbox_cpu_id_t cpu_id, uint32_t mboxData)
{
    assert((cpu_id == kMAILBOX_CM0Plus) || (cpu_id == kMAILBOX_CM4));
    base->MBOXIRQ[cpu_id].IRQ = mboxData;
}

/*!
 * @brief Get data in the mailbox based on the CPU ID.
 *
 * @param base MAILBOX peripheral base address.
 * @param cpu_id CPU id, kMAILBOX_CM0Plus is M0+ or kMAILBOX_CM4 is M4.
 *
 * @return Current mailbox data.
 */
static inline uint32_t MAILBOX_GetValue(MAILBOX_Type *base, mailbox_cpu_id_t cpu_id)
{
    assert((cpu_id == kMAILBOX_CM0Plus) || (cpu_id == kMAILBOX_CM4));
    return base->MBOXIRQ[cpu_id].IRQ;
}

/*!
 * @brief Set data bits in the mailbox based on the CPU ID.
 *
 * @param base MAILBOX peripheral base address.
 * @param cpu_id CPU id, kMAILBOX_CM0Plus is M0+ or kMAILBOX_CM4 is M4.
 * @param mboxSetBits Data bits to set in the mailbox.
 *
 * @note Sets data bits to send via the MAILBOX to the other core. A value of 0 will
 * do nothing. Only sets bits selected with a 1 in it's bit position.
 */
static inline void MAILBOX_SetValueBits(MAILBOX_Type *base, mailbox_cpu_id_t cpu_id, uint32_t mboxSetBits)
{
    assert((cpu_id == kMAILBOX_CM0Plus) || (cpu_id == kMAILBOX_CM4));
    base->MBOXIRQ[cpu_id].IRQSET = mboxSetBits;
}

/*!
 * @brief Clear data bits in the mailbox based on the CPU ID.
 *
 * @param base MAILBOX peripheral base address.
 * @param cpu_id CPU id, kMAILBOX_CM0Plus is M0+ or kMAILBOX_CM4 is M4.
 * @param mboxClrBits Data bits to clear in the mailbox.
 *
 * @note Clear data bits to send via the MAILBOX to the other core. A value of 0 will
 * do nothing. Only clears bits selected with a 1 in it's bit position.
 */
static inline void MAILBOX_ClearValueBits(MAILBOX_Type *base, mailbox_cpu_id_t cpu_id, uint32_t mboxClrBits)
{
    assert((cpu_id == kMAILBOX_CM0Plus) || (cpu_id == kMAILBOX_CM4));
    base->MBOXIRQ[cpu_id].IRQCLR = mboxClrBits;
}

/*!
 * @brief Get MUTEX state and lock mutex
 *
 * @param base MAILBOX peripheral base address.
 *
 * @return See note
 *
 * @note Returns '1' if the mutex was taken or '0' if another resources has the
 * mutex locked. Once a mutex is taken, it can be returned with the MAILBOX_SetMutex()
 * function.
 */
static inline uint32_t MAILBOX_GetMutex(MAILBOX_Type *base)
{
    return (base->MUTEX & MAILBOX_MUTEX_EX_MASK);
}

/*!
 * @brief Set MUTEX state
 *
 * @param base MAILBOX peripheral base address.
 *
 * @note Sets mutex state to '1' and allows other resources to get the mutex.
 */
static inline void MAILBOX_SetMutex(MAILBOX_Type *base)
{
    base->MUTEX = MAILBOX_MUTEX_EX_MASK;
}

#if defined(__cplusplus)
}
#endif /*_cplusplus*/
/*@}*/

#endif /* _FSL_MAILBOX_H_ */
