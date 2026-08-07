/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_rv32_sys_timer

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/interrupt_controller/intc_max32_rv32.h>
#include <soc.h>
#include <zephyr/irq.h>

#include <wrap_max32_tmr.h>

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_INST_IRQN(0);
#endif

#define CYC_PER_SEC  sys_clock_hw_cycles_per_sec()
#define CYC_PER_TICK (CYC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC / DT_INST_PROP(0, prescaler))

static mxc_tmr_regs_t *regs = (mxc_tmr_regs_t *)DT_INST_REG_ADDR(0);

static const struct max32_perclk perclk = {
	.bus = DT_INST_CLOCKS_CELL(0, offset),
	.bit = DT_INST_CLOCKS_CELL(0, bit),
};

#define TIMER_CLOCK DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0))
#define PRESCALER   DT_INST_PROP(0, prescaler)

#define COMPARE_VAL ((CYC_PER_TICK))

#if PRESCALER == 0
#define PRES_VAL TMR_PRES_1
#else
#define PRES_VAL CONCAT(TMR_PRES_, PRESCALER)
#endif

/*
 * Free-running counter plus a compare register whose match is on equality, so
 * a value written after the counter has passed it is missed for a whole
 * counter period: a COMPARE_EXACT backend. The core writes the comparator
 * through its verify loop, which replaces the driver's own push-forward and
 * its sixth-of-a-tick minimum. The compare register is as wide as the counter,
 * so there is no arm range to state and the core's own bound applies.
 *
 * The counter runs behind the prescaler, so the driver states that rate and
 * the core derives the same cycles per tick CYC_PER_TICK is. The kernel's
 * cycle unit is the pre-prescaler one, so the driver keeps its own
 * sys_clock_cycle_get_32().
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_CYCLES_PER_SEC (CYC_PER_SEC / PRESCALER)
#define TIMER_CORE_HAVE_CYCLE_GET_32

static inline uint32_t timer_driver_cycle_get(void)
{
	return MXC_TMR_GetCount(regs);
}

static inline void timer_driver_set_compare(uint32_t cycles)
{
	MXC_TMR_SetCompare(regs, cycles);
}

#include "system_timer_generic.h"

static void rv32_sys_timer_irq_handler(const struct device *unused)
{
	ARG_UNUSED(unused);

	MXC_TMR_ClearFlags(regs);
	/* The IRQ will re-assert until the flags on the timer are cleared. */
	intc_max32_rv32_irq_clear_pending(DT_INST_IRQN(0));

	timer_core_announce();
}

uint32_t sys_clock_cycle_get_32(void)
{
	return MXC_TMR_GetCount(regs) * DT_INST_PROP(0, prescaler);
}

static int sys_clock_driver_init(void)
{
	wrap_mxc_tmr_cfg_t tmr_cfg;
	int ret;

	IRQ_CONNECT(DT_INST_IRQN(0), 0, rv32_sys_timer_irq_handler, NULL, 0);

	tmr_cfg.pres = PRES_VAL;
	tmr_cfg.mode = TMR_MODE_COMPARE;
	/* Placeholder: timer_core_init() below arms the first real deadline. */
	tmr_cfg.cmp_cnt = COMPARE_VAL;

	tmr_cfg.bitMode = 0; /* Timer Mode 32 bit */
	tmr_cfg.pol = 0;

	tmr_cfg.clock = Wrap_MXC_TMR_GetClockIndex(DT_INST_PROP(0, clock_source));
	if (tmr_cfg.clock < 0) {
		return -ENOTSUP;
	}

	MXC_TMR_Shutdown(regs);

	/* enable clock */
	ret = clock_control_on(TIMER_CLOCK, (clock_control_subsys_t)&perclk);
	if (ret) {
		return ret;
	}

	ret = Wrap_MXC_TMR_Init(regs, &tmr_cfg);
	if (ret != E_NO_ERROR) {
		return ret;
	}

	/* Be sure our start point is 0x0, not 0x1 as set on reset. */
	MXC_TMR_SetCount(regs, 0);

	MXC_TMR_ClearFlags(regs);
	Wrap_MXC_TMR_EnableInt(regs);

	MXC_TMR_Start(regs);

	/* Seed the announce baseline and arm the first tick. */
	timer_core_init();

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
