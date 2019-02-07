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

#ifndef NRF_BITMASK_H
#define NRF_BITMASK_H

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_bitmask Bitmask module
 * @{
 * @ingroup nrfx
 * @brief   Bitmask managing module.
 */

#define BITMASK_BYTE_GET(abs_bit) ((abs_bit)/8)
#define BITMASK_RELBIT_GET(abs_bit) ((abs_bit) & 0x00000007)

/**
 * Function for checking if bit in the multi-byte bit mask is set.
 *
 * @param bit    Bit index.
 * @param p_mask A pointer to mask with bit fields.
 *
 * @return 0 if bit is not set, positive value otherwise.
 */
__STATIC_INLINE uint32_t nrf_bitmask_bit_is_set(uint32_t bit, void const * p_mask)
{
    uint8_t const * p_mask8 = (uint8_t const *)p_mask;
    uint32_t byte_idx = BITMASK_BYTE_GET(bit);
    bit = BITMASK_RELBIT_GET(bit);
    return (1 << bit) & p_mask8[byte_idx];
}

/**
 * Function for setting a bit in the multi-byte bit mask.
 *
 * @param bit    Bit index.
 * @param p_mask A pointer to mask with bit fields.
 */
__STATIC_INLINE void nrf_bitmask_bit_set(uint32_t bit, void * p_mask)
{
    uint8_t * p_mask8 = (uint8_t *)p_mask;
    uint32_t byte_idx = BITMASK_BYTE_GET(bit);
    bit = BITMASK_RELBIT_GET(bit);
    p_mask8[byte_idx] |= (1 << bit);
}

/**
 * Function for clearing a bit in the multi-byte bit mask.
 *
 * @param bit    Bit index.
 * @param p_mask A pointer to mask with bit fields.
 */
__STATIC_INLINE void nrf_bitmask_bit_clear(uint32_t bit, void * p_mask)
{
    uint8_t * p_mask8 = (uint8_t *)p_mask;
    uint32_t byte_idx = BITMASK_BYTE_GET(bit);
    bit = BITMASK_RELBIT_GET(bit);
    p_mask8[byte_idx] &= ~(1 << bit);
}

/**
 * Function for performing bitwise OR operation on two multi-byte bit masks.
 *
 * @param p_mask1    A pointer to the first bit mask.
 * @param p_mask2    A pointer to the second bit mask.
 * @param p_out_mask A pointer to the output bit mask.
 * @param length     Length of output mask in bytes.
 */
__STATIC_INLINE void nrf_bitmask_masks_or(void const *   p_mask1,
                                          void const *   p_mask2,
                                          void *         p_out_mask,
                                          uint32_t       length)
{
    uint8_t const * p_mask8_1 = (uint8_t const *)p_mask1;
    uint8_t const * p_mask8_2 = (uint8_t const *)p_mask2;
    uint8_t * p_mask8_out = (uint8_t *)p_out_mask;
    uint32_t i;
    for (i = 0; i < length; i++)
    {
        p_mask8_out[i] = p_mask8_1[i] | p_mask8_2[i];
    }
}

/**
 * Function for performing bitwise AND operation on two multi-byte bit masks.
 *
 * @param p_mask1    A pointer to the first bit mask.
 * @param p_mask2    A pointer to the second bit mask.
 * @param p_out_mask A pointer to the output bit mask.
 * @param length     Length of output mask in bytes.
 */
__STATIC_INLINE void nrf_bitmask_masks_and(void const *   p_mask1,
                                           void const *   p_mask2,
                                           void *         p_out_mask,
                                           uint32_t       length)
{
    uint8_t const * p_mask8_1 = (uint8_t const *)p_mask1;
    uint8_t const * p_mask8_2 = (uint8_t const *)p_mask2;
    uint8_t * p_mask8_out = (uint8_t *)p_out_mask;
    uint32_t i;
    for (i = 0; i < length; i++)
    {
        p_mask8_out[i] = p_mask8_1[i] & p_mask8_2[i];
    }
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_BITMASK_H
