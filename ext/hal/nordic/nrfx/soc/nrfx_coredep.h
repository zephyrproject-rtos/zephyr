/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
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

#ifndef NRFX_COREDEP_H__
#define NRFX_COREDEP_H__

/**
 * @defgroup nrfx_coredep Core-dependent functionality
 * @{
 * @ingroup nrfx
 * @brief Module containing functions with core-dependent implementation, like delay.
 */

#if defined(__NRFX_DOXYGEN__)

/** @brief Core frequency (in MHz). */
#define NRFX_DELAY_CPU_FREQ_MHZ
/** @brief Availability of DWT unit in the given SoC. */
#define NRFX_DELAY_DWT_PRESENT

#elif defined(NRF51)
    #define NRFX_DELAY_CPU_FREQ_MHZ 16
    #define NRFX_DELAY_DWT_PRESENT  0
#elif defined(NRF52810_XXAA)
    #define NRFX_DELAY_CPU_FREQ_MHZ 64
    #define NRFX_DELAY_DWT_PRESENT  0
#elif defined(NRF52832_XXAA) || defined (NRF52832_XXAB)
    #define NRFX_DELAY_CPU_FREQ_MHZ 64
    #define NRFX_DELAY_DWT_PRESENT  1
#elif defined(NRF52840_XXAA)
    #define NRFX_DELAY_CPU_FREQ_MHZ 64
    #define NRFX_DELAY_DWT_PRESENT  1
#else
    #error "Unknown device."
#endif

/**
 * @brief Function for delaying execution for a number of microseconds.
 *
 * The value of @p time_us is multiplied by the frequency in MHz. Therefore, the delay is limited to
 * maximum uint32_t capacity divided by frequency. For example:
 * - For SoCs working at 64MHz: 0xFFFFFFFF/64 = 0x03FFFFFF (67108863 microseconds)
 * - For SoCs working at 16MHz: 0xFFFFFFFF/16 = 0x0FFFFFFF (268435455 microseconds)
 *
 * @param time_us Number of microseconds to wait.
 */
__STATIC_INLINE void nrfx_coredep_delay_us(uint32_t time_us);

/** @} */

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

#if NRFX_CHECK(NRFX_DELAY_DWT_BASED)

#if !NRFX_DELAY_DWT_PRESENT
#error "DWT unit not present in the SoC that is used."
#endif

__STATIC_INLINE void nrfx_coredep_delay_us(uint32_t time_us)
{
    if (time_us == 0)
    {
        return;
    }
    uint32_t time_cycles = time_us * NRFX_DELAY_CPU_FREQ_MHZ;

    // Save the current state of the DEMCR register to be able to restore it before exiting
    // this function. Enable the trace and debug blocks (DWT is one of them).
    uint32_t core_debug = CoreDebug->DEMCR;
    CoreDebug->DEMCR = core_debug | CoreDebug_DEMCR_TRCENA_Msk;

    // Save the current state of the CTRL register in DWT block. Make sure
    // that cycle counter is enabled.
    uint32_t dwt_ctrl = DWT->CTRL;
    DWT->CTRL = dwt_ctrl | DWT_CTRL_CYCCNTENA_Msk;

    // Store start value of cycle counter.
    uint32_t cyccnt_initial = DWT->CYCCNT;

    // Delay required time.
    while ((DWT->CYCCNT - cyccnt_initial) < time_cycles)
    {}

    // Restore preserved registers.
    DWT->CTRL = dwt_ctrl;
    CoreDebug->DEMCR = core_debug;
}
#else // NRFX_CHECK(NRFX_DELAY_DWT_BASED)


__STATIC_INLINE void nrfx_coredep_delay_us(uint32_t time_us)
{
    if (time_us == 0)
    {
        return;
    }

    #if defined(NRF51)
    // The loop takes 4 cycles: 1 for SUBS and 3 for BHI.
    static const uint16_t delay_bytecode[] = {
        0x3804, // SUBS r0, #4
        0xd8fd, // BHI .-2
        0x4770  // BX LR
        };
    #elif defined(NRF52810_XXAA)
    // The loop takes 7 cycles: 1 for SUBS and 2 for BHI and 2 for flash wait states.
    static const uint16_t delay_bytecode[] = {
        0x3807, // SUBS r0, #7
        0xd8fd, // BHI .-2
        0x4770  // BX LR
        };
    #elif  defined(NRF52832_XXAA) || defined (NRF52832_XXAB) ||  defined(NRF52840_XXAA)
    // The loop takes 3 cycles: 1 for SUBS and 2 for BHI.
    // Make sure that code will be cached properly, so that no extra wait states appear.
    __ALIGN(16)
    static const uint16_t delay_bytecode[] = {
        0x3803, // SUBS r0, #3
        0xd8fd, // BHI .-2
        0x4770  // BX LR
        };
    #endif

    typedef void (* delay_func_t)(uint32_t);
    // Set LSB to 1 to execute code in Thumb mode.
    const delay_func_t delay_cycles = (delay_func_t)((((uint32_t)delay_bytecode) | 1));
    uint32_t cycles = time_us * NRFX_DELAY_CPU_FREQ_MHZ;
    delay_cycles(cycles);
}

#endif // !NRFX_CHECK(NRFX_DELAY_DWT_BASED_DELAY)

#endif // SUPPRESS_INLINE_IMPLEMENTATION

#endif // NRFX_COREDEP_H__
