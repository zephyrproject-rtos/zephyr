/*
 * Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TI SimpleLink CC35XX timer driver based on RTC */

#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#include <cmsis_core.h>
#include <inc/hw_types.h>
#include <inc/hw_rtc.h>
#include <inc/hw_ints.h>

/* Kernel tick period in microseconds (same timebase as RTC) */
#define TICK_PERIOD_SYS (USEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * Max number of RTC ticks into the future
 *
 * Under the hood, the kernel timer uses the RTC whose events trigger
 * immediately if the compare value is less than 2^22 RTC ticks in the past
 * (4.194sec at 1us resolution). Therefore, the max number of RTC ticks you
 * can schedule into the future is 2^32 - 2^22 - 1 ticks (~= 4290 sec at 1us
 * resolution).
 */
#define RTC_TIMEOUT_MAX 0xFFBFFFFFU

/* Set RTC interrupt to lowest priority */
#define RTC_ISR_PRIORITY 3U
#define CC35XX_RTC_NODE  DT_NODELABEL(rtc)
#define CC35XX_RTC_BASE  DT_REG_ADDR(CC35XX_RTC_NODE)

static struct k_spinlock lock;

/* Keep track of RTC counter at previous announcement to the kernel */
static uint32_t last_rtc_count;

static void rtc_isr(const void *arg);
static int sys_clock_driver_init(void);
static uint32_t sys_clock_elapsed_ticks(uint32_t current, uint32_t last);

/*
 * Set system clock timeout.
 */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	k_spinlock_key_t key = k_spin_lock(&lock);
	/* If timeout is necessary */
	if (ticks != K_TICKS_FOREVER) {
		uint32_t now_tick = sys_read32(CC35XX_RTC_BASE + RTC_O_TIME1U);
		uint32_t elapsed = now_tick - last_rtc_count;
		uint32_t timeout;

		/*
		 * Zephyr passes the number of ticks until the next timeout.
		 * Program the comparator relative to the last announced
		 * kernel tick so we do not leave CH0 behind "now" when PM
		 * reaches the sleep hook late in the current tick.
		 */
		ticks = CLAMP(ticks - 1, 0, (int32_t)(RTC_TIMEOUT_MAX / TICK_PERIOD_SYS));
		timeout = (ticks * TICK_PERIOD_SYS) + elapsed;
		timeout = DIV_ROUND_UP(timeout, TICK_PERIOD_SYS) * TICK_PERIOD_SYS;

		if (timeout > RTC_TIMEOUT_MAX) {
			timeout = RTC_TIMEOUT_MAX;
			/* Make sure timeout is a multiple of TICK_PERIOD_SYS */
			timeout -= timeout % TICK_PERIOD_SYS;
		}
		/* This should wrap around */
		sys_write32(last_rtc_count + timeout, CC35XX_RTC_BASE + RTC_O_CH0CC1U);
	} else {
		sys_write32(last_rtc_count + RTC_TIMEOUT_MAX, CC35XX_RTC_BASE + RTC_O_CH0CC1U);
	}
	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	/* Get current value as early as possible */
	uint32_t current_rtc_count = sys_read32(CC35XX_RTC_BASE + RTC_O_TIME1U);

	int32_t elapsed_ticks = sys_clock_elapsed_ticks(current_rtc_count, last_rtc_count);

	k_spin_unlock(&lock, key);
	return elapsed_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return sys_read32(CC35XX_RTC_BASE + RTC_O_TIME1U);
}

void sys_clock_idle_exit(void)
{
	/*
	 * PM resume can return with the RTC interrupt disabled after the TI
	 * sleep path manipulated NVIC state internally. Do not clear RTC/NVIC
	 * pending state here, as that can consume the wake event before the
	 * normal RTC ISR path handles it.
	 */
	irq_enable(INT_RTC_EVENT_IRQn);
}

void rtc_isr(const void *arg)
{
	k_spinlock_key_t key;
	uint32_t current_rtc_count;
	uint32_t elapsed_ticks;

	ARG_UNUSED(arg);

	/* Clear CH0 interrupt — RTC requires explicit clear unlike SYSTIM */
	sys_write32(RTC_ICLR_EV0_CLR, CC35XX_RTC_BASE + RTC_O_ICLR);

	key = k_spin_lock(&lock);
	/* Get current value as early as possible */
	current_rtc_count = sys_read32(CC35XX_RTC_BASE + RTC_O_TIME1U);
	elapsed_ticks = sys_clock_elapsed_ticks(current_rtc_count, last_rtc_count);
	last_rtc_count = current_rtc_count;
	k_spin_unlock(&lock, key);
	sys_clock_announce(elapsed_ticks);

	/* Do not re-arm RTC. Zephyr will do so through sys_clock_set_timeout */
}

static int sys_clock_driver_init(void)
{
	uint32_t nowTick;

	/* Get current value as early as possible */
	nowTick = sys_read32(CC35XX_RTC_BASE + RTC_O_TIME1U);
	last_rtc_count = nowTick;

	/* Clear any pending interrupts on RTC channel 0 */
	sys_write32(RTC_ICLR_EV0_CLR, CC35XX_RTC_BASE + RTC_O_ICLR);

	/* Make RTC halt on CPU debug halt */
	sys_write32(RTC_EMU_HALT_STOP, CC35XX_RTC_BASE + RTC_O_EMU);

	/*
	 * Set IMASK for channel 0. IMASK is used by the power driver to know
	 * which RTC channels are active.
	 */
	sys_write32(RTC_IMSET_EV0_SET, CC35XX_RTC_BASE + RTC_O_IMSET);

	/* This should wrap around and set a maximum timeout */
	sys_write32(nowTick + RTC_TIMEOUT_MAX, CC35XX_RTC_BASE + RTC_O_CH0CC1U);

	/* Take RTC interrupt (CMSIS IRQn 49) */
	IRQ_CONNECT(INT_RTC_EVENT_IRQn, RTC_ISR_PRIORITY, rtc_isr, 0, 0);
	irq_enable(INT_RTC_EVENT_IRQn);

	return 0;
}

static uint32_t sys_clock_elapsed_ticks(uint32_t current, uint32_t last)
{
	if (current >= last) {
		return (current / TICK_PERIOD_SYS) - (last / TICK_PERIOD_SYS);
	} else {
		return ((0xFFFFFFFF - last) / TICK_PERIOD_SYS) + (current / TICK_PERIOD_SYS);
	}
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
