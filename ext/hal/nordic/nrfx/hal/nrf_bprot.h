/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
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

#ifndef NRF_BPROT_H__
#define NRF_BPROT_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_bprot_hal BPROT HAL
 * @{
 * @ingroup nrf_bprot
 * @brief   Hardware access layer for managing the Block Protection (BPROT) mechanism.
 */

/**
 * @brief Function for enabling protection for specified non-volatile memory blocks.
 *
 * Blocks are arranged into groups of 32 blocks each. Each block size is 4 kB.
 * Any attempt to write or erase a protected block will result in hard fault.
 * The memory block protection can be disabled only by resetting the device.
 *
 * @param[in] p_reg      Pointer to the structure of registers of the peripheral.
 * @param[in] group_idx  Non-volatile memory group containing memory blocks to protect.
 * @param[in] block_mask Non-volatile memory blocks to protect. Each bit in bitmask represents
 *                       one memory block in the specified group.
 */
__STATIC_INLINE void nrf_bprot_nvm_blocks_protection_enable(NRF_BPROT_Type * p_reg,
                                                            uint8_t          group_idx,
                                                            uint32_t         block_mask);

/**
 * @brief Function for setting the non-volatile memory (NVM) protection during debug.
 *
 * NVM protection is disabled by default while debugging.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if NVM protection during debug is to be enabled.
 *                   False if otherwise.
 */
__STATIC_INLINE void nrf_bprot_nvm_protection_in_debug_set(NRF_BPROT_Type * p_reg,
                                                           bool             enable);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_bprot_nvm_blocks_protection_enable(NRF_BPROT_Type * p_reg,
                                                            uint8_t          group_idx,
                                                            uint32_t         block_mask)
{
    switch (group_idx)
    {
        case 0:
            p_reg->CONFIG0 = block_mask;
            break;

        case 1:
            p_reg->CONFIG1 = block_mask;
            break;

#if defined(BPROT_CONFIG2_REGION64_Pos)
        case 2:
            p_reg->CONFIG2 = block_mask;
            break;
#endif

#if defined(BPROT_CONFIG3_REGION96_Pos)
        case 3:
            p_reg->CONFIG3 = block_mask;
            break;
#endif

        default:
            NRFX_ASSERT(false);
            break;
    }
}

__STATIC_INLINE void nrf_bprot_nvm_protection_in_debug_set(NRF_BPROT_Type * p_reg,
                                                           bool             enable)
{
    p_reg->DISABLEINDEBUG =
        (enable ? 0 : BPROT_DISABLEINDEBUG_DISABLEINDEBUG_Msk);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_BPROT_H__
