/*
 * Copyright 2022-2023, 2025-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>
#include <fsl_common.h>
#include "fsl_power.h"
#include "pmu.h"

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#endif /* !_ASMLANGUAGE */

#define nbu_handler             BLE_MCI_WAKEUP0_DriverIRQHandler
#define nbu_wakeup_done_handler BLE_MCI_WAKEUP_DONE0_DriverIRQHandler

/* Handle variation to implement Wakeup Interrupt */
#define NXP_ENABLE_WAKEUP_SIGNAL(irqn) nxp_pmu_enable_wakeup(irqn)
#define NXP_DISABLE_WAKEUP_SIGNAL(irqn) nxp_pmu_disable_wakeup(irqn)
#define NXP_GET_WAKEUP_SIGNAL_STATUS(irqn) nxp_pmu_get_wakeup_status(irqn)

#ifdef CONFIG_MEMC
int flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate);
#endif

#endif /* _SOC__H_ */
