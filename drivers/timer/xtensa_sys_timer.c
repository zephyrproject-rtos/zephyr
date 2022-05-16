/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>

#define TIMER_IRQ UTIL_CAT(XCHAL_TIMER,		\
			   UTIL_CAT(CONFIG_XTENSA_TIMER_ID, _INTERRUPT))

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_CYC 0xffffffffu
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1000

static struct k_spinlock lock;
static unsigned int last_count;

static void set_ccompare(uint32_t val)
{
	__asm__ volatile ("wsr.CCOMPARE" STRINGIFY(CONFIG_XTENSA_TIMER_ID) " %0"
			  :: "r"(val));
}

static uint32_t ccount(void)
{
	uint32_t val;

	__asm__ volatile ("rsr.CCOUNT %0" : "=r"(val));
	return val;
}

static void ccompare_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t curr = ccount();
	uint32_t dticks = (curr - last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t next = last_count + CYC_PER_TICK;

		if ((int32_t)(next - curr) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_ccompare(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)
	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t curr = ccount(), cyc, adj;

	/* Round up to next tick boundary */
	cyc = ticks * CYC_PER_TICK;
	adj = (curr - last_count) + (CYC_PER_TICK - 1);
	if (cyc <= MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = MAX_CYC;
	}
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;
	cyc += last_count;

	if ((cyc - curr) < MIN_DELAY) {
		cyc += CYC_PER_TICK;
	}

	set_ccompare(cyc);
	k_spin_unlock(&lock, key);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = (ccount() - last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return ccount();
}

#ifdef CONFIG_SMP
void smp_timer_init(void)
{
	set_ccompare(ccount() + CYC_PER_TICK);
	irq_enable(TIMER_IRQ);
}
#endif

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(TIMER_IRQ, 0, ccompare_isr, 0, 0);
	set_ccompare(ccount() + CYC_PER_TICK);
	irq_enable(TIMER_IRQ);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
