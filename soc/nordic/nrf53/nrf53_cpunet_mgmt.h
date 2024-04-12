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

#endif /* NRF53_CPUNET_MGMT_H__ */
