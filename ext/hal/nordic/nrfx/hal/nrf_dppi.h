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

#ifndef NRF_DPPI_H__
#define NRF_DPPI_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_dppi_hal DPPI Controller HAL
 * @{
 * @ingroup nrf_dppi
 * @brief   Hardware access layer for managing the Distributed Programmable Peripheral
 *          Interconnect Controller (DPPIC).
 */

/** @brief DPPI channel groups. */
typedef enum
{
    NRF_DPPI_CHANNEL_GROUP0 = 0, /**< Channel group 0. */
    NRF_DPPI_CHANNEL_GROUP1 = 1, /**< Channel group 1. */
    NRF_DPPI_CHANNEL_GROUP2 = 2, /**< Channel group 2. */
    NRF_DPPI_CHANNEL_GROUP3 = 3, /**< Channel group 3. */
    NRF_DPPI_CHANNEL_GROUP4 = 4, /**< Channel group 4. */
    NRF_DPPI_CHANNEL_GROUP5 = 5  /**< Channel group 5. */
} nrf_dppi_channel_group_t;

/** @brief DPPI tasks. */
typedef enum
{
    NRF_DPPI_TASK_CHG0_EN  = offsetof(NRF_DPPIC_Type, TASKS_CHG[0].EN),  /**< Enable channel group 0. */
    NRF_DPPI_TASK_CHG0_DIS = offsetof(NRF_DPPIC_Type, TASKS_CHG[0].DIS), /**< Disable channel group 0. */
    NRF_DPPI_TASK_CHG1_EN  = offsetof(NRF_DPPIC_Type, TASKS_CHG[1].EN),  /**< Enable channel group 1. */
    NRF_DPPI_TASK_CHG1_DIS = offsetof(NRF_DPPIC_Type, TASKS_CHG[1].DIS), /**< Disable channel group 1. */
    NRF_DPPI_TASK_CHG2_EN  = offsetof(NRF_DPPIC_Type, TASKS_CHG[2].EN),  /**< Enable channel group 2. */
    NRF_DPPI_TASK_CHG2_DIS = offsetof(NRF_DPPIC_Type, TASKS_CHG[2].DIS), /**< Disable channel group 2. */
    NRF_DPPI_TASK_CHG3_EN  = offsetof(NRF_DPPIC_Type, TASKS_CHG[3].EN),  /**< Enable channel group 3. */
    NRF_DPPI_TASK_CHG3_DIS = offsetof(NRF_DPPIC_Type, TASKS_CHG[3].DIS), /**< Disable channel group 3. */
    NRF_DPPI_TASK_CHG4_EN  = offsetof(NRF_DPPIC_Type, TASKS_CHG[4].EN),  /**< Enable channel group 4. */
    NRF_DPPI_TASK_CHG4_DIS = offsetof(NRF_DPPIC_Type, TASKS_CHG[4].DIS), /**< Disable channel group 4. */
    NRF_DPPI_TASK_CHG5_EN  = offsetof(NRF_DPPIC_Type, TASKS_CHG[5].EN),  /**< Enable channel group 5. */
    NRF_DPPI_TASK_CHG5_DIS = offsetof(NRF_DPPIC_Type, TASKS_CHG[5].DIS)  /**< Disable channel group 5. */
} nrf_dppi_task_t;

/**
 * @brief Function for activating a DPPI task.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] dppi_task Task to be activated.
 */
__STATIC_INLINE void nrf_dppi_task_trigger(NRF_DPPIC_Type * p_reg, nrf_dppi_task_t dppi_task);

/**
 * @brief Function for getting the address of the specified DPPI task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t nrf_dppi_task_address_get(NRF_DPPIC_Type const * p_reg,
                                                   nrf_dppi_task_t        task);

/**
 * @brief Function for checking the state of a specific DPPI channel.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] channel Channel to be checked.
 *
 * @retval true  The channel is enabled.
 * @retval false The channel is not enabled.
 */
__STATIC_INLINE bool nrf_dppi_channel_check(NRF_DPPIC_Type const * p_reg, uint8_t channel);

/**
 * @brief Function for enabling multiple DPPI channels.
 *
 * The bits in @c mask value correspond to particular channels. It means that
 * writing 1 to bit 0 enables channel 0, writing 1 to bit 1 enables channel 1 etc.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Channel mask.
 */
__STATIC_INLINE void nrf_dppi_channels_enable(NRF_DPPIC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling multiple DPPI channels.
 *
 * The bits in @c mask value correspond to particular channels. It means that
 * writing 1 to bit 0 disables channel 0, writing 1 to bit 1 disables channel 1 etc.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Channel mask.
 */
__STATIC_INLINE void nrf_dppi_channels_disable(NRF_DPPIC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling all DPPI channels.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
__STATIC_INLINE void nrf_dppi_channels_disable_all(NRF_DPPIC_Type * p_reg);

/**
 * @brief Function for setting the subscribe configuration for a given
 *        DPPI task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
__STATIC_INLINE void nrf_dppi_subscribe_set(NRF_DPPIC_Type * p_reg,
                                            nrf_dppi_task_t  task,
                                            uint8_t          channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        DPPI task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
__STATIC_INLINE void nrf_dppi_subscribe_clear(NRF_DPPIC_Type * p_reg, nrf_dppi_task_t task);

/**
 * @brief Function for including multiple DPPI channels in a channel group.
 *
 * @details This function adds all specified channels to the group.
 * The bits in @p channel_mask value correspond to particular channels. It means that
 * writing 1 to bit 0 includes channel 0, writing 1 to bit 1 includes channel 1 etc.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] channel_mask  Channels to be included in the group.
 * @param[in] channel_group Channel group.
 */
__STATIC_INLINE void nrf_dppi_channels_include_in_group(NRF_DPPIC_Type *         p_reg,
                                                        uint32_t                 channel_mask,
                                                        nrf_dppi_channel_group_t channel_group);

/**
 * @brief Function for removing multiple DPPI channels from a channel group.
 *
 * @details This function removes all specified channels from the group.
 * The bits in @c channel_mask value correspond to particular channels. It means that
 * writing 1 to bit 0 removes channel 0, writing 1 to bit 1 removes channel 1 etc.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] channel_mask  Channels to be removed from the group.
 * @param[in] channel_group Channel group.
 */
__STATIC_INLINE void nrf_dppi_channels_remove_from_group(NRF_DPPIC_Type *         p_reg,
                                                         uint32_t                 channel_mask,
                                                         nrf_dppi_channel_group_t channel_group);

/**
 * @brief Function for removing all DPPI channels from a channel group.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] group Channel group.
 */
__STATIC_INLINE void nrf_dppi_group_clear(NRF_DPPIC_Type *         p_reg,
                                          nrf_dppi_channel_group_t group);

/**
 * @brief Function for enabling a channel group.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] group Channel group.
 */
__STATIC_INLINE void nrf_dppi_group_enable(NRF_DPPIC_Type *         p_reg,
                                           nrf_dppi_channel_group_t group);

/**
 * @brief Function for disabling a channel group.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] group Channel group.
 */
__STATIC_INLINE void nrf_dppi_group_disable(NRF_DPPIC_Type *         p_reg,
                                            nrf_dppi_channel_group_t group);

/**
 * @brief Function for getting the ENABLE task associated with the specified channel group.
 *
 * @param[in] index Channel group index.
 *
 * @return Requested ENABLE task.
 */
__STATIC_INLINE nrf_dppi_task_t nrf_dppi_group_enable_task_get(uint8_t index);

/**
 * @brief Function for getting the DISABLE task associated with the specified channel group.
 *
 * @param[in] index Channel group index.
 *
 * @return Requested DISABLE task.
 */
__STATIC_INLINE nrf_dppi_task_t nrf_dppi_group_disable_task_get(uint8_t index);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_dppi_task_trigger(NRF_DPPIC_Type * p_reg, nrf_dppi_task_t dppi_task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) dppi_task)) = 1;
}

__STATIC_INLINE uint32_t nrf_dppi_task_address_get(NRF_DPPIC_Type const * p_reg,
                                                   nrf_dppi_task_t        task)
{
    return (uint32_t) ((uint8_t *) p_reg + (uint32_t ) task);
}

__STATIC_INLINE bool nrf_dppi_channel_check(NRF_DPPIC_Type const * p_reg, uint8_t channel)
{
    return ((p_reg->CHEN & (DPPIC_CHEN_CH0_Enabled << (DPPIC_CHEN_CH0_Pos + channel))) != 0);
}

__STATIC_INLINE void nrf_dppi_channels_disable_all(NRF_DPPIC_Type * p_reg)
{
    p_reg->CHENCLR = 0xFFFFFFFFuL;
}

__STATIC_INLINE void nrf_dppi_channels_enable(NRF_DPPIC_Type * p_reg, uint32_t mask)
{
    p_reg->CHENSET = mask;
}

__STATIC_INLINE void nrf_dppi_channels_disable(NRF_DPPIC_Type * p_reg, uint32_t mask)
{
    p_reg->CHENCLR = mask;
}

__STATIC_INLINE void nrf_dppi_subscribe_set(NRF_DPPIC_Type * p_reg,
                                            nrf_dppi_task_t  task,
                                            uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | DPPIC_SUBSCRIBE_CHG_EN_EN_Msk);
}

__STATIC_INLINE void nrf_dppi_subscribe_clear(NRF_DPPIC_Type * p_reg, nrf_dppi_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

__STATIC_INLINE void nrf_dppi_channels_include_in_group(NRF_DPPIC_Type *         p_reg,
                                                        uint32_t                 channel_mask,
                                                        nrf_dppi_channel_group_t channel_group)
{
    p_reg->CHG[(uint32_t) channel_group] =
        p_reg->CHG[(uint32_t) channel_group] | (channel_mask);
}

__STATIC_INLINE void nrf_dppi_channels_remove_from_group(NRF_DPPIC_Type *         p_reg,
                                                         uint32_t                 channel_mask,
                                                         nrf_dppi_channel_group_t channel_group)
{
    p_reg->CHG[(uint32_t) channel_group] =
        p_reg->CHG[(uint32_t) channel_group] & ~(channel_mask);
}

__STATIC_INLINE void nrf_dppi_group_clear(NRF_DPPIC_Type *         p_reg,
                                                  nrf_dppi_channel_group_t group)
{
    p_reg->CHG[(uint32_t) group] = 0;
}

__STATIC_INLINE void nrf_dppi_group_enable(NRF_DPPIC_Type * p_reg, nrf_dppi_channel_group_t group)
{
    p_reg->TASKS_CHG[(uint32_t) group].EN = 1;
}

__STATIC_INLINE void nrf_dppi_group_disable(NRF_DPPIC_Type *         p_reg,
                                            nrf_dppi_channel_group_t group)
{
    p_reg->TASKS_CHG[(uint32_t) group].DIS = 1;
}

__STATIC_INLINE nrf_dppi_task_t nrf_dppi_group_enable_task_get(uint8_t index)
{
    NRFX_ASSERT(index < NRFX_ARRAY_SIZE(NRF_DPPIC->TASKS_CHG));
    return (nrf_dppi_task_t)NRFX_OFFSETOF(NRF_DPPIC_Type, TASKS_CHG[index].EN);
}

__STATIC_INLINE nrf_dppi_task_t nrf_dppi_group_disable_task_get(uint8_t index)
{
    NRFX_ASSERT(index < NRFX_ARRAY_SIZE(NRF_DPPIC->TASKS_CHG));
    return (nrf_dppi_task_t)NRFX_OFFSETOF(NRF_DPPIC_Type, TASKS_CHG[index].DIS);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_DPPIC_H__
