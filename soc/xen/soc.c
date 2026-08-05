/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/xen/events.h>

#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
void soc_per_core_init_hook(void)
{
#ifdef CONFIG_SMP
	if (arch_curr_cpu()->id == 0) {
		return;
	}

	xen_evtchn_secondary_cpu_init();
#endif
}
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */
