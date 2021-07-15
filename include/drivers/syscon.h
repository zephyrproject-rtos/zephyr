/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public SYSCON driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SYSCON_H_
#define ZEPHYR_INCLUDE_DRIVERS_SYSCON_H_

/**
 * @brief SYSCON Interface
 * @defgroup syscon_interface SYSCON Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the syscon base address
 *
 * This function returns the syscon base address
 *
 * @return 0 on error, the base address on success
 */
uintptr_t syscon_get_base(void);

/**
 * @brief Read from syscon register
 *
 * This function reads from a specific register in the syscon area
 *
 * @param reg The register offset
 * @param val The returned value read from the syscon register
 *
 * @return 0 on success, negative on error
 */
int syscon_read_reg(uint16_t reg, uint32_t *val);

/**
 * @brief Write to syscon register
 *
 * This function writes to a specific register in the syscon area
 *
 * @param reg The register offset
 * @param val The value to be written in the register
 *
 * @return 0 on success, negative on error
 */
int syscon_write_reg(uint16_t reg, uint32_t val);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_DRIVERS_SYSCON_H_ */
