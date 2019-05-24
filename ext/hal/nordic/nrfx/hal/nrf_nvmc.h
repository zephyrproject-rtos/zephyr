/*
 * Copyright (c) 2012 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_NVMC_H__
#define NRF_NVMC_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_nvmc_hal_deprecated NVMC HAL (deprecated)
 * @{
 * @ingroup nrf_nvmc
 * @brief   Hardware access layer (HAL) for managing the Non-Volatile Memory Controller (NVMC) peripheral.
 *
 * This driver allows writing to the non-volatile memory (NVM) regions
 * of the chip. To write to NVM, the controller must be powered
 * on and the relevant page must be erased.
 */

/**
 * @brief Erase a page in flash. This is required before writing to any address in the page.
 *
 * @param address Start address of the page.
 */
void nrf_nvmc_page_erase(uint32_t address);

/**
 * @brief Write a single byte to flash.
 *
 * The function reads the word containing the byte, and then rewrites the entire word.
 *
 * @param address Address to write to.
 * @param value   Value to write.
 */
void nrf_nvmc_write_byte(uint32_t address , uint8_t value);

/**
 * @brief Write a 32-bit word to flash.
 *
 * @param address Address to write to.
 * @param value   Value to write.
 */
void nrf_nvmc_write_word(uint32_t address, uint32_t value);

/**
 * @brief Write consecutive bytes to flash.
 *
 * @param address   Address to write to.
 * @param src       Pointer to data to copy from.
 * @param num_bytes Number of bytes in src to write.
 */
void nrf_nvmc_write_bytes(uint32_t  address, const uint8_t * src, uint32_t num_bytes);

/**
 * @brief Write consecutive words to flash.
 *
 * @param address   Address to write to.
 * @param src       Pointer to data to copy from.
 * @param num_words Number of words in src to write.
 */
void nrf_nvmc_write_words(uint32_t address, const uint32_t * src, uint32_t num_words);

/** @} */

/**
 * @defgroup nrf_nvmc_hal NVMC HAL
 * @{
 * @ingroup nrf_nvmc
 * @brief   Hardware access layer (HAL) for managing the Non-Volatile Memory Controller (NVMC) peripheral.
 */

#if defined(NVMC_ICACHECNF_CACHEEN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether Instruction Cache (ICache) is present. */
#define NRF_NVMC_ICACHE_PRESENT
#endif

#if defined(NVMC_ERASEPAGEPARTIALCFG_DURATION_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether the option of page partial erase is present. */
#define NRF_NVMC_PARTIAL_ERASE_PRESENT
#endif

/** @brief NVMC modes. */
typedef enum
{
    NRF_NVMC_MODE_READONLY      = NVMC_CONFIG_WEN_Ren, ///< NVMC in read-only mode.
    NRF_NVMC_MODE_WRITE         = NVMC_CONFIG_WEN_Wen, ///< NVMC in read and write mode.
    NRF_NVMC_MODE_ERASE         = NVMC_CONFIG_WEN_Een, ///< NVMC in read and erase mode.
#if defined(NVMC_CONFIG_WEN_PEen)
    NRF_NVMC_MODE_PARTIAL_ERASE = NVMC_CONFIG_WEN_PEen ///< NVMC in read and partial erase mode.
#endif
} nrf_nvmc_mode_t;

#if defined(NVMC_CONFIGNS_WEN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Non-secure NVMC modes. */
typedef enum
{
    NRF_NVMC_NS_MODE_READONLY = NVMC_CONFIGNS_WEN_Ren, ///< Non-secure NVMC in read-only mode.
    NRF_NVMC_NS_MODE_WRITE    = NVMC_CONFIGNS_WEN_Wen, ///< Non-secure NVMC in read and write mode.
    NRF_NVMC_NS_MODE_ERASE    = NVMC_CONFIGNS_WEN_Een, ///< Non-secure NVMC in read and erase mode.
} nrf_nvmc_ns_mode_t;
#endif

#if defined(NRF_NVMC_ICACHE_PRESENT)
/** @brief NVMC ICache configuration. */
typedef enum
{
    NRF_NVMC_ICACHE_DISABLE               = NVMC_ICACHECNF_CACHEEN_Disabled, ///< Instruction Cache disabled.
    NRF_NVMC_ICACHE_ENABLE                = NVMC_ICACHECNF_CACHEEN_Enabled,  ///< Instruction Cache enabled.
    NRF_NVMC_ICACHE_ENABLE_WITH_PROFILING = NVMC_ICACHECNF_CACHEEN_Enabled | ///< Instruction Cache with cache profiling enabled.
                                            NVMC_ICACHECNF_CACHEPROFEN_Msk
} nrf_nvmc_icache_config_t;
#endif // defined(NRF_NVMC_ICACHE_PRESENT)

/**
 * @brief Function for checking if NVMC is ready to perform write or erase operation.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  NVMC can perform write or erase.
 * @retval false NVMC is busy and cannot perform next operation yet.
 */
__STATIC_INLINE bool nrf_nvmc_ready_check(NRF_NVMC_Type const * p_reg);

#if defined(NVMC_READYNEXT_READYNEXT_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for checking if NVMC is ready to accept the next write operation.
 *
 * NVM writing time can be reduced by using this function.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  NVMC can accept the next write. It will be buffered and will be taken
 *               into account as soon as the ongoing write operation is completed.
 * @retval false NVMC is busy and cannot accept the next write yet.
 */
__STATIC_INLINE bool nrf_nvmc_write_ready_check(NRF_NVMC_Type const * p_reg);
#endif // defined(NVMC_READYNEXT_READYNEXT_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for setting the NVMC mode.
 *
 * Only activate erase and write modes when they are actively used.
 * If Instruction Cache (ICache) is present, enabling write or erase will
 * invalidate the cache and keep it invalidated.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mode  Desired operating mode for NVMC.
 */
__STATIC_INLINE void nrf_nvmc_mode_set(NRF_NVMC_Type * p_reg,
                                       nrf_nvmc_mode_t mode);

#if defined(NVMC_CONFIGNS_WEN_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the NVMC mode for non-secure Flash page operations.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mode  Desired operating mode for NVMC.
 */
__STATIC_INLINE void nrf_nvmc_nonsecure_mode_set(NRF_NVMC_Type *    p_reg,
                                                 nrf_nvmc_ns_mode_t mode);
#endif

/**
 * @brief Function for starting a single page erase in the Flash memory.
 *
 * The NVMC mode must be correctly configured with @ref nrf_nvmc_mode_set
 * before starting the erase operation.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] page_addr Address of the first word of the page to erase.
 */
__STATIC_INLINE void nrf_nvmc_page_erase_start(NRF_NVMC_Type * p_reg,
                                               uint32_t        page_addr);

#if defined(NVMC_ERASEUICR_ERASEUICR_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for starting the user information configuration registers (UICR) erase.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
__STATIC_INLINE void nrf_nvmc_uicr_erase_start(NRF_NVMC_Type * p_reg);
#endif

/**
 * @brief Function for starting the erase of the whole NVM, including UICR.
 *
 * This function purges all user code.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
__STATIC_INLINE void nrf_nvmc_erase_all_start(NRF_NVMC_Type * p_reg);

#if defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)
/**
 * @brief Function for configuring the page partial erase duration in milliseconds.
 *
 * @param[in] p_reg       Pointer to the structure of registers of the peripheral.
 * @param[in] duration_ms Page partial erase duration in milliseconds.
 */
__STATIC_INLINE void nrf_nvmc_partial_erase_duration_set(NRF_NVMC_Type * p_reg,
                                                         uint32_t        duration_ms);

/**
 * @brief Function for getting the current setting for the page partial erase duration.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval Interval duration setting in milliseconds.
 */
__STATIC_INLINE uint32_t nrf_nvmc_partial_erase_duration_get(NRF_NVMC_Type const * p_reg);

/**
 * @brief Function for starting a partial erase operation.
 *
 * It must be called successively until the page erase time is reached.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] page_addr Address of the first word of the page to erase.
 */
__STATIC_INLINE void nrf_nvmc_page_partial_erase_start(NRF_NVMC_Type * p_reg,
                                                       uint32_t        page_addr);
#endif // defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)

#if defined(NRF_NVMC_ICACHE_PRESENT)
/**
 * @brief Function for applying the Instruction Cache (ICache) configuration.
 *
 * Enabling the cache can increase CPU performance and reduce power
 * consumption by reducing the number of wait cycles and the number
 * of flash accesses.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] config ICache configuration.
 */
__STATIC_INLINE void nrf_nvmc_icache_config_set(NRF_NVMC_Type *          p_reg,
                                                nrf_nvmc_icache_config_t config);

/**
 * @brief Function for checking if ICache is enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  ICache enabled.
 * @retval false ICache disabled.
 */
__STATIC_INLINE bool nrf_nvmc_icache_enable_check(NRF_NVMC_Type const * p_reg);

/**
 * @brief Function for checking if the ICache profiling option is enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  ICache profiling enabled.
 * @retval false ICache profiling disabled.
 */
__STATIC_INLINE bool nrf_nvmc_icache_profiling_enable_check(NRF_NVMC_Type const * p_reg);

/**
 * @brief Function for getting the number of ICache hits.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval Number of the ICache hits.
 */
__STATIC_INLINE uint32_t nrf_nvmc_icache_hit_get(NRF_NVMC_Type const * p_reg);

/**
 * @brief Function for getting the number of ICache misses.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval Number of the ICache misses.
 */
__STATIC_INLINE uint32_t nrf_nvmc_icache_miss_get(NRF_NVMC_Type const * p_reg);

/**
 * @brief Function for resetting the ICache hit and miss counters.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
 __STATIC_INLINE void nrf_nvmc_icache_hit_miss_reset(NRF_NVMC_Type * p_reg);
#endif // defined(NRF_NVMC_ICACHE_PRESENT)

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE bool nrf_nvmc_ready_check(NRF_NVMC_Type const * p_reg)
{
    return (bool)(p_reg->READY & NVMC_READY_READY_Msk);
}

#if defined(NVMC_READYNEXT_READYNEXT_Msk)
__STATIC_INLINE bool nrf_nvmc_write_ready_check(NRF_NVMC_Type const * p_reg)
{
    return (bool)(p_reg->READYNEXT & NVMC_READYNEXT_READYNEXT_Msk);
}
#endif

__STATIC_INLINE void nrf_nvmc_mode_set(NRF_NVMC_Type * p_reg,
                                       nrf_nvmc_mode_t mode)
{
    p_reg->CONFIG = (uint32_t)mode;
}

#if defined(NVMC_CONFIGNS_WEN_Msk)
__STATIC_INLINE void nrf_nvmc_nonsecure_mode_set(NRF_NVMC_Type *    p_reg,
                                                 nrf_nvmc_ns_mode_t mode)
{
    p_reg->CONFIGNS = (uint32_t)mode;
}
#endif

__STATIC_INLINE void nrf_nvmc_page_erase_start(NRF_NVMC_Type * p_reg,
                                               uint32_t        page_addr)
{
#if defined(NRF51)
    /* On nRF51, the code area can be divided into two regions: CR0 and CR1.
     * The length of CR0 is specified in the CLENR0 register of UICR.
     * If CLENR0 contains the 0xFFFFFFFF value,  CR0 is not set.
     * Moreover, the page from CR0 can be written or erased only from code
     * running in CR0.*/
    uint32_t cr0_len = NRF_UICR->CLENR0 == 0xFFFFFFFF ? 0 : NRF_UICR->CLENR0;
    if (page_addr < cr0_len)
    {
        p_reg->ERASEPCR0 = page_addr;
    }
    else
    {
        p_reg->ERASEPCR1 = page_addr;
    }
#elif defined(NRF52_SERIES)
    p_reg->ERASEPAGE = page_addr;
#elif defined(NRF9160_XXAA)
    *(volatile uint32_t *)page_addr = 0xFFFFFFFF;
    (void)p_reg;
#else
    #error "Unknown device."
#endif
}

#if defined(NVMC_ERASEUICR_ERASEUICR_Msk)
__STATIC_INLINE void nrf_nvmc_uicr_erase_start(NRF_NVMC_Type * p_reg)
{
    p_reg->ERASEUICR = 1;
}
#endif

__STATIC_INLINE void nrf_nvmc_erase_all_start(NRF_NVMC_Type * p_reg)
{
    p_reg->ERASEALL = 1;
}

#if defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)
__STATIC_INLINE void nrf_nvmc_partial_erase_duration_set(NRF_NVMC_Type * p_reg,
                                                         uint32_t        duration_ms)
{
    p_reg->ERASEPAGEPARTIALCFG = duration_ms;
}

__STATIC_INLINE uint32_t nrf_nvmc_partial_erase_duration_get(NRF_NVMC_Type const * p_reg)
{
    return p_reg->ERASEPAGEPARTIALCFG;
}

__STATIC_INLINE void nrf_nvmc_page_partial_erase_start(NRF_NVMC_Type * p_reg,
                                                       uint32_t        page_addr)
{
#if defined(NVMC_ERASEPAGEPARTIAL_ERASEPAGEPARTIAL_Msk)
    p_reg->ERASEPAGEPARTIAL = page_addr;
#elif defined(NRF9160_XXAA)
    nrf_nvmc_page_erase_start(p_reg, page_addr);
#else
    #error "Unknown device."
#endif
}
#endif // defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)

#if defined(NRF_NVMC_ICACHE_PRESENT)
__STATIC_INLINE void nrf_nvmc_icache_config_set(NRF_NVMC_Type *          p_reg,
                                                nrf_nvmc_icache_config_t config)
{
    p_reg->ICACHECNF = (uint32_t)config;
}

__STATIC_INLINE bool nrf_nvmc_icache_enable_check(NRF_NVMC_Type const * p_reg)
{
    return (bool)(p_reg->ICACHECNF & NVMC_ICACHECNF_CACHEEN_Msk);
}

__STATIC_INLINE bool nrf_nvmc_icache_profiling_enable_check(NRF_NVMC_Type const * p_reg)
{
    return (bool)(p_reg->ICACHECNF & NVMC_ICACHECNF_CACHEPROFEN_Msk);
}

__STATIC_INLINE uint32_t nrf_nvmc_icache_hit_get(NRF_NVMC_Type const * p_reg)
{
    return p_reg->IHIT;
}

__STATIC_INLINE uint32_t nrf_nvmc_icache_miss_get(NRF_NVMC_Type const * p_reg)
{
    return p_reg->IMISS;
}

__STATIC_INLINE void nrf_nvmc_icache_hit_miss_reset(NRF_NVMC_Type * p_reg)
{
    p_reg->IHIT = 0;
    p_reg->IMISS = 0;
}
#endif // defined(NRF_NVMC_ICACHE_PRESENT)

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_NVMC_H__
