/*
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_SYSTICK_H__
#define NRF_SYSTICK_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_systick_hal SYSTICK HAL
 * @{
 * @ingroup nrf_systick
 * @brief   Hardware access layer (HAL) for managing the SYSTICK peripheral.
 *
 * SYSTICK is a peripheral designed by ARM.
 * This means that it does not feature the typical Nordic interface with Tasks and Events.
 *
 * Its usage is limited here to the implementation of simple delays.
 * Moreover, keep in mind that this timer will be stopped when CPU is sleeping
 * (WFE/WFI instruction is successfully executed).
 */

/**
 * @brief Mask of usable bits in the SysTick value.
 */
#define NRF_SYSTICK_VAL_MASK SysTick_VAL_CURRENT_Msk

/**
 * @brief Flags used by SysTick configuration.
 *
 * @sa nrf_systick_csr_set
 * @sa nrf_systick_csr_get
 */
typedef enum {
    NRF_SYSTICK_CSR_COUNTFLAG_MASK  = SysTick_CTRL_COUNTFLAG_Msk,       /**< Status flag: Returns 1 if timer counted to 0 since the last read of this register. */

    NRF_SYSTICK_CSR_CLKSOURCE_MASK  = SysTick_CTRL_CLKSOURCE_Msk,       /**< Configuration bit: Select the SysTick clock source. */
    NRF_SYSTICK_CSR_CLKSOURCE_REF   = 0U << SysTick_CTRL_CLKSOURCE_Pos, /**< Configuration value: Select reference clock. */
    NRF_SYSTICK_CSR_CLKSOURCE_CPU   = 1U << SysTick_CTRL_CLKSOURCE_Pos, /**< Configuration value: Select CPU clock. */

    NRF_SYSTICK_CSR_TICKINT_MASK    = SysTick_CTRL_TICKINT_Msk,         /**< Configuration bit: Enables SysTick exception request. */
    NRF_SYSTICK_CSR_TICKINT_ENABLE  = 1U << SysTick_CTRL_TICKINT_Pos,   /**< Configuration value: Counting down to zero does not assert the SysTick exception request. */
    NRF_SYSTICK_CSR_TICKINT_DISABLE = 0U << SysTick_CTRL_TICKINT_Pos,   /**< Configuration value: Counting down to zero to asserts the SysTick exception request. */

    NRF_SYSTICK_CSR_ENABLE_MASK     = SysTick_CTRL_ENABLE_Msk,          /**< Configuration bit: Enable the SysTick timer. */
    NRF_SYSTICK_CSR_ENABLE          = 1U << SysTick_CTRL_ENABLE_Pos,    /**< Configuration value: Counter enabled. */
    NRF_SYSTICK_CSR_DISABLE         = 0U << SysTick_CTRL_ENABLE_Pos     /**< Configuration value: Counter disabled. */
} nrf_systick_csr_flags_t;

/**
 * @brief Function for getting Configuration and Status Register.
 *
 * @note The @ref NRF_SYSTICK_CSR_COUNTFLAG_MASK value is cleared when CSR register is read.
 * @return Values composed by @ref nrf_systick_csr_flags_t.
 */
__STATIC_INLINE uint32_t nrf_systick_csr_get(void);

/**
 * @brief Function for setting Configuration and Status Register.
 *
 * @param[in] val The value composed from @ref nrf_systick_csr_flags_t.
 */
__STATIC_INLINE void nrf_systick_csr_set(uint32_t val);

/**
 * @brief Function for getting the current reload value.
 *
 * @return The reload register value.
 */
__STATIC_INLINE uint32_t nrf_systick_load_get(void);

/**
 * @brief Function for configuring the reload value.
 *
 * @param[in] val The value to be set in the reload register.
 */
__STATIC_INLINE void nrf_systick_load_set(uint32_t val);

/**
 * @brief Function for reading the SysTick current value.
 *
 * @return The current SysTick value
 * @sa NRF_SYSTICK_VAL_MASK
 */
__STATIC_INLINE uint32_t nrf_systick_val_get(void);

/**
 * @brief Function for clearing the SysTick current value.
 *
 * @note The SysTick does not allow setting current value.
 *       Any write to VAL register would clear the timer.
 */
__STATIC_INLINE void nrf_systick_val_clear(void);

/**
 * @brief Function for reading the calibration register.
 *
 * @return The calibration register value.
 */
__STATIC_INLINE uint32_t nrf_systick_calib_get(void);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE uint32_t nrf_systick_csr_get(void)
{
    return SysTick->CTRL;
}

__STATIC_INLINE void nrf_systick_csr_set(uint32_t val)
{
    SysTick->CTRL = val;
}

__STATIC_INLINE uint32_t nrf_systick_load_get(void)
{
    return SysTick->LOAD;
}

__STATIC_INLINE void nrf_systick_load_set(uint32_t val)
{
    SysTick->LOAD = val;
}

__STATIC_INLINE uint32_t nrf_systick_val_get(void)
{
    return SysTick->VAL;
}

__STATIC_INLINE void nrf_systick_val_clear(void)
{
    SysTick->VAL = 0;
}

__STATIC_INLINE uint32_t nrf_systick_calib_get(void)
{
    return SysTick->CALIB;
}

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_SYSTICK_H__ */
