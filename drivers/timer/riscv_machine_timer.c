/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>

/* andestech,machine-timer */
#if DT_HAS_COMPAT_STATUS_OKAY(andestech_machine_timer)
#define DT_DRV_COMPAT andestech_machine_timer

#define MTIME_REG	DT_INST_REG_ADDR(0)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
/* neorv32-machine-timer */
#elif DT_HAS_COMPAT_STATUS_OKAY(neorv32_machine_timer)
#define DT_DRV_COMPAT neorv32_machine_timer

#define MTIME_REG	DT_INST_REG_ADDR(0)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
/* nuclei,systimer */
#elif DT_HAS_COMPAT_STATUS_OKAY(nuclei_systimer)
#define DT_DRV_COMPAT nuclei_systimer

#define MTIME_REG	DT_INST_REG_ADDR(0)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQ_BY_IDX(0, 1, irq)
/* sifive,clint0 */
#elif DT_HAS_COMPAT_STATUS_OKAY(sifive_clint0)
#define DT_DRV_COMPAT sifive_clint0

#define MTIME_REG	(DT_INST_REG_ADDR(0) + 0xbff8U)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 0x4000U)
#define TIMER_IRQN	DT_INST_IRQ_BY_IDX(0, 1, irq)
/* telink,machine-timer */
#elif DT_HAS_COMPAT_STATUS_OKAY(telink_machine_timer)
#define DT_DRV_COMPAT telink_machine_timer

#define MTIME_REG	DT_INST_REG_ADDR(0)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 8)
#define TIMER_IRQN	DT_INST_IRQN(0)
/* lowrisc,machine-timer */
#elif DT_HAS_COMPAT_STATUS_OKAY(lowrisc_machine_timer)
#define DT_DRV_COMPAT lowrisc_machine_timer

#define MTIME_REG	(DT_INST_REG_ADDR(0) + 0x110)
#define MTIMECMP_REG	(DT_INST_REG_ADDR(0) + 0x118)
#define TIMER_IRQN	DT_INST_IRQN(0)
#endif

#define CYC_PER_TICK ((uint32_t)((uint64_t) (sys_clock_hw_cycles_per_sec()			 \
					     >> CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER) \
				 / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define MAX_CYC INT_MAX
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY CONFIG_RISCV_MACHINE_TIMER_MIN_DELAY

#define TICKLESS IS_ENABLED(CONFIG_TICKLESS_KERNEL)

static struct k_spinlock lock;
static uint64_t last_count;
#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQN;
#endif

static uint64_t get_hart_mtimecmp(void)
{
	return MTIMECMP_REG + (arch_proc_id() * 8);
}

static void set_mtimecmp(uint64_t time)
{
#ifdef CONFIG_64BIT
	*(volatile uint64_t *)get_hart_mtimecmp() = time;
#else
	volatile uint32_t *r = (uint32_t *)(uint32_t)get_hart_mtimecmp();

	/* Per spec, the RISC-V MTIME/MTIMECMP registers are 64 bit,
	 * but are NOT internally latched for multiword transfers.  So
	 * we have to be careful about sequencing to avoid triggering
	 * spurious interrupts: always set the high word to a max
	 * value first.
	 */
	r[1] = 0xffffffff;
	r[0] = (uint32_t)time;
	r[1] = (uint32_t)(time >> 32);
#endif
}

static uint64_t mtime(void)
{
#ifdef CONFIG_64BIT
	return *(volatile uint64_t *)MTIME_REG;
#else
	volatile uint32_t *r = (uint32_t *)MTIME_REG;
	uint32_t lo, hi;

	/* Likewise, must guard against rollover when reading */
	do {
		hi = r[1];
		lo = r[0];
	} while (r[1] != hi);

	return (((uint64_t)hi) << 32) | lo;
#endif
}

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = mtime();
	uint32_t dticks = (uint32_t)((now - last_count) / CYC_PER_TICK);

	last_count = now;

	if (!TICKLESS) {
		uint64_t next = last_count + CYC_PER_TICK;

		if ((int64_t)(next - now) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_mtimecmp(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)
	/* RISCV has no idle handler yet, so if we try to spin on the
	 * logic below to reset the comparator, we'll always bump it
	 * forward to the "next tick" due to MIN_DELAY handling and
	 * the interrupt will never fire!  Just rely on the fact that
	 * the OS gave us the proper timeout already.
	 */
	if (idle) {
		return;
	}

	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = mtime();
	uint32_t adj, cyc = ticks * CYC_PER_TICK;

	/* Round up to next tick boundary. */
	adj = (uint32_t)(now - last_count) + (CYC_PER_TICK - 1);
	if (cyc <= MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = MAX_CYC;
	}
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;

	if ((int32_t)(cyc + last_count - now) < MIN_DELAY) {
		cyc += CYC_PER_TICK;
	}

	set_mtimecmp(cyc + last_count);
	k_spin_unlock(&lock, key);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = ((uint32_t)mtime() - (uint32_t)last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)(mtime() << CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER);
}

uint64_t sys_clock_cycle_get_64(void)
{
	return (mtime() << CONFIG_RISCV_MACHINE_TIMER_SYSTEM_CLOCK_DIVIDER);
}

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(TIMER_IRQN, 0, timer_isr, NULL, 0);
	last_count = mtime();
	set_mtimecmp(last_count + CYC_PER_TICK);
	irq_enable(TIMER_IRQN);
	return 0;
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	set_mtimecmp(last_count + CYC_PER_TICK);
	irq_enable(TIMER_IRQN);
}
#endif

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
