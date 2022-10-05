/*
 * Copyright (c) 2019, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_rtc

/*
 * TI SimpleLink CC13X2/CC26X2 RTC-based system timer
 *
 * This system timer implementation supports both tickless and ticking modes.
 * RTC counts continually in 64-bit mode and timeouts are
 * scheduled using the RTC comparator. An interrupt is triggered whenever
 * the comparator value set is reached.
 */

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>

#include <driverlib/interrupt.h>
#include <driverlib/aon_rtc.h>
#include <driverlib/aon_event.h>

#define RTC_COUNTS_PER_SEC 0x100000000ULL

/* Number of counts per rtc timer cycle */
#define RTC_COUNTS_PER_CYCLE (RTC_COUNTS_PER_SEC / \
	sys_clock_hw_cycles_per_sec())

/* Number of counts per system clock tick */
#define RTC_COUNTS_PER_TICK (RTC_COUNTS_PER_SEC / \
	CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* Number of RTC cycles per system clock tick */
#define CYCLES_PER_TICK (sys_clock_hw_cycles_per_sec() / \
	CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * Maximum number of ticks.
 */
#define MAX_CYC 0x7FFFFFFFFFFFULL
#define MAX_TICKS (MAX_CYC / RTC_COUNTS_PER_TICK)

/*
 * Due to the nature of clock synchronization, the comparator cannot be set
 * to a value that is too close to the current time. This constant defines
 * a safe threshold for the comparator.
 */
#define COMPARE_MARGIN 6

/* RTC count of the last announce call, rounded down to tick boundary. */
static volatile uint64_t rtc_last;

#ifdef CONFIG_TICKLESS_KERNEL
static struct k_spinlock lock;
#else
static uint64_t nextThreshold = RTC_COUNTS_PER_TICK;
#endif /* CONFIG_TICKLESS_KERNEL */


static void setThreshold(uint32_t next)
{
	uint32_t now;
	unsigned int key;

	key = irq_lock();

	/* get the current RTC count corresponding to compare window */
	now = AONRTCCurrentCompareValueGet();

	/* if next is too soon, set at least one RTC tick in future */
	/* assume next never be more than half the maximum 32 bit count value */
	if ((next - now) > (uint32_t)0x80000000) {
		/* now is past next */
		next = now + COMPARE_MARGIN;
	} else if ((now + COMPARE_MARGIN - next) < (uint32_t)0x80000000) {
		if (next < now + COMPARE_MARGIN) {
			next = now + COMPARE_MARGIN;
		}
	}

	/* set next compare threshold in RTC */
	AONRTCCompareValueSet(AON_RTC_CH0, next);

	irq_unlock(key);
}

void rtc_isr(const void *arg)
{
#ifndef CONFIG_TICKLESS_KERNEL
	uint64_t newThreshold;
	uint32_t next;
#else
	uint64_t ticks, currCount;
#endif

	ARG_UNUSED(arg);

	AONRTCEventClear(AON_RTC_CH0);

#ifdef CONFIG_TICKLESS_KERNEL
	k_spinlock_key_t key = k_spin_lock(&lock);
	currCount = (uint64_t)AONRTCCurrent64BitValueGet();
	ticks = (currCount - rtc_last) / RTC_COUNTS_PER_TICK;

	rtc_last += ticks * RTC_COUNTS_PER_TICK;
	k_spin_unlock(&lock, key);

	sys_clock_announce(ticks);

#else /* !CONFIG_TICKLESS_KERNEL */

	/* calculate new 64-bit RTC count for next interrupt */
	newThreshold = nextThreshold + RTC_COUNTS_PER_TICK;

	next = (uint32_t)((uint64_t)newThreshold >> 16);
	setThreshold(next);

	nextThreshold = newThreshold;

	rtc_last += RTC_COUNTS_PER_TICK;

	sys_clock_announce(1);

#endif /* CONFIG_TICKLESS_KERNEL */
}

static void initDevice(void)
{
	AONRTCDisable();
	AONRTCReset();

	HWREG(AON_RTC_BASE + AON_RTC_O_SYNC) = 1;
	/* read sync register to complete reset */
	HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);

	AONRTCEventClear(AON_RTC_CH0);
	IntPendClear(INT_AON_RTC_COMB);

	HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
}

static void startDevice(void)
{
	uint32_t compare;
	uint64_t period;
	unsigned int key;

	key = irq_lock();

	/* reset timer */
	AONRTCReset();
	AONRTCEventClear(AON_RTC_CH0);
	IntPendClear(INT_AON_RTC_COMB);

	/*
	 * set the compare register to one period.
	 * For a very small period round up to interrupt upon 4th tick in
	 * compare register
	 */
	period = RTC_COUNTS_PER_TICK;
	if (period < 0x40000) {
		compare = 0x4; /* 4 * 15.5us ~= 62us */
	} else {
		/* else, interrupt on first period expiration */
		compare = period >> 16;
	}

	/* set the compare value at the RTC */
	AONRTCCompareValueSet(AON_RTC_CH0, compare);

	/* enable compare channel 0 */
	AONEventMcuWakeUpSet(AON_EVENT_MCU_WU0, AON_EVENT_RTC0);
	AONRTCChannelEnable(AON_RTC_CH0);
	AONRTCCombinedEventConfig(AON_RTC_CH0);

	/* start timer */
	AONRTCEnable();

	irq_unlock(key);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#ifdef CONFIG_TICKLESS_KERNEL

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t) MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* Compute number of RTC cycles until the next timeout. */
	uint64_t count = AONRTCCurrent64BitValueGet();
	uint64_t timeout = ticks * RTC_COUNTS_PER_TICK +
		(count - rtc_last);

	/* Round to the nearest tick boundary. */
	timeout = (timeout + RTC_COUNTS_PER_TICK - 1) / RTC_COUNTS_PER_TICK
		  * RTC_COUNTS_PER_TICK;
	timeout = MIN(timeout, MAX_CYC);
	timeout += rtc_last;

	/* Set the comparator */
	setThreshold(timeout >> 16);

	k_spin_unlock(&lock, key);
#endif /* CONFIG_TICKLESS_KERNEL */
}

uint32_t sys_clock_elapsed(void)
{
	uint32_t ret = (AONRTCCurrent64BitValueGet() - rtc_last) /
		RTC_COUNTS_PER_TICK;

	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)(AONRTCCurrent64BitValueGet() / RTC_COUNTS_PER_CYCLE);
}

uint64_t sys_clock_cycle_get_64(void)
{
	return AONRTCCurrent64BitValueGet() / RTC_COUNTS_PER_CYCLE;
}

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	rtc_last = 0U;

	initDevice();
	startDevice();

	/* Enable RTC interrupt. */
	IRQ_CONNECT(DT_INST_IRQN(0),
		DT_INST_IRQ(0, priority),
		rtc_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
