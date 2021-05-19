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
#include <linker/sections.h>

#include <dt-bindings/interrupt-controller/intel-ioapic.h>

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
 *
 * HPET_CMP_MIN_DELAY can be overridden in soc.h to better match
 * the frequency of the timers. Default is 1000 where the value
 * written to the comparator must be 1000 larger than the current
 * main counter value.
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

/*
 * The following MMIO initialization and register access functions
 * should work on generic x86 hardware. If the targeted SoC requires
 * special handling of HPET registers, these functions will need to be
 * implemented in the SoC layer by first defining the macro
 * HPET_USE_CUSTOM_REG_ACCESS_FUNCS in soc.h to signal such intent.
 *
 * This is a list of functions which must be implemented in the SoC
 * layer:
 *   void hpet_mmio_init(void)
 *   uint32_t hpet_counter_get(void)
 *   uint32_t hpet_counter_clk_period_get(void)
 *   uint32_t hpet_gconf_get(void)
 *   void hpet_gconf_set(uint32_t val)
 *   void hpet_int_sts_set(uint32_t val)
 *   uint32_t hpet_timer_conf_get(void)
 *   void hpet_timer_conf_set(uint32_t val)
 *   void hpet_timer_comparator_set(uint32_t val)
 */
#ifndef HPET_USE_CUSTOM_REG_ACCESS_FUNCS
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
#define MAIN_COUNTER_REG		HPET_REG_ADDR(0xf0)

/* Timer 0 Configuration and Capabilities register */
#define TIMER0_CONF_REG			HPET_REG_ADDR(0x100)

/* Timer 0 Comparator Register */
#define TIMER0_COMPARATOR_REG		HPET_REG_ADDR(0x108)

/**
 * @brief Setup memory mappings needed to access HPET registers.
 *
 * This is called in sys_clock_driver_init() to setup any memory
 * mappings needed to access HPET registers.
 */
static inline void hpet_mmio_init(void)
{
	DEVICE_MMIO_TOPLEVEL_MAP(hpet_regs, K_MEM_CACHE_NONE);
}

/**
 * @brief Return the value of the main counter.
 *
 * @return Value of Main Counter
 */
static inline uint32_t hpet_counter_get(void)
{
	return sys_read32(MAIN_COUNTER_REG);
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

/**
 * @brief Write to the Timer Comparator Value Register
 *
 * This writes the specified value to the Timer Comparator
 * Value Register of Timer #0.
 *
 * @param val Value to be written to the register
 */
static inline void hpet_timer_comparator_set(uint32_t val)
{
	sys_write32(val, TIMER0_COMPARATOR_REG);
}
#endif /* HPET_USE_CUSTOM_REG_ACCESS_FUNCS */

#ifndef HPET_COUNTER_CLK_PERIOD
/* COUNTER_CLK_PERIOD (CLK_PERIOD_REG) is in femtoseconds (1e-15 sec) */
#define HPET_COUNTER_CLK_PERIOD		(1000000000000000ULL)
#endif

#ifndef HPET_CMP_MIN_DELAY
/* Minimal delay for comparator before the next timer event */
#define HPET_CMP_MIN_DELAY		(1000)
#endif

#define MAX_TICKS			0x7FFFFFFFUL

static __pinned_bss struct k_spinlock lock;
static __pinned_bss unsigned int last_count;

#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
static __pinned_bss unsigned int cyc_per_tick;
static __pinned_bss unsigned int max_ticks;
#else
#define cyc_per_tick			\
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define max_ticks			\
	((MAX_TICKS - cyc_per_tick) / cyc_per_tick)
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

__isr
static void hpet_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t now = hpet_counter_get();

#if ((DT_INST_IRQ(0, sense) & IRQ_TYPE_LEVEL) == IRQ_TYPE_LEVEL)
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
		int32_t diff = (int32_t)(now - last_count);

		if (last_count && diff < 0) {
			now = last_count;
		}
	}
	uint32_t dticks = (now - last_count) / cyc_per_tick;

	last_count += dticks * cyc_per_tick;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t next = last_count + cyc_per_tick;

		if ((int32_t)(next - now) < HPET_CMP_MIN_DELAY) {
			next += cyc_per_tick;
		}
		hpet_timer_comparator_set(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}

__pinned_func
static void set_timer0_irq(unsigned int irq)
{
	uint32_t val = hpet_timer_conf_get();

	/* 5-bit IRQ field starting at bit 9 */
	val = (val & ~(0x1f << 9)) | ((irq & 0x1f) << 9);

#if ((DT_INST_IRQ(0, sense) & IRQ_TYPE_LEVEL) == IRQ_TYPE_LEVEL)
	/* Level trigger */
	val |= TIMER_CONF_INT_LEVEL;
#endif

	hpet_timer_conf_set(val);
}

__boot_func
int sys_clock_driver_init(const struct device *dev)
{
	extern int z_clock_hw_cycles_per_sec;
	uint32_t hz, reg;

	ARG_UNUSED(dev);
	ARG_UNUSED(hz);
	ARG_UNUSED(z_clock_hw_cycles_per_sec);

	hpet_mmio_init();

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    hpet_isr, 0, DT_INST_IRQ(0, sense));
	set_timer0_irq(DT_INST_IRQN(0));
	irq_enable(DT_INST_IRQN(0));

#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
	hz = (uint32_t)(HPET_COUNTER_CLK_PERIOD / hpet_counter_clk_period_get());
	z_clock_hw_cycles_per_sec = hz;
	cyc_per_tick = hz / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	max_ticks = (MAX_TICKS - cyc_per_tick) / cyc_per_tick;
#endif

	last_count = hpet_counter_get();

	/* Note: we set the legacy routing bit, because otherwise
	 * nothing in Zephyr disables the PIT which then fires
	 * interrupts into the same IRQ.  But that means we're then
	 * forced to use IRQ2 contra the way the kconfig IRQ selection
	 * is supposed to work.  Should fix this.
	 */
	reg = hpet_gconf_get();
	reg |= GCONF_LR | GCONF_ENABLE;
	hpet_gconf_set(reg);

	reg = hpet_timer_conf_get();
	reg &= ~TIMER_CONF_PERIODIC;
	reg &= ~TIMER_CONF_FSB_EN;
	reg |= TIMER_CONF_MODE32;
	reg |= TIMER_CONF_INT_ENABLE;
	hpet_timer_conf_set(reg);

	hpet_timer_comparator_set(last_count + cyc_per_tick);

	return 0;
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

	ticks = ticks == K_TICKS_FOREVER ? max_ticks : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)max_ticks);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t now = hpet_counter_get(), cyc, adj;
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

	if ((cyc - now) < HPET_CMP_MIN_DELAY) {
		cyc += cyc_per_tick;
	}

	hpet_timer_comparator_set(cyc);
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
	uint32_t ret = (hpet_counter_get() - last_count) / cyc_per_tick;

	k_spin_unlock(&lock, key);
	return ret;
}

__pinned_func
uint32_t sys_clock_cycle_get_32(void)
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
