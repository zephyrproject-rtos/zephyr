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

#ifndef NRF_PPI_H__
#define NRF_PPI_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_ppi_hal PPI HAL
 * @{
 * @ingroup nrf_ppi
 * @brief   Hardware access layer for managing the Programmable Peripheral Interconnect (PPI)
 *          channels.
 */

#define NRF_PPI_TASK_SET    (1UL)

/**
 * @enum nrf_ppi_channel_t
 * @brief PPI channels.
 */
typedef enum
{
    NRF_PPI_CHANNEL0  = PPI_CHEN_CH0_Pos,  /**< Channel 0. */
    NRF_PPI_CHANNEL1  = PPI_CHEN_CH1_Pos,  /**< Channel 1. */
    NRF_PPI_CHANNEL2  = PPI_CHEN_CH2_Pos,  /**< Channel 2. */
    NRF_PPI_CHANNEL3  = PPI_CHEN_CH3_Pos,  /**< Channel 3. */
    NRF_PPI_CHANNEL4  = PPI_CHEN_CH4_Pos,  /**< Channel 4. */
    NRF_PPI_CHANNEL5  = PPI_CHEN_CH5_Pos,  /**< Channel 5. */
    NRF_PPI_CHANNEL6  = PPI_CHEN_CH6_Pos,  /**< Channel 6. */
    NRF_PPI_CHANNEL7  = PPI_CHEN_CH7_Pos,  /**< Channel 7. */
    NRF_PPI_CHANNEL8  = PPI_CHEN_CH8_Pos,  /**< Channel 8. */
    NRF_PPI_CHANNEL9  = PPI_CHEN_CH9_Pos,  /**< Channel 9. */
    NRF_PPI_CHANNEL10 = PPI_CHEN_CH10_Pos, /**< Channel 10. */
    NRF_PPI_CHANNEL11 = PPI_CHEN_CH11_Pos, /**< Channel 11. */
    NRF_PPI_CHANNEL12 = PPI_CHEN_CH12_Pos, /**< Channel 12. */
    NRF_PPI_CHANNEL13 = PPI_CHEN_CH13_Pos, /**< Channel 13. */
    NRF_PPI_CHANNEL14 = PPI_CHEN_CH14_Pos, /**< Channel 14. */
    NRF_PPI_CHANNEL15 = PPI_CHEN_CH15_Pos, /**< Channel 15. */
#if (PPI_CH_NUM > 16) || defined(__NRFX_DOXYGEN__)
    NRF_PPI_CHANNEL16 = PPI_CHEN_CH16_Pos, /**< Channel 16. */
    NRF_PPI_CHANNEL17 = PPI_CHEN_CH17_Pos, /**< Channel 17. */
    NRF_PPI_CHANNEL18 = PPI_CHEN_CH18_Pos, /**< Channel 18. */
    NRF_PPI_CHANNEL19 = PPI_CHEN_CH19_Pos, /**< Channel 19. */
#endif
    NRF_PPI_CHANNEL20 = PPI_CHEN_CH20_Pos, /**< Channel 20. */
    NRF_PPI_CHANNEL21 = PPI_CHEN_CH21_Pos, /**< Channel 21. */
    NRF_PPI_CHANNEL22 = PPI_CHEN_CH22_Pos, /**< Channel 22. */
    NRF_PPI_CHANNEL23 = PPI_CHEN_CH23_Pos, /**< Channel 23. */
    NRF_PPI_CHANNEL24 = PPI_CHEN_CH24_Pos, /**< Channel 24. */
    NRF_PPI_CHANNEL25 = PPI_CHEN_CH25_Pos, /**< Channel 25. */
    NRF_PPI_CHANNEL26 = PPI_CHEN_CH26_Pos, /**< Channel 26. */
    NRF_PPI_CHANNEL27 = PPI_CHEN_CH27_Pos, /**< Channel 27. */
    NRF_PPI_CHANNEL28 = PPI_CHEN_CH28_Pos, /**< Channel 28. */
    NRF_PPI_CHANNEL29 = PPI_CHEN_CH29_Pos, /**< Channel 29. */
    NRF_PPI_CHANNEL30 = PPI_CHEN_CH30_Pos, /**< Channel 30. */
    NRF_PPI_CHANNEL31 = PPI_CHEN_CH31_Pos  /**< Channel 31. */
} nrf_ppi_channel_t;

/**
 * @enum nrf_ppi_channel_group_t
 * @brief PPI channel groups.
 */
typedef enum
{
    NRF_PPI_CHANNEL_GROUP0 = 0, /**< Channel group 0. */
    NRF_PPI_CHANNEL_GROUP1 = 1, /**< Channel group 1. */
    NRF_PPI_CHANNEL_GROUP2 = 2, /**< Channel group 2. */
    NRF_PPI_CHANNEL_GROUP3 = 3, /**< Channel group 3. */
#if (PPI_GROUP_NUM > 4) || defined(__NRFX_DOXYGEN__)
    NRF_PPI_CHANNEL_GROUP4 = 4, /**< Channel group 4. */
    NRF_PPI_CHANNEL_GROUP5 = 5  /**< Channel group 5. */
#endif
} nrf_ppi_channel_group_t;

/**
 * @enum nrf_ppi_channel_include_t
 * @brief Definition of which PPI channels belong to a group.
 */
typedef enum
{
    NRF_PPI_CHANNEL_EXCLUDE = PPI_CHG_CH0_Excluded, /**< Channel excluded from a group. */
    NRF_PPI_CHANNEL_INCLUDE = PPI_CHG_CH0_Included  /**< Channel included in a group. */
} nrf_ppi_channel_include_t;

/**
 * @enum nrf_ppi_channel_enable_t
 * @brief Definition if a PPI channel is enabled.
 */
typedef enum
{
    NRF_PPI_CHANNEL_DISABLED = PPI_CHEN_CH0_Disabled, /**< Channel disabled. */
    NRF_PPI_CHANNEL_ENABLED  = PPI_CHEN_CH0_Enabled   /**< Channel enabled. */
} nrf_ppi_channel_enable_t;

/**
 * @enum nrf_ppi_task_t
 * @brief PPI tasks.
 */
typedef enum
{
    /*lint -save -e30 -esym(628,__INTADDR__)*/
    NRF_PPI_TASK_CHG0_EN  = offsetof(NRF_PPI_Type, TASKS_CHG[0].EN),  /**< Task for enabling channel group 0 */
    NRF_PPI_TASK_CHG0_DIS = offsetof(NRF_PPI_Type, TASKS_CHG[0].DIS), /**< Task for disabling channel group 0 */
    NRF_PPI_TASK_CHG1_EN  = offsetof(NRF_PPI_Type, TASKS_CHG[1].EN),  /**< Task for enabling channel group 1 */
    NRF_PPI_TASK_CHG1_DIS = offsetof(NRF_PPI_Type, TASKS_CHG[1].DIS), /**< Task for disabling channel group 1 */
    NRF_PPI_TASK_CHG2_EN  = offsetof(NRF_PPI_Type, TASKS_CHG[2].EN),  /**< Task for enabling channel group 2 */
    NRF_PPI_TASK_CHG2_DIS = offsetof(NRF_PPI_Type, TASKS_CHG[2].DIS), /**< Task for disabling channel group 2 */
    NRF_PPI_TASK_CHG3_EN  = offsetof(NRF_PPI_Type, TASKS_CHG[3].EN),  /**< Task for enabling channel group 3 */
    NRF_PPI_TASK_CHG3_DIS = offsetof(NRF_PPI_Type, TASKS_CHG[3].DIS), /**< Task for disabling channel group 3 */
#if (PPI_GROUP_NUM > 4) || defined(__NRFX_DOXYGEN__)
    NRF_PPI_TASK_CHG4_EN  = offsetof(NRF_PPI_Type, TASKS_CHG[4].EN),  /**< Task for enabling channel group 4 */
    NRF_PPI_TASK_CHG4_DIS = offsetof(NRF_PPI_Type, TASKS_CHG[4].DIS), /**< Task for disabling channel group 4 */
    NRF_PPI_TASK_CHG5_EN  = offsetof(NRF_PPI_Type, TASKS_CHG[5].EN),  /**< Task for enabling channel group 5 */
    NRF_PPI_TASK_CHG5_DIS = offsetof(NRF_PPI_Type, TASKS_CHG[5].DIS)  /**< Task for disabling channel group 5 */
#endif
    /*lint -restore*/
} nrf_ppi_task_t;

/**
 * @brief Function for enabling a given PPI channel.
 *
 * @details This function enables only one channel.
 *
 * @param[in] channel Channel to enable.
 *
 *  */
__STATIC_INLINE void nrf_ppi_channel_enable(nrf_ppi_channel_t channel);

/**
 * @brief Function for disabling a given PPI channel.
 *
 * @details This function disables only one channel.
 *
 * @param[in] channel Channel to disable.
 */
__STATIC_INLINE void nrf_ppi_channel_disable(nrf_ppi_channel_t channel);

/**
 * @brief Function for checking if a given PPI channel is enabled.
 *
 * @details This function checks only one channel.
 *
 * @param[in] channel Channel to check.
 *
 * @retval     NRF_PPI_CHANNEL_ENABLED     If the channel is enabled.
 * @retval     NRF_PPI_CHANNEL_DISABLED    If the channel is not enabled.
 *
 */
__STATIC_INLINE nrf_ppi_channel_enable_t nrf_ppi_channel_enable_get(nrf_ppi_channel_t channel);

/**
 * @brief Function for disabling all PPI channels.
 */
__STATIC_INLINE void nrf_ppi_channel_disable_all(void);

/**
 * @brief Function for enabling multiple PPI channels.
 *
 * @param[in] mask Channel mask.
 */
__STATIC_INLINE void nrf_ppi_channels_enable(uint32_t mask);

/**
 * @brief Function for disabling multiple PPI channels.
 *
 * @param[in] mask Channel mask.
 */
__STATIC_INLINE void nrf_ppi_channels_disable(uint32_t mask);

/**
 * @brief Function for setting up event and task endpoints for a given PPI channel.
 *
 * @param[in] eep Event register address.
 *
 * @param[in] tep Task register address.
 *
 * @param[in] channel Channel to which the given endpoints are assigned.
 */
__STATIC_INLINE void nrf_ppi_channel_endpoint_setup(nrf_ppi_channel_t channel,
                                                    uint32_t          eep,
                                                    uint32_t          tep);

#if defined(PPI_FEATURE_FORKS_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting up task endpoint for a given PPI fork.
 *
 * @param[in] fork_tep Task register address.
 *
 * @param[in] channel Channel to which the given fork endpoint is assigned.
 */
__STATIC_INLINE void nrf_ppi_fork_endpoint_setup(nrf_ppi_channel_t channel,
                                                 uint32_t          fork_tep);

/**
 * @brief Function for setting up event and task endpoints for a given PPI channel and fork.
 *
 * @param[in] eep Event register address.
 *
 * @param[in] tep Task register address.
 *
 * @param[in] fork_tep Fork task register address (register value).
 *
 * @param[in] channel Channel to which the given endpoints are assigned.
 */
__STATIC_INLINE void nrf_ppi_channel_and_fork_endpoint_setup(nrf_ppi_channel_t channel,
                                                             uint32_t          eep,
                                                             uint32_t          tep,
                                                             uint32_t          fork_tep);
#endif

/**
 * @brief Function for including a PPI channel in a channel group.
 *
 * @details This function adds only one channel to the group.
 *
 * @param[in] channel       Channel to be included in the group.
 *
 * @param[in] channel_group Channel group.
 *
 */
__STATIC_INLINE void nrf_ppi_channel_include_in_group(nrf_ppi_channel_t       channel,
                                                      nrf_ppi_channel_group_t channel_group);

/**
 * @brief Function for including multiple PPI channels in a channel group.
 *
 * @details This function adds all specified channels to the group.
 *
 * @param[in] channel_mask  Channels to be included in the group.
 *
 * @param[in] channel_group Channel group.
 *
 */
__STATIC_INLINE void nrf_ppi_channels_include_in_group(uint32_t                channel_mask,
                                                       nrf_ppi_channel_group_t channel_group);

/**
 * @brief Function for removing a PPI channel from a channel group.
 *
 * @details This function removes only one channel from the group.
 *
 * @param[in] channel       Channel to be removed from the group.
 *
 * @param[in] channel_group Channel group.
 */
__STATIC_INLINE void nrf_ppi_channel_remove_from_group(nrf_ppi_channel_t       channel,
                                                       nrf_ppi_channel_group_t channel_group);

/**
 * @brief Function for removing multiple PPI channels from a channel group.
 *
 * @details This function removes all specified channels from the group.
 *
 * @param[in] channel_mask  Channels to be removed from the group.
 *
 * @param[in] channel_group Channel group.
 */
__STATIC_INLINE void nrf_ppi_channels_remove_from_group(uint32_t                channel_mask,
                                                        nrf_ppi_channel_group_t channel_group);

/**
 * @brief Function for removing all PPI channels from a channel group.
 *
 * @param[in] group Channel group.
 *
 */
__STATIC_INLINE void nrf_ppi_channel_group_clear(nrf_ppi_channel_group_t group);

/**
 * @brief Function for enabling a channel group.
 *
 * @param[in] group Channel group.
 *
 */
__STATIC_INLINE void nrf_ppi_group_enable(nrf_ppi_channel_group_t group);

/**
 * @brief Function for disabling a channel group.
 *
 * @param[in] group Channel group.
 *
 */
__STATIC_INLINE void nrf_ppi_group_disable(nrf_ppi_channel_group_t group);

/**
 * @brief Function for setting a PPI task.
 *
 * @param[in] ppi_task PPI task to set.
 */
__STATIC_INLINE void nrf_ppi_task_trigger(nrf_ppi_task_t ppi_task);

/**
 * @brief Function for returning the address of a specific PPI task register.
 *
 * @param[in] ppi_task PPI task.
 */
__STATIC_INLINE uint32_t * nrf_ppi_task_address_get(nrf_ppi_task_t ppi_task);

/**
 * @brief Function for returning the PPI enable task address of a specific group.
 *
 * @param[in] group  PPI group.
 */
__STATIC_INLINE uint32_t * nrf_ppi_task_group_enable_address_get(nrf_ppi_channel_group_t group);

/**
 * @brief Function for returning the PPI disable task address of a specific group.
 *
 * @param[in] group  PPI group.
 */
__STATIC_INLINE uint32_t * nrf_ppi_task_group_disable_address_get(nrf_ppi_channel_group_t group);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_ppi_channel_enable(nrf_ppi_channel_t channel)
{
    NRF_PPI->CHENSET = PPI_CHENSET_CH0_Set << ((uint32_t) channel);
}

__STATIC_INLINE void nrf_ppi_channel_disable(nrf_ppi_channel_t channel)
{
    NRF_PPI->CHENCLR = PPI_CHENCLR_CH0_Clear << ((uint32_t) channel);
}

__STATIC_INLINE nrf_ppi_channel_enable_t nrf_ppi_channel_enable_get(nrf_ppi_channel_t channel)
{
    if (NRF_PPI->CHEN & (PPI_CHEN_CH0_Msk << ((uint32_t) channel)))
    {
        return NRF_PPI_CHANNEL_ENABLED;
    }
    else
    {
        return NRF_PPI_CHANNEL_DISABLED;
    }
}

__STATIC_INLINE void nrf_ppi_channel_disable_all(void)
{
    NRF_PPI->CHENCLR = ((uint32_t)0xFFFFFFFFuL);
}

__STATIC_INLINE void nrf_ppi_channels_enable(uint32_t mask)
{
    NRF_PPI->CHENSET = mask;
}

__STATIC_INLINE void nrf_ppi_channels_disable(uint32_t mask)
{
    NRF_PPI->CHENCLR = mask;
}

__STATIC_INLINE void nrf_ppi_channel_endpoint_setup(nrf_ppi_channel_t channel,
                                                    uint32_t          eep,
                                                    uint32_t          tep)
{
    NRF_PPI->CH[(uint32_t) channel].EEP = eep;
    NRF_PPI->CH[(uint32_t) channel].TEP = tep;
}

#if defined(PPI_FEATURE_FORKS_PRESENT)

__STATIC_INLINE void nrf_ppi_fork_endpoint_setup(nrf_ppi_channel_t channel,
                                                 uint32_t          fork_tep)
{
    NRF_PPI->FORK[(uint32_t) channel].TEP = fork_tep;
}

__STATIC_INLINE void nrf_ppi_channel_and_fork_endpoint_setup(nrf_ppi_channel_t channel,
                                                             uint32_t          eep,
                                                             uint32_t          tep,
                                                             uint32_t          fork_tep)
{
    nrf_ppi_channel_endpoint_setup(channel, eep, tep);
    nrf_ppi_fork_endpoint_setup(channel, fork_tep);
}
#endif

__STATIC_INLINE void nrf_ppi_channel_include_in_group(nrf_ppi_channel_t       channel,
                                                      nrf_ppi_channel_group_t channel_group)
{
    NRF_PPI->CHG[(uint32_t) channel_group] =
        NRF_PPI->CHG[(uint32_t) channel_group] | (PPI_CHG_CH0_Included << ((uint32_t)  channel));
}

__STATIC_INLINE void nrf_ppi_channels_include_in_group(uint32_t                channel_mask,
                                                       nrf_ppi_channel_group_t channel_group)
{
    NRF_PPI->CHG[(uint32_t) channel_group] =
        NRF_PPI->CHG[(uint32_t) channel_group] | (channel_mask);
}

__STATIC_INLINE void nrf_ppi_channel_remove_from_group(nrf_ppi_channel_t       channel,
                                                       nrf_ppi_channel_group_t channel_group)
{
    NRF_PPI->CHG[(uint32_t) channel_group] =
        NRF_PPI->CHG[(uint32_t) channel_group] & ~(PPI_CHG_CH0_Included << ((uint32_t) channel));
}

__STATIC_INLINE void nrf_ppi_channels_remove_from_group(uint32_t                channel_mask,
                                                        nrf_ppi_channel_group_t channel_group)
{
    NRF_PPI->CHG[(uint32_t) channel_group] =
        NRF_PPI->CHG[(uint32_t) channel_group] & ~(channel_mask);
}

__STATIC_INLINE void nrf_ppi_channel_group_clear(nrf_ppi_channel_group_t group)
{
    NRF_PPI->CHG[(uint32_t) group] = 0;
}

__STATIC_INLINE void nrf_ppi_group_enable(nrf_ppi_channel_group_t group)
{
    NRF_PPI->TASKS_CHG[(uint32_t) group].EN = NRF_PPI_TASK_SET;
}

__STATIC_INLINE void nrf_ppi_group_disable(nrf_ppi_channel_group_t group)
{
    NRF_PPI->TASKS_CHG[(uint32_t) group].DIS = NRF_PPI_TASK_SET;
}

__STATIC_INLINE void nrf_ppi_task_trigger(nrf_ppi_task_t ppi_task)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_PPI_BASE + (uint32_t) ppi_task)) = NRF_PPI_TASK_SET;
}

__STATIC_INLINE uint32_t * nrf_ppi_task_address_get(nrf_ppi_task_t ppi_task)
{
    return (uint32_t *) ((uint8_t *) NRF_PPI_BASE + (uint32_t) ppi_task);
}

__STATIC_INLINE uint32_t * nrf_ppi_task_group_enable_address_get(nrf_ppi_channel_group_t group)
{
    return (uint32_t *) &NRF_PPI->TASKS_CHG[(uint32_t) group].EN;
}

__STATIC_INLINE uint32_t * nrf_ppi_task_group_disable_address_get(nrf_ppi_channel_group_t group)
{
    return (uint32_t *) &NRF_PPI->TASKS_CHG[(uint32_t) group].DIS;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_PPI_H__
