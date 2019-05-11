/*
 * Copyright (c) 2018 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAM0 series RTC-based system timer
 *
 * This system timer implementation supports both tickless and ticking modes.
 * In tickless mode, RTC counts continually in 32-bit mode and timeouts are
 * scheduled using the RTC comparator. In ticking mode, RTC is configured to
 * generate an interrupt every tick.
 */

#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>

/* RTC registers. */
#define RTC0 ((RtcMode0 *) DT_INST_0_ATMEL_SAM0_RTC_BASE_ADDRESS)

/* Number of sys timer cycles per on tick. */
static u32_t cycles_per_tick;

/* Maximum number of ticks. */
static u32_t max_ticks;

/* Tick/cycle count of the last announce call. */
static volatile u32_t rtc_last;

#ifndef CONFIG_TICKLESS_KERNEL

/* Current tick count. */
static volatile u32_t rtc_counter;

/* Tick value of the next timeout. */
static volatile u32_t rtc_timeout;

#endif /* CONFIG_TICKLESS_KERNEL */

/*
 * Waits for RTC bus synchronization.
 */
static inline void rtc_sync(void)
{
	while (RTC0->STATUS.reg & RTC_STATUS_SYNCBUSY) {
		/* Wait for bus synchronization... */
	}
}

/*
 * Reads RTC COUNT register. First a read request must be written to READREQ,
 * then - when bus synchronization completes - the COUNT register is read and
 * returned.
 */
static u32_t rtc_count(void)
{
	RTC0->READREQ.reg = RTC_READREQ_RREQ;
	rtc_sync();
	return RTC0->COUNT.reg;
}

static void rtc_reset(void)
{
	rtc_sync();

	/* Disable interrupt. */
	RTC0->INTENCLR.reg = RTC_MODE0_INTENCLR_MASK;
	/* Clear interrupt flag. */
	RTC0->INTFLAG.reg = RTC_MODE0_INTFLAG_MASK;

	/* Disable RTC module. */
	RTC0->CTRL.reg &= ~RTC_MODE0_CTRL_ENABLE;

	rtc_sync();

	/* Initiate software reset. */
	RTC0->CTRL.reg |= RTC_MODE0_CTRL_SWRST;
}

static void rtc_isr(void *arg)
{
	ARG_UNUSED(arg);

	/* Read and clear the interrupt flag register. */
	u16_t status = RTC0->INTFLAG.reg;

	RTC0->INTFLAG.reg = status;

#ifdef CONFIG_TICKLESS_KERNEL

	/* Read the current counter and announce the elapsed time in ticks. */
	u32_t count = rtc_count();

	if (count != rtc_last) {
		u32_t ticks = (count - rtc_last) / cycles_per_tick;

		z_clock_announce(ticks);
		rtc_last += ticks * cycles_per_tick;
	}

#else /* !CONFIG_TICKLESS_KERNEL */

	if (status) {
		/* RTC just ticked one more tick... */
		if (++rtc_counter == rtc_timeout) {
			z_clock_announce(rtc_counter - rtc_last);
			rtc_last = rtc_counter;
		}
	} else {
		/* ISR was invoked directly from z_clock_set_timeout. */
		z_clock_announce(0);
	}

#endif /* CONFIG_TICKLESS_KERNEL */
}

int z_clock_driver_init(struct device *device)
{
	struct device *clk;
	u32_t clk_freq;

	ARG_UNUSED(device);

	clk = device_get_binding(DT_INST_0_ATMEL_SAM0_RTC_CLOCK_CONTROLLER);
	__ASSERT(clk != NULL, "invalid RTC clock");

	/* Set up bus clock and GCLK generator. */
	PM->APBAMASK.reg |= PM_APBAMASK_RTC;

	clock_control_on(clk, (clock_control_subsys_t)RTC_GCLK_ID);
	clock_control_get_rate(clk,  (clock_control_subsys_t)RTC_GCLK_ID,
			       &clk_freq);

	/* Reset module to hardware defaults. */
	rtc_reset();

	rtc_last = 0U;

	/* Configure RTC with 32-bit mode, configured prescaler and MATCHCLR. */
	u16_t ctrl = RTC_MODE0_CTRL_MODE(0) | RTC_MODE0_CTRL_PRESCALER(0);
#ifndef CONFIG_TICKLESS_KERNEL
	ctrl |= RTC_MODE0_CTRL_MATCHCLR;
#endif

#if DT_ATMEL_SAM0_RTC_0_CLOCK_FREQUENCY
	u32_t div = clk_freq / DT_ATMEL_SAM0_RTC_0_CLOCK_FREQUENCY;

	div = 31U - __builtin_clz(div);
	if (div > 10) {
		div = 10;
	}

	clk_freq /= (1U << div);

#ifdef RTC_MODE0_CTRLA_PRESCALER_OFF
	div += 1;
#endif

#ifdef RTC_MODE0_CTRLA_PRESCALER
	ctrl |= RTC_MODE0_CTRLA_PRESCALER(div);
#else
	ctrl |= RTC_MODE0_CTRL_PRESCALER(div);
#endif
#endif

	cycles_per_tick = clk_freq / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	max_ticks = UINT32_MAX / cycles_per_tick - 2;

#ifdef CONFIG_TICKLESS_KERNEL
	/*
	 * Due to the nature of clock synchronization, reading from or
	 * writing to some RTC registers takes approximately six RTC_GCLK
	 * cycles. This constant defines a safe threshold for the comparator.
	 */
#define TICK_THRESHOLD	7

	__ASSERT(cycles_per_tick > TICK_THRESHOLD,
		 "cycles_per_tick must be greater than TICK_THRESHOLD for "
		 "tickless mode");

#else /* !CONFIG_TICKLESS_KERNEL */

	/*
	 * For some reason, RTC does not generate interrupts when COMP == 0,
	 * MATCHCLR == 1 and PRESCALER == 0. So we need to check that
	 * cycles_per_tick is more than one.
	 */
	__ASSERT(cycles_per_tick > 1,
		 "cycles_per_tick must be greater than 1 for ticking mode");

#endif /* CONFIG_TICKLESS_KERNEL */

	rtc_sync();
	RTC0->CTRL.reg = ctrl;

#ifdef CONFIG_TICKLESS_KERNEL
	/* Tickless kernel lets RTC count continually and ignores overflows. */
	RTC0->INTENSET.reg = RTC_MODE0_INTENSET_CMP0;
#else
	/* Non-tickless mode uses comparator together with MATCHCLR. */
	rtc_sync();
	RTC0->COMP[0].reg = cycles_per_tick;
	RTC0->INTENSET.reg = RTC_MODE0_INTENSET_OVF;
	rtc_counter = 0U;
	rtc_timeout = 0U;
#endif

	/* Enable RTC module. */
	rtc_sync();
	RTC0->CTRL.reg |= RTC_MODE0_CTRL_ENABLE;

	/* Enable RTC interrupt. */
	NVIC_ClearPendingIRQ(DT_INST_0_ATMEL_SAM0_RTC_IRQ_0);
	IRQ_CONNECT(DT_INST_0_ATMEL_SAM0_RTC_IRQ_0,
		    DT_INST_0_ATMEL_SAM0_RTC_IRQ_0_PRIORITY, rtc_isr, 0, 0);
	irq_enable(DT_INST_0_ATMEL_SAM0_RTC_IRQ_0);

	return 0;
}

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#ifdef CONFIG_TICKLESS_KERNEL

	ticks = (ticks == K_FOREVER) ? max_ticks : ticks;
	ticks = MAX(MIN(ticks - 1, (s32_t) max_ticks), 0);

	/* Compute number of RTC cycles until the next timeout. */
	u32_t count = rtc_count();
	u32_t timeout = ticks * cycles_per_tick + count % cycles_per_tick;

	/* Round to the nearest tick boundary. */
	timeout = (timeout + cycles_per_tick - 1) / cycles_per_tick
		  * cycles_per_tick;

	if (timeout < TICK_THRESHOLD) {
		timeout += cycles_per_tick;
	}

	rtc_sync();
	RTC0->COMP[0].reg = count + timeout;

#else /* !CONFIG_TICKLESS_KERNEL */

	if (ticks == K_FOREVER) {
		/* Disable comparator for K_FOREVER and other negative
		 * values.
		 */
		rtc_timeout = rtc_counter;
		return;
	}

	if (ticks < 1) {
		ticks = 1;
	}

	/* Avoid race condition between reading counter and ISR incrementing
	 * it.
	 */
	int key = irq_lock();

	rtc_timeout = rtc_counter + ticks;
	irq_unlock(key);

#endif /* CONFIG_TICKLESS_KERNEL */
}

u32_t z_clock_elapsed(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (rtc_count() - rtc_last) / cycles_per_tick;
#else
	return rtc_counter - rtc_last;
#endif
}

u32_t z_timer_cycle_get_32(void)
{
	/* Just return the absolute value of RTC cycle counter. */
	return rtc_count();
}
