/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vevif.h>

#if defined(CONFIG_USE_NANOBE_SWITCH)
/* VPRNORDICFEATURESDISABLE INTHWSTACKING field, interrupt hardware stacking */
#define VPR_INTHWSTACKING_Pos         (5UL)
#define VPR_INTHWSTACKING_Msk         (0x3UL << VPR_INTHWSTACKING_Pos)
#define VPR_INTHWSTACKING_AUTOSTACK   (0x0UL << VPR_INTHWSTACKING_Pos)
#define VPR_INTHWSTACKING_NOAUTOSTACK (0x3UL << VPR_INTHWSTACKING_Pos)

/* Enable or disable the interrupt hardware stacking as per
 * CONFIG_NANOBE_HW_STACKING, before interrupts are enabled.
 */
static void vpr_nanobe_stacking_init(void)
{
	uint32_t features = csr_read(VPRCSR_NORDIC_VPRNORDICFEATURESDISABLE);

	features &= ~(VPRCSR_NORDIC_VPRNORDICFEATURESDISABLE_NORDICKEY_Msk | VPR_INTHWSTACKING_Msk);
	features |= (VPRCSR_NORDIC_VPRNORDICFEATURESDISABLE_NORDICKEY_Enabled
		     << VPRCSR_NORDIC_VPRNORDICFEATURESDISABLE_NORDICKEY_Pos);
	features |= IS_ENABLED(CONFIG_NANOBE_HW_STACKING) ? VPR_INTHWSTACKING_AUTOSTACK
							   : VPR_INTHWSTACKING_NOAUTOSTACK;

	csr_write(VPRCSR_NORDIC_VPRNORDICFEATURESDISABLE, features);
}
#endif /* CONFIG_USE_NANOBE_SWITCH */

static int vpr_init(void)
{
	uint32_t sleep_mode;

#if defined(CONFIG_USE_NANOBE_SWITCH)
	vpr_nanobe_stacking_init();
#endif /* CONFIG_USE_NANOBE_SWITCH */

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
