/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_ESP32P4_ESP_DW_GDMA_H_
#define ZEPHYR_SOC_ESPRESSIF_ESP32P4_ESP_DW_GDMA_H_

/**
 * @brief Bring up the DW-GDMA controller shared by the MIPI DSI and CSI drivers.
 *
 * Enables the bus clock and resets the controller once per boot. Later callers find
 * it running and return at once.
 *
 * @retval 0 Success
 * @retval -ETIMEDOUT The bus clock did not come up
 */
int esp_dw_gdma_init(void);

#endif /* ZEPHYR_SOC_ESPRESSIF_ESP32P4_ESP_DW_GDMA_H_ */
