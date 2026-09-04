/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/clock.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/clock_control/renesas_rx_cgc.h>

#define DT_DRV_COMPAT renesas_rx_timer_cmt

#define CMT_NODE  DT_NODELABEL(cmt)
#define CMT0_NODE DT_NODELABEL(cmt0)
#define CMT1_NODE DT_NODELABEL(cmt1)

#define CMT0_IRQ_NUM DT_IRQ_BY_NAME(CMT0_NODE, cmi, irq)
#define CMT1_IRQN    DT_IRQN(CMT1_NODE)

#define ICU_NODE DT_NODELABEL(icu)
#define ICU_IR   ((volatile uint8_t *)DT_REG_ADDR_BY_NAME(ICU_NODE, IR))

#define COUNTER_MAX 0x0000ffff
/* CMT1 counts 0..CMCOR then resets, so a whole period is one more than that. */
#define CYCLES_CYCLE_TIMER (COUNTER_MAX + 1)

static const struct clock_control_rx_subsys_cfg cmt_clk_cfg = {
	.mstp = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(0), 0, mstp),
	.stop_bit = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(0), 0, stop_bit),
};

struct timer_rx_cfg {
	volatile uint16_t *cmstr;
	volatile uint16_t *cmcr;
	volatile uint16_t *cmcnt;
	volatile uint16_t *cmcor;
};

static const struct timer_rx_cfg tick_timer_cfg = {
	.cmstr = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT_NODE, CMSTR0),
	.cmcr = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT0_NODE, CMCR),
	.cmcnt = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT0_NODE, CMCNT),
	.cmcor = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT0_NODE, CMCOR)};

static const struct timer_rx_cfg cycle_timer_cfg = {
	.cmstr = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT_NODE, CMSTR0),
	.cmcr = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT1_NODE, CMCR),
	.cmcnt = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT1_NODE, CMCNT),
	.cmcor = (uint16_t *)DT_REG_ADDR_BY_NAME(CMT1_NODE, CMCOR)};

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = CMT0_IRQ_NUM;
#endif

/*
 * Two timers: CMT1 free-runs as the cycle source, widened by the core from the
 * announce baseline, while CMT0 raises the tick interrupt. CMT0's CMCOR is a
 * period rather than a deadline, since a match resets CMCNT, so this is the
 * RELOAD backend.
 */
#define TIMER_CORE_BACKEND_RELOAD

/* CMCNT is only 16 bits, which is 10.9 ms at 6 MHz, too short to be read
 * straight: k_busy_wait() with interrupts masked has to keep working for well
 * over a second, poor practice though that is, and the core's masked delta
 * aliases past one period. So the count is extended in software here, and the
 * core is handed 32 bits. Each read folds a wrap in, which is what keeps the
 * extension alive while interrupts are masked and no ISR can run. Reading
 * mutates that state, hence TIMER_CORE_COUNTER_NONATOMIC.
 *
 * The alarm is unaffected: CMT0's CMCOR is still 16 bits, so
 * TIMER_CORE_ALARM_MAX_CYCLES below keeps the armed delay inside one period.
 */
#define TIMER_CORE_COUNTER_WIDTH 32
#define TIMER_CORE_COUNTER_NONATOMIC

/* One reload cannot express more than CMT0's 16-bit CMCOR holds. */
#define TIMER_CORE_ALARM_MAX_CYCLES COUNTER_MAX

/* A reload of one would program CMCOR == 0 against a just-cleared CMCNT, which
 * matches on every count. Keep the floor one above that.
 */
#define TIMER_CORE_ALARM_MIN_CYCLES 2

/* Whole CMT1 periods consumed, the upper bits of the extended count. */
static uint32_t cycle_count;

static uint32_t timer_driver_cycle_get(void)
{
	uint16_t val1 = *cycle_timer_cfg.cmcnt;
	bool matched = ICU_IR[CMT1_IRQN] != 0;
	uint16_t val2 = *cycle_timer_cfg.cmcnt;

	/* The compare-match flag catches a wrap that happened before this read,
	 * and val1 > val2 one that happened during it.
	 */
	if (matched || (val1 > val2)) {
		cycle_count += CYCLES_CYCLE_TIMER;
		ICU_IR[CMT1_IRQN] = 0;
	}

	return cycle_count + val2;
}

static inline void timer_driver_set_reload(uint32_t cycles)
{
	/* CMT0 exists only to raise the tick interrupt: the cycle domain is
	 * CMT1, so nothing reads CMT0's count and it can be restarted here.
	 * That makes CMCOR the plain relative delay the core asks for, with no
	 * need to bias it by the count in flight.
	 */
	*tick_timer_cfg.cmcnt = 0;
	*tick_timer_cfg.cmcor = (uint16_t)(cycles - 1U);
}

#include "system_timer_generic.h"

static void cmt0_isr(void)
{
	timer_core_announce();
}

static int sys_clock_driver_init(void)
{
	const struct device *clk = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(0)));
	int ret;

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	ret = clock_control_on(clk, (clock_control_subsys_t)&cmt_clk_cfg);
	if (ret < 0) {
		return ret;
	}

	*tick_timer_cfg.cmcr = 0x00C0;  /* enable CMT0 interrupt */
	*cycle_timer_cfg.cmcr = 0x00C0; /* enable CMT1 interrupt */

	/* A tickful kernel leaves the period alone after this, so set it here.
	 * A tickless one gets it from timer_core_init() below.
	 */
	*tick_timer_cfg.cmcor = (uint16_t)TIMER_CORE_CYC_PER_TICK - 1;
	*cycle_timer_cfg.cmcor = (uint16_t)COUNTER_MAX;

	IRQ_CONNECT(CMT0_IRQ_NUM, 0x01, cmt0_isr, NULL, 0);
	irq_enable(CMT0_IRQ_NUM);

	*tick_timer_cfg.cmstr = 0x0003; /* start cmt0,1 */

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
