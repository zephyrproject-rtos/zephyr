/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief LPM companion timer for PSE84 using MCWDT.
 *
 * When SysTick is the primary system timer, it stops during deepsleep
 * because HF clocks are gated. This module initializes the MCWDT
 * (Multi-Counter Watchdog Timer) as a free-running counter on the PILO
 * (32 kHz) clock and implements the z_sys_clock_lpm_enter/exit hooks
 * so the SysTick driver can reconcile elapsed time after wakeup.
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer_lpm.h>
#include <zephyr/irq.h>

#include <cy_mcwdt.h>

#define DT_DRV_COMPAT infineon_lp_timer

/* Use the first enabled instance - board DTS enables mcwdt0 for M33, mcwdt1 for M55 */
#if !DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#error "No infineon,lp-timer node enabled in devicetree"
#endif

#define MCWDT_NODE     DT_INST(0, DT_DRV_COMPAT)
#define MCWDT_REG_ADDR DT_REG_ADDR(MCWDT_NODE)
#define MCWDT_IRQ_NUM  DT_IRQN(MCWDT_NODE)
#define MCWDT_IRQ_PRIO DT_IRQ(MCWDT_NODE, priority)

/* PILO clock frequency - the MCWDT source clock */
#define PILO_FREQ DT_PROP(DT_PATH(clk_pilo), clock_frequency)

/* Enable counters C0 + C1 cascaded for 32-bit match, C2 free-running */
#define MCWDT_COUNTERS (CY_MCWDT_CTR0 | CY_MCWDT_CTR1 | CY_MCWDT_CTR2)

/* Time in microseconds for MCWDT reset/enable propagation */
#define MCWDT_RESET_TIME_US 62U

static MCWDT_STRUCT_Type *const mcwdt_base = (MCWDT_STRUCT_Type *)MCWDT_REG_ADDR;

/* Counter value captured at LPM entry */
static uint32_t lpm_entry_count;

static void lpm_timer_isr(void)
{
	/* Clear all interrupt flags - the alarm fired to wake the system */
	Cy_MCWDT_ClearInterrupt(mcwdt_base, CY_MCWDT_CTR0 | CY_MCWDT_CTR1);
	Cy_MCWDT_SetInterruptMask(mcwdt_base, 0U);
}

void z_sys_clock_lpm_enter(uint64_t max_lpm_time_us)
{
	uint32_t delay_ticks;

	/* Convert microseconds to PILO ticks.
	 * Cap to avoid overflow in 32-bit arithmetic.
	 */
	if (max_lpm_time_us > ((uint64_t)UINT32_MAX * 1000000ULL / PILO_FREQ)) {
		delay_ticks = UINT32_MAX - 1U;
	} else {
		delay_ticks = (uint32_t)((max_lpm_time_us * PILO_FREQ) / 1000000ULL);
	}

	/* MCWDT match values take effect after 2 PILO cycles (see
	 * Cy_MCWDT_SetMatch()), so the minimum useful delay is 3 ticks.
	 */
	if (delay_ticks < 3U) {
		delay_ticks = 3U;
	}

	/* Capture current counter value */
	lpm_entry_count = Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER2);

	/* Program match on C0/C1 cascade for wakeup.
	 * C0 is 16-bit, C1 cascades on C0 match. Together they form
	 * a 32-bit programmable match against the free-running counter.
	 */
	uint16_t c0_current = (uint16_t)Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER0);
	uint16_t c0_match = (uint16_t)(c0_current + (uint16_t)(delay_ticks & 0xFFFFU));
	uint16_t c1_current = (uint16_t)Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER1);
	uint16_t c1_match = (uint16_t)(c1_current + (uint16_t)(delay_ticks >> 16U));

	MCWDT_MATCH(mcwdt_base) = (uint32_t)c0_match | ((uint32_t)c1_match << 16U);
	Cy_MCWDT_ClearInterrupt(mcwdt_base, CY_MCWDT_CTR1);
	Cy_MCWDT_SetInterruptMask(mcwdt_base, CY_MCWDT_CTR1);
	irq_enable(MCWDT_IRQ_NUM);
}

uint64_t z_sys_clock_lpm_exit(void)
{
	uint32_t current_count;
	uint32_t elapsed_ticks;

	/* Disable further MCWDT interrupts */
	Cy_MCWDT_SetInterruptMask(mcwdt_base, 0U);
	Cy_MCWDT_ClearInterrupt(mcwdt_base, CY_MCWDT_CTR0 | CY_MCWDT_CTR1);
	irq_disable(MCWDT_IRQ_NUM);

	/* Read current free-running counter (C2 is 32-bit, never resets) */
	current_count = Cy_MCWDT_GetCount(mcwdt_base, CY_MCWDT_COUNTER2);

	/* Unsigned subtraction handles wraparound correctly */
	elapsed_ticks = current_count - lpm_entry_count;

	/* Convert PILO ticks to microseconds */
	return ((uint64_t)elapsed_ticks * 1000000ULL) / PILO_FREQ;
}

static int lpm_timer_init(void)
{
	static const cy_stc_mcwdt_config_t cfg = {
		.c0Match = 0xFFFFU,
		.c1Match = 0xFFFFU,
		.c0Mode = CY_MCWDT_MODE_INT,
		.c1Mode = CY_MCWDT_MODE_INT,
		.c2Mode = CY_MCWDT_MODE_NONE,
		.c2ToggleBit = 0U,
		.c0ClearOnMatch = false,
		.c1ClearOnMatch = false,
		.c0c1Cascade = true,
		.c1c2Cascade = false,
	};
	cy_rslt_t rslt;

	rslt = (cy_rslt_t)Cy_MCWDT_Init(mcwdt_base, &cfg);
	if (rslt != CY_RSLT_SUCCESS) {
		return -EINVAL;
	}

	Cy_MCWDT_Enable(mcwdt_base, MCWDT_COUNTERS, MCWDT_RESET_TIME_US);

	IRQ_CONNECT(MCWDT_IRQ_NUM, MCWDT_IRQ_PRIO, lpm_timer_isr, NULL, 0);

	return 0;
}

SYS_INIT(lpm_timer_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
