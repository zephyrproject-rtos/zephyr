/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/clock.h>
#include <zephyr/irq.h>
#include <fsl_rtc.h>

#define RTC_NODE DT_CHOSEN(zephyr_system_timer)

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to an nxp,rtc-jdp node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(RTC_NODE, nxp_rtc_jdp),
	     "zephyr,system-timer must point to an nxp,rtc-jdp compatible node");

#define RTC_JDP_BASE  ((RTC_Type *)DT_REG_ADDR(RTC_NODE))
#define RTC_IRQN      DT_IRQN(RTC_NODE)
#define RTC_IRQ_PRIO  DT_IRQ(RTC_NODE, priority)
#define RTC_CLK_SRC   DT_PROP(RTC_NODE, clock_source)
#define RTC_PRESCALER DT_PROP(RTC_NODE, prescaler)
#define RTC_CLK_FREQ  DT_PROP(RTC_NODE, clock_frequency)

/* The counter rate. Devicetree is the source of truth; the kernel has to agree. */
#define RTC_JDP_RATE (RTC_CLK_FREQ / RTC_PRESCALER)

BUILD_ASSERT(RTC_JDP_RATE * RTC_PRESCALER == RTC_CLK_FREQ,
	     "rtc prescaler must divide clock-frequency exactly");
BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == RTC_JDP_RATE,
	     "CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC must be the rtc clock-frequency "
	     "divided by its prescaler");
BUILD_ASSERT(RTC_JDP_RATE % CONFIG_SYS_CLOCK_TICKS_PER_SEC == 0,
	     "CONFIG_SYS_CLOCK_TICKS_PER_SEC must divide the rtc counter rate, or a "
	     "tick is not a whole number of cycles and uptime runs off");

/* Map the numeric prescaler (1/32/512/16384) to the SDK clock-divide enum */
#define RTC_JDP_DIV_ENUM						\
	(RTC_PRESCALER == 1     ? kRTC_ClockDivide1			\
	 : RTC_PRESCALER == 32  ? kRTC_ClockDivide32			\
	 : RTC_PRESCALER == 512 ? kRTC_ClockDivide512			\
				: kRTC_ClockDivide16384)

/*
 * Bounded spin waiting for an in-flight RTCVAL synchronization to finish (see
 * timer_driver_set_compare). A sync takes a few RTC cycles (~100 us at
 * 32 kHz); this bound is far larger and only prevents an unbounded spin on a
 * stuck peripheral.
 */
#define RTC_JDP_SYNC_SPIN 100000U

/*
 * Cycles the compare must stay ahead of the current count when programmed.
 * RTCVAL is an equality compare that synchronizes into the slow RTC clock
 * domain after the CPU samples the count; this margin absorbs that sync
 * latency so the counter cannot reach the compare before it is armed.
 */
#define RTC_JDP_COMPARE_MARGIN (2U * MINIMUM_RTCVAL)

static inline void rtc_jdp_wait_inv_clear(void)
{
	for (uint32_t i = 0U; i < RTC_JDP_SYNC_SPIN; i++) {
		if ((RTC_GetStatusFlags(RTC_JDP_BASE) & (uint32_t)kRTC_InvalidRTCFlag) == 0U) {
			break;
		}
	}
}

/*
 * A free-running 32-bit counter and RTCVAL, a one-shot equality compare, so a
 * COMPARE_EXACT backend. The core keeps a target whose distance equals the lead,
 * so the lead is one past the margin the compare has to clear.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_ALARM_LEAD_CYCLES (RTC_JDP_COMPARE_MARGIN + 1U)

static uint32_t timer_driver_cycle_get(void)
{
	return RTC_GetCountValue(RTC_JDP_BASE);
}

static void timer_driver_set_compare(uint32_t cycles)
{
	/*
	 * An RTCVAL write starts a synchronization into the RTC clock domain
	 * and sets INV_RTC; while INV_RTC is set the hardware ignores further
	 * RTCVAL writes (RM ch. 66 "Invalid RTC write"). Wait for a previous
	 * write to synchronize before programming.
	 */
	rtc_jdp_wait_inv_clear();
	RTC_ClearInterruptFlags(RTC_JDP_BASE, kRTC_RTCInterruptFlag);

	/*
	 * RTCVAL only raises RTCF above MINIMUM_RTCVAL (fsl_rtc.h, which also
	 * asserts on it). That bounds the register value rather than the distance
	 * to the count, so a tick-aligned target landing just past the counter
	 * wrap has to be nudged clear of it.
	 */
	if (cycles <= MINIMUM_RTCVAL) {
		cycles = MINIMUM_RTCVAL + 1U;
	}
	RTC_SetRTCValue(RTC_JDP_BASE, cycles);
}

#include "system_timer_generic.h"

BUILD_ASSERT(TIMER_CORE_CYC_PER_TICK > TIMER_CORE_ALARM_LEAD_CYCLES,
	     "tick period is shorter than the rtc compare-sync margin");

void sys_clock_disable(void)
{
	/*
	 * Terminal teardown only (e.g. before reboot/power-off). Disabling the
	 * counter resets the RTC logic, so neither the tick nor the cycle counter
	 * resumes afterwards.
	 */
	RTC_DisableInterrupts(RTC_JDP_BASE, kRTC_RTCInterruptEnable);
	RTC_DisableRTC(RTC_JDP_BASE);
	irq_disable(RTC_IRQN);
}

static void mcux_rtc_jdp_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();

	RTC_ClearInterruptFlags(RTC_JDP_BASE, kRTC_RTCInterruptFlag);

	timer_core_announce_from(key);
}

static int sys_clock_driver_init(void)
{
	rtc_config_t config;

	RTC_GetDefaultConfig(&config);
	config.clockSource = (rtc_clock_source_t)RTC_CLK_SRC;
	config.clockDivide = RTC_JDP_DIV_ENUM;
	RTC_Init(RTC_JDP_BASE, &config);
	RTC_ClearInterruptFlags(RTC_JDP_BASE, kRTC_AllInterruptFlags);

	IRQ_CONNECT(RTC_IRQN, RTC_IRQ_PRIO, mcux_rtc_jdp_timer_isr, NULL, 0);
	irq_enable(RTC_IRQN);

	/*
	 * Program the first compare and the match interrupt, then enable the
	 * counter last so it starts from a fully configured state. Enabling
	 * resets the RTC logic and starts the count from zero, committing the
	 * RTCVAL written here (RM ch. 66).
	 */
	RTC_SetRTCValue(RTC_JDP_BASE, TIMER_CORE_CYC_PER_TICK);
	RTC_EnableInterrupts(RTC_JDP_BASE, kRTC_RTCInterruptEnable);
	RTC_EnableRTC(RTC_JDP_BASE);

	/* Seed the announce baseline and arm the first tick. */
	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
