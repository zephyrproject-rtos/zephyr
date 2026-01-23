/*
 * Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMTC_MODEM_HAL_EXT_H
#define SMTC_MODEM_HAL_EXT_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialization of the HAL implementation.
 *
 * This must be called before smtc_modem_init.
 *
 * @param[in] transceiver The device pointer of the transceiver instance that will be used.
 */
void smtc_modem_hal_init(const struct device *transceiver);

/**
 * @brief Sleep for a given time, but can be woken up by an LBM IRQ.
 *
 * @param[in] timeout Maximum time to sleep.
 */
void smtc_modem_hal_interruptible_msleep(k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* SMTC_MODEM_HAL_EXT_H */
