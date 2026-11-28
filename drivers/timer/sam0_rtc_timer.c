/*
 * Copyright (c) 2018 omSquare s.r.o.
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_rtc

/**
 * @file
 * @brief Atmel SAM0 series RTC-based system timer
 *
 * This system timer implementation supports both tickless and ticking modes.
 * In tickless mode, RTC counts continually in 32-bit mode and timeouts are
 * scheduled using the RTC comparator. In ticking mode, RTC is configured to
 * generate an interrupt every tick.
 */

#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/clock.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

/* clang-format off */

/* RTC registers. */
#define RTC0 ((RtcMode0 *) DT_INST_REG_ADDR(0))

#ifdef MCLK
#define RTC_CLOCK_HW_CYCLES_PER_SEC SOC_ATMEL_SAM0_OSC32K_FREQ_HZ
#else
#define RTC_CLOCK_HW_CYCLES_PER_SEC SOC_ATMEL_SAM0_GCLK0_FREQ_HZ
#endif

#ifndef CONFIG_TICKLESS_KERNEL

PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

#endif /* CONFIG_TICKLESS_KERNEL */

/*
 * Waits for RTC bus synchronization.
 */
static inline void rtc_sync(void)
{
	/* Wait for bus synchronization... */
#ifdef RTC_STATUS_SYNCBUSY
	while (RTC0->STATUS.reg & RTC_STATUS_SYNCBUSY) {
	}
#else
	while (RTC0->SYNCBUSY.reg) {
	}
#endif
}

/*
 * Reads RTC COUNT register. First a read request must be written to READREQ,
 * then - when bus synchronization completes - the COUNT register is read and
 * returned.
 */
static uint32_t rtc_count(void)
{
#ifdef RTC_READREQ_RREQ
	RTC0->READREQ.reg = RTC_READREQ_RREQ;
#endif
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
#ifdef RTC_MODE0_CTRL_ENABLE
	RTC0->CTRL.reg &= ~RTC_MODE0_CTRL_ENABLE;
#else
	RTC0->CTRLA.reg &= ~RTC_MODE0_CTRLA_ENABLE;
#endif

	rtc_sync();

	/* Initiate software reset. */
#ifdef RTC_MODE0_CTRL_SWRST
	RTC0->CTRL.bit.SWRST = 1;
	while (RTC0->CTRL.bit.SWRST) {
	}
#else
	RTC0->CTRLA.bit.SWRST = 1;
	while (RTC0->CTRLA.bit.SWRST) {
	}
#endif
}

/*
 * A free-running 32-bit counter and COMP[0] as an absolute compare, matching on
 * equality. timer_driver_set_compare() reads the counter and moves a deadline
 * that is too close to the compare lead ahead of it before its single write,
 * so a deadline already past fires late rather than being lost. That is the ordered
 * contract, so the backend declared is COMPARE_ORDERED and the core's verify
 * loop is left out.
 */
#define TIMER_CORE_BACKEND_COMPARE_ORDERED
#define TIMER_CORE_CYCLES_PER_SEC RTC_CLOCK_HW_CYCLES_PER_SEC
#define TIMER_CORE_HAVE_CYCLE_GET_32

/* A COMP[0] write takes up to 6 GCLK_RTC periods to reach the RTC clock domain
 * (D21 14.3, D20 13.3, L21 16.3.7, D5x 13.3.7).
 */
#define RTC_WRITE_SYNC_CYCLES 6U

#ifdef RTC_READREQ_RREQ
/* A read request returns COUNT once synchronized, so the read is current. */
#define RTC_READ_LAG_CYCLES 0U
#else
/* With COUNTSYNC, a COUNT read returns the value from the last
 * synchronization, up to 3 cycles behind the counter (measured on SAM E54).
 */
#define RTC_READ_LAG_CYCLES 3U

/* The latest count the counter is known to have reached. rtc_isr() raises it
 * to the matched COMP[0] when a read lags behind it, and
 * timer_driver_cycle_get() never returns less, so the core never sees the
 * counter go back behind an announced tick.
 */
static uint32_t rtc_floor;

static inline uint32_t rtc_count_floored(void)
{
	uint32_t count = rtc_count();

	/* A count behind the floor wraps to above half the range. */
	return ((count - rtc_floor) > (UINT32_MAX >> 1)) ? rtc_floor : count;
}
#endif

/* How far ahead of a counter read a compare value must be to be caught. */
#define RTC_COMPARE_LEAD_CYCLES (RTC_WRITE_SYNC_CYCLES + RTC_READ_LAG_CYCLES + 1U)

static inline uint32_t timer_driver_cycle_get(void)
{
#ifdef RTC_READREQ_RREQ
	return rtc_count();
#else
	/* The core calls this under the clock lock, so the update is safe. */
	rtc_floor = rtc_count_floored();
	return rtc_floor;
#endif
}

static inline void timer_driver_set_compare(uint32_t cycles)
{
	uint32_t now = timer_driver_cycle_get();
	uint32_t ahead = cycles - now;

	/* A deadline too close or behind the count, the second test being the
	 * wrapped form of the first, moves to the lead ahead of the count.
	 */
	if ((ahead < RTC_COMPARE_LEAD_CYCLES) || (ahead > (UINT32_MAX >> 1))) {
		cycles = now + RTC_COMPARE_LEAD_CYCLES;
	}

	/* The counter read above waited for SYNCBUSY to clear, so the write
	 * can go out right away. Nothing waits for it to land: the lead covers
	 * that.
	 */
	RTC0->COMP[0].reg = cycles;
}

#include "system_timer_generic.h"

/*
 * The RTC counts at RTC_CLOCK_HW_CYCLES_PER_SEC, which is not what the kernel
 * has been told CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC is: that stays the CPU
 * clock. The count is reported unscaled all the same, which is what this
 * driver has always done, so k_cycle_get_32() keeps reading in RTC units here.
 * Reconciling the two is a separate change.
 */
uint32_t sys_clock_cycle_get_32(void)
{
#ifdef RTC_READREQ_RREQ
	return rtc_count();
#else
	/* Not under the clock lock: read the floor but leave it alone. */
	return rtc_count_floored();
#endif
}

static void rtc_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Read and clear the interrupt flag register. */
	uint16_t flags = RTC0->INTFLAG.reg;

	RTC0->INTFLAG.reg = flags;

#ifdef RTC_READREQ_RREQ
	timer_core_announce();
#else
	k_spinlock_key_t key = sys_clock_lock();

	/* The match means the counter reached COMP[0], but the ISR can run
	 * before a read shows it. Announcing on that read would miss the tick
	 * just reached and the kernel would arm it again. So when a read is
	 * behind COMP[0] by no more than the read lag, take COMP[0] as reached.
	 * A COMP[0] rewritten after the match is at least the compare lead
	 * ahead of the count read when it was written, so it fails this test
	 * unless the counter is within the read lag of reaching it. Then the
	 * count runs ahead of the counter by no more than the read lag.
	 */
	if ((flags & RTC_MODE0_INTFLAG_CMP0) != 0U) {
		uint32_t comp = RTC0->COMP[0].reg;

		if (((comp - rtc_count_floored()) - 1U) < RTC_READ_LAG_CYCLES) {
			rtc_floor = comp;
		}
	}

	timer_core_announce_from(key);
#endif
}

#define ASSIGNED_CLOCKS_CELL_BY_NAME						\
	ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_NAME

static int sys_clock_driver_init(void)
{
	volatile uint32_t *mclk = ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(0);
	uint32_t mclk_mask = ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(0, bit);

	*mclk |= mclk_mask;

#ifdef MCLK
	OSC32KCTRL->RTCCTRL.reg = OSC32KCTRL_RTCCTRL_RTCSEL_ULP32K;
#else
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN
			  | GCLK_CLKCTRL_GEN(ASSIGNED_CLOCKS_CELL_BY_NAME(0, gclk, gen))
			  | GCLK_CLKCTRL_ID(DT_INST_CLOCKS_CELL_BY_NAME(0, gclk, id));

	/* Synchronize GCLK. */
	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
#endif

#ifndef CONFIG_TICKLESS_KERNEL
	int retval = pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);

	if (retval < 0 && retval != -ENOENT) {
		return retval;
	}
#endif

	/* Reset module to hardware defaults. */
	rtc_reset();

	/* Configure RTC with 32-bit mode and the configured prescaler. */
#ifdef RTC_MODE0_CTRL_MODE
	uint16_t ctrl = RTC_MODE0_CTRL_MODE(0) | RTC_MODE0_CTRL_PRESCALER(0);
#else
	uint16_t ctrl = RTC_MODE0_CTRLA_MODE(0) | RTC_MODE0_CTRLA_PRESCALER(0);
#endif

#ifdef RTC_MODE0_CTRLA_COUNTSYNC
	ctrl |= RTC_MODE0_CTRLA_COUNTSYNC;
#endif

	rtc_sync();
#ifdef RTC_MODE0_CTRL_MODE
	RTC0->CTRL.reg = ctrl;
#else
	RTC0->CTRLA.reg = ctrl;
#endif

	/* The RTC counts continually and overflows are ignored: the core masks
	 * every delta to the counter's width.
	 */
	RTC0->INTENSET.reg = RTC_MODE0_INTENSET_CMP0;

	/* Enable RTC module. */
	rtc_sync();
#ifdef RTC_MODE0_CTRL_ENABLE
	RTC0->CTRL.reg |= RTC_MODE0_CTRL_ENABLE;
#else
	RTC0->CTRLA.reg |= RTC_MODE0_CTRLA_ENABLE;
#endif

	/* Enable RTC interrupt. */
	k_irq_clear_pending(DT_INST_IRQN(0));
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority), rtc_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

/* clang-format on */
