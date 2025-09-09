/*
 * Copyright (c) 2025 The Zephyr Contributors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISK_SDMMC_STM32_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISK_SDMMC_STM32_H_

#include <zephyr/device.h>
#include <stdint.h>

/**
 * @brief Get the CID (Card Identification) information from the SD/MMC card.
 *
 * This function copies the Card Identification Data (CID) from the internal
 * HAL SD/MMC struct populated during device initialization. It does not check
 * the current presence or status of the card. If the card was removed after
 * initialization (or initialization failed), the returned CID may be stale or
 * all zeroes.
 *
 * It is the caller's responsibility to verify that the card is present and
 * initialized (e.g., by calling @ref disk_access_status) before invoking this
 * function.
 *
 * @param dev Pointer to the device structure representing the SD/MMC card.
 * @param cid Pointer to an array where the CID data will be stored.
 */
void stm32_sdmmc_get_card_cid(const struct device *dev, uint32_t cid[4]);

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISK_SDMMC_STM32_H_ */
