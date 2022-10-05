/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX public API
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ft8xx_chip_types {
	FT8xx_CHIP_ID_FT800 = 0x0800100,
	FT8xx_CHIP_ID_FT810 = 0x0810100,
	FT8xx_CHIP_ID_FT811 = 0x0811100,
	FT8xx_CHIP_ID_FT812 = 0x0812100,
	FT8xx_CHIP_ID_FT813 = 0x0813100,
}








/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_ */
