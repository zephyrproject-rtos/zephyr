/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_gpt_hw_timer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <fsl_gpt.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/time_units.h>



/* GPT is a 32 bit counter, but we use a lower value to avoid integer overflow */
#define COUNTER_MAX 0x00ffffff
#define TIMER_STOPPED 0xff000000

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec() \
				  / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MAX_TICKS ((COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

/* Minimum cycles in the future to try to program.  Note that this is
 * NOT simply "enough cycles to get the counter read and reprogrammed
 * reliably" -- it becomes the minimum value of the GPT counter flag register,
 * and thus reflects how much time we can reliably see expire between
 * calls to elapsed() to read the COUNTFLAG bit.  So it needs to be
 * set to be larger than the maximum time the interrupt might be
 * masked.  Choosing a fraction of a tick is probably a good enough
 * default, with an absolute minimum of 4 cyc (keep in mind the
 * counter freq is only 32k).
 */
#define MIN_DELAY MAX(4, (CYC_PER_TICK/16))

/* Use the first device defined with GPT HW timer compatible string */
#define GPT_INST DT_INST(0, DT_DRV_COMPAT)

static GPT_Type *base; /* GPT timer base address */
/*
 * Stores the current number of cycles the system has had announced to it.
 * must be a multiple of CYC_PER_TICK.
 */
static volatile uint32_t announced_cycles;

/*
 * Stores the amount of elapsed cycles. Updated in mcux_gpt_isr(), and
 * sys_clock_set_timeout(). At an arbitrary point in time, the current number of
 * elapsed HW cycles is calculated as cycle_count + elapsed()
 */
static volatile uint32_t cycle_count;

/*
 * Stores the elapsed hardware cycles due to the GPT wrapping. The GPT wrap
 * will trigger an interrupt, but if the timer wraps while interrupts are
 * disabled this variable will record the overflow value.
 *
 * Each time cycle_count is updated with this value, overflow cycles should be
 * reset to 0.
 */
static volatile uint32_t wrapped_cycles;

/*
 * Stores the last value loaded to the GPT. This can also be queried from the
 * hardware, but storing it locally lets the compiler attempt to optimize access.
 */
static uint32_t last_load;

/*
 * Used by sys_clock_set_timeout to determine if it was called from an ISR.
 */
static volatile bool gpt_isr_active;

/* GPT timer base address */
static GPT_Type *base;

/* Lock on shared variables */
static struct k_spinlock lock;

/**
 * This function calculates the amount of hardware cycles that have elapsed
 * since the last time the absolute hardware cycles counter was updated.
 * 'cycle_count' will be updated in the ISR, or if the counter capture value is
 * changed in sys_clock_set_timeout().
 *
 * The primary purpose of this function is to aid in catching the edge case
 * where the timer wraps around while ISRs are disabled, and ensure the calling
 * function still correctly reports the timer's state.
 *
 * In order to store state if a wrap occurs, the function will update the
 * 'wrapped_cycles' variable so that the GPT ISR can use it.
 *
 * Prerequisites:
 * - When the GPT capture value is programmed, 'wrapped_cycles' must be zeroed
 * - ISR must clear the 'overflow_cyc' counter.
 * - no more than one counter-wrap has occurred between
 *     - the timer reset or the last time the function was called
 *     - and until the current call of the function is completed.
 * - the function is not able to be interrupted by the GPT ISR
 */
static uint32_t elapsed(void)
{
	/* Read the GPT twice, and read the GPT status flags */
	uint32_t read1 = GPT_GetCurrentTimerCount(base);
	uint32_t status_flags = GPT_GetStatusFlags(base, kGPT_OutputCompare1Flag);
	/* Clear status flag */
	GPT_ClearStatusFlags(base, kGPT_OutputCompare1Flag);
	uint32_t read2 = GPT_GetCurrentTimerCount(base);

	/*
	 * The counter wraps to zero at the output compare value ('last load').
	 * Therefore, if read1 > read2, the counter wrapped. If the status flag
	 * is set the counter also will have wrapped.
	 *
	 * Otherwise, the counter has not wrapped.
	 */
	if (status_flags || (read1 > read2)) {
		/* A wrap occurred. We need to update 'wrapped_cycles' */
		wrapped_cycles += last_load;
		/* We know there was a wrap, but it may not have been cleared. */
		GPT_ClearStatusFlags(base, kGPT_OutputCompare1Flag);
	}

	/* Calculate the cycles since the ISR last fired (the ISR updates 'cycle_count') */
	return read2 + wrapped_cycles;
}


/* Interrupt fires every time GPT timer reaches set value.
 * GPT timer will reset to 0x0.
 *
 */
void mcux_imx_gpt_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Update the value of 'wrapped_cycles' */
	elapsed();

	/* Update the total number of cycles elapsed */
	cycle_count += wrapped_cycles;
	wrapped_cycles = 0;

#if defined(CONFIG_TICKLESS_KERNEL)
	uint32_t tick_delta;

	tick_delta = (cycle_count - announced_cycles) / CYC_PER_TICK;
	announced_cycles += tick_delta * CYC_PER_TICK;
	/* Announce the number of elapsed ticks.
	 *
	 * Note that by the definition of the way that the kernel uses
	 * sys_clock_set_timeout, we should change the GPT counter value here to
	 * occur on a tick boundary. However, the kernel will call
	 * sys_clock_set_timeout within the call to sys_clock_announce, so we
	 * don't have to worry about that.
	 */
	gpt_isr_active = true;
	sys_clock_announce(tick_delta);
#else
	/* If system is tickful, interrupt will fire again at next tick */
	sys_clock_announce(1);
#endif
}

/* Next needed call to sys_clock_announce will not be until the specified number
 * of ticks from the current time have elapsed. Note that this timeout value is
 * persistent, ie if the kernel sets the timeout to 2 ticks this driver must
 * announce ticks to the kernel every 2 ticks until told otherwise
 */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
#if defined(CONFIG_TICKLESS_KERNEL)
	k_spinlock_key_t key;
	uint32_t reload_value, pending_cycles, unannounced_cycles, read1, read2;
	/* Save prior load value (to check for wrap at end of function) */
	uint32_t old_load = last_load;

	if ((ticks == K_TICKS_FOREVER) && idle) {
		/* GPT timer no longer needed. Stop it. */
		GPT_StopTimer(base);
		last_load = TIMER_STOPPED;
		return;
	}
	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	/* Clamp ticks */
	ticks = CLAMP((ticks - 1), 0, (int32_t)MAX_TICKS);

	key = k_spin_lock(&lock);

	/* Update the wrapped cycles value if required */
	pending_cycles = elapsed();
	/* Get first read as soon as possible */
	read1 = GPT_GetCurrentTimerCount(base);

	/* Update cycle count and reset wrapped cycles */
	cycle_count += pending_cycles;
	wrapped_cycles = 0U;


	unannounced_cycles = cycle_count - announced_cycles;

	if ((int32_t)unannounced_cycles < 0) {
		/* Announcement has not occurred for more than half the 32 bit counter
		 * value, since new timeouts keep being set. Force an announcement
		 */
		reload_value = MIN_DELAY;
	} else {
		reload_value = ticks * CYC_PER_TICK;
		/* Round reload value up to a tick boundary */
		reload_value += unannounced_cycles;
		reload_value =
			((reload_value + CYC_PER_TICK - 1) / CYC_PER_TICK) * CYC_PER_TICK;
		reload_value -= unannounced_cycles;
		if (reload_value == ticks * CYC_PER_TICK) {
			/* We are on a tick boundary. Since we subtracted from
			 * 'ticks' earlier, we need to add one tick worth of
			 * cycles to announce to the kernel at the right time.
			 */
			reload_value += CYC_PER_TICK;
		}
		/* Clamp reload value */
		reload_value = CLAMP(reload_value, MIN_DELAY, MAX_CYCLES);
	}
	/* Set reload value (will also reset GPT timer) */
	read2 = GPT_GetCurrentTimerCount(base);
	/* The below checks correspond to the following:
	 * GPT timer is at zero ticks
	 * No requirement to force an announcement to the kernel
	 * called from GPT ISR (pending cycles might be zero in this case)
	 * kernel wants an announcement sooner than we currently will announce
	 */
	if ((pending_cycles != 0) ||
		((int32_t)unannounced_cycles < 0) ||
		gpt_isr_active ||
		(reload_value < last_load)) {
		/*
		 * In cases where sys_clock_set_timeout is repeatedly called by the
		 * kernel outside of the context of sys_clock_annouce, the GPT timer
		 * may be reset before it can "tick" upwards. This prevents progress
		 * from occurring in the kernel. These checks ensure that the GPT timer
		 * gets a chance to tick before being reset.
		 */
		last_load = reload_value;
		GPT_SetOutputCompareValue(base, kGPT_OutputCompare_Channel1, last_load - 1);
		while (GPT_GetCurrentTimerCount(base) != 0) {
			/* Since GPT timer frequency is much slower than system clock, we must
			 * wait for GPT timer to reset here.
			 *
			 * If the GPT timer is switched to a faster clock, this block must
			 * be removed, as the timer will count past zero before we can read it.
			 */
		}
	}
	/* Reset ISR flag */
	gpt_isr_active = false;

	/* read1 and read2 are used to 'time' this function, so we can keep
	 * the cycle count accurate.

	 * Strictly speaking, we should check the counter interrupt flag here fo
	 * wraparound, but if the GPT output compare value we just set has wrapped,
	 * we would erroneously catch that wrap here.
	 */
	if (read1 > read2) {
		/* Timer wrapped while in this function. Update cycle count */
		cycle_count += ((old_load - read1) + read2);
	} else {
		cycle_count += (read2 - read1);
	}
	k_spin_unlock(&lock, key);
#endif
}

/* Get the number of ticks since the last call to sys_clock_announce() */
uint32_t sys_clock_elapsed(void)
{
#if defined(CONFIG_TICKLESS_KERNEL)
	uint32_t cyc;
	k_spinlock_key_t key = k_spin_lock(&lock);

	cyc = elapsed() + cycle_count - announced_cycles;
	k_spin_unlock(&lock, key);
	return cyc / CYC_PER_TICK;
#else
	/* 0 ticks will always have elapsed */
	return 0;
#endif
}

/* Get the number of elapsed hardware cycles of the clock */
uint32_t sys_clock_cycle_get_32(void)
{
	uint32_t ret;
	k_spinlock_key_t key = k_spin_lock(&lock);

	ret = elapsed() + cycle_count;
	k_spin_unlock(&lock, key);
	return ret;
}

/*
 * @brief Initialize system timer driver
 *
 * Enable the hw timer, setting its tick period, and setup its interrupt
 */
int sys_clock_driver_init(const struct device *dev)
{
	gpt_config_t gpt_config;

	ARG_UNUSED(dev);
	/* Configure ISR. Use instance 0 of the GPT timer */
	IRQ_CONNECT(DT_IRQN(GPT_INST), DT_IRQ(GPT_INST, priority),
		    mcux_imx_gpt_isr, NULL, 0);

	base = (GPT_Type *)DT_REG_ADDR(GPT_INST);

	GPT_GetDefaultConfig(&gpt_config);
	/* Enable GPT timer to run in SOC low power states */
	gpt_config.enableRunInStop = true;
	gpt_config.enableRunInWait = true;
	gpt_config.enableRunInDoze = true;
	/* Use 32KHz clock frequency */
	gpt_config.clockSource = kGPT_ClockSource_LowFreq;
	gpt_config.enableFreeRun = false; /* Set GPT to reset mode */

	/* Initialize the GPT timer in reset mode, and enable the relevant interrupts */
	GPT_Init(base, &gpt_config);

	last_load = CYC_PER_TICK;
	wrapped_cycles = 0U;
	/* Set initial trigger value to one tick worth of cycles */
	GPT_SetOutputCompareValue(base, kGPT_OutputCompare_Channel1, last_load - 1);
	while (GPT_GetCurrentTimerCount(base)) {
		/* Wait for timer count to clear.
		 * Writes to the GPT output compare register occur after 1 cycle of
		 * wait state
		 */
	}
	/* Enable GPT interrupt */
	GPT_EnableInterrupts(base, kGPT_OutputCompare1InterruptEnable);
	/* Enable IRQ */
	irq_enable(DT_IRQN(GPT_INST));
	/* Start timer */
	GPT_StartTimer(base);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
