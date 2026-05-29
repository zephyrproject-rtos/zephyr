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
#define MAX_TIMEOUT ((UINT32_MAX / COMPARE_VAL) - 1)

static struct k_spinlock lock;
static uint32_t last_cycle;
static uint32_t last_tick;
static uint32_t last_elapsed;

#define CYCLE_DIFF_MAX (~(uint32_t)0)

#define CYCLES_MAX_1 ((uint64_t)INT32_MAX * (uint64_t)CYC_PER_TICK)
#define CYCLES_MAX_2 ((uint64_t)CYCLE_DIFF_MAX)
#define CYCLES_MAX_3 MIN(CYCLES_MAX_1, CYCLES_MAX_2)
#define CYCLES_MAX_4 (CYCLES_MAX_3 / 2 + CYCLES_MAX_3 / 4)
#define CYCLES_MAX_5 (CYCLES_MAX_4 + LSB_GET(CYCLES_MAX_4))

#define CYCLES_MAX CYCLES_MAX_5

#if PRESCALER == 0
#define PRES_VAL TMR_PRES_1
#else
#define PRES_VAL CONCAT(TMR_PRES_, PRESCALER)
#endif

static void rv32_sys_timer_irq_handler(const struct device *unused)
{
	ARG_UNUSED(unused);
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);

	uint32_t curr_cycle = MXC_TMR_GetCount(regs);
	uint32_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = (uint32_t)delta_cycles / CYC_PER_TICK;

	last_cycle += (uint32_t)delta_ticks * CYC_PER_TICK;
	last_tick += delta_ticks;
	last_elapsed = 0;

	MXC_TMR_ClearFlags(regs);
	/* The IRQ will re-assert until the flags on the timer are cleared. */
	intc_max32_rv32_irq_clear_pending(DT_INST_IRQN(0));

#if !IS_ENABLED(CONFIG_TICKLESS_KERNEL)
	MXC_TMR_SetCompare(regs, last_cycle + CYC_PER_TICK);
#endif

	k_spin_unlock(&lock, key);

	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? delta_ticks : 1);
}

uint32_t sys_clock_cycle_get_32(void)
{
	return MXC_TMR_GetCount(regs) * DT_INST_PROP(0, prescaler);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	uint32_t curr_cycle = MXC_TMR_GetCount(regs);
	k_spinlock_key_t key = k_spin_lock(&lock);

	int32_t delta_cycles = curr_cycle - last_cycle;
	int32_t delta_ticks = (uint32_t)delta_cycles / CYC_PER_TICK;

	last_elapsed = delta_ticks;
	k_spin_unlock(&lock, key);

	return delta_ticks;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (idle && ticks == K_TICKS_FOREVER) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t next_cycle;
	uint32_t count;

	if (ticks == INT32_MAX) {
		next_cycle = (last_tick * CYC_PER_TICK) + CYCLES_MAX;
	} else if (ticks == 0) {
		next_cycle = MXC_TMR_GetCount(regs) + (CYC_PER_TICK * 3 / 2);
		next_cycle -= (next_cycle % CYC_PER_TICK);
	} else {
		next_cycle = (last_tick + last_elapsed + ticks) * CYC_PER_TICK;
		if ((next_cycle - last_cycle) > CYCLES_MAX) {
			next_cycle = (last_tick * CYC_PER_TICK) + CYCLES_MAX;
		} else {
			count = MXC_TMR_GetCount(regs);
			if (next_cycle < count) {
				ticks = DIV_ROUND_UP(count - next_cycle, CYC_PER_TICK);
				ticks += 1;
				next_cycle += ticks * CYC_PER_TICK;
			} else if (next_cycle - count < CYC_PER_TICK / 6) {
				next_cycle += CYC_PER_TICK;
			}
		}
	}

	MXC_TMR_SetCompare(regs, next_cycle);
	k_spin_unlock(&lock, key);
}

static int sys_clock_driver_init(void)
{
	wrap_mxc_tmr_cfg_t tmr_cfg;
	int ret;

	IRQ_CONNECT(DT_INST_IRQN(0), 0, rv32_sys_timer_irq_handler, NULL, 0);

	tmr_cfg.pres = PRES_VAL;
	tmr_cfg.mode = TMR_MODE_COMPARE;
#if IS_ENABLED(CONFIG_TICKLESS_KERNEL)
	tmr_cfg.cmp_cnt = (MAX_TIMEOUT * COMPARE_VAL);
#else
	tmr_cfg.cmp_cnt = COMPARE_VAL;
#endif

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

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
