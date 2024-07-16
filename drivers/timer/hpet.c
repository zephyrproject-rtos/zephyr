/*
 * Copyright (c) 2018-2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_hpet
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/linker/sections.h>

#include <zephyr/dt-bindings/interrupt-controller/intel-ioapic.h>

#include <soc.h>

/**
 * @file
 * @brief HPET (High Precision Event Timers) driver
 *
 * HPET hardware contains a number of timers which can be used by
 * the operating system, where the number of timers is implementation
 * specific. The timers are implemented as a single up-counter with
 * a set of comparators where the counter increases monotonically.
 * Each timer has a match register and a comparator, and can generate
 * an interrupt when the value in the match register equals the value of
 * the free running counter. Some of these timers can be enabled to
 * generate periodic interrupt.
 *
 * The HPET registers are usually mapped to memory space on x86
 * hardware. If this is not the case, custom register access functions
 * can be used by defining macro HPET_USE_CUSTOM_REG_ACCESS_FUNCS in
 * soc.h, and implementing necessary initialization and access
 * functions as described below.
 *
 * HPET_COUNTER_CLK_PERIOD can be overridden in soc.h if
 * COUNTER_CLK_PERIOD is not in femtoseconds (1e-15 sec).
 */

/* General Configuration register */
#define GCONF_ENABLE			BIT(0)
#define GCONF_LR			BIT(1) /* legacy interrupt routing, */
					       /* disables PIT              */

/* General Interrupt Status register */
#define TIMER0_INT_STS			BIT(0)

/* Timer Configuration and Capabilities register */
#define TIMER_CONF_INT_LEVEL		BIT(1)
#define TIMER_CONF_INT_ENABLE		BIT(2)
#define TIMER_CONF_PERIODIC		BIT(3)
#define TIMER_CONF_VAL_SET		BIT(6)
#define TIMER_CONF_MODE32		BIT(8)
#define TIMER_CONF_FSB_EN		BIT(14) /* FSB interrupt delivery   */
						/* enable                   */

DEVICE_MMIO_TOPLEVEL_STATIC(hpet_regs, DT_DRV_INST(0));

#define HPET_REG_ADDR(off)			\
	((mm_reg_t)(DEVICE_MMIO_TOPLEVEL_GET(hpet_regs) + (off)))

/* High dword of General Capabilities and ID register */
#define CLK_PERIOD_REG			HPET_REG_ADDR(0x04)

/* General Configuration register */
#define GCONF_REG			HPET_REG_ADDR(0x10)

/* General Interrupt Status register */
#define INTR_STATUS_REG			HPET_REG_ADDR(0x20)

/* Main Counter Register */
#define MAIN_COUNTER_LOW_REG		HPET_REG_ADDR(0xf0)
#define MAIN_COUNTER_HIGH_REG		HPET_REG_ADDR(0xf4)

/* Timer 0 Configuration and Capabilities register */
#define TIMER0_CONF_REG			HPET_REG_ADDR(0x100)

/* Timer 0 Comparator Register */
#define TIMER0_COMPARATOR_LOW_REG	HPET_REG_ADDR(0x108)
#define TIMER0_COMPARATOR_HIGH_REG	HPET_REG_ADDR(0x10c)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(DT_INST(0, intel_hpet));
#endif

/**
 * @brief Return the value of the main counter.
 *
 * @return Value of Main Counter
 */
static inline uint64_t hpet_counter_get(void)
{
#ifdef CONFIG_64BIT
	uint64_t val = sys_read64(MAIN_COUNTER_LOW_REG);

	return val;
#else
	uint32_t high;
	uint32_t low;

	do {
		high = sys_read32(MAIN_COUNTER_HIGH_REG);
		low = sys_read32(MAIN_COUNTER_LOW_REG);
	} while (high != sys_read32(MAIN_COUNTER_HIGH_REG));

	return ((uint64_t)high << 32) | low;
#endif
}

/**
 * @brief Get COUNTER_CLK_PERIOD
 *
 * Read and return the COUNTER_CLK_PERIOD, which is the high
 * 32-bit of the General Capabilities and ID Register. This can
 * be used to calculate the frequency of the main counter.
 *
 * Usually the period is in femtoseconds. If this is not
 * the case, define HPET_COUNTER_CLK_PERIOD in soc.h so
 * it can be used to calculate frequency.
 *
 * @return COUNTER_CLK_PERIOD
 */
static inline uint32_t hpet_counter_clk_period_get(void)
{
	return sys_read32(CLK_PERIOD_REG);
}

/**
 * @brief Return the value of the General Configuration Register
 *
 * @return Value of the General Configuration Register
 */
static inline uint32_t hpet_gconf_get(void)
{
	return sys_read32(GCONF_REG);
}

/**
 * @brief Write to General Configuration Register
 *
 * @param val Value to be written to the register
 */
static inline void hpet_gconf_set(uint32_t val)
{
	sys_write32(val, GCONF_REG);
}

/**
 * @brief Return the value of the Timer Configuration Register
 *
 * This reads and returns the value of the Timer Configuration
 * Register of Timer #0.
 *
 * @return Value of the Timer Configuration Register
 */
static inline uint32_t hpet_timer_conf_get(void)
{
	return sys_read32(TIMER0_CONF_REG);
}

/**
 * @brief Write to the Timer Configuration Register
 *
 * This writes the specified value to the Timer Configuration
 * Register of Timer #0.
 *
 * @param val Value to be written to the register
 */
static inline void hpet_timer_conf_set(uint32_t val)
{
	sys_write32(val, TIMER0_CONF_REG);
}

/*
 * The following register access functions should work on generic x86
 * hardware. If the targeted SoC requires special handling of HPET
 * registers, these functions will need to be implemented in the SoC
 * layer by first defining the macro HPET_USE_CUSTOM_REG_ACCESS_FUNCS
 * in soc.h to signal such intent.
 *
 * This is a list of functions which must be implemented in the SoC
 * layer:
 *   void hpet_timer_comparator_set(uint32_t val)
 */
#ifndef HPET_USE_CUSTOM_REG_ACCESS_FUNCS

/**
 * @brief Write to the Timer Comparator Value Register
 *
 * This writes the specified value to the Timer Comparator
 * Value Register of Timer #0.
 *
 * @param val Value to be written to the register
 */
static inline void hpet_timer_comparator_set(uint64_t val)
{
#if CONFIG_X86_64
	sys_write64(val, TIMER0_COMPARATOR_LOW_REG);
#else
	sys_write32((uint32_t)val, TIMER0_COMPARATOR_LOW_REG);
	sys_write32((uint32_t)(val >> 32), TIMER0_COMPARATOR_HIGH_REG);
#endif
}
#endif /* HPET_USE_CUSTOM_REG_ACCESS_FUNCS */

#ifndef HPET_COUNTER_CLK_PERIOD
/* COUNTER_CLK_PERIOD (CLK_PERIOD_REG) is in femtoseconds (1e-15 sec) */
#define HPET_COUNTER_CLK_PERIOD		(1000000000000000ULL)
#endif

/*
 * HPET_INT_LEVEL_TRIGGER is used to set HPET interrupt as level trigger
 * for ARM CPU with NVIC like EHL PSE, whose DTS interrupt setting
 * has no "sense" cell.
 */
#if (DT_INST_IRQ_HAS_CELL(0, sense))
#ifdef HPET_INT_LEVEL_TRIGGER
__WARN("HPET_INT_LEVEL_TRIGGER has no effect, DTS setting is used instead")
#undef HPET_INT_LEVEL_TRIGGER
#endif
#if ((DT_INST_IRQ(0, sense) & IRQ_TYPE_LEVEL) == IRQ_TYPE_LEVEL)
#define HPET_INT_LEVEL_TRIGGER
#endif
#endif /* (DT_INST_IRQ_HAS_CELL(0, sense)) */

static __pinned_bss struct k_spinlock lock;
static __pinned_bss uint64_t last_count;
static __pinned_bss uint64_t last_tick;

#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
static __pinned_bss unsigned int cyc_per_tick;
#else
#define cyc_per_tick			\
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

#define HPET_MAX_TICKS ((int32_t)0x7fffffff)

#ifdef HPET_INT_LEVEL_TRIGGER
/**
 * @brief Write to General Interrupt Status Register
 *
 * This is used to acknowledge and clear interrupt bits.
 *
 * @param val Value to be written to the register
 */
static inline void hpet_int_sts_set(uint32_t val)
{
	sys_write32(val, INTR_STATUS_REG);
}
#endif

/* ensure the comparator is always set ahead of the current counter value */
static inline void hpet_timer_comparator_set_safe(uint64_t next)
{
	hpet_timer_comparator_set(next);

	uint64_t now = hpet_counter_get();

	if (unlikely((int64_t)(next - now) <= 0)) {
		uint32_t bump = 1;

		do {
			next = now + bump;
			bump *= 2;
			hpet_timer_comparator_set(next);
			now = hpet_counter_get();
		} while ((int64_t)(next - now) <= 0);
	}
}

__isr
static void hpet_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t now = hpet_counter_get();

#ifdef HPET_INT_LEVEL_TRIGGER
	/*
	 * Clear interrupt only if level trigger is selected.
	 * When edge trigger is selected, spec says only 0 can
	 * be written.
	 */
	hpet_int_sts_set(TIMER0_INT_STS);
#endif

	if (IS_ENABLED(CONFIG_SMP) &&
	    IS_ENABLED(CONFIG_QEMU_TARGET)) {
		/* Qemu in SMP mode has observed the clock going
		 * "backwards" relative to interrupts already received
		 * on the other CPU, despite the HPET being
		 * theoretically a global device.
		 */
		int64_t diff = (int64_t)(now - last_count);

		if (last_count && diff < 0) {
			now = last_count;
		}
	}
	uint32_t dticks = (uint32_t)((now - last_count) / cyc_per_tick);

	last_count += (uint64_t)dticks * cyc_per_tick;
	last_tick += dticks;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t next = last_count + cyc_per_tick;

		hpet_timer_comparator_set_safe(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(dticks);
}

__pinned_func
static void config_timer0(unsigned int irq)
{
	uint32_t val = hpet_timer_conf_get();

	/* 5-bit IRQ field starting at bit 9 */
	val = (val & ~(0x1f << 9)) | ((irq & 0x1f) << 9);

#ifdef HPET_INT_LEVEL_TRIGGER
	/* Set level trigger if selected */
	val |= TIMER_CONF_INT_LEVEL;
#endif

	val &=  ~((uint32_t)(TIMER_CONF_MODE32 | TIMER_CONF_PERIODIC |
			TIMER_CONF_FSB_EN));
	val |= TIMER_CONF_INT_ENABLE;

	hpet_timer_conf_set(val);
}

__boot_func
void smp_timer_init(void)
{
	/* Noop, the HPET is a single system-wide device and it's
	 * configured to deliver interrupts to every CPU, so there's
	 * nothing to do at initialization on auxiliary CPUs.
	 */
}

__pinned_func
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)
	uint32_t reg;

	if (ticks == K_TICKS_FOREVER && idle) {
		reg = hpet_gconf_get();
		reg &= ~GCONF_ENABLE;
		hpet_gconf_set(reg);
		return;
	}

	ticks = ticks == K_TICKS_FOREVER ? HPET_MAX_TICKS : ticks;
	ticks = CLAMP(ticks, 0, HPET_MAX_TICKS/2);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t cyc = (last_tick + ticks) * cyc_per_tick;

	hpet_timer_comparator_set_safe(cyc);
	k_spin_unlock(&lock, key);
#endif
}

__pinned_func
uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t elapsed_cyc = hpet_counter_get() - last_count;
	uint32_t ret = (uint32_t)DIV_ROUND_UP(elapsed_cyc, cyc_per_tick);

	k_spin_unlock(&lock, key);
	return ret;
}

__pinned_func
uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)hpet_counter_get();
}

__pinned_func
uint64_t sys_clock_cycle_get_64(void)
{
	return hpet_counter_get();
}

__pinned_func
void sys_clock_idle_exit(void)
{
	uint32_t reg;

	reg = hpet_gconf_get();
	reg |= GCONF_ENABLE;
	hpet_gconf_set(reg);
}

__boot_func
static int sys_clock_driver_init(void)
{
	extern int z_clock_hw_cycles_per_sec;
	uint32_t hz, reg;

	ARG_UNUSED(hz);
	ARG_UNUSED(z_clock_hw_cycles_per_sec);

	DEVICE_MMIO_TOPLEVEL_MAP(hpet_regs, K_MEM_CACHE_NONE);

#if DT_INST_IRQ_HAS_CELL(0, sense)
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    hpet_isr, 0, DT_INST_IRQ(0, sense));
#else
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    hpet_isr, 0, 0);
#endif
	config_timer0(DT_INST_IRQN(0));
	irq_enable(DT_INST_IRQN(0));

#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
	hz = (uint32_t)(HPET_COUNTER_CLK_PERIOD / hpet_counter_clk_period_get());
	z_clock_hw_cycles_per_sec = hz;
	cyc_per_tick = hz / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#endif

	reg = hpet_gconf_get();
	reg |= GCONF_ENABLE;

#if (DT_INST_PROP(0, no_legacy_irq) == 0)
	/* Note: we set the legacy routing bit, because otherwise
	 * nothing in Zephyr disables the PIT which then fires
	 * interrupts into the same IRQ.  But that means we're then
	 * forced to use IRQ2 contra the way the kconfig IRQ selection
	 * is supposed to work.  Should fix this.
	 */
	reg |= GCONF_LR;
#endif

	hpet_gconf_set(reg);

	last_tick = hpet_counter_get() / cyc_per_tick;
	last_count = last_tick * cyc_per_tick;
	hpet_timer_comparator_set_safe(last_count + cyc_per_tick);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
