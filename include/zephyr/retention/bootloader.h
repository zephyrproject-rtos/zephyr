/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for bootloader configuration interface
 */

#ifndef ZEPHYR_INCLUDE_RETENTION_BOOTLOADER_
#define ZEPHYR_INCLUDE_RETENTION_BOOTLOADER_

#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>
#include <bootutil/image.h>
#include <bootutil/zephyr/mcuboot_shared.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bootloader configuration interface
 * @defgroup bootloader_config_interface Bootloader configuration interface
 * @ingroup retention
 * @{
 */

/**
 * @brief		Loads mcuboot bootloader configuration from shared memory area.
 *
 * @param config	Configuration struct which will be updated.
 *
 * @retval		0 if successful, else negative errno code.
 */
int bootloader_load_config(struct mcuboot_configuration *config);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RETENTION_BOOTLOADER_ */
