/*
 * Copyright 2022-2023 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>
#include <fsl_common.h>
#include "fsl_power.h"

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#endif /* !_ASMLANGUAGE */

#define ble_hci_handler         BLE_MCI_WAKEUP0_DriverIRQHandler
#define ble_wakeup_done_handler BLE_MCI_WAKEUP_DONE0_DriverIRQHandler

/* Wrapper Function to deal with SDK differences in power API */
static inline void EnableDeepSleepIRQ(IRQn_Type irq)
{
	POWER_EnableWakeup(irq);
}

#ifdef CONFIG_MEMC
int flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate);
#endif


#endif /* _SOC__H_ */
