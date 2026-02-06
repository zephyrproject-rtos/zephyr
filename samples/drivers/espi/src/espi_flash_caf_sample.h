/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESPI_FLASH_SAMPLE_H
#define ESPI_FLASH_SAMPLE_H

#include <zephyr/device.h>

#define MAX_TEST_BUF_SIZE   1024u
#define MAX_FLASH_REQUEST   64u
#define TARGET_FLASH_REGION 0x72000ul

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
