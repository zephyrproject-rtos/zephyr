/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LORA_LBM_TRANSCEIVER_H
#define LORA_LBM_TRANSCEIVER_H

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback upon firing event trigger
 *
 */
typedef void (*event_cb_t)(const struct device *dev);

/**
 * @brief Attach interrupt cb to event pin.
 *
 * @param dev context
 * @param cb cb function
 */
void lora_transceiver_board_attach_interrupt(const struct device *dev, event_cb_t cb);

/**
 * @brief Enable interrupt on event pin.
 *
 * @param dev context
 */
void lora_transceiver_board_enable_interrupt(const struct device *dev);

/**
 * @brief Disable interrupt on event pin.
 *
 * @param dev context
 */
void lora_transceiver_board_disable_interrupt(const struct device *dev);

/**
 * @brief Helper to get the tcxo startup delay for any model of transceiver
 *
 * @param dev context
 */
uint32_t lora_transceiver_get_tcxo_startup_delay_ms(const struct device *dev);

/**
 * @brief Returns lr11xx_system_version_type_t or -1
 *
 * @param dev context
 */

int32_t lora_transceiver_get_model(const struct device *dev);

/**
 * @brief Set the Tx power offset in dB
 *
 * @param [in] context Chip implementation context
 * @param [in] tx_pwr_offset_db
 */
void radio_utilities_set_tx_power_offset(const void *context, uint8_t tx_pwr_offset_db);

/**
 * @brief Get the Tx power offset in dB
 *
 * @param [in] context Chip implementation context
 *
 * @return Tx power offset in dB
 */
uint8_t radio_utilities_get_tx_power_offset(const void *context);

#ifdef __cplusplus
}
#endif

#endif /* LORA_LBM_TRANSCEIVER_H */
