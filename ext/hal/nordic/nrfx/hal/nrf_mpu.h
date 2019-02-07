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

#ifndef NRF_MPU_H__
#define NRF_MPU_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_mpu_hal MPU HAL
 * @{
 * @ingroup nrf_mpu
 * @brief   Hardware access layer for managing the Memory Protection Unit (MPU) peripheral.
 */

/**
 * @brief Macro for getting MPU region configuration mask for the specified peripheral.
 *
 * @param[in] base_addr Peripheral base address.
 *
 * @return MPU configuration mask for the specified peripheral.
 */
#define NRF_MPU_PERIPHERAL_MASK_GET(base_addr) (1UL << NRFX_PERIPHERAL_ID_GET(base_addr))

/**
 * @brief Function for setting the size of the RAM region 0.
 *
 * When memory protection is enabled, the Memory Protection Unit enforces
 * runtime protection and readback protection of resources classified as region 0.
 * See the product specification for more information.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] size  Size of the RAM region 0, in bytes. Must be word-aligned.
 */
__STATIC_INLINE void nrf_mpu_region0_ram_size_set(NRF_MPU_Type * p_reg, uint32_t size);

/**
 * @brief Function for configuring specified peripherals in the memory region 0.
 *
 * When the memory protection is enabled, the Memory Protection Unit enforces
 * runtime protection and readback protection of resources classified as region 0.
 * See the product specification for more information.
 *
 * After reset, all peripherals are configured as *not* assigned to region 0.
 *
 * @param[in] p_reg           Pointer to the structure of registers of the peripheral.
 * @param[in] peripheral_mask Mask that specifies peripherals to be configured in the memory region 0.
 *                            Compose this mask using @ref NRF_MPU_PERIPHERAL_MASK_GET macro.
 */
__STATIC_INLINE void nrf_mpu_region0_peripherals_set(NRF_MPU_Type * p_reg,
                                                     uint32_t       peripheral_mask);

/**
 * @brief Function for getting the bitmask that specifies peripherals configured in the memory region 0.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Bitmask representing peripherals configured in region 0.
 */
__STATIC_INLINE uint32_t nrf_mpu_region0_peripherals_get(NRF_MPU_Type const * p_reg);

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
__STATIC_INLINE void nrf_mpu_nvm_blocks_protection_enable(NRF_MPU_Type * p_reg,
                                                          uint8_t        group_idx,
                                                          uint32_t       block_mask);

/**
 * @brief Function for setting the non-volatile memory (NVM) protection during debug.
 *
 * NVM protection during debug is disabled by default.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if NVM protection during debug is to be enabled.
 *                   False if otherwise.
 */
__STATIC_INLINE void nrf_mpu_nvm_protection_in_debug_set(NRF_MPU_Type * p_reg,
                                                         bool           enable);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_mpu_region0_ram_size_set(NRF_MPU_Type * p_reg, uint32_t size)
{
    NRFX_ASSERT(nrfx_is_word_aligned((const void *)size));
    p_reg->RLENR0 = size;
}

__STATIC_INLINE void nrf_mpu_region0_peripherals_set(NRF_MPU_Type * p_reg,
                                                     uint32_t       peripheral_mask)
{
    p_reg->PERR0 = peripheral_mask;
}

__STATIC_INLINE uint32_t nrf_mpu_region0_peripherals_get(NRF_MPU_Type const * p_reg)
{
    return p_reg->PERR0;
}

__STATIC_INLINE void nrf_mpu_nvm_blocks_protection_enable(NRF_MPU_Type * p_reg,
                                                          uint8_t        group_idx,
                                                          uint32_t       block_mask)
{
    switch (group_idx)
    {
        case 0:
            p_reg->PROTENSET0 = block_mask;
            break;

        case 1:
            p_reg->PROTENSET1 = block_mask;
            break;

        default:
            NRFX_ASSERT(false);
            break;
    }
}

__STATIC_INLINE void nrf_mpu_nvm_protection_in_debug_set(NRF_MPU_Type * p_reg,
                                                         bool           enable)
{
    p_reg->DISABLEINDEBUG =
        (enable ? 0 : MPU_DISABLEINDEBUG_DISABLEINDEBUG_Msk);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_MPU_H__
