/*
 * Copyright (c) 2019 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ESPI_TAF_SAMPLE_H__
#define __ESPI_TAF_SAMPLE_H__


/* TAF Base Address */
#define TAF_BASE_ADDR DT_REG_ADDR(DT_NODELABEL(espi_saf0))

/* TAF Test Configuration */
#define TAF_TEST_FREQ_HZ  24000000U
#define TAF_TEST_BUF_SIZE 4096U

/* SPI address of 4KB sector modified by test */
#define TAF_SPI_TEST_ADDRESS 0x1000U

#define READ_4KB_SECTOR 4096U

/* SPI Command Definitions */
#define SPI_WRITE_STATUS1   0x01U
#define SPI_WRITE_STATUS2   0x31U
#define SPI_WRITE_DISABLE   0x04U
#define SPI_READ_STATUS1    0x05U
#define SPI_WRITE_ENABLE    0x06U
#define SPI_READ_STATUS2    0x35U
#define SPI_WRITE_ENABLE_VS 0x50U
#define SPI_READ_JEDEC_ID   0x9FU

/* SPI Status Register Bits */
#define SPI_STATUS1_BUSY 0x80U
#define SPI_STATUS2_QE   0x02U

/* W25Q128 JEDEC ID */
#define W25Q128_JEDEC_ID 0x001840efU

/* TAF Erase Sizes */
enum taf_erase_size {
	TAF_ERASE_4K = 0,
	TAF_ERASE_32K = 1,
	TAF_ERASE_64K = 2,
	TAF_ERASE_MAX
};

/* TAF Address Info Structure */
struct taf_addr_info {
	uintptr_t taf_struct_addr;
	uintptr_t taf_exp_addr;
};

/**
 * @brief Initialize the SPI flash for TAF operations
 *
 * Initializes the local attached SPI flash by:
 * 1. Getting SPI driver binding
 * 2. Reading JEDEC ID and verifying it's a W25Q128
 * 3. Reading STATUS2 and checking QE bit
 * 4. If QE bit is not set, enabling volatile QE bit
 *
 * @return 0 on success, negative errno on failure
 */
int spi_taf_init(void);

/**
 * @brief Initialize eSPI TAF (Trusted Attached Flash)
 *
 * Configures the eSPI TAF controller with flash device parameters
 * and protection regions.
 *
 * @return 0 on success, negative errno on failure
 */
int espi_taf_init(void);

/**
 * @brief Run eSPI TAF test sequence
 *
 * Tests TAF operations including:
 * - Activation
 * - Erase operations
 * - Read operations
 * - Page program operations
 * - Data verification
 *
 * @param spi_addr SPI flash address to test (should be 4KB aligned)
 * @return 0 on success, negative errno on failure
 */
int espi_taf_test1(uint32_t spi_addr);

#endif /* __ESPI_TAF_SAMPLE_H__ */
