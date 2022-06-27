/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX common functions
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COMMON_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COMMON_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FT8xx functions to write and read memory
 * @defgroup ft8xx_common FT8xx common functions
 * @ingroup ft8xx_interface
 * @{
 */

/**
 * @brief Write 1 byte (8 bits) to FT8xx memory
 *
 * @param address Memory address to write to
 * @param data Byte to write
 */
void ft8xx_wr8(uint32_t address, uint8_t data);

/**
 * @brief Write 2 bytes (16 bits) to FT8xx memory
 *
 * @param address Memory address to write to
 * @param data Value to write
 */
void ft8xx_wr16(uint32_t address, uint16_t data);

/**
 * @brief Write 4 bytes (32 bits) to FT8xx memory
 *
 * @param address Memory address to write to
 * @param data Value to write
 */
void ft8xx_wr32(uint32_t address, uint32_t data);

/**
 * @brief Read 1 byte (8 bits) from FT8xx memory
 *
 * @param address Memory address to read from
 *
 * @return Value read from memory
 */
uint8_t ft8xx_rd8(uint32_t address);

/**
 * @brief Read 2 bytes (16 bits) from FT8xx memory
 *
 * @param address Memory address to read from
 *
 * @return Value read from memory
 */
uint16_t ft8xx_rd16(uint32_t address);

/**
 * @brief Read 4 bytes (32 bits) from FT8xx memory
 *
 * @param address Memory address to read from
 *
 * @return Value read from memory
 */
uint32_t ft8xx_rd32(uint32_t address);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COMMON_H_ */
