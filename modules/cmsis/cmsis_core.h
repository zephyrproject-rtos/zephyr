/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_CMSIS_CMSIS_H_
#define ZEPHYR_MODULES_CMSIS_CMSIS_H_

#if defined(CONFIG_CPU_CORTEX_M)
#include "cmsis_core_m.h"
#elif defined(CONFIG_CPU_AARCH32_CORTEX_A) || defined(CONFIG_CPU_AARCH32_CORTEX_R)
#include "cmsis_core_a_r.h"
#endif

#endif /* ZEPHYR_MODULES_CMSIS_CMSIS_H_ */
