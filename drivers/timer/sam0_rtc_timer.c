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
#include <zephyr/sys_clock.h>
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

/* Number of sys timer cycles per on tick. */
#define CYCLES_PER_TICK (RTC_CLOCK_HW_CYCLES_PER_SEC \
			 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* Maximum number of ticks. */
#define MAX_TICKS (UINT32_MAX / CYCLES_PER_TICK - 2)

#ifdef CONFIG_TICKLESS_KERNEL

/*
 * Due to the nature of clock synchronization, reading from or writing to some
 * RTC registers takes approximately six RTC_GCLK cycles. This constant defines
 * a safe threshold for the comparator.
 */
#define TICK_THRESHOLD 7

BUILD_ASSERT(CYCLES_PER_TICK > TICK_THRESHOLD,
	     "CYCLES_PER_TICK must be greater than TICK_THRESHOLD for "
	     "tickless mode");

#else /* !CONFIG_TICKLESS_KERNEL */

/*
 * For some reason, RTC does not generate interrupts when COMP == 0,
 * MATCHCLR == 1 and PRESCALER == 0. So we need to check that CYCLES_PER_TICK
 * is more than one.
 */
BUILD_ASSERT(CYCLES_PER_TICK > 1,
	     "CYCLES_PER_TICK must be greater than 1 for ticking mode");

#endif /* CONFIG_TICKLESS_KERNEL */

/* Tick/cycle count of the last announce call. */
static volatile uint32_t rtc_last;

#ifndef CONFIG_TICKLESS_KERNEL

/* Current tick count. */
static volatile uint32_t rtc_counter;

/* Tick value of the next timeout. */
static volatile uint32_t rtc_timeout;

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

static void rtc_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Read and clear the interrupt flag register. */
	uint16_t status = RTC0->INTFLAG.reg;

	RTC0->INTFLAG.reg = status;

#ifdef CONFIG_TICKLESS_KERNEL

	/* Read the current counter and announce the elapsed time in ticks. */
	uint32_t count = rtc_count();

	if (count != rtc_last) {
		uint32_t ticks = (count - rtc_last) / CYCLES_PER_TICK;

		sys_clock_announce(ticks);
		rtc_last += ticks * CYCLES_PER_TICK;
	}

#else /* !CONFIG_TICKLESS_KERNEL */

	if (status) {
		/* RTC just ticked one more tick... */
		if (++rtc_counter == rtc_timeout) {
			sys_clock_announce(rtc_counter - rtc_last);
			rtc_last = rtc_counter;
		}
	} else {
		/* ISR was invoked directly from sys_clock_set_timeout. */
		sys_clock_announce(0);
	}

#endif /* CONFIG_TICKLESS_KERNEL */
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#ifdef CONFIG_TICKLESS_KERNEL

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t) MAX_TICKS);

	/* Compute number of RTC cycles until the next timeout. */
	uint32_t count = rtc_count();
	uint32_t timeout = ticks * CYCLES_PER_TICK + count % CYCLES_PER_TICK;

	/* Round to the nearest tick boundary. */
	timeout = DIV_ROUND_UP(timeout, CYCLES_PER_TICK) * CYCLES_PER_TICK;

	if (timeout < TICK_THRESHOLD) {
		timeout += CYCLES_PER_TICK;
	}

	rtc_sync();
	RTC0->COMP[0].reg = count + timeout;

#else /* !CONFIG_TICKLESS_KERNEL */

	if (ticks == K_TICKS_FOREVER) {
		/* Disable comparator for K_TICKS_FOREVER and other negative
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
	unsigned int key = irq_lock();

	rtc_timeout = rtc_counter + ticks;
	irq_unlock(key);

#endif /* CONFIG_TICKLESS_KERNEL */
}

uint32_t sys_clock_elapsed(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (rtc_count() - rtc_last) / CYCLES_PER_TICK;
#else
	return rtc_counter - rtc_last;
#endif
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* Just return the absolute value of RTC cycle counter. */
	return rtc_count();
}

#define ASSIGNED_CLOCKS_CELL_BY_NAME						\
	ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_NAME

static int sys_clock_driver_init(void)
{
	volatile uint32_t *mclk = ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(0);
	uint32_t mclk_mask = ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(0, bit);

	*cfg->mclk |= cfg->mclk_mask;

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

	rtc_last = 0U;

	/* Configure RTC with 32-bit mode, configured prescaler and MATCHCLR. */
#ifdef RTC_MODE0_CTRL_MODE
	uint16_t ctrl = RTC_MODE0_CTRL_MODE(0) | RTC_MODE0_CTRL_PRESCALER(0);
#else
	uint16_t ctrl = RTC_MODE0_CTRLA_MODE(0) | RTC_MODE0_CTRLA_PRESCALER(0);
#endif

#ifdef RTC_MODE0_CTRLA_COUNTSYNC
	ctrl |= RTC_MODE0_CTRLA_COUNTSYNC;
#endif

#ifndef CONFIG_TICKLESS_KERNEL
#ifdef RTC_MODE0_CTRL_MATCHCLR
	ctrl |= RTC_MODE0_CTRL_MATCHCLR;
#else
	ctrl |= RTC_MODE0_CTRLA_MATCHCLR;
#endif
#endif
	rtc_sync();
#ifdef RTC_MODE0_CTRL_MODE
	RTC0->CTRL.reg = ctrl;
#else
	RTC0->CTRLA.reg = ctrl;
#endif

#ifdef CONFIG_TICKLESS_KERNEL
	/* Tickless kernel lets RTC count continually and ignores overflows. */
	RTC0->INTENSET.reg = RTC_MODE0_INTENSET_CMP0;
#else
	/* Non-tickless mode uses comparator together with MATCHCLR. */
	rtc_sync();
	RTC0->COMP[0].reg = CYCLES_PER_TICK;
	RTC0->INTENSET.reg = RTC_MODE0_INTENSET_OVF;
	rtc_counter = 0U;
	rtc_timeout = 0U;
#endif

	/* Enable RTC module. */
	rtc_sync();
#ifdef RTC_MODE0_CTRL_ENABLE
	RTC0->CTRL.reg |= RTC_MODE0_CTRL_ENABLE;
#else
	RTC0->CTRLA.reg |= RTC_MODE0_CTRLA_ENABLE;
#endif

	/* Enable RTC interrupt. */
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority), rtc_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

/* clang-format on */
