/*
 * Copyright (c) 2017-2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Driver for the timer model of the POSIX native_posix board
 * It provides the interfaces required by the kernel and the sanity testcases
 * It also provides a custom k_busy_wait() which can be used with the
 * POSIX arch and InfClock SOC
 */
#include "zephyr/types.h"
#include "irq.h"
#include "device.h"
#include <drivers/timer/system_timer.h>
#include "sys_clock.h"
#include "timer_model.h"
#include "soc.h"
#include <arch/posix/posix_trace.h>

static uint64_t tick_period; /* System tick period in microseconds */
/* Time (microseconds since boot) of the last timer tick interrupt */
static uint64_t last_tick_time;

/**
 * Return the current HW cycle counter
 * (number of microseconds since boot in 32bits)
 */
uint32_t z_timer_cycle_get_32(void)
{
	return hwm_get_time();
}

/**
 * Interrupt handler for the timer interrupt
 * Announce to the kernel that a number of ticks have passed
 */
static void np_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint64_t now = hwm_get_time();
	int32_t elapsed_ticks = (now - last_tick_time)/tick_period;

	last_tick_time += elapsed_ticks*tick_period;
	z_clock_announce(elapsed_ticks);
}

/*
 * @brief Initialize system timer driver
 *
 * Enable the hw timer, setting its tick period, and setup its interrupt
 */
int z_clock_driver_init(const struct device *device)
{
	ARG_UNUSED(device);

	tick_period = 1000000ul / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	last_tick_time = hwm_get_time();
	hwtimer_enable(tick_period);

	IRQ_CONNECT(TIMER_TICK_IRQ, 1, np_timer_isr, 0, 0);
	irq_enable(TIMER_TICK_IRQ);

	return 0;
}

/**
 * @brief Set system clock timeout
 *
 * Informs the system clock driver that the next needed call to
 * z_clock_announce() will not be until the specified number of ticks
 * from the the current time have elapsed.
 *
 * See system_timer.h for more information
 *
 * @param ticks Timeout in tick units
 * @param idle Hint to the driver that the system is about to enter
 *        the idle state immediately after setting the timeout
 */
void z_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)
	uint64_t silent_ticks;

	/* Note that we treat INT_MAX literally as anyhow the maximum amount of
	 * ticks we can report with z_clock_announce() is INT_MAX
	 */
	if (ticks == K_TICKS_FOREVER) {
		silent_ticks = INT64_MAX;
	} else if (ticks > 0) {
		silent_ticks = ticks - 1;
	} else {
		silent_ticks = 0;
	}
	hwtimer_set_silent_ticks(silent_ticks);
#endif
}

/**
 * @brief Ticks elapsed since last z_clock_announce() call
 *
 * Queries the clock driver for the current time elapsed since the
 * last call to z_clock_announce() was made.  The kernel will call
 * this with appropriate locking, the driver needs only provide an
 * instantaneous answer.
 */
uint32_t z_clock_elapsed(void)
{
	return (hwm_get_time() - last_tick_time)/tick_period;
}


#if defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
/**
 * Replacement to the kernel k_busy_wait()
 * Will block this thread (and therefore the whole zephyr) during usec_to_wait
 *
 * Note that interrupts may be received in the meanwhile and that therefore this
 * thread may loose context
 *
 * This special arch_busy_wait() is necessary due to how the POSIX arch/SOC INF
 * models a CPU. Conceptually it could be thought as if the MCU was running
 * at an infinitely high clock, and therefore no simulated time passes while
 * executing instructions(*1).
 * Therefore to be able to busy wait this function does the equivalent of
 * programming a dedicated timer which will raise a non-maskable interrupt,
 * and halting the CPU.
 *
 * (*1) In reality simulated time is simply not advanced just due to the "MCU"
 * running. Meaning, the SW running on the MCU is assumed to take 0 time.
 */
void arch_busy_wait(uint32_t usec_to_wait)
{
	uint64_t time_end = hwm_get_time() + usec_to_wait;

	while (hwm_get_time() < time_end) {
		/*
		 * There may be wakes due to other interrupts including
		 * other threads calling arch_busy_wait
		 */
		hwtimer_wake_in_time(time_end);
		posix_halt_cpu();
	}
}
#endif

#if defined(CONFIG_SYSTEM_CLOCK_DISABLE)
/**
 *
 * @brief Stop announcing sys ticks into the kernel
 *
 * Disable the system ticks generation
 *
 * @return N/A
 */
void sys_clock_disable(void)
{
	irq_disable(TIMER_TICK_IRQ);
	hwtimer_set_silent_ticks(INT64_MAX);
}
#endif /* CONFIG_SYSTEM_CLOCK_DISABLE */
