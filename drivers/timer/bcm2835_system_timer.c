/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2835_system_timer

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define BCM2835_STIMER_BASE	DT_INST_REG_ADDR(0)
#define BCM2835_STIMER_IRQN	DT_INST_IRQN(0)
#define BCM2835_STIMER_CS	(BCM2835_STIMER_BASE + 0x00)
#define BCM2835_STIMER_CLO	(BCM2835_STIMER_BASE + 0x04)
#define BCM2835_STIMER_CHI	(BCM2835_STIMER_BASE + 0x08)
#define BCM2835_STIMER_C0	(BCM2835_STIMER_BASE + 0x0c)
#define BCM2835_STIMER_C1	(BCM2835_STIMER_BASE + 0x10)
#define BCM2835_STIMER_C2	(BCM2835_STIMER_BASE + 0x14)
#define BCM2835_STIMER_C3	(BCM2835_STIMER_BASE + 0x18)
#define BCM2835_STIMER_MATCH3	BIT(3)
#define BCM2835_STIMER_MIN_DELAY_CYCLES 2U

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = BCM2835_STIMER_IRQN;
#endif

static struct k_spinlock lock;
static uint32_t cycles_per_tick;
static uint32_t last_cycle;
static uint32_t last_tick;
static uint32_t last_elapsed;

static inline uint32_t max_programmable_ticks(void)
{
	return (UINT32_MAX / 2U) / cycles_per_tick;
}

static void bcm2835_program_compare(uint32_t next_cycle)
{
	uint32_t now = sys_read32(BCM2835_STIMER_CLO);

	if ((int32_t)(next_cycle - now) < (int32_t)BCM2835_STIMER_MIN_DELAY_CYCLES) {
		next_cycle = now + BCM2835_STIMER_MIN_DELAY_CYCLES;
	}

	sys_write32(next_cycle, BCM2835_STIMER_C3);
}

static void bcm2835_system_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t now;
	uint32_t delta_cycles;
	uint32_t delta_ticks;
	k_spinlock_key_t key = k_spin_lock(&lock);

	sys_write32(BCM2835_STIMER_MATCH3, BCM2835_STIMER_CS);

	now = sys_read32(BCM2835_STIMER_CLO);
	delta_cycles = now - last_cycle;
	delta_ticks = delta_cycles / cycles_per_tick;
	if (delta_ticks == 0U) {
		bcm2835_program_compare(last_cycle + cycles_per_tick);
		k_spin_unlock(&lock, key);
		return;
	}

	last_cycle += delta_ticks * cycles_per_tick;
	last_tick += delta_ticks;
	last_elapsed = 0U;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		bcm2835_program_compare(last_cycle + cycles_per_tick);
	}

	k_spin_unlock(&lock, key);

	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	k_spinlock_key_t key;
	uint32_t now;
	uint32_t max_ticks;
	uint32_t next_cycle;
	uint32_t target_ticks;

	key = k_spin_lock(&lock);

	now = sys_read32(BCM2835_STIMER_CLO);
	last_elapsed = (now - last_cycle) / cycles_per_tick;

	max_ticks = max_programmable_ticks();
	if (ticks == K_TICKS_FOREVER) {
		ticks = max_ticks;
	} else {
		ticks = CLAMP(ticks, 0, (int32_t)max_ticks);
	}

	target_ticks = last_tick + last_elapsed + MAX(ticks, 1);
	next_cycle = target_ticks * cycles_per_tick;
	if ((next_cycle - last_cycle) > (max_ticks * cycles_per_tick)) {
		next_cycle = last_cycle + max_ticks * cycles_per_tick;
	}
	while ((int32_t)(next_cycle - now) < (int32_t)BCM2835_STIMER_MIN_DELAY_CYCLES) {
		next_cycle += cycles_per_tick;
	}

	bcm2835_program_compare(next_cycle);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0U;
	}

	k_spinlock_key_t key;
	uint32_t now;
	uint32_t delta_ticks;

	key = k_spin_lock(&lock);

	now = sys_read32(BCM2835_STIMER_CLO);
	delta_ticks = (now - last_cycle) / cycles_per_tick;
	last_elapsed = delta_ticks;

	k_spin_unlock(&lock, key);

	return delta_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return sys_read32(BCM2835_STIMER_CLO);
}

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
uint64_t sys_clock_cycle_get_64(void)
{
	uint32_t hi0;
	uint32_t lo;
	uint32_t hi1;

	do {
		hi0 = sys_read32(BCM2835_STIMER_CHI);
		lo = sys_read32(BCM2835_STIMER_CLO);
		hi1 = sys_read32(BCM2835_STIMER_CHI);
	} while (hi0 != hi1);

	return ((uint64_t)hi1 << 32) | lo;
}
#endif

static int sys_clock_driver_init(void)
{
	cycles_per_tick = DT_INST_PROP(0, clock_frequency) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	last_cycle = sys_read32(BCM2835_STIMER_CLO);
	last_tick = last_cycle / cycles_per_tick;
	last_cycle = last_tick * cycles_per_tick;
	last_elapsed = 0U;

	IRQ_CONNECT(BCM2835_STIMER_IRQN, 0, bcm2835_system_timer_isr, NULL, 0);
	sys_write32(BCM2835_STIMER_MATCH3, BCM2835_STIMER_CS);
	bcm2835_program_compare(last_cycle + cycles_per_tick);
	irq_enable(BCM2835_STIMER_IRQN);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
