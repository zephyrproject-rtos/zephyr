/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Nordic Semiconductor NRF54L family processors.
 */

#ifndef _NORDICSEMI_NRF54L_SOC_H_
#define _NORDICSEMI_NRF54L_SOC_H_

#define __ICACHE_PRESENT    1

#include <soc_nrf_common.h>

#define FLASH_PAGE_ERASE_MAX_TIME_US 8000UL
#define FLASH_PAGE_MAX_CNT	     381UL

#endif /* _NORDICSEMI_NRF54L_SOC_H_ */
