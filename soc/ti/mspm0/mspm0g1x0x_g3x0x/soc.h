/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_TI_MSPM0_SOC_H_
#define SOC_TI_MSPM0_SOC_H_

#define SYSCONFIG_WEAK __attribute__((weak))

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */

#define POWER_STARTUP_DELAY (16)

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* SOC_TI_MSPM0_SOC_H_ */
