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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_DPPI_ENABLED)

#include <nrfx_dppi.h>

#define NRFX_LOG_MODULE DPPI
#include <nrfx_log.h>

#if !defined(NRFX_DPPI_CHANNELS_USED)
// Default mask of DPPI channels reserved for other modules.
#define NRFX_DPPI_CHANNELS_USED 0x00000000uL
#endif

#if !defined(NRFX_DPPI_GROUPS_USED)
// Default mask of DPPI groups reserved for other modules.
#define NRFX_DPPI_GROUPS_USED 0x00000000uL
#endif

#define DPPI_AVAILABLE_CHANNELS_MASK \
    (((1UL << DPPI_CH_NUM) - 1) & (~NRFX_DPPI_CHANNELS_USED))

#define DPPI_AVAILABLE_GROUPS_MASK   \
    (((1UL << DPPI_GROUP_NUM) - 1)   & (~NRFX_DPPI_GROUPS_USED))

/** @brief Set bit at given position. */
#define DPPI_BIT_SET(pos) (1uL << (pos))

static uint32_t m_allocated_channels;

static uint8_t  m_allocated_groups;

__STATIC_INLINE bool channel_is_allocated(uint8_t channel)
{
    return ((m_allocated_channels & DPPI_BIT_SET(channel)) != 0);
}

__STATIC_INLINE bool group_is_allocated(nrf_dppi_channel_group_t group)
{
    return ((m_allocated_groups & DPPI_BIT_SET(group)) != 0);
}

void nrfx_dppi_free(void)
{
    uint32_t mask = m_allocated_groups;
    nrf_dppi_channel_group_t group = NRF_DPPI_CHANNEL_GROUP0;

    // Disable all channels
    nrf_dppi_channels_disable(NRF_DPPIC, m_allocated_channels);

    // Clear all groups configurations
    while (mask)
    {
        if (mask & DPPI_BIT_SET(group))
        {
            nrf_dppi_group_clear(NRF_DPPIC, group);
            mask &= ~DPPI_BIT_SET(group);
        }
        group++;
    }

    // Clear all allocated channels.
    m_allocated_channels = 0;

    // Clear all allocated groups.
    m_allocated_groups = 0;
}

nrfx_err_t nrfx_dppi_channel_alloc(uint8_t * p_channel)
{
    nrfx_err_t err_code;

    // Get mask of available DPPI channels
    uint32_t remaining_channels = DPPI_AVAILABLE_CHANNELS_MASK & ~(m_allocated_channels);
    uint8_t channel = 0;

    if (!remaining_channels)
    {
        err_code = NRFX_ERROR_NO_MEM;
        NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    // Find first free channel
    while (!(remaining_channels & DPPI_BIT_SET(channel)))
    {
        channel++;
    }

    m_allocated_channels |= DPPI_BIT_SET(channel);
    *p_channel = channel;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Allocated channel: %d.", channel);
    return err_code;
}

nrfx_err_t nrfx_dppi_channel_free(uint8_t channel)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!channel_is_allocated(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        // First disable this channel
        nrf_dppi_channels_disable(NRF_DPPIC, DPPI_BIT_SET(channel));
        // Clear channel allocated indication.
        m_allocated_channels &= ~DPPI_BIT_SET(channel);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_dppi_channel_enable(uint8_t channel)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!channel_is_allocated(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        nrf_dppi_channels_enable(NRF_DPPIC, DPPI_BIT_SET(channel));
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_dppi_channel_disable(uint8_t channel)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!channel_is_allocated(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        nrf_dppi_channels_disable(NRF_DPPIC, DPPI_BIT_SET(channel));
        err_code = NRFX_SUCCESS;
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_dppi_group_alloc(nrf_dppi_channel_group_t * p_group)
{
    nrfx_err_t err_code;

    // Get mask of available DPPI groups
    uint32_t remaining_groups = DPPI_AVAILABLE_GROUPS_MASK & ~(m_allocated_groups);
    nrf_dppi_channel_group_t group = NRF_DPPI_CHANNEL_GROUP0;

    if (!remaining_groups)
    {
        err_code = NRFX_ERROR_NO_MEM;
        NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    // Find first free group
    while (!(remaining_groups & DPPI_BIT_SET(group)))
    {
        group++;
    }

    m_allocated_groups |= DPPI_BIT_SET(group);
    *p_group = group;

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Allocated channel: %d.", group);
    return err_code;
}

nrfx_err_t nrfx_dppi_group_free(nrf_dppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!group_is_allocated(group))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        nrf_dppi_group_disable(NRF_DPPIC, group);
        // Set bit value to zero at position corresponding to the group number.
        m_allocated_groups &= ~DPPI_BIT_SET(group);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_dppi_channel_include_in_group(uint8_t                  channel,
                                              nrf_dppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!group_is_allocated(group) || !channel_is_allocated(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        nrf_dppi_channels_include_in_group(NRF_DPPIC, DPPI_BIT_SET(channel), group);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_dppi_channel_remove_from_group(uint8_t                  channel,
                                               nrf_dppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!group_is_allocated(group) || !channel_is_allocated(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        nrf_dppi_channels_remove_from_group(NRF_DPPIC, DPPI_BIT_SET(channel), group);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_dppi_group_clear(nrf_dppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!group_is_allocated(group))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        nrf_dppi_channels_remove_from_group(NRF_DPPIC, DPPI_AVAILABLE_CHANNELS_MASK, group);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_dppi_group_enable(nrf_dppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!group_is_allocated(group))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        nrf_dppi_group_enable(NRF_DPPIC, group);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_dppi_group_disable(nrf_dppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!group_is_allocated(group))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        nrf_dppi_group_disable(NRF_DPPIC, group);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

#endif // NRFX_CHECK(NRFX_DPPI_ENABLED)
