/*
 * Copyright (c) 2015 - 2019, Nordic Semiconductor ASA
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

#ifndef NRFX_PPI_H__
#define NRFX_PPI_H__

#include <nrfx.h>
#include <hal/nrf_ppi.h>

/**
 * @defgroup nrfx_ppi PPI allocator
 * @{
 * @ingroup nrf_ppi
 * @brief   Programmable Peripheral Interconnect (PPI) allocator.
 */

#ifdef __cplusplus
extern "C" {
#endif

#if !defined (NRFX_PPI_CHANNELS_USED) || defined(__NRFX_DOXYGEN__)
/** @brief Bitfield representing PPI channels used by external modules. */
#define NRFX_PPI_CHANNELS_USED 0
#endif

#if !defined(NRFX_PPI_GROUPS_USED) || defined(__NRFX_DOXYGEN__)
/** @brief Bitfield representing PPI groups used by external modules. */
#define NRFX_PPI_GROUPS_USED 0
#endif

#if (PPI_CH_NUM > 16) || defined(__NRFX_DOXYGEN__)
/** @brief Bitfield representing all PPI channels available to the application. */
#define NRFX_PPI_ALL_APP_CHANNELS_MASK   ((uint32_t)0xFFFFFFFFuL & ~(NRFX_PPI_CHANNELS_USED))
/** @brief Bitfield representing programmable PPI channels available to the application. */
#define NRFX_PPI_PROG_APP_CHANNELS_MASK  ((uint32_t)0x000FFFFFuL & ~(NRFX_PPI_CHANNELS_USED))
#else
#define NRFX_PPI_ALL_APP_CHANNELS_MASK   ((uint32_t)0xFFF0FFFFuL & ~(NRFX_PPI_CHANNELS_USED))
#define NRFX_PPI_PROG_APP_CHANNELS_MASK  ((uint32_t)0x0000FFFFuL & ~(NRFX_PPI_CHANNELS_USED))
#endif

/** @brief Bitfield representing all PPI groups available to the application. */
#define NRFX_PPI_ALL_APP_GROUPS_MASK     (((1uL << PPI_GROUP_NUM) - 1) & ~(NRFX_PPI_GROUPS_USED))

/**
 * @brief Function for uninitializing the PPI module.
 *
 * This function disables all channels and clears the channel groups.
 */
void nrfx_ppi_free_all(void);

/**
 * @brief Function for allocating a PPI channel.
 * @details This function allocates the first unused PPI channel.
 *
 * @param[out] p_channel Pointer to the PPI channel that has been allocated.
 *
 * @retval NRFX_SUCCESS      The channel was successfully allocated.
 * @retval NRFX_ERROR_NO_MEM There is no available channel to be used.
 */
nrfx_err_t nrfx_ppi_channel_alloc(nrf_ppi_channel_t * p_channel);

/**
 * @brief Function for freeing a PPI channel.
 * @details This function also disables the chosen channel.
 *
 * @param[in] channel PPI channel to be freed.
 *
 * @retval NRFX_SUCCESS             The channel was successfully freed.
 * @retval NRFX_ERROR_INVALID_PARAM The channel is not user-configurable.
 */
nrfx_err_t nrfx_ppi_channel_free(nrf_ppi_channel_t channel);

/**
 * @brief Function for assigning task and event endpoints to the PPI channel.
 *
 * @param[in] channel PPI channel to be assigned endpoints.
 * @param[in] eep     Event endpoint address.
 * @param[in] tep     Task endpoint address.
 *
 * @retval NRFX_SUCCESS             The channel was successfully assigned.
 * @retval NRFX_ERROR_INVALID_STATE The channel is not allocated for the user.
 * @retval NRFX_ERROR_INVALID_PARAM The channel is not user-configurable.
 */
nrfx_err_t nrfx_ppi_channel_assign(nrf_ppi_channel_t channel, uint32_t eep, uint32_t tep);

/**
 * @brief Function for assigning fork endpoint to the PPI channel or clearing it.
 *
 * @param[in] channel  PPI channel to be assigned endpoints.
 * @param[in] fork_tep Fork task endpoint address or 0 to clear.
 *
 * @retval NRFX_SUCCESS             The channel was successfully assigned.
 * @retval NRFX_ERROR_INVALID_STATE The channel is not allocated for the user.
 * @retval NRFX_ERROR_NOT_SUPPORTED Function is not supported.
 */
nrfx_err_t nrfx_ppi_channel_fork_assign(nrf_ppi_channel_t channel, uint32_t fork_tep);

/**
 * @brief Function for enabling a PPI channel.
 *
 * @param[in] channel PPI channel to be enabled.
 *
 * @retval NRFX_SUCCESS             The channel was successfully enabled.
 * @retval NRFX_ERROR_INVALID_STATE The user-configurable channel is not allocated.
 * @retval NRFX_ERROR_INVALID_PARAM The channel cannot be enabled by the user.
 */
nrfx_err_t nrfx_ppi_channel_enable(nrf_ppi_channel_t channel);

/**
 * @brief Function for disabling a PPI channel.
 *
 * @param[in] channel PPI channel to be disabled.
 *
 * @retval NRFX_SUCCESS             The channel was successfully disabled.
 * @retval NRFX_ERROR_INVALID_STATE The user-configurable channel is not allocated.
 * @retval NRFX_ERROR_INVALID_PARAM The channel cannot be disabled by the user.
 */
nrfx_err_t nrfx_ppi_channel_disable(nrf_ppi_channel_t channel);

/**
 * @brief Function for allocating a PPI channel group.
 * @details This function allocates the first unused PPI group.
 *
 * @param[out] p_group Pointer to the PPI channel group that has been allocated.
 *
 * @retval NRFX_SUCCESS      The channel group was successfully allocated.
 * @retval NRFX_ERROR_NO_MEM There is no available channel group to be used.
 */
nrfx_err_t nrfx_ppi_group_alloc(nrf_ppi_channel_group_t * p_group);

/**
 * @brief Function for freeing a PPI channel group.
 * @details This function also disables the chosen group.
 *
 * @param[in] group PPI channel group to be freed.
 *
 * @retval NRFX_SUCCESS             The channel group was successfully freed.
 * @retval NRFX_ERROR_INVALID_PARAM The channel group is not user-configurable.
 */
nrfx_err_t nrfx_ppi_group_free(nrf_ppi_channel_group_t group);

/**
 * @brief  Compute a channel mask for NRF_PPI registers.
 *
 * @param[in] channel Channel number to transform to a mask.
 *
 * @return Channel mask.
 */
__STATIC_INLINE uint32_t nrfx_ppi_channel_to_mask(nrf_ppi_channel_t channel)
{
    return (1uL << (uint32_t) channel);
}

/**
 * @brief Function for including multiple PPI channels in a channel group.
 *
 * @param[in] channel_mask PPI channels to be added.
 * @param[in] group        Channel group in which to include the channels.
 *
 * @retval NRFX_SUCCESS             The channels was successfully included.
 * @retval NRFX_ERROR_INVALID_PARAM Group is not an application group or channels are not an
 *                                  application channels.
 * @retval NRFX_ERROR_INVALID_STATE Group is not an allocated group.
 */
nrfx_err_t nrfx_ppi_channels_include_in_group(uint32_t                channel_mask,
                                              nrf_ppi_channel_group_t group);

/**
 * @brief Function for including a PPI channel in a channel group.
 *
 * @param[in] channel PPI channel to be added.
 * @param[in] group   Channel group in which to include the channel.
 *
 * @retval NRFX_SUCCESS             The channel was successfully included.
 * @retval NRFX_ERROR_INVALID_PARAM Group is not an application group or channel is not an
 *                                  application channel.
 * @retval NRFX_ERROR_INVALID_STATE Group is not an allocated group.
 */
__STATIC_INLINE nrfx_err_t nrfx_ppi_channel_include_in_group(nrf_ppi_channel_t       channel,
                                                             nrf_ppi_channel_group_t group)
{
    return nrfx_ppi_channels_include_in_group(nrfx_ppi_channel_to_mask(channel), group);
}

/**
 * @brief Function for removing multiple PPI channels from a channel group.
 *
 * @param[in] channel_mask PPI channels to be removed.
 * @param[in] group        Channel group from which to remove the channels.
 *
 * @retval NRFX_SUCCESS             The channel was successfully removed.
 * @retval NRFX_ERROR_INVALID_PARAM Group is not an application group or channels are not an
 *                                  application channels.
 * @retval NRFX_ERROR_INVALID_STATE Group is not an allocated group.
 */
nrfx_err_t nrfx_ppi_channels_remove_from_group(uint32_t                channel_mask,
                                               nrf_ppi_channel_group_t group);

/**
 * @brief Function for removing a single PPI channel from a channel group.
 *
 * @param[in] channel PPI channel to be removed.
 * @param[in] group   Channel group from which to remove the channel.
 *
 * @retval NRFX_SUCCESS             The channel was successfully removed.
 * @retval NRFX_ERROR_INVALID_PARAM Group is not an application group or channel is not an
 *                                  application channel.
 * @retval NRFX_ERROR_INVALID_STATE Group is not an allocated group.
 */
__STATIC_INLINE nrfx_err_t nrfx_ppi_channel_remove_from_group(nrf_ppi_channel_t       channel,
                                                              nrf_ppi_channel_group_t group)
{
    return nrfx_ppi_channels_remove_from_group(nrfx_ppi_channel_to_mask(channel), group);
}

/**
 * @brief Function for clearing a PPI channel group.
 *
 * @param[in] group Channel group to be cleared.
 *
 * @retval NRFX_SUCCESS             The group was successfully cleared.
 * @retval NRFX_ERROR_INVALID_PARAM Group is not an application group.
 * @retval NRFX_ERROR_INVALID_STATE Group is not an allocated group.
 */
__STATIC_INLINE nrfx_err_t nrfx_ppi_group_clear(nrf_ppi_channel_group_t group)
{
    return nrfx_ppi_channels_remove_from_group(NRFX_PPI_ALL_APP_CHANNELS_MASK, group);
}

/**
 * @brief Function for enabling a PPI channel group.
 *
 * @param[in] group Channel group to be enabled.
 *
 * @retval NRFX_SUCCESS             The group was successfully enabled.
 * @retval NRFX_ERROR_INVALID_PARAM Group is not an application group.
 * @retval NRFX_ERROR_INVALID_STATE Group is not an allocated group.
 */
nrfx_err_t nrfx_ppi_group_enable(nrf_ppi_channel_group_t group);

/**
 * @brief Function for disabling a PPI channel group.
 *
 * @param[in] group Channel group to be disabled.
 *
 * @retval NRFX_SUCCESS             The group was successfully disabled.
 * @retval NRFX_ERROR_INVALID_PARAM Group is not an application group.
 * @retval NRFX_ERROR_INVALID_STATE Group is not an allocated group.
 */
nrfx_err_t nrfx_ppi_group_disable(nrf_ppi_channel_group_t group);

/**
 * @brief Function for getting the address of a PPI task.
 *
 * @param[in] task Task.
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrfx_ppi_task_addr_get(nrf_ppi_task_t task)
{
    return (uint32_t) nrf_ppi_task_address_get(task);
}

/**
 * @brief Function for getting the address of the enable task of a PPI group.
 *
 * @param[in] group PPI channel group
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrfx_ppi_task_addr_group_enable_get(nrf_ppi_channel_group_t group)
{
    return (uint32_t) nrf_ppi_task_group_enable_address_get(group);
}

/**
 * @brief Function for getting the address of the enable task of a PPI group.
 *
 * @param[in] group PPI channel group
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrfx_ppi_task_addr_group_disable_get(nrf_ppi_channel_group_t group)
{
    return (uint32_t) nrf_ppi_task_group_disable_address_get(group);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_PPI_H__
