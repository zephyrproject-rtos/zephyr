/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Nordic Semiconductor NRF71 family processors.
 */

#ifndef _NORDICSEMI_NRF71_SOC_H_
#define _NORDICSEMI_NRF71_SOC_H_

#include <soc_nrf_common.h>

#define FLASH_PAGE_ERASE_MAX_TIME_US 42000UL
#define FLASH_PAGE_MAX_CNT           381UL

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwr_antswc)

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) && !defined(__NRF_TFM__)
#define PWR_ANTSWC_REG (0x4010F780UL)
#else /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
#define PWR_ANTSWC_REG (0x5010F780UL)
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

#define PWR_ANTSWC_ENABLE (0x3UL)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwr_antswc) */

#endif /* _NORDICSEMI_NRF71_SOC_H_ */
