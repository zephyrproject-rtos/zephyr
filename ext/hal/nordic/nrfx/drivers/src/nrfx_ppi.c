/*
 * Copyright (c) 2015 - 2018, Nordic Semiconductor ASA
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

#if NRFX_CHECK(NRFX_PPI_ENABLED)

#include <nrfx_ppi.h>

#define NRFX_LOG_MODULE PPI
#include <nrfx_log.h>


static uint32_t         m_channels_allocated; /**< Bitmap representing channels availability. 1 when a channel is allocated, 0 otherwise. */
static uint8_t          m_groups_allocated;   /**< Bitmap representing groups availability. 1 when a group is allocated, 0 otherwise.*/


/**
 * @brief Compute a group mask (needed for driver internals, not used for NRF_PPI registers).
 *
 * @param[in] group Group number to transform to a mask.
 *
 * @retval Group mask.
 */
__STATIC_INLINE uint32_t group_to_mask(nrf_ppi_channel_group_t group)
{
    return (1uL << (uint32_t) group);
}


/**
 * @brief Check whether a channel is a programmable channel and can be used by an application.
 *
 * @param[in] channel Channel to check.
 *
 * @retval true  The channel is a programmable application channel.
 * @retval false The channel is used by a stack (for example SoftDevice) or is preprogrammed.
 */
__STATIC_INLINE bool is_programmable_app_channel(nrf_ppi_channel_t channel)
{
    return ((NRFX_PPI_PROG_APP_CHANNELS_MASK & nrfx_ppi_channel_to_mask(channel)) != 0);
}


/**
 * @brief Check whether channels can be used by an application.
 *
 * @param[in] channel_mask Channel mask to check.
 *
 * @retval true  All specified channels can be used by an application.
 * @retval false At least one specified channel is used by a stack (for example SoftDevice).
 */
__STATIC_INLINE bool are_app_channels(uint32_t channel_mask)
{
    //lint -e(587)
    return ((~(NRFX_PPI_ALL_APP_CHANNELS_MASK) & channel_mask) == 0);
}


/**
 * @brief Check whether a channel can be used by an application.
 *
 * @param[in] channel Channel to check.
 *
 * @retval true  The channel can be used by an application.
 * @retval false The channel is used by a stack (for example SoftDevice).
 */
__STATIC_INLINE bool is_app_channel(nrf_ppi_channel_t channel)
{
    return are_app_channels(nrfx_ppi_channel_to_mask(channel));
}


/**
 * @brief Check whether a channel group can be used by an application.
 *
 * @param[in] group Group to check.
 *
 * @retval true  The group is an application group.
 * @retval false The group is not an application group (this group either does not exist or
 *               it is used by a stack (for example SoftDevice)).
 */
__STATIC_INLINE bool is_app_group(nrf_ppi_channel_group_t group)
{
    return ((NRFX_PPI_ALL_APP_GROUPS_MASK & group_to_mask(group)) != 0);
}


/**
 * @brief Check whether a channel is allocated.
 *
 * @param[in] channel_num  Channel number to check.
 *
 * @retval true  The channel is allocated.
 * @retval false The channel is not allocated.
 */
__STATIC_INLINE bool is_allocated_channel(nrf_ppi_channel_t channel)
{
    return ((m_channels_allocated & nrfx_ppi_channel_to_mask(channel)) != 0);
}


/**
 * @brief Set channel allocated indication.
 *
 * @param[in] channel_num Specifies the channel to set the "allocated" indication.
 */
__STATIC_INLINE void channel_allocated_set(nrf_ppi_channel_t channel)
{
    m_channels_allocated |= nrfx_ppi_channel_to_mask(channel);
}


/**
 * @brief Clear channel allocated indication.
 *
 * @param[in] channel_num Specifies the channel to clear the "allocated" indication.
 */
__STATIC_INLINE void channel_allocated_clr(nrf_ppi_channel_t channel)
{
    m_channels_allocated &= ~nrfx_ppi_channel_to_mask(channel);
}


/**
 * @brief Clear all allocated channels.
 */
__STATIC_INLINE void channel_allocated_clr_all(void)
{
    m_channels_allocated &= ~NRFX_PPI_ALL_APP_CHANNELS_MASK;
}


/**
 * @brief Check whether a group is allocated.
 *
 * @param[in] group_num Group number to check.
 *
 * @retval true  The group is allocated.
 *         false The group is not allocated.
 */
__STATIC_INLINE bool is_allocated_group(nrf_ppi_channel_group_t group)
{
    return ((m_groups_allocated & group_to_mask(group)) != 0);
}


/**
 * @brief Set group allocated indication.
 *
 * @param[in] group_num Specifies the group to set the "allocated" indication.
 */
__STATIC_INLINE void group_allocated_set(nrf_ppi_channel_group_t group)
{
    m_groups_allocated |= group_to_mask(group);
}


/**
 * @brief Clear group allocated indication.
 *
 * @param[in] group_num Specifies the group to clear the "allocated" indication.
 */
__STATIC_INLINE void group_allocated_clr(nrf_ppi_channel_group_t group)
{
    m_groups_allocated &= ~group_to_mask(group);
}


/**
 * @brief Clear all allocated groups.
 */
__STATIC_INLINE void group_allocated_clr_all()
{
    m_groups_allocated &= ~NRFX_PPI_ALL_APP_GROUPS_MASK;
}


void nrfx_ppi_free_all(void)
{
    uint32_t mask = NRFX_PPI_ALL_APP_GROUPS_MASK;
    nrf_ppi_channel_group_t group;

    // Disable all channels and groups
    nrf_ppi_channels_disable(NRFX_PPI_ALL_APP_CHANNELS_MASK);

    for (group = NRF_PPI_CHANNEL_GROUP0; mask != 0; mask &= ~group_to_mask(group), group++)
    {
        if (mask & group_to_mask(group))
        {
            nrf_ppi_channel_group_clear(group);
        }
    }
    channel_allocated_clr_all();
    group_allocated_clr_all();
}


nrfx_err_t nrfx_ppi_channel_alloc(nrf_ppi_channel_t * p_channel)
{
    nrfx_err_t err_code = NRFX_SUCCESS;
    nrf_ppi_channel_t channel;
    uint32_t mask = 0;
    err_code = NRFX_ERROR_NO_MEM;

    mask = NRFX_PPI_PROG_APP_CHANNELS_MASK;
    for (channel = NRF_PPI_CHANNEL0;
         mask != 0;
         mask &= ~nrfx_ppi_channel_to_mask(channel), channel++)
    {
        NRFX_CRITICAL_SECTION_ENTER();
        if ((mask & nrfx_ppi_channel_to_mask(channel)) && (!is_allocated_channel(channel)))
        {
            channel_allocated_set(channel);
            *p_channel = channel;
            err_code   = NRFX_SUCCESS;
        }
        NRFX_CRITICAL_SECTION_EXIT();
        if (err_code == NRFX_SUCCESS)
        {
            NRFX_LOG_INFO("Allocated channel: %d.", channel);
            break;
        }
    }

    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_ppi_channel_free(nrf_ppi_channel_t channel)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!is_programmable_app_channel(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        // First disable this channel
        nrf_ppi_channel_disable(channel);
        NRFX_CRITICAL_SECTION_ENTER();
        channel_allocated_clr(channel);
        NRFX_CRITICAL_SECTION_EXIT();
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_ppi_channel_assign(nrf_ppi_channel_t channel, uint32_t eep, uint32_t tep)
{
    if ((uint32_t *)eep == NULL || (uint32_t *)tep == NULL)
    {
        return NRFX_ERROR_NULL;
    }

    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!is_programmable_app_channel(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else if (!is_allocated_channel(channel))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else
    {
        nrf_ppi_channel_endpoint_setup(channel, eep, tep);
        NRFX_LOG_INFO("Assigned channel: %d, event end point: %x, task end point: %x.",
                      channel,
                      eep,
                      tep);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_ppi_channel_fork_assign(nrf_ppi_channel_t channel, uint32_t fork_tep)
{
    nrfx_err_t err_code = NRFX_SUCCESS;
#ifdef PPI_FEATURE_FORKS_PRESENT
    if (!is_programmable_app_channel(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else if (!is_allocated_channel(channel))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else
    {
        nrf_ppi_fork_endpoint_setup(channel, fork_tep);
        NRFX_LOG_INFO("Fork assigned channel: %d, task end point: %d.", channel, fork_tep);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
#else
    err_code = NRFX_ERROR_NOT_SUPPORTED;
    NRFX_LOG_WARNING("Function: %s, error code: %s.",
                     __func__,
                     NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
#endif
}

nrfx_err_t nrfx_ppi_channel_enable(nrf_ppi_channel_t channel)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!is_app_channel(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else if (is_programmable_app_channel(channel) && !is_allocated_channel(channel))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else
    {
        nrf_ppi_channel_enable(channel);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_ppi_channel_disable(nrf_ppi_channel_t channel)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!is_app_channel(channel))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else if (is_programmable_app_channel(channel) && !is_allocated_channel(channel))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else
    {
        nrf_ppi_channel_disable(channel);
        err_code = NRFX_SUCCESS;
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_ppi_group_alloc(nrf_ppi_channel_group_t * p_group)
{
    nrfx_err_t err_code;
    uint32_t mask = 0;
    nrf_ppi_channel_group_t group;

    err_code = NRFX_ERROR_NO_MEM;

    mask = NRFX_PPI_ALL_APP_GROUPS_MASK;
    for (group = NRF_PPI_CHANNEL_GROUP0; mask != 0; mask &= ~group_to_mask(group), group++)
    {
        NRFX_CRITICAL_SECTION_ENTER();
        if ((mask & group_to_mask(group)) && (!is_allocated_group(group)))
        {
            group_allocated_set(group);
            *p_group = group;
            err_code = NRFX_SUCCESS;
        }
        NRFX_CRITICAL_SECTION_EXIT();
        if (err_code == NRFX_SUCCESS)
        {
            NRFX_LOG_INFO("Allocated group: %d.", group);
            break;
        }
    }

    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_ppi_group_free(nrf_ppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!is_app_group(group))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    if (!is_allocated_group(group))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else
    {
        nrf_ppi_group_disable(group);
        NRFX_CRITICAL_SECTION_ENTER();
        group_allocated_clr(group);
        NRFX_CRITICAL_SECTION_EXIT();
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_ppi_group_enable(nrf_ppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!is_app_group(group))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else if (!is_allocated_group(group))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else
    {
        nrf_ppi_group_enable(group);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_ppi_group_disable(nrf_ppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!is_app_group(group))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        nrf_ppi_group_disable(group);
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_ppi_channels_remove_from_group(uint32_t                channel_mask,
                                               nrf_ppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!is_app_group(group))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else if (!is_allocated_group(group))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else if (!are_app_channels(channel_mask))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        NRFX_CRITICAL_SECTION_ENTER();
        nrf_ppi_channels_remove_from_group(channel_mask, group);
        NRFX_CRITICAL_SECTION_EXIT();
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_ppi_channels_include_in_group(uint32_t                channel_mask,
                                              nrf_ppi_channel_group_t group)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    if (!is_app_group(group))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else if (!is_allocated_group(group))
    {
        err_code = NRFX_ERROR_INVALID_STATE;
    }
    else if (!are_app_channels(channel_mask))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
    }
    else
    {
        NRFX_CRITICAL_SECTION_ENTER();
        nrf_ppi_channels_include_in_group(channel_mask, group);
        NRFX_CRITICAL_SECTION_EXIT();
    }
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}
#endif // NRFX_CHECK(NRFX_PPI_ENABLED)
