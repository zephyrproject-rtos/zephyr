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
/**
 *@file
 *@brief NMVC driver implementation
 */

#include <nrfx.h>
#include "nrf_nvmc.h"

static inline void wait_for_flash_ready(void)
{
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {;}
}

void nrf_nvmc_page_erase(uint32_t address)
{
    // Enable erase.
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
    __ISB();
    __DSB();

    // Erase the page
    NRF_NVMC->ERASEPAGE = address;
    wait_for_flash_ready();

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    __ISB();
    __DSB();
}


void nrf_nvmc_write_byte(uint32_t address, uint8_t value)
{
    uint32_t byte_shift = address & (uint32_t)0x03;
    uint32_t address32 = address & ~byte_shift; // Address to the word this byte is in.
    uint32_t value32 = (*(uint32_t*)address32 & ~((uint32_t)0xFF << (byte_shift << (uint32_t)3)));
    value32 = value32 + ((uint32_t)value << (byte_shift << 3));

    // Enable write.
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);
    __ISB();
    __DSB();

    *(uint32_t*)address32 = value32;
    wait_for_flash_ready();

    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    __ISB();
    __DSB();
}

void nrf_nvmc_write_word(uint32_t address, uint32_t value)
{
    // Enable write.
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
    __ISB();
    __DSB();

    *(uint32_t*)address = value;
    wait_for_flash_ready();

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    __ISB();
    __DSB();
}

void nrf_nvmc_write_bytes(uint32_t address, const uint8_t * src, uint32_t num_bytes)
{
    uint32_t i;
    for (i = 0; i < num_bytes; i++)
    {
       nrf_nvmc_write_byte(address + i,src[i]);
    }
}

void nrf_nvmc_write_words(uint32_t address, const uint32_t * src, uint32_t num_words)
{
    uint32_t i;

    // Enable write.
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
    __ISB();
    __DSB();

    for (i = 0; i < num_words; i++)
    {
        ((uint32_t*)address)[i] = src[i];
        wait_for_flash_ready();
    }

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    __ISB();
    __DSB();
}

