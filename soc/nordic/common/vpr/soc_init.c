/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vevif.h>

static int vpr_init(void)
{
#ifdef CONFIG_SOC_NRF54H20_CPUPPR
	/* Notify parent core that core is ready and can accept IPC communication. */
	nrf_vpr_csr_vevif_events_set(BIT(15));
#endif

	/* RT peripherals for VPR all share one enable.
	 * To prevent redundant calls, do it here once.
	 */
	nrf_vpr_csr_rtperiph_enable_set(true);

	return 0;
}

SYS_INIT(vpr_init, EARLY, 0);
