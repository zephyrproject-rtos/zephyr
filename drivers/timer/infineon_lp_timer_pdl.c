/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
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
#include <zephyr/sys/clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_cat1_lp_timer_pdl, CONFIG_KERNEL_LOG_LEVEL);

/* The application only needs one lptimer. Report an error if more than one is selected. */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error Only one LPTIMER instance should be enabled
#endif /* DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1 */

/*
 * Counters 0 and 1 are cascaded into one 32-bit counter with one 32-bit
 * compare. That needs the cascade to carry on counter0 rollover rather than on
 * its match, which only CAT1B and CAT1D can select.
 */
#if !defined(CY_IP_MXS40SSRSS) && !defined(CY_IP_MXS22SRSS)
#error The lp-timer needs a C0/C1 cascade carrying on rollover (CAT1B or CAT1D)
#endif

#define CLK_FREQ         DT_INST_PROP(0, clock_frequency)
#define LPTIMER_COUNTERS (CY_MCWDT_CTR0 | CY_MCWDT_CTR1)

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
 * MCWDT_CNTLOW is counter1:counter0 and MCWDT_MATCH is their two compares, so
 * each is read or written as one 32-bit value. The match is on equality, so a
 * compare set behind the count is missed until the counter wraps.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT

/* A match takes two lf_clk cycles to reach the compare logic. Arming 3 cycles
 * ahead keeps the count below the compare until then.
 */
#define TIMER_CORE_ALARM_LEAD_CYCLES 3

static uint32_t timer_driver_cycle_get(void)
{
	return MCWDT_CNTLOW(lptimer);
}

static void timer_driver_set_compare(uint32_t cycles)
{
	MCWDT_MATCH(lptimer) = cycles;
}

#include "system_timer_generic.h"

static void lptimer_isr(void)
{
	Cy_MCWDT_ClearInterrupt(lptimer, LPTIMER_COUNTERS);

	timer_core_announce();
}

static int lptimer_init(void)
{
	cy_rslt_t rslt = CY_MCWDT_BAD_PARAM;
	cy_stc_mcwdt_config_t cfg = lptimer_default_cfg;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), lptimer_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	rslt = (cy_rslt_t)Cy_MCWDT_Init(lptimer, &cfg);

	/* Failing to initialize MCWDT indicates a programming error in the initial configuration */
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_WRN("Failed to initialize lp_timer");
		return -EINVAL;
	}

	/* Match on the full 32 bits: counter1 signals only when counter0 matches too. */
	Cy_MCWDT_SetCascadeMatchCombined(lptimer, CY_MCWDT_CASCADE_C0C1, true);

	/* Carry on counter0 rollover, not on its match, so counter1 counts
	 * 65536-cycle periods whatever the compare holds.
	 */
	Cy_MCWDT_SetCascadeCarryOutRollOver(lptimer, CY_MCWDT_CASCADE_C0C1, true);

	/* Counter0 matches a whole 16-bit period before the pair does, so leave
	 * its interrupt masked.
	 */
	Cy_MCWDT_SetInterruptMask(lptimer, CY_MCWDT_CTR1);

	/* 93 us is the PDL's wait for the counters to actually start. */
	Cy_MCWDT_Enable(lptimer, LPTIMER_COUNTERS, 93);

	timer_core_init();

	return 0;
}

SYS_INIT(lptimer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
