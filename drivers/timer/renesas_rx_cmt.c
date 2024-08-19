/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys_clock.h"
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <platform.h>

#define DT_DRV_COMPAT renesas_rx_timer_cmt

#define CMT_NODE  DT_NODELABEL(cmt)
#define CMT0_NODE DT_NODELABEL(cmt0)

#define CMT_CMSTR0_ADDR DT_REG_ADDR_BY_NAME(CMT_NODE, CMSTR0)
#define CMT0_CMCR_ADDR  DT_REG_ADDR_BY_NAME(CMT0_NODE, CMCR)
#define CMT0_CMCNT_ADDR DT_REG_ADDR_BY_NAME(CMT0_NODE, CMCNT)
#define CMT0_CMCOR_ADDR DT_REG_ADDR_BY_NAME(CMT0_NODE, CMCOR)

#define ICU_NODE DT_NODELABEL(icu)

#define ICU_IR_ADDR  DT_REG_ADDR_BY_NAME(ICU_NODE, IR)
#define ICU_IER_ADDR DT_REG_ADDR_BY_NAME(ICU_NODE, IER)
#define ICU_IPR_ADDR DT_REG_ADDR_BY_NAME(ICU_NODE, IPR)
#define ICU_FIR_ADDR DT_REG_ADDR_BY_NAME(ICU_NODE, FIR)

#define COUNTER_MAX   0x0000ffff
#define TIMER_STOPPED 0xff000000

/* The following macro values will be changed later to be calculated using the clock control driver.
 * This macro is the CMT count clock. (=PCLKB/8)
 */
#define CYCLES_PER_SEC  (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#define TICKS_PER_SEC   (CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define CYCLES_PER_TICK (CYCLES_PER_SEC / TICKS_PER_SEC)
#define MAX_TICKS       ((COUNTER_MAX / CYCLES_PER_TICK) - 1)

#define CMT0_IRQ_NUM DT_IRQ_BY_NAME(CMT0_NODE, cmi, irq)

#if (IS_ENABLED(CONFIG_TICKLESS_KERNEL))
#error Error: Tickless mode is not yet implemented.
#endif

typedef uint32_t cycle_t;
static cycle_t cycle_count;

/* clock_cycles_per_tick is calculated once in
 * sys_clock_driver_init() to avoid repeated division of sub_clk_freq_hz
 * and TICKS_PER_SEC. This is justified as clock frequency should not
 * change during operation without undesired side effects on peripheral
 * devices.
 */
static uint16_t clock_cycles_per_tick;

static uint32_t last_load;

uint32_t sys_clock_cycle_get_32(void)
{
	uint32_t ret = cycle_count;

	volatile uint16_t *cmt0_cmcnt = (uint16_t *)CMT0_CMCNT_ADDR; /* CMCNT */
	ret += (uint32_t)(*cmt0_cmcnt);

	return ret;
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful operation */
		return 0;
	}

	volatile uint16_t *cmt0_cmcnt = (uint16_t *)CMT0_CMCNT_ADDR; /* CMCNT */
	return ((uint32_t)((*cmt0_cmcnt) / clock_cycles_per_tick));
}

static void cmt0_isr(void)
{
	k_ticks_t dticks;

	cycle_count += CYCLES_PER_TICK;
	dticks = 1;
	sys_clock_announce(dticks);
}

static int sys_clock_driver_init(void)
{
	volatile uint16_t *cmt_cmstr0 = (uint16_t *)CMT_CMSTR0_ADDR; /* CMSTR0 */
	volatile uint16_t *cmt0_cmcr = (uint16_t *)CMT0_CMCR_ADDR;   /* CMCR */
	volatile uint16_t *cmt0_cmcor = (uint16_t *)CMT0_CMCOR_ADDR; /* CMCOR */

	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_CGC_SWR);
	MSTP(CMT0) = 0; /* Release STOP moduce for CMT0(bit15) */
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_CGC_SWR);

	*cmt0_cmcr = 0x00C0; /* enable CMT0 interrupt */

	clock_cycles_per_tick = (uint16_t)(CYCLES_PER_TICK);
	*cmt0_cmcor = clock_cycles_per_tick - 1;

	last_load = CYCLES_PER_TICK;

	IRQ_CONNECT(CMT0_IRQ_NUM, 0x01, cmt0_isr, NULL, 0);
	irq_enable(CMT0_IRQ_NUM);

	WRITE_BIT(*cmt_cmstr0, 0, 1); /* start cmt0 */

	cycle_count = 0;

	return 0;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL) && idle && ticks == K_TICKS_FOREVER) {
		volatile uint16_t *cmt_cmstr0 = (uint16_t *)CMT_CMSTR0_ADDR; /* CMSTR0 */
		WRITE_BIT(*cmt_cmstr0, 0, 0);                                /* stop cmt0 */
		last_load = TIMER_STOPPED;
		return;
	}

#if defined(CONFIG_TICKLESS_KERNEL)
	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	volatile uint16_t *cmt0_cmcor = (uint16_t *)CMT0_CMCOR_ADDR; /* CMCOR */
	*cmt0_cmcor = ((uint16_t)ticks) * clock_cycles_per_tick;
#endif /* CONFIG_TICKLESS_KERNEL */
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
