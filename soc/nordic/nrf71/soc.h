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

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || !defined(__ZEPHYR__)
#define PWR_ANTSWC_REG (0x5010F780UL)
#else
#define PWR_ANTSWC_REG (0x4010F780UL)
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

#define PWR_ANTSWC_ENABLE (0x3UL)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwr_antswc) */

/* PORTCNF.PWRCTRL of GPIO port 4. The register is not exposed by the MDK
 * yet, so it is defined here.
 *
 * The selected mode must match the voltage actually driving the port's
 * VDDIO pin on the board mismatching them can damage the port.
 */
#define P4_PWRCTRL_OFFSET (0x34UL)
#define P4_PWRCTRL_REG    ((uintptr_t)NRF_P4 + P4_PWRCTRL_OFFSET)

#define P4_PWRCTRL_OFF (0x0UL)
/* Static floating ground tied to 0 V, giving a 1.8 V pad swing when VDDIO_P4 is 1.8 V */
#define P4_PWRCTRL_1V8 (0x1UL)
/* Buffered floating ground, VDDIO - 1.8 V, i.e. 3.3 V mode */
#define P4_PWRCTRL_3V3 (0x3UL)

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio4)) && \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(gpio4), nordic_pad_voltage, 1v8)
#define P4_PWRCTRL_ON P4_PWRCTRL_1V8
#else
#define P4_PWRCTRL_ON P4_PWRCTRL_3V3
#endif

#endif /* _NORDICSEMI_NRF71_SOC_H_ */
