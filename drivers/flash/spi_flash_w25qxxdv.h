/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief This file defines the private data structures for spi flash driver
 */

#ifndef __SPI_FLASH_W25QXXDV_H__
#define __SPI_FLASH_W25QXXDV_H__


struct spi_flash_data {
	struct device *spi;
	uint8_t buf[CONFIG_SPI_FLASH_W25QXXDV_MAX_DATA_LEN +
		    W25QXXDV_LEN_CMD_ADDRESS];
	struct k_sem sem;
};


#endif /* __SPI_FLASH_W25QXXDV_H__ */
