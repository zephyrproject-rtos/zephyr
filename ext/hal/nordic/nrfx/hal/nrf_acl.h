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

#ifndef NRF_ACL_H__
#define NRF_ACL_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_ACL_REGION_SIZE_MAX (512 * 1024UL)

/**
 * @defgroup nrf_acl_hal ACL HAL
 * @{
 * @ingroup nrf_acl
 * @brief   Hardware access layer for managing the Access Control List (ACL) peripheral.
 */

/** @brief ACL permissions. */
typedef enum
{
    NRF_ACL_PERM_READ_NO_WRITE    = ACL_ACL_PERM_WRITE_Msk,                        /**< Read allowed, write disallowed. */
    NRF_ACL_PERM_NO_READ_WRITE    = ACL_ACL_PERM_READ_Msk,                         /**< Read disallowed, write allowed. */
    NRF_ACL_PERM_NO_READ_NO_WRITE = ACL_ACL_PERM_READ_Msk | ACL_ACL_PERM_WRITE_Msk /**< Read disallowed, write disallowed. */
} nrf_acl_perm_t;

/**
 * @brief Function for setting region parameters for given ACL region.
 *
 * Address must be word and page aligned. Size must be page aligned.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] region_id ACL region index.
 * @param[in] address   Start address.
 * @param[in] size      Size of region to protect in bytes.
 * @param[in] perm      Permissions to set for region to protect.
 */
__STATIC_INLINE void nrf_acl_region_set(NRF_ACL_Type * p_reg,
                                        uint32_t       region_id,
                                        uint32_t       address,
                                        size_t         size,
                                        nrf_acl_perm_t perm);

/**
 * @brief Function for getting the configured region address of a specific ACL region.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] region_id ACL region index.
 *
 * @return Configured region address of given ACL region.
 */
__STATIC_INLINE uint32_t nrf_acl_region_address_get(NRF_ACL_Type * p_reg, uint32_t region_id);

/**
 * @brief Function for getting the configured region size of a specific ACL region.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] region_id ACL region index.
 *
 * @return Configured region size of given ACL region.
 */
__STATIC_INLINE size_t nrf_acl_region_size_get(NRF_ACL_Type * p_reg, uint32_t region_id);

/**
 * @brief Function for getting the configured region permissions of a specific ACL region.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] region_id ACL region index.
 *
 * @return Configured region permissions of given ACL region.
 */
__STATIC_INLINE nrf_acl_perm_t nrf_acl_region_perm_get(NRF_ACL_Type * p_reg, uint32_t region_id);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_acl_region_set(NRF_ACL_Type * p_reg,
                                        uint32_t       region_id,
                                        uint32_t       address,
                                        size_t         size,
                                        nrf_acl_perm_t perm)
{
    NRFX_ASSERT(region_id < ACL_REGIONS_COUNT);
    NRFX_ASSERT(address % NRF_FICR->CODEPAGESIZE == 0);
    NRFX_ASSERT(size <= NRF_ACL_REGION_SIZE_MAX);
    NRFX_ASSERT(size != 0);

    p_reg->ACL[region_id].ADDR = address;
    p_reg->ACL[region_id].SIZE = size;
    p_reg->ACL[region_id].PERM = perm;
}

__STATIC_INLINE uint32_t nrf_acl_region_address_get(NRF_ACL_Type * p_reg, uint32_t region_id)
{
    return (uint32_t)p_reg->ACL[region_id].ADDR;
}

__STATIC_INLINE size_t nrf_acl_region_size_get(NRF_ACL_Type * p_reg, uint32_t region_id)
{
    return (size_t)p_reg->ACL[region_id].SIZE;
}

__STATIC_INLINE nrf_acl_perm_t nrf_acl_region_perm_get(NRF_ACL_Type * p_reg, uint32_t region_id)
{
    return (nrf_acl_perm_t)p_reg->ACL[region_id].PERM;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_ACL_H__
