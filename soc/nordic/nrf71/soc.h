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

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(__NRF_TFM__)
#define PWR_ANTSWC_REG (0x5010F780UL)
#else
#define PWR_ANTSWC_REG (0x4010F780UL)
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

#define PWR_ANTSWC_ENABLE (0x3UL)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwr_antswc) */

/* PORTCNF.PWRCTRL of GPIO port 4. It selects where the floating ground of the P4 pins sits, in
 * other words whether the pins run in 1.8 V or 3.3 V mode. The register is not exposed by the MDK
 * yet, so it is defined here.
 * P4 is powered on by default so that the pins work out of the box, but that costs roughly 40 uA
 * of constant current, also in System OFF. The port is therefore powered off again when P4 is not
 * enabled in devicetree.
 */
#define P4_PWRCTRL_OFFSET (0x34UL)

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(__NRF_TFM__)
#define P4_PWRCTRL_REG (NRF_P4_S_BASE + P4_PWRCTRL_OFFSET)
#else
#define P4_PWRCTRL_REG (NRF_P4_NS_BASE + P4_PWRCTRL_OFFSET)
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

#define P4_PWRCTRL_OFF (0x0UL)

#endif /* _NORDICSEMI_NRF71_SOC_H_ */
