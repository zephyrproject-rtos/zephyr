/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Nordic nRF51 family processors.
 */

#ifndef _NORDICSEMI_NRF51_SOC_H_
#define _NORDICSEMI_NRF51_SOC_H_

#include <soc_nrf_common.h>

#define NRF51_POWER_RAMON_ADDRESS              0x40000524
#define NRF51_POWER_RAMONB_ADDRESS             0x40000554
#define NRF51_POWER_RAMONx_RAMxON_ONMODE_Msk   0x3

#define FLASH_PAGE_ERASE_MAX_TIME_US 22300UL
#define FLASH_PAGE_MAX_CNT 256UL

#endif /* _NORDICSEMI_NRF51_SOC_H_ */
