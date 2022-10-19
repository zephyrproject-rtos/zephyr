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
#include <zephyr/irq.h>


/*
 * By limiting counter to 30 bits, we ensure that
 * timeout calculations will never overflow in sys_clock_set_timeout
 */
#define COUNTER_MAX 0x3fffffff

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec() \
				  / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MAX_TICKS ((COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

/* Use the first device defined with GPT HW timer compatible string */
#define GPT_INST DT_INST(0, DT_DRV_COMPAT)

/*
 * Stores the current number of cycles the system has had announced to it,
 * since the last rollover of the free running counter.
 */
static uint32_t announced_cycles;

/*
 * Stores the number of cycles that have elapsed due to counter rollover.
 * this value is updated in the GPT isr, and is used to keep the value
 * returned by sys_clock_cycle_get_32 accurate after a timer rollover.
 */
static uint32_t rollover_cycles;

/* GPT timer base address */
static GPT_Type *base;

/* Lock on shared variables */
static struct k_spinlock lock;

/* Helper function to set GPT compare value, so we don't set a compare point in past */
static void gpt_set_safe(uint32_t next)
{

	uint32_t now;

	next = MIN(MAX_CYCLES, next);
	GPT_SetOutputCompareValue(base, kGPT_OutputCompare_Channel2, next - 1);
	now = GPT_GetCurrentTimerCount(base);

	/* GPT fires interrupt at next counter cycle after a compare point is
	 * hit, so we should bump the compare point if 1 cycle or less exists
	 * between now and compare point.
	 *
	 * We will exit this loop if next==MAX_CYCLES, as we already
	 * have a rollover interrupt set up for that point, so we
	 * no longer need to keep bumping the compare point.
	 */
	if (unlikely(((int32_t)(next - now)) <= 1)) {
		uint32_t bump = 1;

		do {
			next = now + bump;
			bump *= 2;
			next = MIN(MAX_CYCLES, next);
			GPT_SetOutputCompareValue(base,
					kGPT_OutputCompare_Channel2, next - 1);
			now = GPT_GetCurrentTimerCount(base);
		} while ((((int32_t)(next - now)) <= 1) && (next < MAX_CYCLES));
	}
}

/* Interrupt fires every time GPT reaches the current capture value */
void mcux_imx_gpt_isr(const void *arg)
{
	ARG_UNUSED(arg);
	k_spinlock_key_t key;
	uint32_t tick_delta = 0, now, status;

	key = k_spin_lock(&lock);
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Get current timer count */
		now = GPT_GetCurrentTimerCount(base);
		status = GPT_GetStatusFlags(base,
			kGPT_OutputCompare2Flag | kGPT_OutputCompare1Flag);
		/* Clear GPT capture interrupts */
		GPT_ClearStatusFlags(base, status);
		if (status & kGPT_OutputCompare1Flag) {
			/* Counter has just rolled over. We should
			 * reset the announced cycles counter, and record the
			 * cycles that remained before rollover.
			 *
			 * Since rollover occurs on a tick boundary, we don't
			 * need to worry about losing time here due to rounding.
			 */
			tick_delta += (MAX_CYCLES - announced_cycles) / CYC_PER_TICK;
			announced_cycles = 0U;
			/* Update count of rolled over cycles */
			rollover_cycles += MAX_CYCLES;
		}
		if (status & kGPT_OutputCompare2Flag) {
			/* Normal counter interrupt. Get delta since last announcement */
			tick_delta += (now - announced_cycles) / CYC_PER_TICK;
			announced_cycles += (((now - announced_cycles) / CYC_PER_TICK) *
					CYC_PER_TICK);
		}
	} else {
		GPT_ClearStatusFlags(base, kGPT_OutputCompare1Flag);
		/* Update count of rolled over cycles */
		rollover_cycles += CYC_PER_TICK;
	}

	/* Announce progress to the kernel */
	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? tick_delta : 1);
}

/*
 * Next needed call to sys_clock_announce will not be until the specified number
 * of ticks from the current time have elapsed.
 */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Not supported on tickful kernels */
		return;
	}
	k_spinlock_key_t key;
	uint32_t next, adj, now;

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	/* Clamp ticks. We subtract one since we round up to next tick */
	ticks = CLAMP((ticks - 1), 0, (int32_t)MAX_TICKS);

	key = k_spin_lock(&lock);

	/* Read current timer value */
	now = GPT_GetCurrentTimerCount(base);

	/* Adjustment value, used to ensure next capture is on tick boundary */
	adj = (now - announced_cycles) + (CYC_PER_TICK - 1);

	next = ticks * CYC_PER_TICK;
	/*
	 * The following section rounds the capture value up to the next tick
	 * boundary
	 */
	next += adj;
	next = (next / CYC_PER_TICK) * CYC_PER_TICK;
	next += announced_cycles;
	/* Set GPT output value */
	gpt_set_safe(next);
	k_spin_unlock(&lock, key);
}

/* Get the number of ticks since the last call to sys_clock_announce() */
uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful kernel system */
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t cyc = GPT_GetCurrentTimerCount(base);

	cyc -= announced_cycles;
	k_spin_unlock(&lock, key);
	return cyc / CYC_PER_TICK;
}

/* Get the number of elapsed hardware cycles of the clock */
uint32_t sys_clock_cycle_get_32(void)
{
	return rollover_cycles + GPT_GetCurrentTimerCount(base);
}

/*
 * @brief Initialize system timer driver
 *
 * Enable the hw timer, setting its tick period, and setup its interrupt
 */
int sys_clock_driver_init(void)
{
	gpt_config_t gpt_config;

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
	/* We use reset mode, but reset at MAX ticks- see comment below */
	gpt_config.enableFreeRun = false;

	/* Initialize the GPT timer, and enable the relevant interrupts */
	GPT_Init(base, &gpt_config);
	announced_cycles = 0U;
	rollover_cycles = 0U;

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * Set GPT capture value 1 to MAX_CYCLES, and use GPT capture
		 * value 2 as the source for GPT interrupts. This way, we can
		 * use the counter as a free running timer, but it will roll
		 * over on a tick boundary.
		 */
		GPT_SetOutputCompareValue(base, kGPT_OutputCompare_Channel1,
			MAX_CYCLES - 1);

		/* Set initial trigger value to one tick worth of cycles */
		GPT_SetOutputCompareValue(base, kGPT_OutputCompare_Channel2,
			CYC_PER_TICK - 1);
		/* Enable GPT interrupts for timer match, and reset at capture value 1 */
		GPT_EnableInterrupts(base, kGPT_OutputCompare1InterruptEnable |
					kGPT_OutputCompare2InterruptEnable);
	} else {
		/* For a tickful kernel, just roll the counter over every tick */
		GPT_SetOutputCompareValue(base, kGPT_OutputCompare_Channel1,
			CYC_PER_TICK - 1);
		GPT_EnableInterrupts(base, kGPT_OutputCompare1InterruptEnable);
	}
	/* Enable IRQ */
	irq_enable(DT_IRQN(GPT_INST));
	/* Start timer */
	GPT_StartTimer(base);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
