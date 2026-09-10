/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vevif.h>

static int vpr_init(void)
{
	uint32_t sleep_mode;

	if (IS_ENABLED(CONFIG_NORDIC_VPR_DEEPSLEEP)) {
		sleep_mode = VPRCSR_NORDIC_VPRNORDICSLEEPCTRL_SLEEPSTATE_DEEPSLEEP;
	} else if (IS_ENABLED(CONFIG_NORDIC_VPR_HIBERNATE)) {
		sleep_mode = VPRCSR_NORDIC_VPRNORDICSLEEPCTRL_SLEEPSTATE_HIBERNATE;
	} else {
		sleep_mode = VPRCSR_NORDIC_VPRNORDICSLEEPCTRL_SLEEPSTATE_SLEEP;
	}

	csr_write(VPRCSR_NORDIC_VPRNORDICSLEEPCTRL, sleep_mode);

	/* RT peripherals for VPR all share one enable.
	 * To prevent redundant calls, do it here once.
	 */
	nrf_vpr_csr_rtperiph_enable_set(true);

#if DT_NODE_HAS_PROP(DT_NODELABEL(cpu), nordic_vpr_ready_event)
	/* Notify parent core that core is ready and can accept IPC communication. */
	nrf_vpr_csr_vevif_events_set(BIT(DT_PROP(DT_NODELABEL(cpu), nordic_vpr_ready_event)));
#endif
	return 0;
}

SYS_INIT(vpr_init, PRE_KERNEL_1, 0);
