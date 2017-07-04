/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Headers target/devboard.h are copies of board headers form mcuboot sources */
/* see <mcuboot>/boot/zephyr/targets/                                         */

#ifndef __MCUBOOT_CONSTRAINS_H__
#define __MCUBOOT_CONSTRAINS_H__

/* Flash specific configs */
#if defined(CONFIG_SOC_SERIES_NRF52X)
#define FLASH_DRIVER_NAME		CONFIG_SOC_FLASH_NRF5_DEV_NAME
#define FLASH_ALIGN			4
#else
#error Unknown SoC family
#endif /* CONFIG_SOC_SERIES_NRF52X */

#endif	/* __MCUBOOT_CONSTRAINS_H__ */
