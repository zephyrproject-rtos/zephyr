/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Low Power timer driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_lp_timer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_cat1_lp_timer_pdl, CONFIG_KERNEL_LOG_LEVEL);

/* The application only needs one lptimer. Report an error if more than one is selected. */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error Only one LPTIMER instance should be enabled
#endif /* DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1 */

#define CLK_FREQ         DT_INST_PROP(0, clock_frequency)
#define LPTIMER_COUNTERS (CY_MCWDT_CTR0 | CY_MCWDT_CTR1 | CY_MCWDT_CTR2)

/* Counter 2 is reported to the kernel as it reads, so the two rates are one. */
BUILD_ASSERT(CLK_FREQ == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "lp-timer clock-frequency must match CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC");

static MCWDT_STRUCT_Type *lptimer = (MCWDT_STRUCT_Type *)DT_INST_REG_ADDR(0);

#include "cy_mcwdt.h"

static const cy_stc_mcwdt_config_t lptimer_default_cfg = {.c0Match = 0xFFFF,
							  .c1Match = 0xFFFF,
							  .c0Mode = CY_MCWDT_MODE_INT,
							  .c1Mode = CY_MCWDT_MODE_INT,
							  .c2Mode = CY_MCWDT_MODE_NONE,
							  .c2ToggleBit = 0,
							  .c0ClearOnMatch = false,
							  .c1ClearOnMatch = false,
							  .c0c1Cascade = true,
							  .c1c2Cascade = false};

/*
 * Since we do not have a 32bit counter and compare we use counter0/counter1 as a
 * one shot delay that is entirely reset from scratch each time we update our delay.
 *
 * In order to ensure we can sleep longer than a 16bit counter alone would allow for
 * we can use the cascaded counters to only wake up on the cascaded counter1 but
 * sleeps for this will be multiples of 2 seconds (2^16/32768) = 2.
 *
 * There's some significant caveats, it takes some non-negligble number of lf clock
 * cycles to reset, set the match, and then enable the counters. This is almost
 * guaranteed to cost 2-3 low frequency clock cycles of time.
 *
 * That's ok though actually as the only thing kernel timers guarantee is a minimum delay!
 */
static void lptimer_delay(uint32_t cycles)
{
	bool timeout_status;

	Cy_MCWDT_Disable(lptimer, CY_MCWDT_CTR0 | CY_MCWDT_CTR1, 0);
	timeout_status =
		WAIT_FOR(Cy_MCWDT_GetEnabledStatus(lptimer, CY_MCWDT_CTR0) == 0, 10000, NULL);
	__ASSERT(timeout_status == true, "Timeout after Cy_MCWDT_Disable function call.");

	Cy_MCWDT_ClearInterrupt(lptimer, LPTIMER_COUNTERS);

	/* Per the PDL documentation we reset both counters with a delay between to
	 * account for the cascading. The PDL recommends > 100us for cascaded counters.
	 *
	 * This actually means we delay 210us here to reset in total.
	 */
	Cy_MCWDT_ResetCounters(lptimer, CY_MCWDT_CTR0 | CY_MCWDT_CTR1, 105);
	__ASSERT(MCWDT_CNTLOW(lptimer) == 0, "Issue with Cy_MCWDT_ResetCounters function call.");

	MCWDT_MATCH(lptimer) = cycles;

	Cy_MCWDT_SetInterruptMask(lptimer, CY_MCWDT_CTR0 | CY_MCWDT_CTR1);

	Cy_MCWDT_Enable(lptimer, CY_MCWDT_CTR0 | CY_MCWDT_CTR1, 0);
}

/*
 * Counter 2 free-runs at the low-frequency clock and is the cycle domain, while
 * counters 0 and 1, cascaded, are armed as a one-shot relative delay: a RELOAD
 * backend whose alarm is a separate timer from the counter. Arming resets that
 * pair before setting the match, so the delay always runs from zero and cannot
 * be programmed into the past.
 */
#define TIMER_CORE_BACKEND_RELOAD

static inline uint32_t timer_driver_cycle_get(void)
{
	return MCWDT_CNTHIGH(lptimer);
}

static void timer_driver_set_reload(uint32_t cycles)
{
	lptimer_delay(cycles);
}

#include "system_timer_generic.h"

static void lptimer_isr(void)
{
	Cy_MCWDT_ClearInterrupt(lptimer, LPTIMER_COUNTERS);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* The alarm is one-shot, so a ticked kernel re-arms it here:
		 * the core leaves the period of a reload backend to the driver.
		 */
		lptimer_delay(TIMER_CORE_CYC_PER_TICK);
	}

	timer_core_announce();
}

static int lptimer_init(void)
{
	cy_rslt_t rslt = CY_MCWDT_BAD_PARAM;
	cy_stc_mcwdt_config_t cfg = lptimer_default_cfg;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), lptimer_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	rslt = (cy_rslt_t)Cy_MCWDT_Init(lptimer, &cfg);

	/* Effectively AND the interrupt lines from counter0/counter1 match
	 * into the interrupt signaling. Meaning we only interrupt when both
	 * counters match like a 32bit counter and compare.
	 */
	Cy_MCWDT_SetCascadeMatchCombined(lptimer, CY_MCWDT_CASCADE_C0C1, true);

	/* Failing to initialize MCWDT indicates a programming error in the initial configuration */
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_WRN("Failed to initialize lp_timer");
		return -EINVAL;
	}

	Cy_MCWDT_Enable(lptimer, CY_MCWDT_CTR2, 0);
	WAIT_FOR(MCWDT_CNTHIGH(lptimer) > 0, 10000, NULL);

	/* Seed the announce baseline and arm the first tick. */
	timer_core_init();

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		lptimer_delay(TIMER_CORE_CYC_PER_TICK);
	}

	return 0;
}

SYS_INIT(lptimer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
