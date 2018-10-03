/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <system_timer.h>
#include <board.h>

typedef struct {
	u32_t val_low;
	u32_t val_high;
} riscv_machine_timer_t;

static volatile riscv_machine_timer_t *mtime =
	(riscv_machine_timer_t *)RISCV_MTIME_BASE;
static volatile riscv_machine_timer_t *mtimecmp =
	(riscv_machine_timer_t *)RISCV_MTIMECMP_BASE;

/*
 * The RISCV machine-mode timer is a one shot timer that needs to be rearm upon
 * every interrupt. Timer clock is a 64-bits ART.
 * To arm timer, we need to read the RTC value and update the
 * timer compare register by the RTC value + time interval we want timer
 * to interrupt.
 */
static ALWAYS_INLINE void riscv_machine_rearm_timer(void)
{
	u64_t rtc;

	/*
	 * Disable timer interrupt while rearming the timer
	 * to avoid generation of interrupts while setting
	 * the mtimecmp->val_low register.
	 */
	irq_disable(RISCV_MACHINE_TIMER_IRQ);

	/*
	 * Following machine-mode timer implementation in QEMU, the actual
	 * RTC read is performed when reading low timer value register.
	 * Reading high timer value just reads the most significant 32-bits
	 * of a cache value, obtained from a previous read to the low
	 * timer value register. Hence, always read timer->val_low first.
	 * This also works for other implementations.
	 */
	rtc = mtime->val_low;
	rtc |= ((u64_t)mtime->val_high << 32);

	/*
	 * Rearm timer to generate an interrupt after
	 * sys_clock_hw_cycles_per_tick
	 */
	rtc += sys_clock_hw_cycles_per_tick;
	mtimecmp->val_low = (u32_t)(rtc & 0xffffffff);
	mtimecmp->val_high = (u32_t)((rtc >> 32) & 0xffffffff);

	/* Enable timer interrupt */
	irq_enable(RISCV_MACHINE_TIMER_IRQ);
}

static void riscv_machine_timer_irq_handler(void *unused)
{
	ARG_UNUSED(unused);
#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_tick_handler(void);
	read_timer_start_of_tick_handler();
#endif

	_sys_clock_tick_announce();

	/* Rearm timer */
	riscv_machine_rearm_timer();

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_end_of_tick_handler(void);
	read_timer_end_of_tick_handler();
#endif
}

#ifdef CONFIG_TICKLESS_IDLE
#error "Tickless idle not yet implemented for riscv-machine timer"
#endif

int _sys_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	IRQ_CONNECT(RISCV_MACHINE_TIMER_IRQ, 0,
		    riscv_machine_timer_irq_handler, NULL, 0);

	/* Initialize timer, just call riscv_machine_rearm_timer */
	riscv_machine_rearm_timer();

	return 0;
}

/**
 *
 * @brief Read the platform's timer hardware
 *
 * This routine returns the current time in terms of timer hardware clock
 * cycles.
 *
 * @return up counter of elapsed clock cycles
 */
u32_t _timer_cycle_get_32(void)
{
	/* We just want a cycle count so just post what's in the low 32
	 * bits of the mtime real-time counter
	 */
	return mtime->val_low;
}
