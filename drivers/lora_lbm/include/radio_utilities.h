/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RADIO_UTILITIES_H
#define RADIO_UTILITIES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the Tx power offset in dB
 *
 * @param [in] context Chip implementation context
 *
 * @return Tx power offset in dB
 */
uint8_t radio_utilities_get_tx_power_offset(const void *context);

/**
 * @brief Set the Tx power offset in dB
 *
 * @param [in] context Chip implementation context
 * @param [in] tx_pwr_offset_db
 */
void radio_utilities_set_tx_power_offset(const void *context, uint8_t tx_pwr_offset_db);

#ifdef __cplusplus
}
#endif

#endif /* RADIO_UTILITIES_H */
