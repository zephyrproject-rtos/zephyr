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
#if defined(CONFIG_SPI_FLASH_W25QXXDV_GPIO_SPI_CS)
	struct spi_cs_control cs_ctrl;
#endif /* CONFIG_SPI_FLASH_W25QXXDV_GPIO_SPI_CS */
	struct spi_config spi_cfg;
#if defined(CONFIG_MULTITHREADING)
	struct k_sem sem;
#endif /* CONFIG_MULTITHREADING */
};


#endif /* __SPI_FLASH_W25QXXDV_H__ */
