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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_NVMC_ENABLED)

#include <nrfx_nvmc.h>

/**
 * Value representing the number of bytes in a word.
 *
 * It is used in loops iterating over bytes contained in a word
 * or in word-alignment checks.
 */
#define NVMC_BYTES_IN_WORD  4

/**
 * Value representing non-volatile memory (NVM) page count.
 *
 * This symbol is needed to determine NVM page count for chips that cannot
 * always access FICR for this information.
 */
#if defined(NRF9160_XXAA)
    #define NVMC_FLASH_PAGE_COUNT  256
#endif

/**
 * Value representing non-volatile memory (NVM) page size in bytes.
 *
 * This symbol is needed to determine NVM page size for chips that cannot
 * always access FICR for this information.
 */
#if defined(NRF9160_XXAA)
    #define NVMC_FLASH_PAGE_SIZE  0x1000 ///< 4 kB
#endif

#if defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)
/**
 * Value representing the page erase time.
 *
 * This value is used to determine whether the partial erase is still in progress.
 */
#if defined(NRF52810_XXAA) || defined(NRF52811_XXAA) || defined(NRF52840_XXAA)
    #define NVMC_PAGE_ERASE_DURATION_MS  85
#elif defined(NRF9160_XXAA)
    #define NVMC_PAGE_ERASE_DURATION_MS  87
#else
    #error "Page partial erase present but could not determine its total duration for given SoC"
#endif

/**
 * Value representing the invalid page partial erase address.
 *
 * This value is used for representing a NULL pointer for
 * partial erase, as that address 0 can be a valid
 * memory address in flash.
 */
#define NVMC_PARTIAL_ERASE_INVALID_ADDR  0xFFFFFFFF

/** Internal counter for page partial erase. */
static uint32_t m_partial_erase_time_elapsed;

/** Partial erase page address. */
static uint32_t m_partial_erase_page_addr = NVMC_PARTIAL_ERASE_INVALID_ADDR;

#endif // defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)

static uint32_t flash_page_size_get(void)
{
    uint32_t flash_page_size = 0;

#if defined(NRF51) || defined(NRF52_SERIES)
    flash_page_size = nrf_ficr_codepagesize_get(NRF_FICR);
#elif defined(NVMC_FLASH_PAGE_SIZE)
    flash_page_size = NVMC_FLASH_PAGE_SIZE;
#else
    #error "Cannot determine flash page size for a given SoC."
#endif

    return flash_page_size;
}

static uint32_t flash_page_count_get(void)
{
    uint32_t page_count = 0;

#if defined(NRF51) || defined(NRF52_SERIES)
    page_count = nrf_ficr_codesize_get(NRF_FICR);
#elif defined(NVMC_FLASH_PAGE_COUNT)
    page_count = NVMC_FLASH_PAGE_COUNT;
#else
    #error "Cannot determine flash page count for a given SoC."
#endif

    return page_count;
}

static uint32_t flash_total_size_get(void)
{
    return flash_page_size_get() * flash_page_count_get();
}


static bool is_page_aligned_check(uint32_t addr)
{
    /* If the modulo operation returns '0', then the address is aligned. */
    return !(addr % flash_page_size_get());
}

static uint32_t partial_word_create(uint32_t addr, uint8_t const * bytes, uint32_t bytes_count)
{
    uint32_t value32;
    uint32_t byte_shift;

    byte_shift = addr % NVMC_BYTES_IN_WORD;

    NRFX_ASSERT(bytes_count <= (NVMC_BYTES_IN_WORD - byte_shift));

    value32 = 0xFFFFFFFF;
    for (uint32_t i = 0; i < bytes_count; i++)
    {
        ((uint8_t *)&value32)[byte_shift] = bytes[i];
        byte_shift++;
    }

    return value32;
}

static void nvmc_readonly_mode_set(void)
{
#if defined(NRF_TRUSTZONE_NONSECURE)
    nrf_nvmc_nonsecure_mode_set(NRF_NVMC, NRF_NVMC_NS_MODE_READONLY);
#else
    nrf_nvmc_mode_set(NRF_NVMC, NRF_NVMC_MODE_READONLY);
#endif
}

static void nvmc_write_mode_set(void)
{
#if defined(NRF_TRUSTZONE_NONSECURE)
    nrf_nvmc_nonsecure_mode_set(NRF_NVMC, NRF_NVMC_NS_MODE_WRITE);
#else
    nrf_nvmc_mode_set(NRF_NVMC, NRF_NVMC_MODE_WRITE);
#endif
}

static void nvmc_erase_mode_set(void)
{
#if defined(NRF_TRUSTZONE_NONSECURE)
    nrf_nvmc_nonsecure_mode_set(NRF_NVMC, NRF_NVMC_NS_MODE_ERASE);
#else
    nrf_nvmc_mode_set(NRF_NVMC, NRF_NVMC_MODE_ERASE);
#endif
}

static void nvmc_word_write(uint32_t addr, uint32_t value)
{
#if defined(NRF9160_XXAA)
    while (!nrf_nvmc_write_ready_check(NRF_NVMC))
    {}
#else
    while (!nrf_nvmc_ready_check(NRF_NVMC))
    {}
#endif

    *(volatile uint32_t *)addr = value;
    __DMB();
}

static void nvmc_words_write(uint32_t addr, void const * src, uint32_t num_words)
{
    for (uint32_t i = 0; i < num_words; i++)
    {
        nvmc_word_write(addr + (NVMC_BYTES_IN_WORD * i), ((uint32_t const *)src)[i]);
    }
}

nrfx_err_t nrfx_nvmc_page_erase(uint32_t addr)
{
    NRFX_ASSERT(addr < flash_total_size_get());

    if (!is_page_aligned_check(addr))
    {
        return NRFX_ERROR_INVALID_ADDR;
    }

    nvmc_erase_mode_set();
    nrf_nvmc_page_erase_start(NRF_NVMC, addr);
    while (!nrf_nvmc_ready_check(NRF_NVMC))
    {}
    nvmc_readonly_mode_set();

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_nvmc_uicr_erase(void)
{
#if defined(NVMC_ERASEUICR_ERASEUICR_Msk)
    nvmc_erase_mode_set();
    nrf_nvmc_uicr_erase_start(NRF_NVMC);
    while (!nrf_nvmc_ready_check(NRF_NVMC))
    {}
    nvmc_readonly_mode_set();
    return NRFX_SUCCESS;
#else
    return NRFX_ERROR_NOT_SUPPORTED;
#endif
}

void nrfx_nvmc_all_erase(void)
{
    nvmc_erase_mode_set();
    nrf_nvmc_erase_all_start(NRF_NVMC);
    while (!nrf_nvmc_ready_check(NRF_NVMC))
    {}
    nvmc_readonly_mode_set();
}

#if defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)
nrfx_err_t nrfx_nvmc_page_partial_erase_init(uint32_t addr, uint32_t duration_ms)
{
    NRFX_ASSERT(addr < flash_total_size_get());

    if (!is_page_aligned_check(addr))
    {
        return NRFX_ERROR_INVALID_ADDR;
    }

    m_partial_erase_time_elapsed = 0;
    m_partial_erase_page_addr = addr;
    nrf_nvmc_partial_erase_duration_set(NRF_NVMC, duration_ms);

    return NRFX_SUCCESS;
}

bool nrfx_nvmc_page_partial_erase_continue(void)
{
    NRFX_ASSERT(m_partial_erase_page_addr != NVMC_PARTIAL_ERASE_INVALID_ADDR);

    uint32_t duration_ms = nrf_nvmc_partial_erase_duration_get(NRF_NVMC);

#if defined(NVMC_CONFIG_WEN_PEen)
    nrf_nvmc_mode_set(NRF_NVMC, NRF_NVMC_MODE_PARTIAL_ERASE);
#else
    nrf_nvmc_mode_set(NRF_NVMC, NRF_NVMC_MODE_ERASE);
#endif

    nrf_nvmc_page_partial_erase_start(NRF_NVMC, m_partial_erase_page_addr);
    while (!nrf_nvmc_ready_check(NRF_NVMC))
    {}
    nvmc_readonly_mode_set();

    m_partial_erase_time_elapsed += duration_ms;
    if (m_partial_erase_time_elapsed < NVMC_PAGE_ERASE_DURATION_MS)
    {
        return false;
    }
    else
    {
        m_partial_erase_page_addr = NVMC_PARTIAL_ERASE_INVALID_ADDR;
        return true;
    }
}
#endif // defined(NRF_NVMC_PARTIAL_ERASE_PRESENT)

bool nrfx_nvmc_byte_writable_check(uint32_t addr, uint8_t val_to_check)
{
    NRFX_ASSERT(addr < flash_total_size_get());

    uint8_t val_on_addr = *(uint8_t const *)addr;
    return (val_to_check & val_on_addr) == val_to_check;
}

bool nrfx_nvmc_word_writable_check(uint32_t addr, uint32_t val_to_check)
{
    NRFX_ASSERT(addr < flash_total_size_get());
    NRFX_ASSERT(nrfx_is_word_aligned((void const *)addr));

    uint32_t val_on_addr = *(uint32_t const *)addr;
    return (val_to_check & val_on_addr) == val_to_check;
}

void nrfx_nvmc_byte_write(uint32_t addr, uint8_t value)
{
    uint32_t aligned_addr = addr & ~(0x03UL);

    nrfx_nvmc_word_write(aligned_addr, partial_word_create(addr, &value, 1));
}

void nrfx_nvmc_word_write(uint32_t addr, uint32_t value)
{
    NRFX_ASSERT(addr < flash_total_size_get());
    NRFX_ASSERT(nrfx_is_word_aligned((void const *)addr));

    nvmc_write_mode_set();

    nvmc_word_write(addr, value);

    nvmc_readonly_mode_set();
}

void nrfx_nvmc_bytes_write(uint32_t addr, void const * src, uint32_t num_bytes)
{
    NRFX_ASSERT(addr < flash_total_size_get());

    nvmc_write_mode_set();

    uint8_t const * bytes_src = (uint8_t const *)src;

    uint32_t unaligned_bytes = addr % NVMC_BYTES_IN_WORD;
    if (unaligned_bytes != 0)
    {
        uint32_t leading_bytes = NVMC_BYTES_IN_WORD - unaligned_bytes;
        if (leading_bytes > num_bytes)
        {
            leading_bytes = num_bytes;
        }

        nvmc_word_write(addr - unaligned_bytes,
                        partial_word_create(addr, bytes_src, leading_bytes));
        num_bytes -= leading_bytes;
        addr      += leading_bytes;
        bytes_src += leading_bytes;
    }

#if defined(__CORTEX_M) && (__CORTEX_M == 0U)
    if (!nrfx_is_word_aligned((void const *)bytes_src))
    {
        /* Cortex-M0 allows only word-aligned RAM access.
           If source address is not word-aligned, bytes are combined
           into words explicitly. */
        for (uint32_t i = 0; i < num_bytes / NVMC_BYTES_IN_WORD; i++)
        {
            uint32_t word = (uint32_t)bytes_src[0]
                            | ((uint32_t)bytes_src[1]) << 8
                            | ((uint32_t)bytes_src[2]) << 16
                            | ((uint32_t)bytes_src[3]) << 24;

            nvmc_word_write(addr, word);
            bytes_src += NVMC_BYTES_IN_WORD;
            addr += NVMC_BYTES_IN_WORD;
        }
    }
    else
#endif
    {
        uint32_t word_count = num_bytes / NVMC_BYTES_IN_WORD;

        nvmc_words_write(addr, (uint32_t const *)bytes_src, word_count);

        addr += word_count * NVMC_BYTES_IN_WORD;
        bytes_src += word_count * NVMC_BYTES_IN_WORD;
    }

    uint32_t trailing_bytes = num_bytes % NVMC_BYTES_IN_WORD;
    if (trailing_bytes != 0)
    {
        nvmc_word_write(addr, partial_word_create(addr, bytes_src, trailing_bytes));
    }

    nvmc_readonly_mode_set();
}

void nrfx_nvmc_words_write(uint32_t addr, void const * src, uint32_t num_words)
{
    NRFX_ASSERT(addr < flash_total_size_get());
    NRFX_ASSERT(nrfx_is_word_aligned((void const *)addr));
    NRFX_ASSERT(nrfx_is_word_aligned(src));

    nvmc_write_mode_set();

    nvmc_words_write(addr, src, num_words);

    nvmc_readonly_mode_set();
}

uint32_t nrfx_nvmc_flash_size_get(void)
{
    return flash_total_size_get();
}

uint32_t nrfx_nvmc_flash_page_size_get(void)
{
    return flash_page_size_get();
}

uint32_t nrfx_nvmc_flash_page_count_get(void)
{
    return flash_page_count_get();
}

#endif // NRFX_CHECK(NRFX_NVMC_ENABLED)
