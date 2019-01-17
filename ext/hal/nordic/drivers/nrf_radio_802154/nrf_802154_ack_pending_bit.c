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
 *   This file implements procedures to set pending bit in nRF 802.15.4 radio driver.
 *
 */

#include "nrf_802154_ack_pending_bit.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nrf_802154_config.h"
#include "nrf_802154_const.h"

#include "hal/nrf_radio.h"

/// Maximum number of Short Addresses of nodes for which there is pending data in buffer.
#define NUM_PENDING_SHORT_ADDRESSES     NRF_802154_PENDING_SHORT_ADDRESSES
/// Maximum number of Extended Addresses of nodes for which there is pending data in buffer.
#define NUM_PENDING_EXTENDED_ADDRESSES  NRF_802154_PENDING_EXTENDED_ADDRESSES
/// Value used to mark Short Address as unused.
#define UNUSED_PENDING_SHORT_ADDRESS    ((uint8_t[SHORT_ADDRESS_SIZE]) {0xff, 0xff})
/// Value used to mark Extended Address as unused.
#define UNUSED_PENDING_EXTENDED_ADDRESS ((uint8_t[EXTENDED_ADDRESS_SIZE]) {0})

/// If pending bit in ACK frame should be set to valid or default value.
static bool m_setting_pending_bit_enabled;
/// Array of Short Addresses of nodes for which there is pending data in the buffer.
static uint8_t m_pending_short[NUM_PENDING_SHORT_ADDRESSES][SHORT_ADDRESS_SIZE];
/// Array of Extended Addresses of nodes for which there is pending data in the buffer.
static uint8_t m_pending_extended[NUM_PENDING_EXTENDED_ADDRESSES][EXTENDED_ADDRESS_SIZE];
/// Current number of Short Addresses of nodes for which there is pending data in the buffer.
static uint8_t m_num_of_pending_short;
/// Current number of Extended Addresses of nodes for which there is pending data in the buffer.
static uint8_t m_num_of_pending_extended;

/**
 * @brief Compare two extended addresses.
 *
 * @param[in]  p_first_addr     Pointer to a first address that should be compared.
 * @param[in]  p_second_addr    Pointer to a second address that should be compared.
 *
 * @retval -1  First address is less than the second address.
 * @retval  0  First address is equal to the second address.
 * @retval  1  First address is greater than the second address.
 */
static int8_t extended_addr_compare(const uint8_t * p_first_addr, const uint8_t * p_second_addr)
{
    uint32_t first_addr;
    uint32_t second_addr;

    // Compare extended address in two steps to prevent unaligned access error.
    for (uint32_t i = 0; i < EXTENDED_ADDRESS_SIZE / sizeof(uint32_t); i++)
    {
        first_addr  = *(uint32_t *)(p_first_addr + (i * sizeof(uint32_t)));
        second_addr = *(uint32_t *)(p_second_addr + (i * sizeof(uint32_t)));

        if (first_addr < second_addr)
        {
            return -1;
        }
        else if (first_addr > second_addr)
        {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Compare two short addresses.
 *
 * @param[in]  p_first_addr     Pointer to a first address that should be compared.
 * @param[in]  p_second_addr    Pointer to a second address that should be compared.
 *
 * @retval -1  First address is less than the second address.
 * @retval  0  First address is equal to the second address.
 * @retval  1  First address is greater than the second address.
 */
static int8_t short_addr_compare(const uint8_t * p_first_addr, const uint8_t * p_second_addr)
{
    uint16_t first_addr  = *(uint16_t *)(p_first_addr);
    uint16_t second_addr = *(uint16_t *)(p_second_addr);

    if (first_addr < second_addr)
    {
        return -1;
    }
    else if (first_addr > second_addr)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Compare two addresses.
 *
 * @param[in]  p_first_addr     Pointer to a first address that should be compared.
 * @param[in]  p_second_addr    Pointer to a second address that should be compared.
 * @param[in]  extended         Indication if @p p_first_addr and @p p_second_addr are extended or short addresses.
 *
 * @retval -1  First address is less than the second address.
 * @retval  0  First address is equal to the second address.
 * @retval  1  First address is greater than the second address.
 */
static int8_t addr_compare(const uint8_t * p_first_addr,
                           const uint8_t * p_second_addr,
                           bool            extended)
{
    if (extended)
    {
        return extended_addr_compare(p_first_addr, p_second_addr);
    }
    else
    {
        return short_addr_compare(p_first_addr, p_second_addr);
    }
}

/**
 * @brief Perform a binary search for an address in a list of addresses.
 *
 * @param[in]  p_addr           Pointer to an address that is searched for.
 * @param[in]  p_addr_array     Pointer to a list of addresses to be searched.
 * @param[out] p_location       If the address @p p_addr appears in the list, this is its index in the address list.
 *                              Otherwise, it is the index which @p p_addr would have if it was placed in the list
 *                              (ascending order assumed).
 * @param[in]  extended         Indication if @p p_addr is an extended or a short addresses.
 *
 * @retval true   Address @p p_addr is in the list.
 * @retval false  Address @p p_addr is not in the list.
 */
static bool addr_binary_search(const uint8_t * p_addr,
                               const uint8_t * p_addr_array,
                               uint8_t       * p_location,
                               bool            extended)
{
    uint8_t addr_array_len = extended ? m_num_of_pending_extended : m_num_of_pending_short;
    uint8_t entry_size     = extended ? EXTENDED_ADDRESS_SIZE : SHORT_ADDRESS_SIZE;
    int8_t  low            = 0;
    int8_t  midpoint       = 0;
    int8_t  high           = addr_array_len;

    while (high >= low)
    {
        midpoint = low + (high - low) / 2;

        if (midpoint >= addr_array_len)
        {
            break;
        }

        switch (addr_compare(p_addr, p_addr_array + entry_size * midpoint, extended))
        {
            case -1:
                high = midpoint - 1;
                break;

            case 0:
                *p_location = midpoint;
                return true;

            case 1:
                low = midpoint + 1;
                break;

            default:
                break;
        }
    }

    /* If in the last iteration of the loop the last case was utilized, it means that the midpoint
     * found by the algorithm is less than the address to be added. The midpoint should be therefore
     * shifted to the next position. As a simplified example, a { 1, 3, 4 } array can be considered.
     * Suppose that a number equal to 2 is about to be added to the array. At the beginning of the
     * last iteration, midpoint is equal to 1 and low and high are equal to 0. Midpoint is then set
     * to 0 and with last case being utilized, low is set to 1. However, midpoint equal to 0 is
     * incorrect, as in the last iteration first element of the array proves to be less than the
     * element to be added to the array. With the below code, midpoint is then shifted to 1. */
    if (low == midpoint + 1)
    {
        midpoint++;
    }

    *p_location = midpoint;
    return false;
}

/**
 * @brief Find an address in a list of addresses.
 *
 * @param[in]  p_addr           Pointer to an address that is searched for.
 * @param[out] p_location       If the address @p p_addr appears in the list, this is its index in the address list.
 *                              Otherwise, it is the index which @p p_addr would have if it was placed in the list
 *                              (ascending order assumed).
 * @param[in]  extended         Indication if @p p_addr is an extended or a short addresses.
 *
 * @retval true   Address @p p_addr is in the list.
 * @retval false  Address @p p_addr is not in the list.
 */
static bool addr_index_find(const uint8_t * p_addr,
                            uint8_t       * p_location,
                            bool            extended)
{
    if (extended)
    {
        return addr_binary_search(p_addr, (uint8_t *)m_pending_extended, p_location, extended);
    }
    else
    {
        return addr_binary_search(p_addr, (uint8_t *)m_pending_short, p_location, extended);
    }
}

/**
 * @brief Add an address to the address list in ascending order.
 *
 * @param[in]  p_addr           Pointer to the address to be added.
 * @param[in]  location         Index of the location where @p p_addr should be added.
 * @param[in]  extended         Indication if @p p_addr is an extended or a short addresses.
 *
 * @retval true   Address @p p_addr has been added to the list successfully.
 * @retval false  Address @p p_addr could not be added to the list.
 */
static bool addr_add(const uint8_t * p_addr, uint8_t location, bool extended)
{
    uint8_t * p_addr_array;
    uint8_t   max_addr_array_len;
    uint8_t * p_addr_array_len;
    uint8_t   entry_size;

    p_addr_array       = extended ? (uint8_t *)m_pending_extended : (uint8_t *)m_pending_short;
    max_addr_array_len = extended ? NUM_PENDING_EXTENDED_ADDRESSES : NUM_PENDING_SHORT_ADDRESSES;
    p_addr_array_len   = extended ? &m_num_of_pending_extended : &m_num_of_pending_short;
    entry_size         = extended ? EXTENDED_ADDRESS_SIZE : SHORT_ADDRESS_SIZE;

    if (*p_addr_array_len == max_addr_array_len)
    {
        return false;
    }

    memmove(p_addr_array + entry_size * (location + 1),
            p_addr_array + entry_size * (location),
            (*p_addr_array_len - location) * entry_size);

    memcpy(p_addr_array + entry_size * location, p_addr, entry_size);

    (*p_addr_array_len)++;

    return true;
}

/**
 * @brief Remove an address from the address list keeping it in ascending order.
 *
 * @param[in]  location     Index of the element to be removed from the list.
 * @param[in]  extended     Indication if address to remove is an extended or a short address.
 *
 * @retval true   Address @p p_addr has been removed from the list successfully.
 * @retval false  Address @p p_addr could not removed from the list.
 */
static bool addr_remove(uint8_t location, bool extended)
{
    uint8_t * p_addr_array;
    uint8_t * p_addr_array_len;
    uint8_t   entry_size;

    p_addr_array     = extended ? (uint8_t *)m_pending_extended : (uint8_t *)m_pending_short;
    p_addr_array_len = extended ? &m_num_of_pending_extended : &m_num_of_pending_short;
    entry_size       = extended ? EXTENDED_ADDRESS_SIZE : SHORT_ADDRESS_SIZE;

    if (*p_addr_array_len == 0)
    {
        return false;
    }

    memmove(p_addr_array + entry_size * location,
            p_addr_array + entry_size * (location + 1),
            (*p_addr_array_len - location - 1) * entry_size);

    (*p_addr_array_len)--;

    return true;
}

void nrf_802154_ack_pending_bit_init(void)
{
    m_setting_pending_bit_enabled = true;
    m_num_of_pending_extended     = 0;
    m_num_of_pending_short        = 0;
}

void nrf_802154_ack_pending_bit_set(bool enabled)
{
    m_setting_pending_bit_enabled = enabled;
}

bool nrf_802154_ack_pending_bit_for_addr_set(const uint8_t * p_addr, bool extended)
{
    uint8_t location = 0;

    if (addr_index_find(p_addr, &location, extended))
    {
        return true;
    }
    else
    {
        return addr_add(p_addr, location, extended);
    }
}

bool nrf_802154_ack_pending_bit_for_addr_clear(const uint8_t * p_addr, bool extended)
{
    uint8_t location = 0;

    if (addr_index_find(p_addr, &location, extended))
    {
        return addr_remove(location, extended);
    }
    else
    {
        return false;
    }
}

void nrf_802154_ack_pending_bit_for_addr_reset(bool extended)
{
    if (extended)
    {
        m_num_of_pending_extended = 0;
    }
    else
    {
        m_num_of_pending_short = 0;
    }
}

bool nrf_802154_ack_pending_bit_should_be_set(const uint8_t * p_psdu)
{
    const uint8_t * p_src_addr;
    uint8_t         location;
    bool            extended;

    // If automatic setting of pending bit in ACK frames is disabled the pending bit is always set.
    if (!m_setting_pending_bit_enabled)
    {
        return true;
    }

    switch (p_psdu[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK)
    {
        case DEST_ADDR_TYPE_SHORT:
            p_src_addr = &p_psdu[SRC_ADDR_OFFSET_SHORT_DST];
            break;

        case DEST_ADDR_TYPE_EXTENDED:
            p_src_addr = &p_psdu[SRC_ADDR_OFFSET_EXTENDED_DST];
            break;

        default:
            return true;
    }

    if (0 == (p_psdu[PAN_ID_COMPR_OFFSET] & PAN_ID_COMPR_MASK))
    {
        p_src_addr += PAN_ID_SIZE;
    }

    switch (p_psdu[SRC_ADDR_TYPE_OFFSET] & SRC_ADDR_TYPE_MASK)
    {
        case SRC_ADDR_TYPE_SHORT:
            extended = false;
            break;

        case SRC_ADDR_TYPE_EXTENDED:
            extended = true;
            break;

        default:
            return true;
    }

    return addr_index_find(p_src_addr, &location, extended);
}
