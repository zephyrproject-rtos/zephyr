/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <system_timer.h>
#include <sys_clock.h>
#include <nrf_rtc.h>
#include <spinlock.h>

/* Zephyr RTC resource use:
 * - RTC0 is used for Bluetooth.
 * - RTC1 is used for system timer, here; only CC[0] is used.
 * - RTC2 is used for Nordic HAL 802.15.4.
 */

#define RTC NRF_RTC1

/* Alarm rework notes.
 *
 * A counter *wraps* when the counter value increments to zero.
 *
 * A counter *laps* when the counter value increments back to the
 * reference counter value.
 *
 * The *span* of a counter is the number of counter increments
 * required to lap the counter.
 *
 * The span of a counter is required to be 2^S, i.e. the counter
 * values exactly match the values of an S-bit unsigned integer.
 *
 * The signed difference between two counter values with an S-bit span
 * is the 2s-complement interpretation of the unsigned S-bit
 * difference between the values.
 *
 * The implementation here assumes:
 * * A 64-bit cycle clock counting at 32 KiHz.
 * * Deadlines that are expressed as 32-bit values matching the low 32
 *   bits of the cycle clock.
 * * The hardware counter is 24-bit.  The cycle clock matches the
 *   hardware counter in its low 24 bits.
 *
 * Deadlines are in the past if the signed difference between the
 * cycle clock and the deadline is non-positive.
 *
 * The minimum interval between alarm processing is 2^23 ticks, to
 * ensure that a timer FLIH delayed by higher-priority tasks will not
 * result in the hardware counter lapping.
 */

/* RTC counter has 24 valid bits. */
#define COUNTER_SPAN (1U << 24)

/* Mask to isolate the valid bits of the counter. */
#define COUNTER_MASK (COUNTER_SPAN - 1)

/* RTC requires that a stored compare value be at least 2 ticks in
 * advance of the counter value in order to guarantee the compare
 * event is detected.  Assume that the counter will increment at most
 * twice between when it is read and the CC register is updated with
 * the value derived from the reading.
 */
#define COUNTER_MIN_DELTA 4U

/* The system clock infrastructure updates the 64-bit clock base value
 * in the FLIH whenever an alarm event occurs.  Snsure that there's an
 * alarm event at least twice for each counter wrap, to avoid the
 * possibility of delayed FLIH execution causing a counter lap to be
 * missed.
 */
#define COUNTER_MAX_DELTA (COUNTER_SPAN / 2)

/* The number of system clock ticks per configured tick.
 *
 * Note that unless CONFIG_SYS_CLOCK_TICKS_PER_SEC is an integral
 * power of 2 the resulting system will not be synchronized to the
 * standard understanding of how long 1 s actually takes.
 */
#define SC_PER_TICK (u32_t)((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC	\
			     / CONFIG_SYS_CLOCK_TICKS_PER_SEC))

/* The maximum number of ticks that we can be sure will, when scaled
 * and added to last_tick_sc, produce a value that is interpreted as
 * being in the future.  The chosen SC maximum is one quarter the 32
 * bit counter span.
 *
 * This serves as the upper bound for any requested tick-based delay
 * in z_clock_set_timeout().
 */
#define MAX_TICKS ((1U << 30) / SC_PER_TICK)

static struct k_spinlock lock;

/* Last checkpointed cycle counter value.
 *
 * By design this is updated at least every COUNTER_MAX_DELTA.
 */
static u64_t last_cycles;

/* Flag indicating that the RTC FLIH is active.  This is used to
 * bypass compare register updates while the alarm queue is
 * potentially in flux.
 */
static bool volatile in_flih;

/* The number of cycles that must be added to last_cycles to produce
 * the current cycle counter value.
 */
static ALWAYS_INLINE u32_t cycles_delta_di(void)
{
	u32_t now24 = RTC->COUNTER;
	s32_t delta24 = now24 - (COUNTER_MASK & (u32_t)last_cycles);

	if (delta24 < 0) {
		delta24 += COUNTER_SPAN;
	}
	return delta24;
}

static ALWAYS_INLINE u32_t sysclock_get_32(void)
{
	return last_cycles + cycles_delta_di();
}

/* The low 32-bits of the cycle counter. */
u32_t z_timer_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	u32_t ret = sysclock_get_32();

	k_spin_unlock(&lock, key);
	return ret;
}

/* The full cycle counter.  This is not standard API; it should be. */
u64_t z_timer_cycle_get_64(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	u64_t ret = last_cycles;

	ret += cycles_delta_di();

	k_spin_unlock(&lock, key);
	return ret;
}

/* In a non-tickless system a tick should occur every SC_PER_TICK
 * system clock increments.  z_clock_announce() is invoked with 1 at
 * each alarm event.
 *
 * In a tickless system the timeout infrastructure uses an alarm
 * deadline that is an integer multiple of SC_PER_TICK, where the
 * multiplier is stored in next_tick_delta and the difference in ticks
 * is reflected in the value of next_tick_sc.  z_clock_announce() is
 * invoked with value that was in next_tick_delta when the alarm
 * fires.
 */

/* Low 32 bits of the system clock at the last tick event. */
static u32_t last_tick_sc;

/* Low 32 bits of the system clock at the next scheduled tick.  This
 * is also the deadline of tick_alarm when it is scheduled/ready.
 */
static u32_t next_tick_sc;

/* The number of SC_PER_TICK increments expressed by the difference
 * between next_tick_sc and last_tick_sc.
 */
static u32_t next_tick_delta;

/* Flag indicating that the tick alarm has been scheduled to fire as a
 * soon as possible, and rescheduling it is only going to delay the
 * announcement.
 */
static bool tick_asap;

static void
tick_alarm_handler (struct k_alarm *alarm,
		    void *ud)
{
	u32_t ntd;
	u32_t nts;
	k_spinlock_key_t key = k_spin_lock(&lock);

	ntd = next_tick_delta;
	if (ntd != 0) {
		last_tick_sc = next_tick_sc;
		if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
			next_tick_delta = 0;
			tick_asap = false;
		} else {
			next_tick_sc += SC_PER_TICK;
			nts = next_tick_sc;
		}
	}

	k_spin_unlock(&lock, key);

	if (ntd != 0) {
		if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
			(void)k_alarm_schedule(alarm, nts, 0);
		}
		z_clock_announce(ntd);
	}
}

static K_ALARM_DEFINE(tick_alarm, tick_alarm_handler, NULL, NULL);

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#ifdef CONFIG_TICKLESS_KERNEL
	/* Behavior interpreted from the documentation for this
	 * function:
	 *
	 * ticks=K_FOREVER disables the tick alarm.
	 *
	 * ticks=INT_MAX enables the tick alarm at the maximum
	 * possible delay.
	 *
	 * It is permitted to timeout early, as long as the wakeup is
	 * aligned to a tick boundary and the tick is properly
	 * announced.
	 *
	 * The number of ticks to announce is tied to the deadline,
	 * and must be positive.
	 *
	 * A non-positive tick schedules a wakeup for the next tick
	 * that can be announced.  This may produce a deadline that's
	 * passed (causing immediate callback) if uncounted ticks have
	 * occurred.
	 *
	 * A positive tick schedules for the requested number of ticks
	 * after the last announced tick.  This too may be in the
	 * past.
	 */

	/* Explain why we need to do this. */
	(void)k_alarm_cancel(&tick_alarm);

	if (ticks == K_FOREVER) {
		/* "no future timer interrupts are expected or
		 * required"
		 *
		 * At this point we have no obligation to maintain the
		 * tick clock; consequently in tick calculation code
		 * we are entitled to act as though last_tick_sc is
		 * within a half 32-bit span of the current time, an
		 * invariant maintained by MAX_TICKS.
		 */
		return;
	}

	u32_t nts;
	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ntd = (sysclock_get_32() - last_tick_sc) / SC_PER_TICK;

	if (ntd == 0) {
		/* Can't announce zero ticks */
		ntd = 1;
	}
	if (ticks > 0) {
		/* Kernel wants a timeout with a positive offset
		 * relative to the last announced tick.
		 */
		ntd = MAX(ntd, MIN(ticks, MAX_TICKS));
	}
	next_tick_delta = ntd;
	next_tick_sc = last_tick_sc + next_tick_delta * SC_PER_TICK;
	nts = next_tick_sc;
	k_spin_unlock(&lock, key);

	(void)k_alarm_schedule(&tick_alarm, nts, 0);
#endif
}

u32_t z_clock_elapsed(void)
{
	u32_t rv = 0;
#ifdef CONFIG_TICKLESS_KERNEL
	k_spinlock_key_t key = k_spin_lock(&lock);

	rv = (sysclock_get_32() - last_tick_sc) / SC_PER_TICK;

	k_spin_unlock(&lock, key);
#endif
	return rv;
}

/* Implement z_alarm_update_deadline. */
static void alarm_update_deadline_(u32_t now)
{
	u32_t compare = now;
	u32_t deadline = 0;
	int rc = k_alarm_next_deadline_(&deadline);

	if (rc < 0) {
		/* No unscheduled alarms, use maximum delay */
		compare += COUNTER_MAX_DELTA;
	} else if (rc > 0) {
		/* Next event at deadline.  If that's now or in the
		 * past target a compare of now; otherwise queue an
		 * event at that deadline or the maximum delta from
		 * now whichever's sooner.
		 */
		u32_t delay = deadline - now;

		if ((s32_t)delay > 0) {
			if (delay < COUNTER_MAX_DELTA) {
				compare += delay;
			} else {
				compare += COUNTER_MAX_DELTA;
			}
		} else {
			/* Alarm is (past) due, fire ASAP */
		}
	} else {
		/* Something is ready, fire ASAP */
	}

	/* compare is at most COUNTER_MAX_DELTA past now.  The RTC
	 * counter should not have advanced more than
	 * COUNTER_MAX_DELTA - 2 ticks past now.
	 *
	 * If the next event is already due and we haven't yet cleared
	 * the last event, leave it enabled so we'll re-enter the
	 * FLIH.
	 */
	if ((compare == now)
	    && RTC->EVENTS_COMPARE[0]) {
		return;
	}

	/* Make sure compare is at least COUNTER_MIN_DELTA past now,
	 * then clear the COMPARE event and set the compare value.
	 */
	if (COUNTER_MIN_DELTA > (compare - now)) {
		compare = now + COUNTER_MIN_DELTA;
	}
	RTC->EVENTS_COMPARE[0] = 0;
	RTC->CC[0] = compare;
}

void z_alarm_update_deadline(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	/*
	 * Skip the update if we get here because somebody scheduled
	 * an alarm during a timer or alarm callback, because we're
	 * going to do an update just before we leave the FLIH when we
	 * can adjust for time that passed while processing those
	 * callbacks.
	 */
	if (!in_flih) {
		alarm_update_deadline_(sysclock_get_32());
	}
	k_spin_unlock(&lock, key);
}

/* Note: this function has public linkage, and MUST have this
 * particular name.  The platform architecture itself doesn't care,
 * but there is a test (tests/kernel/arm_irq_vector_table) that needs
 * to find it to it can set it in a custom vector table.  Should
 * probably better abstract that at some point (e.g. query and reset
 * it by pointer at runtime, maybe?) so we don't have this leaky
 * symbol.
 */
void rtc1_nrf_isr(void *arg)
{
	ARG_UNUSED(arg);

	bool do_ready = false;
	u32_t now;
	k_spinlock_key_t key = k_spin_lock(&lock);

	in_flih = true;
	if (RTC->EVENTS_COMPARE[0]) {
		/*
		 * Refresh the captured system clock.  Transfer all
		 * alarms due at or before that clock to the ready
		 * queue.
		 *
		 * Note that we don't clear EVENTS_COMPARE here.  Both
		 * timer and alarm callbacks may be invoked after the
		 * currently-held lock is released; those callbacks
		 * will take time, and the next scheduled alarm may
		 * come due before we get back to complete the ISR.
		 * By leaving EVENTS_COMPARE enabled we can
		 * immediately re-enter the FLIH to process alarms
		 * that became due while we were busy, without having
		 * to delay 122 us just to be sure the COMPARE event
		 * is triggered.
		 */
		last_cycles += cycles_delta_di();
		do_ready = true;
		now = last_cycles;
	}

	k_spin_unlock(&lock, key);
	if (do_ready
	    && (k_alarm_split_(now) != 0)) {
		k_alarm_process_ready_();
	}

	/* Update the alarm COMPARE register for the next scheduled
	 * alarm event.  If it's already due because of time spent in
	 * callbacks any pending EVENTS_COMPARE will remain set so we
	 * don't incur COMPARE_MIN_DELAY.
	 */
	in_flih = false;
	z_alarm_update_deadline();
}

int z_clock_driver_init(struct device *device)
{
	struct device *clock;

	ARG_UNUSED(device);

	clock = device_get_binding(DT_NORDIC_NRF_CLOCK_0_LABEL "_32K");
	if (!clock) {
		return -1;
	}

	clock_control_on(clock, (void *)CLOCK_CONTROL_NRF_K32SRC);

	nrf_rtc_prescaler_set(RTC, 0);
	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_CLEAR);

	RTC->EVENTS_COMPARE[0] = 0;
	RTC->INTENSET = (RTC_INTENSET_COMPARE0_Set
			 << RTC_INTENSET_COMPARE0_Pos);
	RTC->CC[0] = COUNTER_MAX_DELTA;

	IRQ_CONNECT(RTC1_IRQn, 1, rtc1_nrf_isr, 0, 0);
	NVIC_ClearPendingIRQ(RTC1_IRQn);
	irq_enable(RTC1_IRQn);

	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_START);

	if (!IS_ENABLED(TICKLESS_KERNEL)) {
		next_tick_delta = 1;
		next_tick_sc = SC_PER_TICK;
		k_alarm_schedule(&tick_alarm, SC_PER_TICK, 0);
	}

	return 0;
}
