/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
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

#ifndef _FSL_XBARB_H_
#define _FSL_XBARB_H_

#include "fsl_common.h"

/*!
 * @addtogroup xbarb
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define FSL_XBARB_DRIVER_VERSION (MAKE_VERSION(2, 0, 1)) /*!< Version 2.0.1. */

/* Macros for entire XBARB_SELx register. */
#define XBARB_SELx(base, output) (*(volatile uint16_t *)((uintptr_t) & (base->SEL0) + ((output) / 2U) * 2U))
/* Set the SELx field to a new value. */
#define XBARB_WR_SELx_SELx(base, input, output)                                                    \
    (XBARB_SELx((base), (output)) =                                                                \
         ((XBARB_SELx((base), (output)) & ~(0xFFU << (XBARB_SEL0_SEL1_SHIFT * ((output) % 2U)))) | \
          ((input) << (XBARB_SEL0_SEL1_SHIFT * ((output) % 2U)))))

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name XBARB functional Operation.
 * @{
 */

/*!
 * @brief Initializes the XBARB module.
 *
 * This function un-gates the XBARB clock.
 *
 * @param base XBARB peripheral address.
 */
void XBARB_Init(XBARB_Type *base);

/*!
 * @brief Shuts down the XBARB module.
 *
 * This function disables XBARB clock.
 *
 * @param base XBARB peripheral address.
 */
void XBARB_Deinit(XBARB_Type *base);

/*!
 * @brief Configures a connection between the selected XBARB_IN[*] input and the XBARB_OUT[*] output signal.
 *
 * This function configures which XBARB input is connected to the selected XBARB output.
 * If more than one XBARB module is available, only the inputs and outputs from the same module
 * can be connected.
 *
 * @param base XBARB peripheral address.
 * @param input XBARB input signal.
 * @param output XBARB output signal.
 */
void XBARB_SetSignalsConnection(XBARB_Type *base, xbar_input_signal_t input, xbar_output_signal_t output);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/

/*!* @} */

#endif /* _FSL_XBARB_H_ */
