/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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

/**
 * @file
 *   This file implements helpers for checking nRF SoC revision.
 *
 */

#include "nrf_802154_revision.h"

#include <assert.h>

/** @breif Types of nRF52840 revisions. */
typedef enum
{
    NRF52840_REVISION_AAAA,
    NRF52840_REVISION_AABA,
    NRF52840_REVISION_UNKNOWN,
} nrf_802154_chip_revision;

static nrf_802154_chip_revision m_nrf52840_revision = NRF52840_REVISION_UNKNOWN;

/**
 * @brief Internal auxiliary function to check if the program is running on NRF52840 chip.
 *
 * @retval true  If it is NRF52480 chip.
 * @retval false If it is other chip.
 */
static inline bool nrf_revision_type_52840(void)
{
    return ((((*(uint32_t *)0xF0000FE0) & 0xFF) == 0x08) &&
            (((*(uint32_t *)0xF0000FE4) & 0x0F) == 0x00));
}

/**
 * @brief Internal auxiliary function to check if the program is running
 *        on the AAAA revision of the nRF52840 chip.
 *
 * @retval true  If it is NRF52480_AAAA chip revision.
 * @retval false It is other chip revision.
 */
static inline bool nrf_revision_type_52840_aaaa(void)
{
    return (nrf_revision_type_52840() &&
            (((*(uint32_t *)0xF0000FE8) & 0xF0) == 0x00) && // revision
            (((*(uint32_t *)0xF0000FEC) & 0xF0) == 0x00));  // sub-revision
}

/**
 * @brief Internal auxiliary function to check if the program is running
 *        on the AABA revision of the nRF52840 chip.
 *
 * @retval true  If it is NRF52480_AAAA chip revision.
 * @retval false It is other chip revision.
 */
static inline bool nrf_revision_type_52840_aaba(void)
{
    return (nrf_revision_type_52840() &&
            (((*(uint32_t *)0xF0000FE8) & 0xF0) == 0x10) && // revision
            (((*(uint32_t *)0xF0000FEC) & 0xF0) == 0x00));  // sub-revision
}

void nrf_802154_revision_init(void)
{
    if (nrf_revision_type_52840_aaaa())
    {
        m_nrf52840_revision = NRF52840_REVISION_AAAA;
    }
    else if (nrf_revision_type_52840_aaba())
    {
        m_nrf52840_revision = NRF52840_REVISION_AABA;
    }
    else
    {
        m_nrf52840_revision = NRF52840_REVISION_UNKNOWN;
    }
}

bool nrf_802154_revision_has_phyend_event(void)
{
#if NRF52840_AAAA
    return false;
#elif NRF52840_AABA
    return true;
#else
    bool result = false;

    switch (m_nrf52840_revision)
    {
        case NRF52840_REVISION_AAAA:
            result = false;
            break;

        case NRF52840_REVISION_AABA:
        case NRF52840_REVISION_UNKNOWN:
            result = true;
            break;

        default:
            assert(false);
    }

    return result;
#endif // NRF52840_AAAA
}
