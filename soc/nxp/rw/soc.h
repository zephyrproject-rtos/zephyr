/*
 * Copyright 2022-2023, 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>
#include <fsl_common.h>
#include <fsl_power.h>

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#ifdef CONFIG_DSP_POWERQUAD_ADDR_REMAP
/*
 * On RW6xx, PowerQuad must access FlexSPI memory through non-cached aliases.
 *
 * Address translation:
 *   0x08xxxxxx -> 0x48xxxxxx
 *   0x28xxxxxx -> 0x48xxxxxx
 */
static inline const void *soc_powerquad_remap_addr(const void *addr)
{
	uintptr_t tmp_addr = (uintptr_t)addr;

	tmp_addr &= 0xE8000000;
	if (tmp_addr == 0x08000000 || tmp_addr == 0x28000000) {
		tmp_addr = 0x48000000;
	}

	return (const void *)(((uintptr_t)addr & ~0xE8000000U) | tmp_addr);
}
#endif /* CONFIG_DSP_POWERQUAD_ADDR_REMAP */

#endif /* !_ASMLANGUAGE */

#define nbu_handler             BLE_MCI_WAKEUP0_DriverIRQHandler
#define nbu_wakeup_done_handler BLE_MCI_WAKEUP_DONE0_DriverIRQHandler

/* Handle variation to implement Wakeup Interrupt */
#define NXP_ENABLE_WAKEUP_SIGNAL(irqn) POWER_EnableWakeup(irqn)
#define NXP_DISABLE_WAKEUP_SIGNAL(irqn) POWER_DisableWakeup(irqn)
#define NXP_GET_WAKEUP_SIGNAL_STATUS(irqn) POWER_GetWakeupStatus(irqn)

#ifdef CONFIG_MEMC
int flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate);
#endif

#endif /* _SOC__H_ */
