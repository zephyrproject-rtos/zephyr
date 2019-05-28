/*
 * Copyright (c) 2019 Yurii Hamann.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header for SD/MMC interface on STM32 family processor.
 *
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_SDMMC_STM32_H_
#define ZEPHYR_DRIVERS_SERIAL_SDMMC_STM32_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sdmmc/sdmmc.h>

/** @brief SD/MMC device configuration
 * for STM32Fx series MCUs
 */
struct sdmmc_stm32_config {
	struct stm32_pclken pclken;
};

/** @brief SD/MMC device data
 * for STM32Fx series MCUs
 */
struct sdmmc_stm32_data {
	/* common device data */
	struct sdmmc_data generic;
	/* device base address */
	u32_t *base;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SERIAL_SDMMC_STM32_H_ */
