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

#define DT_DRV_COMPAT winbond_w25q16

struct spi_flash_data {
	struct device *spi;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif
	struct spi_config spi_cfg;
#if defined(CONFIG_MULTITHREADING)
	struct k_sem sem;
#endif /* CONFIG_MULTITHREADING */
};


#endif /* ZEPHYR_DRIVERS_FLASH_SPI_FLASH_W25QXXDV_H_ */
