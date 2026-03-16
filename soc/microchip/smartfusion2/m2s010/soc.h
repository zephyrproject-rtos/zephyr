/*
 * SPDX-FileCopyrightText: Copyright Bavariamatic GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public SoC declarations for the SmartFusion2 M2S010 port.
 */

#ifndef _SOC_H_
#define _SOC_H_

#include <cmsis_core_m_defaults.h>
#include <soc_registers.h>

#ifndef _ASMLANGUAGE
/** System core clock value exported for CMSIS-compatible code. */
extern uint32_t SystemCoreClock;

/** Initialize core platform state before Zephyr drivers start. */
void SystemInit(void);

/** Refresh the exported SystemCoreClock value. */
void SystemCoreClockUpdate(void);
#endif

#endif