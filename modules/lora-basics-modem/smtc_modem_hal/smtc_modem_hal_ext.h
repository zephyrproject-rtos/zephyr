/*
 * Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMTC_MODEM_HAL_EXT_H
#define SMTC_MODEM_HAL_EXT_H

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialization of the hal implementation.
 *
 * This must be called before smtc_modem_init
 *
 * @param[in] transceiver The device pointer of the transceiver instance that will be used.
 */
void smtc_modem_hal_init(const struct device *transceiver);


#ifdef __cplusplus
}
#endif

#endif /* SMTC_MODEM_HAL_EXT_H */
