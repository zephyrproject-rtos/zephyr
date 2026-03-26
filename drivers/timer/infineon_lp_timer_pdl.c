/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
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
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_cat1_lp_timer_pdl, CONFIG_KERNEL_LOG_LEVEL);

/* The application only needs one lptimer. Report an error if more than one is selected. */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error Only one LPTIMER instance should be enabled
#endif /* DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1 */

#define CLK_FREQ         DT_INST_PROP(0, clock_frequency)
#define LPTIMER_COUNTERS (CY_MCWDT_CTR0 | CY_MCWDT_CTR1 | CY_MCWDT_CTR2)
#define CYCLES_PER_TICK  (CLK_FREQ / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MAX_DELAY UINT32_MAX
#define MAX_TICKS (MAX_DELAY/CYCLES_PER_TICK)

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

static uint32_t announced_cycles;

static struct k_spinlock lock;

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
	Cy_MCWDT_Disable(lptimer, CY_MCWDT_CTR0 | CY_MCWDT_CTR1, 0);
	WAIT_FOR(Cy_MCWDT_GetEnabledStatus(lptimer, CY_MCWDT_CTR0) == 0, 10000, NULL);

	Cy_MCWDT_ClearInterrupt(lptimer, LPTIMER_COUNTERS);

	/* Per the PDL documentation we reset both counters with a delay between to
	 * account for the cascading. The PDL recommends > 100us for cascaded counters.
	 *
	 * This actually means we delay 200us here to reset in total.
	 */
	Cy_MCWDT_ResetCounters(lptimer, CY_MCWDT_CTR0 | CY_MCWDT_CTR1, 0);
	WAIT_FOR(MCWDT_CNTLOW(lptimer) == 0, 100, NULL);

	MCWDT_MATCH(lptimer) = cycles;

	Cy_MCWDT_SetInterruptMask(lptimer, CY_MCWDT_CTR0 | CY_MCWDT_CTR1);

	Cy_MCWDT_Enable(lptimer, CY_MCWDT_CTR0 | CY_MCWDT_CTR1, 0);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	uint32_t delay_cycles;

	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	if (ticks == K_TICKS_FOREVER || ticks > MAX_TICKS) {
		delay_cycles = MAX_DELAY;
	} else {
		delay_cycles = MAX(ticks * CYCLES_PER_TICK, 1);
	}

	lptimer_delay(delay_cycles);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t current_cycles = MCWDT_CNTHIGH(lptimer);
	uint32_t delta_cycles = current_cycles - announced_cycles;

	k_spin_unlock(&lock, key);

	return delta_cycles / CYCLES_PER_TICK;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return MCWDT_CNTHIGH(lptimer);
}

static void lptimer_isr(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	Cy_MCWDT_ClearInterrupt(lptimer, LPTIMER_COUNTERS);

	uint32_t cycle_count = MCWDT_CNTHIGH(lptimer);
	uint32_t delta_cycles = cycle_count - announced_cycles;
	uint32_t delta_ticks = (delta_cycles / CYCLES_PER_TICK);

	announced_cycles += delta_ticks * CYCLES_PER_TICK;

	k_spin_unlock(&lock, key);

	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? delta_ticks : (delta_ticks > 0));
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
	__ASSERT(rslt == CY_RSLT_SUCCESS, "Failed to initialize lp_timer");

	Cy_MCWDT_Enable(lptimer, CY_MCWDT_CTR2, 0);
	WAIT_FOR(MCWDT_CNTHIGH(lptimer) > 0, 10000, NULL);

	lptimer_delay(MAX_DELAY);

	return 0;
}

SYS_INIT(lptimer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
