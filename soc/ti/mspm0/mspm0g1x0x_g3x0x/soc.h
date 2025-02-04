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

/*
 * Per TRM Section 2.2.7 Peripheral Power Enable Control:
 *
 * After setting the ENABLE | KEY bits in the PWREN Register to enable a
 * peripheral, wait at least 4 ULPCLK clock cycles before accessing the rest of
 * the peripheral's memory-mapped registers. The 4 cycles allow for the bus
 * isolation signals at the peripheral's bus interface to update.
 *
 * ULPCLK will either be equivalent or half of the main MCLK and CPUCLK,
 * yielding the delay time of 8 cycles
 */
#define POWER_STARTUP_DELAY (8)

#ifdef __cplusplus
}
#endif

#endif /* SOC_TI_MSPM0_SOC_H_ */
