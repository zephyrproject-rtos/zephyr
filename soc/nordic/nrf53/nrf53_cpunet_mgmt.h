/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Nordic Semiconductor nRF53 processors family management helper for the network CPU.
 */

#ifndef NRF53_CPUNET_MGMT_H__
#define NRF53_CPUNET_MGMT_H__

#include <stdbool.h>

/**
 * @brief Enables or disables nRF53 network CPU.
 *
 * This function shall be used to control the network CPU exclusively. Internally, it keeps track of
 * the requests to enable or disable nRF53 network CPU. It guarantees to enable the network CPU if
 * at least one user requests it and to keep it enabled until all users release it.
 *
 * In conseqeuence, if @p on equals @c true then the network CPU is guaranteed to be enabled when
 * this function returns. If @p on equals @c false then nothing can be inferred about the state of
 * the network CPU when the function returns.
 *
 * @param on indicates whether the network CPU shall be powered on or off.
 */
void nrf53_cpunet_enable(bool on);

/**
 * @brief Configure the security setting of the network CPU and then enable it.
 */
void nrf53_cpunet_init(void);

/**
 * @brief Initialize the CPU network management system for the nRF5340.
 *
 * This function sets up an on-off manager for managing the network core of the nRF5340.
 * It configures the necessary transitions for starting and stopping the network core.
 *
 * The function also ensures proper handling of GPIO pin forwarding when the
 * `CONFIG_SOC_NRF_GPIO_FORWARDER_FOR_NRF5340` configuration is enabled. Pins are
 * forwarded to the network core during startup and reassigned to the application
 * core during shutdown.
 *
 * @return 0 on success, or a negative error code on failure.
 *
 * @note This function relies on the `onoff_manager` API to manage the start and stop
 * transitions for the network core.
 */
int nrf53_cpunet_mgmt_init(void);

#endif /* NRF53_CPUNET_MGMT_H__ */
