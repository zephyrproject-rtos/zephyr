/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDED_DRIVERS_ESP_HOSTED_HAL_H
#define ZEPHYR_INCLUDED_DRIVERS_ESP_HOSTED_HAL_H
int esp_hosted_hal_init(const struct device *dev);
bool esp_hosted_hal_data_ready(const struct device *dev);
int esp_hosted_hal_spi_transfer(const struct device *dev, void *tx, void *rx, uint32_t size);
#endif /* ZEPHYR_INCLUDED_DRIVERS_ESP_HOSTED_HAL_H */
