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


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief DMAMUX driver version 2.0.2. */
#define FSL_DMAMUX_DRIVER_VERSION (MAKE_VERSION(2, 0, 2))
/*@}*/

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name DMAMUX Initialization and de-initialization
 * @{
 */

/*!
 * @brief Initializes the DMAMUX peripheral.
 *
 * This function ungates the DMAMUX clock.
 *
 * @param base DMAMUX peripheral base address.
 *
 */
void DMAMUX_Init(DMAMUX_Type *base);

/*!
 * @brief Deinitializes the DMAMUX peripheral.
 *
 * This function gates the DMAMUX clock.
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
 * @brief Enables the DMAMUX channel.
 *
 * This function enables the DMAMUX channel.
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
 * @brief Disables the DMAMUX channel.
 *
 * This function disables the DMAMUX channel.
 *
 * @note The user must disable the DMAMUX channel before configuring it.
 * @param base DMAMUX peripheral base address.
 * @param channel DMAMUX channel number.
 */
static inline void DMAMUX_DisableChannel(DMAMUX_Type *base, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->CHCFG[channel] &= ~DMAMUX_CHCFG_ENBL_MASK;
}

/*!
 * @brief Configures the DMAMUX channel source.
 *
 * @param base DMAMUX peripheral base address.
 * @param channel DMAMUX channel number.
 * @param source Channel source, which is used to trigger the DMA transfer.
 */
static inline void DMAMUX_SetSource(DMAMUX_Type *base, uint32_t channel, uint32_t source)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    base->CHCFG[channel] = ((base->CHCFG[channel] & ~DMAMUX_CHCFG_SOURCE_MASK) | DMAMUX_CHCFG_SOURCE(source));
}

#if defined(FSL_FEATURE_DMAMUX_HAS_TRIG) && FSL_FEATURE_DMAMUX_HAS_TRIG > 0U
/*!
 * @brief Enables the DMAMUX period trigger.
 *
 * This function enables the DMAMUX period trigger feature.
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
 * @brief Disables the DMAMUX period trigger.
 *
 * This function disables the DMAMUX period trigger.
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

#if (defined(FSL_FEATURE_DMAMUX_HAS_A_ON) && FSL_FEATURE_DMAMUX_HAS_A_ON)
/*!
 * @brief Enables the DMA channel to be always ON.
 *
 * This function enables the DMAMUX channel always ON feature.
 *
 * @param base DMAMUX peripheral base address.
 * @param channel DMAMUX channel number.
 * @param enable Switcher of the always ON feature. "true" means enabled, "false" means disabled.
 */
static inline void DMAMUX_EnableAlwaysOn(DMAMUX_Type *base, uint32_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMAMUX_MODULE_CHANNEL);

    if (enable)
    {
        base->CHCFG[channel] |= DMAMUX_CHCFG_A_ON_MASK;
    }
    else
    {
        base->CHCFG[channel] &= ~DMAMUX_CHCFG_A_ON_MASK;
    }
}
#endif /* FSL_FEATURE_DMAMUX_HAS_A_ON */

/* @} */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/* @} */

#endif /* _FSL_DMAMUX_H_ */
