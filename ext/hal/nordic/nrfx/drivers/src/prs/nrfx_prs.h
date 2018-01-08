/**
 * Copyright (c) 2017, Nordic Semiconductor ASA
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_PRS_H__
#define NRFX_PRS_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_prs Peripheral Resource Sharing (PRS)
 * @{
 * @ingroup nrfx
 *
 * @brief Peripheral Resource Sharing interface (PRS).
 */

#if defined(NRF51)
    // SPI0, TWI0
    #define NRFX_PRS_BOX_0_ADDR     NRF_SPI0
    // SPI1, SPIS1, TWI1
    #define NRFX_PRS_BOX_1_ADDR     NRF_SPI1
#elif defined(NRF52810_XXAA)
    // TWIM0, TWIS0
    #define NRFX_PRS_BOX_0_ADDR     NRF_TWIM0
    // SPIM0, SPIS0
    #define NRFX_PRS_BOX_1_ADDR     NRF_SPIM0
#elif defined(NRF52832_XXAA) || defined (NRF52832_XXAB)
    // SPIM0, SPIS0, TWIM0, TWIS0, SPI0, TWI0
    #define NRFX_PRS_BOX_0_ADDR     NRF_SPIM0
    // SPIM1, SPIS1, TWIM1, TWIS1, SPI1, TWI1
    #define NRFX_PRS_BOX_1_ADDR     NRF_SPIM1
    // SPIM2, SPIS2, SPI2
    #define NRFX_PRS_BOX_2_ADDR     NRF_SPIM2
    // COMP, LPCOMP
    #define NRFX_PRS_BOX_3_ADDR     NRF_COMP
    // UARTE0, UART0
    #define NRFX_PRS_BOX_4_ADDR     NRF_UARTE0
#elif defined(NRF52840_XXAA)
    // SPIM0, SPIS0, TWIM0, TWIS0, SPI0, TWI0
    #define NRFX_PRS_BOX_0_ADDR     NRF_SPIM0
    // SPIM1, SPIS1, TWIM1, TWIS1, SPI1, TWI1
    #define NRFX_PRS_BOX_1_ADDR     NRF_SPIM1
    // SPIM2, SPIS2, SPI2
    #define NRFX_PRS_BOX_2_ADDR     NRF_SPIM2
    // COMP, LPCOMP
    #define NRFX_PRS_BOX_3_ADDR     NRF_COMP
    // UARTE0, UART0
    #define NRFX_PRS_BOX_4_ADDR     NRF_UARTE0
#else
    #error "Unknown device."
#endif

/**
 * @brief Function for acquiring shared peripheral resources associated with
 *        the specified peripheral.
 *
 * Certain resources and registers are shared among peripherals that have
 * the same ID (for example: SPI0, SPIM0, SPIS0, TWI0, TWIM0, and TWIS0 in
 * nRF52832). Only one of them can be utilized at a given time. This function
 * reserves proper resources to be used by the specified peripheral.
 * If NRFX_PRS_ENABLED is set to a non-zero value, IRQ handlers for peripherals
 * that are sharing resources with others are implemented by the @ref nrfx_prs
 * module instead of individual drivers. The drivers must then specify their
 * interrupt handling routines and register them by using this function.
 *
 * @param[in] p_base_addr Requested peripheral base pointer.
 * @param[in] irq_handler Interrupt handler to register.
 *
 * @retval NRFX_SUCCESS    If resources were acquired successfully or the
 *                         specified peripheral is not handled by the PRS
 *                         subsystem and there is no need to acquire resources
 *                         for it.
 * @retval NRFX_ERROR_BUSY If resources were already acquired.
 */
nrfx_err_t nrfx_prs_acquire(void       const * p_base_addr,
                            nrfx_irq_handler_t irq_handler);

/**
 * @brief Function for releasing shared resources reserved previously by
 *        @ref nrfx_prs_acquire() for the specified peripheral.
 *
 * @param[in] p_base_addr Released peripheral base pointer.
 */
void nrfx_prs_release(void const * p_base_addr);


void nrfx_prs_box_0_irq_handler(void);
void nrfx_prs_box_1_irq_handler(void);
void nrfx_prs_box_2_irq_handler(void);
void nrfx_prs_box_3_irq_handler(void);
void nrfx_prs_box_4_irq_handler(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_PRS_H__
