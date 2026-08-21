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

/* Number of sys timer cycles per on tick. */
#define CYCLES_PER_TICK (RTC_CLOCK_HW_CYCLES_PER_SEC \
			 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

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
 * equality: a COMPARE_EXACT backend.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_CYCLES_PER_SEC RTC_CLOCK_HW_CYCLES_PER_SEC
#define TIMER_CORE_HAVE_CYCLE_GET_32

static inline uint32_t timer_driver_cycle_get(void)
{
	return rtc_count();
}

static inline void timer_driver_set_compare(uint32_t cycles)
{
	RTC0->COMP[0].reg = cycles;

	/* Wait for the write to cross into the RTC clock domain, so the counter
	 * the core reads next cannot predate the compare going live (D21 14.3,
	 * D20 13.3, L21 16.3.7, D5x 13.3.7). Every path leaves SYNCBUSY clear
	 * on return, so none is owed on entry.
	 */
	rtc_sync();
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
	return rtc_count();
}

static void rtc_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Read and clear the interrupt flag register. */
	RTC0->INTFLAG.reg = RTC0->INTFLAG.reg;

	timer_core_announce();
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
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority), rtc_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

/* clang-format on */
