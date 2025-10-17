/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_rtc_timer

#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#include <inc/hw_rtc.h>
#include <inc/hw_types.h>
#include <inc/hw_evtsvt.h>
#include <inc/hw_memmap.h>

#define RTC_TIMEOUT_MAX 0xFFBFFFFFU

/* Set rtc interrupt to lowest priority */
#define SYSTIM_ISR_PRIORITY 3U

/* Keep track of rtc counter at previous announcement to the kernel */
static uint32_t last_rtc_count;

#define TICK_PERIOD_MICRO_SEC (1000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	/* If timeout is necessary */
	if (ticks != K_TICKS_FOREVER) {
		/* Get current value as early as possible */
		uint32_t ticks_now = HWREG(RTC_BASE + RTC_O_TIME8U);

		if ((ticks_now + ticks) >= RTC_TIMEOUT_MAX) {
			/* Reset timer and start from 0 */
			HWREG(RTC_BASE + RTC_O_CTL) = RTC_CTL_RST_CLR;
			HWREG(RTC_BASE + RTC_O_CH0CC8U) = ticks;
		}

		HWREG(RTC_BASE + RTC_O_CH0CC8U) = ticks_now + ticks;
	}
}

uint32_t get_elapsed_ticks_rtc(uint32_t current_rtc_count)
{
	if (current_rtc_count >= last_rtc_count) {
		return (current_rtc_count - last_rtc_count);
	} else {
		return ((0xFFFFFFFF - last_rtc_count) + current_rtc_count);
	}
}

uint32_t sys_clock_elapsed(void)
{
	int32_t elapsed_ticks = get_elapsed_ticks_rtc(HWREG(RTC_BASE + RTC_O_TIME8U)) /
						      TICK_PERIOD_MICRO_SEC;

	return elapsed_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return HWREG(RTC_BASE + RTC_O_TIME8U);
}

static void rtc_isr(const void *arg)
{
	uint32_t current_rtc_count = HWREG(RTC_BASE + RTC_O_TIME8U);
	int32_t elapsed_ticks = get_elapsed_ticks_rtc(current_rtc_count);

	HWREG(RTC_BASE + RTC_O_ICLR) = RTC_ICLR_EV0_CLR;

	sys_clock_announce(elapsed_ticks);

	last_rtc_count = current_rtc_count;
}

static int sys_clock_driver_init(void)
{
	uint32_t now_ticks;

	now_ticks = HWREG(RTC_BASE + RTC_O_TIME8U);
	last_rtc_count = now_ticks;

	HWREG(RTC_BASE + RTC_O_ICLR) = RTC_ICLR_EV0_CLR | RTC_ICLR_EV1_CLR;
	HWREG(RTC_BASE + RTC_O_IMCLR) = RTC_IMCLR_EV0_CLR | RTC_IMCLR_EV1_CLR;

	HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ16SEL) = EVTSVT_CPUIRQ16SEL_PUBID_AON_RTC_COMB;
	HWREG(RTC_BASE + RTC_O_CH0CC8U) = now_ticks + RTC_TIMEOUT_MAX;

	HWREG(RTC_BASE + RTC_O_IMASK) = RTC_IMASK_EV0_EN;
	HWREG(RTC_BASE + RTC_O_ARMSET) = RTC_ARMSET_CH0_SET;

	/* Take configurable interrupt IRQ16 for rtc */
	IRQ_CONNECT(CPUIRQ16_IRQn, SYSTIM_ISR_PRIORITY, rtc_isr, 0, 0);
	irq_enable(CPUIRQ16_IRQn);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
