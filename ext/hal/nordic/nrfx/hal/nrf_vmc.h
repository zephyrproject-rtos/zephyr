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

#ifndef NRF_VMC_H__
#define NRF_VMC_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_vmc_hal VMC HAL
 * @{
 * @ingroup nrf_vmc
 * @brief   Hardware access layer for managing the Volatile Memory Controller (VMC) peripheral.
 */

/** @brief Power configuration bits for each section in particular RAM block. */
typedef enum
{
    NRF_VMC_POWER_S0  = VMC_RAM_POWER_S0POWER_Msk, ///< Keep retention on RAM section S0 of the particular RAM block when RAM section is switched off.
    NRF_VMC_POWER_S1  = VMC_RAM_POWER_S1POWER_Msk, ///< Keep retention on RAM section S1 of the particular RAM block when RAM section is switched off.
    NRF_VMC_POWER_S2  = VMC_RAM_POWER_S2POWER_Msk, ///< Keep retention on RAM section S2 of the particular RAM block when RAM section is switched off.
    NRF_VMC_POWER_S3  = VMC_RAM_POWER_S3POWER_Msk, ///< Keep retention on RAM section S3 of the particular RAM block when RAM section is switched off.
} nrf_vmc_power_t;

/** @brief Retention configuration bits for each section in particular RAM block. */
typedef enum
{
    NRF_VMC_RETENTION_S0  = VMC_RAM_POWER_S0RETENTION_Msk, ///< Keep RAM section S0 of the particular RAM block on or off in System ON mode.
    NRF_VMC_RETENTION_S1  = VMC_RAM_POWER_S1RETENTION_Msk, ///< Keep RAM section S1 of the particular RAM block on or off in System ON mode.
    NRF_VMC_RETENTION_S2  = VMC_RAM_POWER_S2RETENTION_Msk, ///< Keep RAM section S2 of the particular RAM block on or off in System ON mode.
    NRF_VMC_RETENTION_S3  = VMC_RAM_POWER_S3RETENTION_Msk, ///< Keep RAM section S3 of the particular RAM block on or off in System ON mode.
} nrf_vmc_retention_t;

/**
 * @brief Function for setting power configuration for the particular RAM block.
 *
 * @note Overrides current configuration.
 *
 * @param[in] p_reg          Pointer to the structure of registers of the peripheral.
 * @param[in] ram_block_num  RAM block number.
 * @param[in] power_mask     Bitmask with sections configuration of particular RAM block.
 *                           @ref nrf_vmc_power_t should be use to prepare this bitmask.
 * @param[in] retention_mask Bitmask with sections configuration of particular RAM block.
 *                           @ref nrf_vmc_retention_t should be use to prepare this bitmask.
 */
__STATIC_INLINE void nrf_vmc_ram_block_config(NRF_VMC_Type * p_reg,
                                              uint8_t        ram_block_num,
                                              uint32_t       power_mask,
                                              uint32_t       retention_mask);

/**
 * @brief Function for clearing power configuration for the particular RAM block.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] ram_block_num RAM block number.
 */
__STATIC_INLINE void nrf_vmc_ram_block_clear(NRF_VMC_Type * p_reg, uint8_t ram_block_num);

/**
 * @brief Function for setting power configuration for the particular RAM block.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] ram_block_num RAM block number.
 * @param[in] sect_power    Paricular section of the RAM block.
 */
__STATIC_INLINE void nrf_vmc_ram_block_power_set(NRF_VMC_Type *  p_reg,
                                                 uint8_t         ram_block_num,
                                                 nrf_vmc_power_t sect_power);

/**
 * @brief Function for clearing power configuration for the particular RAM block.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] ram_block_num RAM block number.
 * @param[in] sect_power    Paricular section of the RAM block.
 */
__STATIC_INLINE void nrf_vmc_ram_block_power_clear(NRF_VMC_Type *  p_reg,
                                                   uint8_t         ram_block_num,
                                                   nrf_vmc_power_t sect_power);

/**
 * @brief Function for getting power configuration of the particular RAM block.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] ram_block_num RAM block number.
 *
 * @return Bitmask with power configuration of sections of particular RAM block.
 */
__STATIC_INLINE uint32_t nrf_vmc_ram_block_power_mask_get(NRF_VMC_Type const * p_reg,
                                                          uint8_t              ram_block_num);

/**
 * @brief Function for setting retention configuration for the particular RAM block.
 *
 * @param[in] p_reg          Pointer to the structure of registers of the peripheral.
 * @param[in] ram_block_num  RAM block number.
 * @param[in] sect_retention Paricular section of the RAM block.
 */
__STATIC_INLINE void nrf_vmc_ram_block_retention_set(NRF_VMC_Type *      p_reg,
                                                     uint8_t             ram_block_num,
                                                     nrf_vmc_retention_t sect_retention);

/**
 * @brief Function for clearing retention configuration for the particular RAM block.
 *
 * @param[in] p_reg          Pointer to the structure of registers of the peripheral.
 * @param[in] ram_block_num  RAM block number.
 * @param[in] sect_retention Paricular section of the RAM block.
 */
__STATIC_INLINE void nrf_vmc_ram_block_retention_clear(NRF_VMC_Type *      p_reg,
                                                       uint8_t             ram_block_num,
                                                       nrf_vmc_retention_t sect_retention);

/**
 * @brief Function for getting retention configuration of the particular RAM block.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] ram_block_num RAM block number.
 *
 * @return Bitmask with retention configuration of sections of particular RAM block
 */
__STATIC_INLINE uint32_t nrf_vmc_ram_block_retention_mask_get(NRF_VMC_Type const * p_reg,
                                                              uint8_t              ram_block_num);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_vmc_ram_block_config(NRF_VMC_Type * p_reg,
                                              uint8_t        ram_block_num,
                                              uint32_t       power_mask,
                                              uint32_t       retention_mask)
{
    p_reg->RAM[ram_block_num].POWER =
            (power_mask & (
                VMC_RAM_POWER_S0POWER_Msk |
                VMC_RAM_POWER_S1POWER_Msk |
                VMC_RAM_POWER_S2POWER_Msk |
                VMC_RAM_POWER_S3POWER_Msk)) |
            (retention_mask & (
                VMC_RAM_POWER_S0RETENTION_Msk |
                VMC_RAM_POWER_S1RETENTION_Msk |
                VMC_RAM_POWER_S2RETENTION_Msk |
                VMC_RAM_POWER_S3RETENTION_Msk));
    // Perform dummy read of the POWER register to ensure that configuration of sections was
    // written to the VMC peripheral.
    volatile uint32_t dummy = p_reg->RAM[ram_block_num].POWER;
    (void)dummy;
}

__STATIC_INLINE void nrf_vmc_ram_block_clear(NRF_VMC_Type * p_reg, uint8_t ram_block_num)
{
    p_reg->RAM[ram_block_num].POWER = 0;
}

__STATIC_INLINE void nrf_vmc_ram_block_power_set(NRF_VMC_Type *  p_reg,
                                                 uint8_t         ram_block_num,
                                                 nrf_vmc_power_t sect_power)
{
    p_reg->RAM[ram_block_num].POWERSET = (uint32_t)sect_power;
    // Perform dummy read of the POWERSET register to ensure that configuration of sections was
    // written to the VMC peripheral.
    volatile uint32_t dummy = p_reg->RAM[ram_block_num].POWERSET;
    (void)dummy;
}

__STATIC_INLINE void nrf_vmc_ram_block_power_clear(NRF_VMC_Type *  p_reg,
                                                   uint8_t         ram_block_num,
                                                   nrf_vmc_power_t sect_power)
{
    p_reg->RAM[ram_block_num].POWERCLR = (uint32_t)sect_power;
}

__STATIC_INLINE uint32_t nrf_vmc_ram_block_power_mask_get(NRF_VMC_Type const * p_reg,
                                                          uint8_t              ram_block_num)
{
    return p_reg->RAM[ram_block_num].POWER & (
                VMC_RAM_POWER_S0POWER_Msk |
                VMC_RAM_POWER_S1POWER_Msk |
                VMC_RAM_POWER_S2POWER_Msk |
                VMC_RAM_POWER_S3POWER_Msk);
}

__STATIC_INLINE void nrf_vmc_ram_block_retention_set(NRF_VMC_Type *      p_reg,
                                                     uint8_t             ram_block_num,
                                                     nrf_vmc_retention_t sect_retention)
{
    p_reg->RAM[ram_block_num].POWERSET = (uint32_t)sect_retention;
    // Perform dummy read of the POWERSET register to ensure that configuration of sections was
    // written to the VMC peripheral.
    volatile uint32_t dummy = p_reg->RAM[ram_block_num].POWERSET;
    (void)dummy;
}

__STATIC_INLINE void nrf_vmc_ram_block_retention_clear(NRF_VMC_Type *      p_reg,
                                                       uint8_t             ram_block_num,
                                                       nrf_vmc_retention_t sect_retention)
{
    p_reg->RAM[ram_block_num].POWERCLR = (uint32_t)sect_retention;
}

__STATIC_INLINE uint32_t nrf_vmc_ram_block_retention_mask_get(NRF_VMC_Type const * p_reg,
                                                              uint8_t              ram_block_num)
{
    return p_reg->RAM[ram_block_num].POWER & (
                VMC_RAM_POWER_S0RETENTION_Msk |
                VMC_RAM_POWER_S1RETENTION_Msk |
                VMC_RAM_POWER_S2RETENTION_Msk |
                VMC_RAM_POWER_S3RETENTION_Msk);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_VMC_H__
