/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the
 *       Nordic Semiconductor nRF53 family processors.
 */

#ifndef _NORDICSEMI_NRF53_SOC_H_
#define _NORDICSEMI_NRF53_SOC_H_

#include <soc_nrf_common.h>

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#define FLASH_PAGE_ERASE_MAX_TIME_US  89700UL
#define FLASH_PAGE_MAX_CNT  256UL
#elif defined(CONFIG_SOC_NRF5340_CPUNET)
#define FLASH_PAGE_ERASE_MAX_TIME_US  89700UL
#define FLASH_PAGE_MAX_CNT  128UL
#endif

#endif /* _NORDICSEMI_NRF53_SOC_H_ */
