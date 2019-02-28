/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>

#define HPET_REG32(off) (*(volatile u32_t *)(long)			\
		       (CONFIG_HPET_TIMER_BASE_ADDRESS + (off)))

#define CLK_PERIOD_REG        HPET_REG32(0x04) /* High dword of caps reg */
#define GENERAL_CONF_REG      HPET_REG32(0x10)
#define MAIN_COUNTER_REG      HPET_REG32(0xf0)
#define TIMER0_CONF_REG       HPET_REG32(0x100)
#define TIMER0_COMPARATOR_REG HPET_REG32(0x108)

/* GENERAL_CONF_REG bits */
#define GCONF_ENABLE BIT(0)
#define GCONF_LR     BIT(1) /* legacy interrupt routing, disables PIT */

/* TIMERn_CONF_REG bits */
#define TCONF_INT_ENABLE BIT(2)
#define TCONF_PERIODIC   BIT(3)
#define TCONF_VAL_SET    BIT(6)
#define TCONF_MODE32     BIT(8)

#define MIN_DELAY 1000

static struct k_spinlock lock;
static unsigned int max_ticks;
static unsigned int cyc_per_tick;
static unsigned int last_count;

static void hpet_isr(void *arg)
{
	ARG_UNUSED(arg);
	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t now = MAIN_COUNTER_REG;
	u32_t dticks = (now - last_count) / cyc_per_tick;

	last_count += dticks * cyc_per_tick;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL) ||
	    IS_ENABLED(CONFIG_QEMU_TICKLESS_WORKAROUND)) {
		u32_t next = last_count + cyc_per_tick;

		if ((s32_t)(next - now) < MIN_DELAY) {
			next += cyc_per_tick;
		}
		TIMER0_COMPARATOR_REG = next;
	}

	k_spin_unlock(&lock, key);
	z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}

static void set_timer0_irq(unsigned int irq)
{
	/* 5-bit IRQ field starting at bit 9 */
	u32_t val = (TIMER0_CONF_REG & ~(0x1f << 9)) | ((irq & 0x1f) << 9);

	TIMER0_CONF_REG = val;
}

int z_clock_driver_init(struct device *device)
{
	extern int z_clock_hw_cycles_per_sec;
	u32_t hz;

	IRQ_CONNECT(CONFIG_HPET_TIMER_IRQ, CONFIG_HPET_TIMER_IRQ_PRIORITY,
		    hpet_isr, 0, 0);
	set_timer0_irq(CONFIG_HPET_TIMER_IRQ);
	irq_enable(CONFIG_HPET_TIMER_IRQ);

	/* CLK_PERIOD_REG is in femtoseconds (1e-15 sec) */
	hz = (u32_t)(1000000000000000ull / CLK_PERIOD_REG);
	z_clock_hw_cycles_per_sec = hz;
	cyc_per_tick = hz / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	/* Note: we set the legacy routing bit, because otherwise
	 * nothing in Zephyr disables the PIT which then fires
	 * interrupts into the same IRQ.  But that means we're then
	 * forced to use IRQ2 contra the way the kconfig IRQ selection
	 * is supposed to work.  Should fix this.
	 */
	GENERAL_CONF_REG |= GCONF_LR | GCONF_ENABLE;
	TIMER0_CONF_REG &= ~TCONF_PERIODIC;
	TIMER0_CONF_REG |= TCONF_MODE32;

	max_ticks = (0x7fffffff - cyc_per_tick) / cyc_per_tick;
	last_count = MAIN_COUNTER_REG;

	TIMER0_CONF_REG |= TCONF_INT_ENABLE;
	TIMER0_COMPARATOR_REG = MAIN_COUNTER_REG + cyc_per_tick;

	return 0;
}

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL) && !defined(CONFIG_QEMU_TICKLESS_WORKAROUND)
	if (ticks == K_FOREVER && idle) {
		GENERAL_CONF_REG &= ~GCONF_ENABLE;
		return;
	}

	ticks = ticks == K_FOREVER ? max_ticks : ticks;
	ticks = MAX(MIN(ticks - 1, (s32_t)max_ticks), 0);

	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t now = MAIN_COUNTER_REG, cyc;

	/* Round up to next tick boundary */
	cyc = ticks * cyc_per_tick + (now - last_count) + (cyc_per_tick - 1);
	cyc = (cyc / cyc_per_tick) * cyc_per_tick;
	cyc += last_count;

	if ((cyc - now) < MIN_DELAY) {
		cyc += cyc_per_tick;
	}

	TIMER0_COMPARATOR_REG = cyc;
	k_spin_unlock(&lock, key);
#endif
}

u32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ret = (MAIN_COUNTER_REG - last_count) / cyc_per_tick;

	k_spin_unlock(&lock, key);
	return ret;
}

u32_t _timer_cycle_get_32(void)
{
	return MAIN_COUNTER_REG;
}

void z_clock_idle_exit(void)
{
	GENERAL_CONF_REG |= GCONF_ENABLE;
}
