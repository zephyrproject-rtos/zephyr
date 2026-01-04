/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_COMMON_H_
#define ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_COMMON_H_

/**
 * @brief Espressif RMT common functions.
 * @defgroup espressif_rmt_common_interface Espressif RMT Common interface
 * @ingroup misc_interfaces
 * @{
 */

#include <stdint.h>
#include "rmt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RMT carrier wave configuration (for either modulation or demodulation)
 */
typedef struct {
    uint32_t frequency_hz; /**< Carrier wave frequency, in Hz, 0 means disabling the carrier */
    float duty_cycle;      /**< Carrier wave duty cycle (0~100%) */
    struct {
        uint32_t polarity_active_low: 1; /**< Specify the polarity of carrier, by default it's modulated to base signal's high level */
        uint32_t always_on: 1;           /**< If set, the carrier can always exist even there's not transfer undergoing */
    } flags;                             /**< Carrier config flags */
} rmt_carrier_config_t;

/**
 * @brief Delete an RMT channel.
 *
 * @param channel RMT generic channel that created by `rmt_new_tx_channel()` or `rmt_new_rx_channel()`.
 * @retval 0 If successful.
 * @retval -EINVAL Delete RMT channel failed because of invalid argument.
 * @retval -ENODEV Delete RMT channel failed because it is still in working.
 * @retval -ENODEV Delete RMT channel failed because of other error.
 */
int rmt_del_channel(rmt_channel_handle_t channel);

/**
 * @brief Apply modulation feature for TX channel or demodulation feature for RX channel.
 *
 * @param channel RMT generic channel that created by `rmt_new_tx_channel()` or `rmt_new_rx_channel()`.
 * @param config Carrier configuration. Specially, a NULL config means to disable the carrier modulation or demodulation feature.
 * @retval 0 If successful.
 * @retval -EINVAL Apply carrier configuration failed because of invalid argument
 * @retval -ENODEV Apply carrier configuration failed because of other error
 */
int rmt_apply_carrier(rmt_channel_handle_t channel, const rmt_carrier_config_t *config);

/**
 * @brief Enable the RMT channel.
 *
 * @note This function will acquire a PM lock that might be installed during channel allocation.
 *
 * @param channel RMT generic channel that created by `rmt_new_tx_channel()` or `rmt_new_rx_channel()`.
 * @retval 0 If successful.
 * @retval -EINVAL if enabling RMT channel failed because of invalid argument.
 * @retval -ENODEV if enabling RMT channel failed because it's enabled already.
 * @retval -ENODEV if enabling RMT channel failed because of other error.
 */
int rmt_enable(rmt_channel_handle_t channel);

/**
 * @brief Disable the RMT channel.
 *
 * @note This function will release a PM lock that might be installed during channel allocation.
 *
 * @param channel RMT generic channel that created by `rmt_new_tx_channel()` or `rmt_new_rx_channel()`.
 * @retval 0 If successful.
 * @retval -EINVAL if disabling RMT channel failed because of invalid argument.
 * @retval -ENODEV if disabling RMT channel failed because it's not enabled yet.
 * @retval -ENODEV if disabling RMT channel failed because of other error.
 */
int rmt_disable(rmt_channel_handle_t channel);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_ESPRESSIF_RMT_RMT_COMMON_H_ */
