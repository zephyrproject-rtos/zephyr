/*
 * Copyright (c) 2025 Qingdao IotPi Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SF32LB58_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SF32LB58_CLOCK_CONTROL_H_

#include <zephyr/sys/util.h>
#include <soc.h>
#include <hpsys_aon.h>
#include <hpsys_rcc.h>

#define HPSYS_RCC_CSR    offsetof(HPSYS_RCC_TypeDef, CSR)
#define HPSYS_RCC_ENR1    offsetof(HPSYS_RCC_TypeDef, ENR1)

#define HPSYS_AON_ACR    offsetof(HPSYS_AON_TypeDef, ACR)

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SF32LB58_CLOCK_CONTROL_H_ */
