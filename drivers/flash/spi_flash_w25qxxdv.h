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

#ifndef ZEPHYR_DRIVERS_FLASH_SPI_FLASH_W25QXXDV_H_
#define ZEPHYR_DRIVERS_FLASH_SPI_FLASH_W25QXXDV_H_


struct spi_flash_data {
	struct device *spi;
#if defined(CONFIG_SPI_FLASH_W25QXXDV_GPIO_SPI_CS)
	struct spi_cs_control cs_ctrl;
#endif /* CONFIG_SPI_FLASH_W25QXXDV_GPIO_SPI_CS */
	struct spi_config spi_cfg;
	struct k_sem sem;
};


#endif /* ZEPHYR_DRIVERS_FLASH_SPI_FLASH_W25QXXDV_H_ */
