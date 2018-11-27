/*
 * Copyright (c) 2018 Sigma Connectivity.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief This file defines the private data structures for spi flash driver
 */

#ifndef ZEPHYR_DRIVERS_FLASH_SPI_FLASH_MX25R6435F_H_
#define ZEPHYR_DRIVERS_FLASH_SPI_FLASH_MX25R6435F_H_


struct spi_flash_data {
	struct device *spi;
#if defined(CONFIG_SPI_FLASH_MX25R6435F_GPIO_SPI_CS)
	struct spi_cs_control cs_ctrl;
#endif  /* CONFIG_SPI_FLASH_MX25R6435F_GPIO_SPI_CS */
	struct spi_config spi_cfg;
#if defined(CONFIG_MULTITHREADING)
	struct k_sem sem;
#endif  /* CONFIG_MULTITHREADING */
};


#endif /* ZEPHYR_DRIVERS_FLASH_SPI_FLASH_MX25R6435F_H_ */
