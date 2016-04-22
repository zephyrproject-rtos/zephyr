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

#ifndef _FSL_DMAMUX_H_
#define _FSL_DMAMUX_H_

#include "fsl_common.h"

/*!
 * @addtogroup dmamux
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief DMAMUX driver version 2.0.0. */
#define FSL_DMAMUX_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name DMAMUX Initialize and De-initialize
 * @{
 */

/*!
 * @brief Initializes DMAMUX peripheral.
 *
 * This function ungate the DMAMUX clock.
 *
 * @param base DMAMUX peripheral base address.
 *
 */
void DMAMUX_Init(DMAMUX_Type *base);

/*!
 * @brief Deinitializes DMAMUX peripheral.
 *
 * This function gate the DMAMUX clock.
 *
 * @param base DMAMUX peripheral base address.
 */
void DMAMUX_Deinit(DMAMUX_Type *base);

/* @} */
/*!
 * @name DMAMUX Channel Operation
 * @{
 */

/*!
 * @brief Enable DMAMUX channel.
 *
 * This function enable DMAMUX channel to work.
 *
 * @param base DMAMUX peripheral base address.
 * @param channel DMAMUX channel number.
 */
static inline void DMAMUX_EnableChannel(DMAMUX_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->CHCFG[channel] |= DMAMUX_CHCFG_ENBL_MASK;
}

/*!
 * @brief Disable DMAMUX channel.
 *
 * This function disable DMAMUX channel.
 *
 * @note User must disable DMAMUX channel before configure it.
 * @param base DMAMUX peripheral base address.
 * @param channel DMAMUX channel number.
 */
static inline void DMAMUX_DisableChannel(DMAMUX_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->CHCFG[channel] &= ~DMAMUX_CHCFG_ENBL_MASK;
}

/*!
 * @brief Configure DMAMUX channel source.
 *
 * @param base DMAMUX peripheral base address.
 * @param channel DMAMUX channel number.
 * @param source Channel source which is used to trigger DMA transfer.
 */
static inline void DMAMUX_SetSource(DMAMUX_Type *base, uint32_t channel, uint8_t source)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->CHCFG[channel] = ((base->CHCFG[channel] & ~DMAMUX_CHCFG_SOURCE_MASK) | DMAMUX_CHCFG_SOURCE(source));
}

#if defined(FSL_FEATURE_DMAMUX_HAS_TRIG) && FSL_FEATURE_DMAMUX_HAS_TRIG > 0U
/*!
 * @brief Enable DMAMUX period trigger.
 *
 * This function enable DMAMUX period trigger feature.
 *
 * @param base DMAMUX peripheral base address.
 * @param channel DMAMUX channel number.
 */
static inline void DMAMUX_EnablePeriodTrigger(DMAMUX_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->CHCFG[channel] |= DMAMUX_CHCFG_TRIG_MASK;
}

/*!
 * @brief Disable DMAMUX period trigger.
 *
 * This function disable DMAMUX period trigger.
 *
 * @param base DMAMUX peripheral base address.
 * @param channel DMAMUX channel number.
 */
static inline void DMAMUX_DisablePeriodTrigger(DMAMUX_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->CHCFG[channel] &= ~DMAMUX_CHCFG_TRIG_MASK;
}
#endif /* FSL_FEATURE_DMAMUX_HAS_TRIG */

/* @} */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/* @} */

#endif /* _FSL_DMAMUX_H_ */
