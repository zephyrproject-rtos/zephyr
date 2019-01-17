/* Copyright (c) 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef NRF_802154_UTILS_H__
#define NRF_802154_UTILS_H__

#include <assert.h>
#include <stdint.h>
#include "nrf.h"

/**
 * @defgroup nrf_802154_utils Utils definitions used in the 802.15.4 driver.
 * @{
 * @ingroup nrf_802154
 * @brief Definitions of utils used in the 802.15.4 driver.
 */

/**@brief RTC clock frequency. */
#define NRF_802154_RTC_FREQUENCY               32768UL

/**@brief Defines number of microseconds in one second. */
#define NRF_802154_US_PER_S                    1000000ULL

/**@brief Number of microseconds in one RTC tick. (rounded up) */
#define NRF_802154_US_PER_TICK                 NRF_802154_RTC_TICKS_TO_US(1)

/**@brief Number of bits to shift RTC_FREQUENCY and US_PER_S to achieve division by greatest common divisor. */
#define NRF_802154_FREQUENCY_US_PER_S_GCD_BITS 6

/**@brief Ceil division helper */
#define NRF_802154_DIVIDE_AND_CEIL(A, B)       (((A) + (B)-1) / (B))

/**@brief RTC ticks to us conversion. */
#define NRF_802154_RTC_TICKS_TO_US(ticks)                                          \
    NRF_802154_DIVIDE_AND_CEIL(                                                    \
        (ticks) * (NRF_802154_US_PER_S >> NRF_802154_FREQUENCY_US_PER_S_GCD_BITS), \
        (NRF_802154_RTC_FREQUENCY >> NRF_802154_FREQUENCY_US_PER_S_GCD_BITS))

static inline uint64_t NRF_802154_US_TO_RTC_TICKS(uint64_t time)
{
    uint64_t t1, u1;
    uint64_t result;

    /* The required range for time is [0..315360000000000], and the calculation below are
       verified to work within broader range [0...2^49 ~ 17 years]

       This first step in the calculation is to find out how many units
       of 15625 us there are in the input_us, because 512 RTC units
       corresponds _exactly_ to 15625 us. The calculation we want to do is therefore
       t1 = time / 15625, but division is slow and therefore we want to calculate
       t1 = time * k instead. The constant k is 1/15625 shifted up by as many bits
       as we can without causing overflow during the calculation.

       49 bits are needed to store the maximum value that time can have, and the
       lowest 13 bits in that value can be shifted away because a minimum of 14 bits
       are needed to store the divisor.

       This means that time can be reduced to 49 - 13 = 36 bits to make space
       for k.

       The most suitable number of shift for the value 1 / 15625 = 0.000064
       (binary 0.00000000000001000011000110111101111...) is 41, because that results
       in a 28 bits number that does not cause overflow in the multiplication.

       (2^41)/15625) is equal to 0x8637bd0, and is written in hexadecimal representation
       to show the bit width of the number. Shifting is limited to 41 bits because:
         1  The time uses up to 49 bits, and
         2) The time can only be shifted down 13 bits to avoid shifting away
            a full unit of 15625 microseconds, and
         3) The maximum value of the calculation would otherwise overflow (i.e.
            (315360000000000 >> 13) * 0x8637bd0 = 0x4b300bfcd0aefde0, would no longer be less than
            0Xffffffffffffffff).

       There is a possible loss of precision so that t1 will be up to 93*15625 _smaller_
       than the accurate number. This is taken into account in the next step.
     */

    t1     = ((time >> 13) * 0x8637bd0) >> 28; // ((time >> 13) * (2^41 / 15625)) >> (41 - 13)
    result = t1 * 512;
    t1     = time - t1 * 15625;

    /* This second step of the calculation is to find out how many RTC units there are
       still left in the remaining microseconds.

       (2^56)/15625) is equal to 0x431bde82d7b, and is written in hexadecimal representation
       to show the bit width of the number. Shifting 56 bits is determined by the worst
       case value of t1. The constant is selected by using the same methodology as in the
       first step of the calculation above.

       The possible loss of precision in the calculation above can make t1 93*15625 lower
       than it should have been here. The worst case found is that t1 can be 1453125, and
       therefore there is no overflow in the calculation
       1453125 * 0x431bde82d7b = 0x5cfffffffff76627 (i.e. it is less than 0xffffffffffffffff).

       15625 below is the binary representation of 30.51757813 (11110.100001001)
       scaled up by 2^9, and the calculation below are therefore using that scale.

       Rounding up to the nearest RTC tick is done by adding the value of the least
       significant bits of the fraction (i.e. adding the value of bits 1..47 of the scaled
       up timer unit size (2^47)) to the calculated value before scaling the final
       value down to RTC ticks.*/

    // ceil((time * (2^56 / 15625)) >> (56 - 9))
    assert(t1 <= 1453125);
    u1   = (t1 * 0x431bde82d7b); // (time * (2^56 / 15625))
    u1  += 0x7fffffffffff;       // round up
    u1 >>= 47;                   // ceil(u1 >> (56 - 9))

    result += u1;

    return result;
}

/**@brief Checks if the provided interrupt is currently enabled.
 *
 * @note This function is valid only for ARM Cortex-M4 core.
 *
 * @params IRQn  Interrupt number.
 *
 * @returns  Zero if interrupt is disabled, non-zero value otherwise.
 */
static inline uint32_t nrf_is_nvic_irq_enabled(IRQn_Type IRQn)
{
    return (NVIC->ISER[(((uint32_t)(int32_t)IRQn) >> 5UL)]) &
           ((uint32_t)(1UL << (((uint32_t)(int32_t)IRQn) & 0x1FUL)));
}

/**
 *@}
 **/

#endif // NRF_802154_UTILS_H__
