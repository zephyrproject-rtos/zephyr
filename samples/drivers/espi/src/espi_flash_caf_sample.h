/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESPI_FLASH_SAMPLE_H
#define ESPI_FLASH_SAMPLE_H

#include <zephyr/device.h>

#define MAX_FLASH_REQUEST   64u
/*
 * Assumes eSPI host is in CAF (Controller Attached Flash) mode
 * and EC has access to the EC flash region.
 * In most cases, this region is intended for EC firmware fetch over eSPI in CAF mode.
 * If running with an eSPI emulator, this region can be configured as desired.
 *
 * Example SPI flash map for CAF mode:
 * ------------------------------------------------------------------------------
 * | 0x00000 - 0x3FFFF  | Reserved for Intel internal use                       |
 * | 0x40000 - 0x90000  | EC firmware region for EC FW fetch over eSPI in CAF   |
 * | 0x90000 - end      | Reserved for Intel use (BIOS, ME, etc.)               |
 * ------------------------------------------------------------------------------
 */
#define TARGET_FLASH_REGION   CONFIG_ESPI_CAF_FLASH_REGION

/**
 * @brief Test eSPI flash read/write operations
 *
 * Performs flash read and write operations via eSPI flash channel APIs to verify
 * communication and data integrity.
 *
 * Test assumes that eSPI host is in CAF (controller attached flash) mode and flash region starting
 * from start_flash_addr is erased and available for test.
 *
 * @param start_flash_addr Starting address in SPI flash memory for the test
 * @param blocks Number of blocks to test
 * @return 0 on success, negative errno code on failure
 */
int espi_flash_test(uint32_t start_flash_addr, uint8_t blocks);


#endif /* ESPI_FLASH_SAMPLE_H */
