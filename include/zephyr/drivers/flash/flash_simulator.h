/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ZEPHYR_INCLUDE_DRIVERS__FLASH_SIMULATOR_H__
#define __ZEPHYR_INCLUDE_DRIVERS__FLASH_SIMULATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Flash simulator specific API.
 *
 * Extension for flash simulator.
 */

/**
 * @brief Obtain a pointer to the RAM buffer used but by the simulator
 *
 * This function allows the caller to get the address and size of the RAM buffer
 * in which the flash simulator emulates its flash memory content.
 *
 * @param[in]  dev flash simulator device pointer.
 * @param[out] mock_size size of the ram buffer.
 *
 * @retval pointer to the ram buffer
 */
__syscall void *flash_simulator_get_memory(const struct device *dev,
					   size_t *mock_size);
#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/flash_simulator.h>

#endif /* __ZEPHYR_INCLUDE_DRIVERS__FLASH_SIMULATOR_H__ */
