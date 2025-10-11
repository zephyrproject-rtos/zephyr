/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_flash.h
 * @brief Microchip Flash Controller Driver Header
 *
 * @details
 * This header provides conditional inclusion of Microchip flash controller
 * driver headers for supported controller versions (e.g., G1, G2).
 * Depending on the build configuration, the appropriate version-specific
 * header file is included to enable support for the corresponding flash
 * controller peripheral.
 *
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_FLASH_H_
#define INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_FLASH_H_

#ifdef CONFIG_FLASH_MCHP_NVMCTRL_G1
#include "mchp_nvmctrl_g1.h"
#endif /* CONFIG_FLASH_MCHP_NVMCTRL_G1 */

#endif /* INCLUDE_ZEPHYR_DRIVERS_FLASH_MCHP_FLASH_H_ */
