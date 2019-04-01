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

#ifndef NRFX_NVMC_H__
#define NRFX_NVMC_H__

#include <nrfx.h>
#include <hal/nrf_nvmc.h>
#include <hal/nrf_ficr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_nvmc NVMC driver
 * @{
 * @ingroup nrf_nvmc
 * @brief   Non-Volatile Memory Controller (NVMC) peripheral driver.
 */

/**
 * @brief Function for erasing a page in flash.
 *
 * This function blocks until the erase operation finishes.
 *
 * @note Depending on the source of the code being executed,
 *       the CPU may be halted during the operation.
 *       Refer to the Product Specification for more information.
 *
 * @param address Address of the first word in the page to erase.
 *
 * @retval NRFX_SUCCESS            Page erase complete.
 * @retval NRFX_ERROR_INVALID_ADDR Address is not aligned to the size of the page.
 */
nrfx_err_t nrfx_nvmc_page_erase(uint32_t address);

/**
 * @brief Function for erasing the user information configuration register (UICR).
 *
 * @note Depending on the source of the code being executed,
 *       the CPU may be halted during the operation.
 *       Refer to the Product Specification for more information.
 *
 * @retval NRFX_SUCCESS             UICR has been successfully erased.
 * @retval NRFX_ERROR_NOT_SUPPORTED UICR erase is not supported.
 */
nrfx_err_t nrfx_nvmc_uicr_erase(void);

/**
 * @brief Function for erasing the whole flash memory.
 *
 * @note All user code and UICR will be erased.
 */
void nrfx_nvmc_all_erase(void);

#if defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)
/**
 * @brief Function for initiating a complete page erase split into parts (also known as partial erase).
 *
 * This function initiates a partial erase with the specified duration.
 * To execute each part of the partial erase, use @ref nrfx_nvmc_page_partial_erase_continue.
 *
 * @param address     Address of the first word in the page to erase.
 * @param duration_ms Time in milliseconds that each partial erase will take.
 *
 * @retval NRFX_SUCCESS            Partial erase started.
 * @retval NRFX_ERROR_INVALID_ADDR Address is not aligned to the size of the page.
 *
 * @sa nrfx_nvmc_page_partial_erase_continue()
 */
nrfx_err_t nrfx_nvmc_page_partial_erase_init(uint32_t address, uint32_t duration_ms);

/**
 * @brief Function for performing a part of the complete page erase (also known as partial erase).
 *
 * Each part takes the amount of time specified during the initialization.
 * This function must be called several times to erase the whole page, once for each erase part.
 *
 * @note Using a page that was not completely erased leads to undefined behavior.
 *       Depending on the source of the code being executed,
 *       the CPU may be halted during the operation.
 *       Refer to the Product Specification for more information.
 *
 * @retval true  Partial erase finished.
 * @retval false Partial erase not finished.
 *               Call the function again to process the next part.
 */
bool nrfx_nvmc_page_partial_erase_continue(void);

#endif // defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)

/**
 * @brief Function for checking whether a byte is writable at the specified address.
 *
 * The NVMC is only able to write '0' to bits in the flash that are erased (set to '1').
 * It cannot rewrite a bit back to '1'. This function checks if the value currently
 * residing at the specified address can be transformed to the desired value
 * without any '0' to '1' transitions.
 *
 * @param address Address to be checked.
 * @param value   Value to be checked.
 *
 * @retval true  Byte can be written at the specified address.
 * @retval false Byte cannot be written at the specified address.
 *               Erase the page or change the address.
 */
bool nrfx_nvmc_byte_writable_check(uint32_t address, uint8_t value);

/**
 * @brief Function for writing a single byte to flash.
 *
 * To determine if the flash write has been completed, use @ref nrfx_nvmc_write_done_check().
 *
 * @note Depending on the source of the code being executed,
 *       the CPU may be halted during the operation.
 *       Refer to the Product Specification for more information.
 *
 * @param address Address to write to.
 * @param value   Value to write.
 */
void nrfx_nvmc_byte_write(uint32_t address, uint8_t value);

/**
 * @brief Function for checking whether a word is writable at the specified address.
 *
 * The NVMC is only able to write '0' to bits in the Flash that are erased (set to '1').
 * It cannot rewrite a bit back to '1'. This function checks if the value currently
 * residing at the specified address can be transformed to the desired value
 * without any '0' to '1' transitions.
 *
 * @param address Address to be checked. Must be word-aligned.
 * @param value   Value to be checked.
 *
 * @retval true  Word can be written at the specified address.
 * @retval false Word cannot be written at the specified address.
 *               Erase page or change address.
 */
bool nrfx_nvmc_word_writable_check(uint32_t address, uint32_t value);

/**
 * @brief Function for writing a 32-bit word to flash.
 *
 * To determine if the flash write has been completed, use @ref nrfx_nvmc_write_done_check().
 *
 * @note Depending on the source of the code being executed,
 *       the CPU may be halted during the operation.
 *       Refer to the Product Specification for more information.
 *
 * @param address Address to write to. Must be word-aligned.
 * @param value   Value to write.
 */
void nrfx_nvmc_word_write(uint32_t address, uint32_t value);

/**
 * @brief Function for writing consecutive bytes to flash.
 *
 * To determine if the last flash write has been completed, use @ref nrfx_nvmc_write_done_check().
 *
 * @note Depending on the source of the code being executed,
 *       the CPU may be halted during the operation.
 *       Refer to the Product Specification for more information.
 *
 * @param address   Address to write to.
 * @param src       Pointer to the data to copy from.
 * @param num_bytes Number of bytes to write.
 */
void nrfx_nvmc_bytes_write(uint32_t address, void const * src, uint32_t num_bytes);

/**
 * @brief Function for writing consecutive words to flash.
 *
 * To determine if the last flash write has been completed, use @ref nrfx_nvmc_write_done_check().
 *
 * @note Depending on the source of the code being executed,
 *       the CPU may be halted during the operation.
 *       Refer to the Product Specification for more information.
 *
 * @param address   Address to write to. Must be word-aligned.
 * @param src       Pointer to data to copy from. Must be word-aligned.
 * @param num_words Number of words to write.
 */
void nrfx_nvmc_words_write(uint32_t address, void const * src, uint32_t num_words);

/**
 * @brief Function for getting the total flash size in bytes.
 *
 * @return Flash total size in bytes.
 */
uint32_t nrfx_nvmc_flash_size_get(void);

/**
 * @brief Function for getting the flash page size in bytes.
 *
 * @return Flash page size in bytes.
 */
uint32_t nrfx_nvmc_flash_page_size_get(void);

/**
 * @brief Function for getting the flash page count.
 *
 * @return Flash page count.
 */
uint32_t nrfx_nvmc_flash_page_count_get(void);

/**
 * @brief Function for checking if the last flash write has been completed.
 *
 * @retval true  Last write completed successfully.
 * @retval false Last write is still in progress.
 */
__STATIC_INLINE bool nrfx_nvmc_write_done_check(void);

#if defined(NRF_NVMC_ICACHE_PRESENT)
/**
 * @brief Function for enabling the Instruction Cache (ICache).
 *
 * Enabling ICache reduces the amount of accesses to flash memory,
 * which can boost performance and lower power consumption.
 */
__STATIC_INLINE void nrfx_nvmc_icache_enable(void);

/** @brief Function for disabling ICache. */
__STATIC_INLINE void nrfx_nvmc_icache_disable(void);

#endif // defined(NRF_NVMC_ICACHE_PRESENT)

#ifndef SUPPRESS_INLINE_IMPLEMENTATION
__STATIC_INLINE bool nrfx_nvmc_write_done_check(void)
{
    return nrf_nvmc_ready_check(NRF_NVMC);
}

#if defined(NRF_NVMC_ICACHE_PRESENT)
__STATIC_INLINE void nrfx_nvmc_icache_enable(void)
{
    nrf_nvmc_icache_config_set(NRF_NVMC, NRF_NVMC_ICACHE_ENABLE_WITH_PROFILING);
}

__STATIC_INLINE void nrfx_nvmc_icache_disable(void)
{
    nrf_nvmc_icache_config_set(NRF_NVMC, NRF_NVMC_ICACHE_DISABLE);
}
#endif // defined(NRF_NVMC_ICACHE_PRESENT)
#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_NVMC_H__
