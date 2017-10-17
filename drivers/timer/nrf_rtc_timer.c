/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <clock_control.h>
#include <system_timer.h>
#include <drivers/clock_control/nrf5_clock_control.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <sys_clock.h>

extern void youve_print(void);
/*
 * Convenience defines.
 */
#define SYS_CLOCK_RTC          NRF_RTC1
#define RTC_COUNTER            SYS_CLOCK_RTC->COUNTER
#define RTC_CC_VALUE           SYS_CLOCK_RTC->CC[0]
#define RTC_CC_EVENT           SYS_CLOCK_RTC->EVENTS_COMPARE[0]

/* Minimum delta between current counter and CC register that the RTC is able
 * to handle
 */
#define RTC_MIN_DELTA          2
#define RTC_MASK               0x00FFFFFF
/* Maximum difference for RTC counter values used. Half the maximum value is
 * selected to be able to detect overflow (a negative value has the same
 * representation as a large positive value).
 */
#define RTC_HALF               (RTC_MASK / 2)

#define RTC_TICKS_PER_SYS_TICK ((u32_t)((((u64_t)1000000UL / \
				 sys_clock_ticks_per_sec) * \
				1000000000UL) / 30517578125UL) & RTC_MASK)

extern s32_t _sys_idle_elapsed_ticks;

/*
 * rtc_past holds the value of RTC_COUNTER at the time the last sys tick was
 * announced, in RTC ticks. It is therefore always a multiple of
 * RTC_TICKS_PER_SYS_TICK.
 */
static u32_t rtc_past;

#ifdef CONFIG_TICKLESS_IDLE
/*
 * Holds the maximum sys ticks the kernel expects to see in the next
 * _sys_clock_tick_announce().
 */
static u32_t expected_sys_ticks;
#endif /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_TICKLESS_KERNEL
int32_t _get_max_clock_time(void);
#endif /* CONFIG_TICKLESS_KERNEL */

/*
 * Set RTC Counter Compare (CC) register to a given value in RTC ticks.
 */
static void rtc_compare_set(u32_t rtc_ticks)
{
	u32_t rtc_now;

	/* Try to set CC value. We assume the procedure is always successful. */
	RTC_CC_VALUE = rtc_ticks;
	rtc_now = RTC_COUNTER;

	/* The following checks if the CC register was set to a valid value.
	 * The first test checks if the distance between the current RTC counter
	 * and the value (in the future) set in the CC register is too small to
	 * guarantee a compare event being triggered.
	 * The second test checks if the current RTC counter is higher than the
	 * value written to the CC register, i.e. the CC value is in the past,
	 * by checking if the unsigned subtraction wraps around.
	 * If either of the above are true then instead of waiting for the CC
	 * event to trigger in the form of an interrupt, trigger it directly
	 * using the NVIC.
	 */
	if ((((rtc_ticks - rtc_now) & RTC_MASK) < RTC_MIN_DELTA) ||
	    (((rtc_ticks - rtc_now) & RTC_MASK) > RTC_HALF)) {
		NVIC_SetPendingIRQ(NRF5_IRQ_RTC1_IRQn);
	}
}
#ifndef CONFIG_TICKLESS_KERNEL
/*
 * @brief Announces the number of sys ticks, if any, that have passed since the
 * last announcement, and programs the RTC to trigger the interrupt on the next
 * sys tick.
 *
 * This function is not reentrant. It is called from:
 *
 * * _timer_idle_exit(), which in turn is called with interrupts disabled when
 * an interrupt fires.
 * * rtc1_nrf5_isr(), which runs with interrupts enabled but at that time the
 * device cannot be idle and hence _timer_idle_exit() cannot be called.
 *
 * Since this function can be preempted, we need to take some provisions to
 * announce all expected sys ticks that have passed.
 *
 */
static void rtc_announce_set_next(void)
{
	u32_t rtc_now, rtc_elapsed, sys_elapsed;

	/* Read the RTC counter one single time in the beginning, so that an
	 * increase in the counter during this procedure leads to no race
	 * conditions.
	 */
	rtc_now = RTC_COUNTER;

	/* Calculate how many RTC ticks elapsed since the last sys tick. */
	rtc_elapsed = (rtc_now - rtc_past) & RTC_MASK;

	/* If no sys ticks have elapsed, there is no point in incrementing the
	 * counters or announcing it.
	 */
	if (rtc_elapsed >= RTC_TICKS_PER_SYS_TICK) {
#ifdef CONFIG_TICKLESS_IDLE
		/* Calculate how many sys ticks elapsed since the last sys tick
		 * and notify the kernel if necessary.
		 */
		sys_elapsed = rtc_elapsed / RTC_TICKS_PER_SYS_TICK;

		if (sys_elapsed > expected_sys_ticks) {
			/* Never announce more sys ticks than the kernel asked
			 * to be idle for. The remainder will be announced when
			 * the RTC ISR runs after rtc_compare_set() is called
			 * after the first announcement.
			 */
			sys_elapsed = expected_sys_ticks;
		}
#else
		/* Never announce more than one sys tick if tickless idle is not
		 * configured.
		 */
		sys_elapsed = 1;
#endif /* CONFIG_TICKLESS_IDLE */

		/* Store RTC_COUNTER floored to the last sys tick. This is
		 * done, so that ISR can properly calculate that 1 sys tick
		 * has passed.
		 */
		rtc_past = (rtc_past +
				(sys_elapsed * RTC_TICKS_PER_SYS_TICK)
			   ) & RTC_MASK;

		_sys_idle_elapsed_ticks = sys_elapsed;
		_sys_clock_tick_announce();
	}

	/* Set the RTC to the next sys tick */
	rtc_compare_set(rtc_past + RTC_TICKS_PER_SYS_TICK);
}
#endif

#ifdef CONFIG_TICKLESS_IDLE
/**
 * @brief Place system timer into idle state.
 *
 * Re-program the timer to enter into the idle state for the given number of
 * sys ticks, counted from the previous sys tick. The timer will fire in the
 * number of sys ticks supplied or the maximum number of sys ticks (converted
 * to RTC ticks) that can be programmed into the hardware.
 *
 * This will only be called from idle context, with IRQs disabled.
 *
 * A value of -1 will result in the maximum number of sys ticks.
 *
 * Example 1: Idle sleep is entered:
 *
 * sys tick timeline:       (1)    (2)    (3)    (4)    (5)    (6)
 * rtc tick timeline : 0----100----200----300----400----500----600
 *                               ******************
 *                              150
 *
 * a) The last sys tick was announced at 100
 * b) The idle context enters sleep at 150, between sys tick 1 and 2, with
 * sys_ticks = 3.
 * c) The RTC is programmed to fire at sys tick 1 + 3 = 4 (RTC tick 400)
 *
 * @return N/A
 */
void _timer_idle_enter(s32_t sys_ticks)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (sys_ticks != K_FOREVER) {
		/* Need to reprograme timer if current program is smaller*/
		if (sys_ticks > expected_sys_ticks) {
			_set_time(sys_ticks);
		}
	} else {
		expected_sys_ticks = 0;
		/* Set time to largest possile RTC Tick*/
		_set_time(_get_max_clock_time());
	}
#else
	/* Restrict ticks to max supported by RTC without risking overflow*/
	if ((sys_ticks < 0) ||
		(sys_ticks > (RTC_HALF / RTC_TICKS_PER_SYS_TICK))) {
		sys_ticks = RTC_HALF / RTC_TICKS_PER_SYS_TICK;
	}

	expected_sys_ticks = sys_ticks;

	/* If ticks is 0, the RTC interrupt handler will be set pending
	 * immediately, meaning that we will not go to sleep.
	 */
	rtc_compare_set(rtc_past + (sys_ticks * RTC_TICKS_PER_SYS_TICK));
#endif
}


#ifdef CONFIG_TICKLESS_KERNEL

/**
 * @brief provides total systicks programmed.
 *
 * returns : total number of sys ticks programmed.
 */

uint32_t _get_program_time(void)
{
	return expected_sys_ticks;
}

/**
 * @brief provides total systicks remaining since last programming of RTC.
 *
 * returns : total number of sys ticks remaining since last RTC programming.
 */

uint32_t _get_remaining_program_time(void)
{

	if (!expected_sys_ticks) {
		return 0;
	}

	return (expected_sys_ticks - _get_elapsed_program_time());
}

/**
 * @brief provides total systicks passed since last programming of RTC.
 *
 * returns : total number of sys ticks passed since last RTC programming.
 */

uint32_t _get_elapsed_program_time(void)
{
	u32_t rtc_now, rtc_prev, rtc_elapsed;

	if (!expected_sys_ticks) {
		return 0;
	}

	rtc_now = RTC_COUNTER;

	/* Discard value of  RTC_COUNTER read at LFCLK transition */
	do {
		/* Number of RTC cycles passed since last RTC Programing*/
		rtc_elapsed = (rtc_now - rtc_past) & RTC_MASK;
		rtc_prev = rtc_now;
		rtc_now = RTC_COUNTER;
	} while (rtc_prev != rtc_now);

	/*Convert number of Machine cycles to SYS TICS*/
	return (rtc_elapsed / RTC_TICKS_PER_SYS_TICK);
}


/**
 * @brief Sets interrupt for rtc compare value for systick time.
 *
 * This Function does following:-
 * 1. Updates expected_sys_ticks equal to time.
 * 2. Update kernel book keeping for time passed since device bootup.
 * 3. Calls routine to set rtc interrupt.
 */

void _set_time(uint32_t time)
{
	if (!time) {
		expected_sys_ticks = 0;
		return;
	}

	/* Update expected_sys_ticls to time to programe*/
	expected_sys_ticks = time;
	_sys_clock_tick_count = _get_elapsed_clock_time();
	/* Update rtc_past to track rtc timer count*/
	rtc_past = (_sys_clock_tick_count * RTC_TICKS_PER_SYS_TICK) & RTC_MASK;

	expected_sys_ticks = expected_sys_ticks > _get_max_clock_time() ?
				_get_max_clock_time() : expected_sys_ticks;

	/* Programe RTC compare register to generate interrupt*/
	rtc_compare_set(rtc_past +
			(expected_sys_ticks * RTC_TICKS_PER_SYS_TICK));

}

/**
 * @brief provides time remaining to reach rtc count overflow.
 *
 * This function returns how many sys RTC remaining for rtc to overflow.
 * This will be required when we will program RTC compare value to maximum
 * possible value.
 *
 * returns : difference between current systick and Maximum possible systick.
 */
int32_t _get_max_clock_time(void)
{
	u32_t rtc_now, rtc_prev, rtc_away, sys_away = 0;

	rtc_now = RTC_COUNTER;
	/* Discard value of  RTC_COUNTER read at LFCLK transition */
	do {
		rtc_away = (RTC_MASK - rtc_now);
		rtc_away = rtc_away > RTC_HALF ? RTC_HALF : rtc_away;
		rtc_prev = rtc_now;
		/* Calculate time to programe into RTC*/
		rtc_now = RTC_COUNTER;
	} while (rtc_now != rtc_prev);

	/* Convert RTC Ticks to SYS TICKS*/
	if (rtc_away >= RTC_TICKS_PER_SYS_TICK) {
		sys_away = rtc_away / RTC_TICKS_PER_SYS_TICK;
	}
	return sys_away;
}

/**
 * @brief Enable sys Clock.
 *
 * This is used to program RTC clock to maximum Clock time in case Clock to
 * remain On.
 */
void _enable_sys_clock(void)
{
	if (!expected_sys_ticks) {
		/* Programe sys tick to Maximum possible value*/
		_set_time(_get_max_clock_time());
	}
}

/**
 * @brief provides total systicks passed since device bootup.
 *
 * returns : total number of sys ticks passed since device bootup.
 */

uint64_t _get_elapsed_clock_time(void)
{
	u64_t elapsed;
	u32_t rtc_now, rtc_elapsed, rtc_prev, sys_elapsed;

	rtc_now = RTC_COUNTER;

	/* Discard value of  RTC_COUNTER read at LFCLK transition */
	do {
		/* Calculate number of rtc cycles elapsed since RTC programing*/
		rtc_elapsed = (rtc_now - rtc_past) & RTC_MASK;
		elapsed = _sys_clock_tick_count;
		rtc_prev = rtc_now;
		rtc_now = RTC_COUNTER;
	} while (rtc_now != rtc_prev);

	if (rtc_elapsed >= RTC_TICKS_PER_SYS_TICK) {
		/* Convert RTC cycles to SYS TICKS*/
		sys_elapsed = rtc_elapsed / RTC_TICKS_PER_SYS_TICK;
		/* Update total number of SYS_TICKS passed*/
		elapsed += sys_elapsed;
	}
	return elapsed;
}
#endif


/**
 *
 * @brief Handling of tickless idle when interrupted
 *
 * The function will be called by _sys_power_save_idle_exit(), called from
 * _arch_isr_direct_pm() for 'direct' interrupts, or from _isr_wrapper for
 * regular ones, which is called on every IRQ handler if the device was
 * idle, and optionally called when a 'direct' IRQ handler executes if the
 * device was idle.
 *
 * Example 1: Idle sleep is interrupted before time:
 *
 * sys tick timeline:       (1)    (2)    (3)    (4)    (5)    (6)
 * rtc tick timeline : 0----100----200----300----400----500----600
 *                               **************!***
 *                              150           350
 *
 * Assume that _timer_idle_enter() is called at 150 (1) to sleep for 3
 * sys ticks. The last sys tick was announced at 100.
 *
 * On wakeup (non-RTC IRQ at 350):
 *
 * a) Notify how many sys ticks have passed, i.e., 350 - 150 / 100 = 2.
 * b) Schedule next sys tick at 400.
 *
 */
void _timer_idle_exit(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (!expected_sys_ticks && _sys_clock_always_on) {
		_set_time(_get_max_clock_time());
	}
#else
	/* Clear the event flag and interrupt in case we woke up on the RTC
	 * interrupt. No need to run the RTC ISR since everything that needs
	 * to run in the ISR will be done in this call.
	 */
	RTC_CC_EVENT = 0;
	NVIC_ClearPendingIRQ(NRF5_IRQ_RTC1_IRQn);

	rtc_announce_set_next();

	/* After exiting idle, the kernel no longer expects more than one sys
	 * ticks to have passed when _sys_clock_tick_announce() is called.
	 */
	expected_sys_ticks = 1;
#endif
}
#endif /* CONFIG_TICKLESS_IDLE */

/*
 * @brief Announces the number of sys ticks that have passed since the last
 * announcement, if any, and programs the RTC to trigger the interrupt on the
 * next sys tick.
 *
 * The ISR is set pending due to a regular sys tick and after exiting idle mode
 * as scheduled.
 *
 * Since this ISR can be preempted, we need to take some provisions to announce
 * all expected sys ticks that have passed.
 *
 * Consider the following example:
 *
 * sys tick timeline:       (1)    (2)    (3)    (4)    (5)    (6)
 * rtc tick timeline : 0----100----200----300----400----500----600
 *                                         !**********
 *                                                  450
 *
 * The last sys tick was anounced at 200, i.e, rtc_past = 200. The ISR is
 * executed at the next sys tick, i.e. 300. The following sys tick is due at
 * 400. However, the ISR is preempted for a number of sys ticks, until 450 in
 * this example. The ISR will then announce the number of sys ticks it was
 * delayed (2), and schedule the next sys tick (5) at 500.
 */
static void rtc1_nrf5_isr(void *arg)
{

	ARG_UNUSED(arg);
	RTC_CC_EVENT = 0;

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_tick_handler(void);
	read_timer_start_of_tick_handler();
#endif

#ifdef CONFIG_TICKLESS_KERNEL
	_sys_idle_elapsed_ticks = expected_sys_ticks;
	/* Initialize expected sys tick,
	 * It will be later updated based on next timeout.
	 */
	expected_sys_ticks = 0;
	/* Anounce elapsed of _sys_idle_elapsed_ticks systicks*/
	_sys_clock_tick_announce();
#else
	rtc_announce_set_next();
#endif

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_end_of_tick_handler(void);
	read_timer_end_of_tick_handler();
#endif

}

int _sys_clock_driver_init(struct device *device)
{
	struct device *clock;

	ARG_UNUSED(device);

	clock = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_K32SRC_DRV_NAME);
	if (!clock) {
		return -1;
	}

	clock_control_on(clock, (void *)CLOCK_CONTROL_NRF5_K32SRC);

	rtc_past = 0;

#ifdef CONFIG_TICKLESS_IDLE
	expected_sys_ticks = 1;
#endif /* CONFIG_TICKLESS_IDLE */

	/* TODO: replace with counter driver to access RTC */
	SYS_CLOCK_RTC->PRESCALER = 0;
	SYS_CLOCK_RTC->CC[0] = RTC_TICKS_PER_SYS_TICK;
	SYS_CLOCK_RTC->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
	SYS_CLOCK_RTC->INTENSET = RTC_INTENSET_COMPARE0_Msk;

	/* Clear the event flag and possible pending interrupt */
	RTC_CC_EVENT = 0;
	NVIC_ClearPendingIRQ(NRF5_IRQ_RTC1_IRQn);

	IRQ_CONNECT(NRF5_IRQ_RTC1_IRQn, 1, rtc1_nrf5_isr, 0, 0);
	irq_enable(NRF5_IRQ_RTC1_IRQn);

	SYS_CLOCK_RTC->TASKS_CLEAR = 1;
	SYS_CLOCK_RTC->TASKS_START = 1;

	return 0;
}

u32_t _timer_cycle_get_32(void)
{
	u32_t elapsed_cycles;
	u32_t sys_clock_tick_count;
	u32_t rtc_prev;
	u32_t rtc_now;

	rtc_now = RTC_COUNTER;
	/* Discard value of  RTC_COUNTER read at LFCLK transition */
	do {
		sys_clock_tick_count = _sys_clock_tick_count;
		elapsed_cycles = (rtc_now - (sys_clock_tick_count *
					     RTC_TICKS_PER_SYS_TICK)) &
				 RTC_MASK;
		rtc_prev = rtc_now;
		rtc_now = RTC_COUNTER;
	} while (rtc_now != rtc_prev);

	return (sys_clock_tick_count * sys_clock_hw_cycles_per_tick) +
	       elapsed_cycles;
}

#ifdef CONFIG_SYSTEM_CLOCK_DISABLE
/**
 *
 * @brief Stop announcing sys ticks into the kernel
 *
 * This routine disables the RTC1 so that timer interrupts are no
 * longer delivered.
 *
 * @return N/A
 */
void sys_clock_disable(void)
{
	unsigned int key;

	key = irq_lock();

	irq_disable(NRF5_IRQ_RTC1_IRQn);

	SYS_CLOCK_RTC->EVTENCLR = RTC_EVTENCLR_COMPARE0_Msk;
	SYS_CLOCK_RTC->INTENCLR = RTC_INTENCLR_COMPARE0_Msk;

	SYS_CLOCK_RTC->TASKS_STOP = 1;
	SYS_CLOCK_RTC->TASKS_CLEAR = 1;

	irq_unlock(key);

	/* TODO: turn off (release) 32 KHz clock source.
	 * Turning off of 32 KHz clock source is not implemented in clock
	 * driver.
	 */
}
#endif /* CONFIG_SYSTEM_CLOCK_DISABLE */
