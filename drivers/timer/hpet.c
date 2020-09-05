/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_hpet
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <irq.h>

#include <dt-bindings/interrupt-controller/intel-ioapic.h>

DEVICE_MMIO_TOPLEVEL_STATIC(hpet_regs, DT_DRV_INST(0));

#define HPET_REG32(off) (*(volatile uint32_t *)(long)			\
			 (DEVICE_MMIO_TOPLEVEL_GET(hpet_regs) + (off)))

#define CLK_PERIOD_REG        HPET_REG32(0x04) /* High dword of caps reg */
#define GENERAL_CONF_REG      HPET_REG32(0x10)
#define INTR_STATUS_REG       HPET_REG32(0x20)
#define MAIN_COUNTER_REG      HPET_REG32(0xf0)
#define TIMER0_CONF_REG       HPET_REG32(0x100)
#define TIMER0_COMPARATOR_REG HPET_REG32(0x108)

/* GENERAL_CONF_REG bits */
#define GCONF_ENABLE BIT(0)
#define GCONF_LR     BIT(1) /* legacy interrupt routing, disables PIT */

/* INTR_STATUS_REG bits */
#define TIMER0_INT_STS   BIT(0)

/* TIMERn_CONF_REG bits */
#define TCONF_INT_LEVEL  BIT(1)
#define TCONF_INT_ENABLE BIT(2)
#define TCONF_PERIODIC   BIT(3)
#define TCONF_VAL_SET    BIT(6)
#define TCONF_MODE32     BIT(8)
#define TCONF_FSB_EN     BIT(14) /* FSB interrupt delivery enable */

#define MIN_DELAY 1000

static struct k_spinlock lock;
static unsigned int max_ticks;
static unsigned int cyc_per_tick;
static unsigned int last_count;

static void hpet_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t now = MAIN_COUNTER_REG;

#if ((DT_INST_IRQ(0, sense) & IRQ_TYPE_LEVEL) == IRQ_TYPE_LEVEL)
	/*
	 * Clear interrupt only if level trigger is selected.
	 * When edge trigger is selected, spec says only 0 can
	 * be written.
	 */
	INTR_STATUS_REG = TIMER0_INT_STS;
#endif

	if (IS_ENABLED(CONFIG_SMP) &&
	    IS_ENABLED(CONFIG_QEMU_TARGET)) {
		/* Qemu in SMP mode has observed the clock going
		 * "backwards" relative to interrupts already received
		 * on the other CPU, despite the HPET being
		 * theoretically a global device.
		 */
		int32_t diff = (int32_t)(now - last_count);

		if (last_count && diff < 0) {
			now = last_count;
		}
	}
	uint32_t dticks = (now - last_count) / cyc_per_tick;

	last_count += dticks * cyc_per_tick;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t next = last_count + cyc_per_tick;

		if ((int32_t)(next - now) < MIN_DELAY) {
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
	uint32_t val = (TIMER0_CONF_REG & ~(0x1f << 9)) | ((irq & 0x1f) << 9);

#if ((DT_INST_IRQ(0, sense) & IRQ_TYPE_LEVEL) == IRQ_TYPE_LEVEL)
	/* Level trigger */
	val |= TCONF_INT_LEVEL;
#endif

	TIMER0_CONF_REG = val;
}

int z_clock_driver_init(const struct device *device)
{
	extern int z_clock_hw_cycles_per_sec;
	uint32_t hz;

	ARG_UNUSED(device);

	DEVICE_MMIO_TOPLEVEL_MAP(hpet_regs, K_MEM_CACHE_NONE);

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    hpet_isr, 0, DT_INST_IRQ(0, sense));
	set_timer0_irq(DT_INST_IRQN(0));
	irq_enable(DT_INST_IRQN(0));

	/* CLK_PERIOD_REG is in femtoseconds (1e-15 sec) */
	hz = (uint32_t)(1000000000000000ull / CLK_PERIOD_REG);
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
	TIMER0_CONF_REG &= ~TCONF_FSB_EN;
	TIMER0_CONF_REG |= TCONF_MODE32;

	max_ticks = (0x7fffffff - cyc_per_tick) / cyc_per_tick;
	last_count = MAIN_COUNTER_REG;

	TIMER0_CONF_REG |= TCONF_INT_ENABLE;
	TIMER0_COMPARATOR_REG = MAIN_COUNTER_REG + cyc_per_tick;

	return 0;
}

void smp_timer_init(void)
{
	/* Noop, the HPET is a single system-wide device and it's
	 * configured to deliver interrupts to every CPU, so there's
	 * nothing to do at initialization on auxiliary CPUs.
	 */
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)
	if (ticks == K_TICKS_FOREVER && idle) {
		GENERAL_CONF_REG &= ~GCONF_ENABLE;
		return;
	}

	ticks = ticks == K_TICKS_FOREVER ? max_ticks : ticks;
	ticks = MAX(MIN(ticks - 1, (int32_t)max_ticks), 0);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t now = MAIN_COUNTER_REG, cyc, adj;
	uint32_t max_cyc = max_ticks * cyc_per_tick;

	/* Round up to next tick boundary. */
	cyc = ticks * cyc_per_tick;
	adj = (now - last_count) + (cyc_per_tick - 1);
	if (cyc <= max_cyc - adj) {
		cyc += adj;
	} else {
		cyc = max_cyc;
	}
	cyc = (cyc / cyc_per_tick) * cyc_per_tick;
	cyc += last_count;

	if ((cyc - now) < MIN_DELAY) {
		cyc += cyc_per_tick;
	}

	TIMER0_COMPARATOR_REG = cyc;
	k_spin_unlock(&lock, key);
#endif
}

uint32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = (MAIN_COUNTER_REG - last_count) / cyc_per_tick;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t z_timer_cycle_get_32(void)
{
	return MAIN_COUNTER_REG;
}

void z_clock_idle_exit(void)
{
	GENERAL_CONF_REG |= GCONF_ENABLE;
}
