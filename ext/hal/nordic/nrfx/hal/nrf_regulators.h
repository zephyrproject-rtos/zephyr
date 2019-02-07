/*
 * Copyright (c) 2018 - 2019, Nordic Semiconductor ASA
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_REGULATORS_H__
#define NRF_REGULATORS_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_regulators_hal REGULATORS HAL
 * @{
 * @ingroup nrf_power
 * @brief   Hardware access layer for managing the REGULATORS peripheral.
 */

/**
 * @brief Function for enabling or disabling DCDC converter.
 *
 * @param[in] p_reg  Pointer to the peripheral registers structure.
 * @param[in] enable Set true to enable or false to disable DCDC converter.
 */
__STATIC_INLINE void nrf_regulators_dcdcen_set(NRF_REGULATORS_Type * p_reg, bool enable);

/**
 * @brief Function for putting CPU in system OFF mode.
 *
 * This function puts the CPU into system off mode.
 * The only way to wake up the CPU is by reset.
 *
 * @note This function never returns.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 */
__STATIC_INLINE void nrf_regulators_system_off(NRF_REGULATORS_Type * p_reg);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_regulators_dcdcen_set(NRF_REGULATORS_Type * p_reg, bool enable)
{
    p_reg->DCDCEN = (enable ? REGULATORS_DCDCEN_DCDCEN_Msk : 0);
}

__STATIC_INLINE void nrf_regulators_system_off(NRF_REGULATORS_Type * p_reg)
{
    p_reg->SYSTEMOFF = REGULATORS_SYSTEMOFF_SYSTEMOFF_Msk;
    __DSB();

    /* Solution for simulated System OFF in debug mode */
    while (true)
    {
        __WFE();
    }
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_REGULATORS_H__
