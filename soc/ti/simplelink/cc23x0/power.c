/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 Baylibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include <ti/drivers/utils/Math.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC23X0.h>

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ckmd.h>
#include <inc/hw_systim.h>
#include <inc/hw_rtc.h>
#include <inc/hw_evtsvt.h>
#include <inc/hw_ints.h>

#include <driverlib/lrfd.h>
#include <driverlib/ull.h>
#include <driverlib/pmctl.h>

/* Configuring TI Power module to not use its policy function (we use Zephyr's
 * instead), and disable oscillator calibration functionality for now.
 */
const PowerCC23X0_Config PowerCC23X0_config = {
	.policyInitFxn = NULL,
	.policyFxn = NULL,
};

#ifdef CONFIG_PM

#ifndef CONFIG_CC23X0_RTC_TIMER

#define MAX_SYSTIMER_DELTA 0xFFBFFFFFU
#define RTC_TO_SYSTIM_TICKS 8U
#define SYSTIM_CH_STEP      4U
#define SYSTIM_CH(idx)      (SYSTIM_O_CH0CC + idx * SYSTIM_CH_STEP)
#define SYSTIM_TO_RTC_SHIFT 3U
#define SYSTIM_CH_CNT       5U
#define RTC_CH_CNT          2U
#define RTC_NEXT(val, now)  (((val - PowerCC23X0_WAKEDELAYSTANDBY) >> SYSTIM_TO_RTC_SHIFT) + now)

#endif /*CONFIG_CC23X0_RTC_TIMER*/

static void pm_cc23x0_enter_standby(void);
static int power_initialize(void);
extern int_fast16_t PowerCC23X0_notify(uint_fast16_t eventType);

#ifndef CONFIG_CC23X0_RTC_TIMER
static void pm_cc23x0_systim_standby_restore(void);

/* Global to stash the SysTimer timeouts while we enter standby */
static uint32_t systim[SYSTIM_CH_CNT];
static uint32_t rtc[RTC_CH_CNT];
static uintptr_t key;
static uint32_t systim_mask;
static uint32_t rtc_mask;

/* Shift values to convert between the different resolutions of the SysTimer
 * channels. Channel 0 can technically support either 1us or 250ns. Until the
 * channel is actively used, we will hard-code it to 1us resolution to improve
 * runtime.
 */
const uint8_t systim_offset[SYSTIM_CH_CNT] = {
	0, /* 1us */
	0, /* 1us */
	2, /* 250ns -> 1us */
	2, /* 250ns -> 1us */
	2  /* 250ns -> 1us */
};
#endif /*CONFIG_CC23X0_RTC_TIMER*/

#ifndef CONFIG_CC23X0_RTC_TIMER
static void pm_cc23x0_systim_standby_restore(void)
{
	HWREG(RTC_BASE + RTC_O_ARMCLR) = RTC_ARMCLR_CH0_CLR;
	HWREG(RTC_BASE + RTC_O_ICLR) = RTC_ICLR_EV0_CLR;

	ULLSync();

	HwiP_clearInterrupt(INT_CPUIRQ16);
	HwiP_clearInterrupt(INT_CPUIRQ3);

	HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ16SEL) = EVTSVT_CPUIRQ16SEL_PUBID_SYSTIM0;
	HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ3SEL) = EVTSVT_CPUIRQ16SEL_PUBID_AON_RTC_COMB;

	while (HWREG(SYSTIM_BASE + SYSTIM_O_STATUS) != SYSTIM_STATUS_VAL_RUN) {
		;
	}

	for (uint8_t idx = 0; idx < SYSTIM_CH_CNT; idx++) {
		if (systim_mask & (1 << idx)) {
			HWREG(SYSTIM_BASE + SYSTIM_CH(idx)) = systim[idx];
		}
	}

	HWREG(SYSTIM_BASE + SYSTIM_O_IMASK) = systim_mask;
	HWREG(RTC_BASE + RTC_O_IMASK) = rtc_mask;

	if (rtc_mask != 0) {
		if (rtc_mask & 0x1) {
			HWREG(RTC_BASE + RTC_O_CH0CC8U) = rtc[0];
		}
		if (rtc_mask & 0x2) {
			HWREG(RTC_BASE + RTC_O_CH1CC8U) = rtc[1];
		}
	}

	LRFDApplyClockDependencies();
	PowerCC23X0_notify(PowerLPF3_AWAKE_STANDBY);
	HwiP_restore(key);
}
#endif /*CONFIG_CC23X0_RTC_TIMER*/

static void pm_cc23x0_enter_standby(void)
{
#ifndef CONFIG_CC23X0_RTC_TIMER
	uint32_t rtc_now = 0;
	uint32_t systim_now = 0;
	uint32_t systim_next = MAX_SYSTIMER_DELTA;
	uint32_t systim_delta = 0;
	uint32_t rtc_delta = 0;

	key = HwiP_disable();

	uint32_t constraints = Power_getConstraintMask();
	bool standby = (constraints & (1 << PowerLPF3_DISALLOW_STANDBY)) == 0;
	bool idle = (constraints & (1 << PowerLPF3_DISALLOW_IDLE)) == 0;

	if (standby && (HWREG(CKMD_BASE + CKMD_O_LFCLKSEL) & CKMD_LFCLKSEL_MAIN_LFOSC) &&
	    !(HWREG(CKMD_BASE + CKMD_O_LFCLKSTAT) & CKMD_LFCLKSTAT_FLTSETTLED_M)) {
		standby = false;
		idle = false;
	}

	if (standby) {
		systim_mask = HWREG(SYSTIM_BASE + SYSTIM_O_IMASK);
		if (systim_mask != 0) {
			systim_next = 0xFFFFFFFF;
			systim_now = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);
			for (uint8_t idx = 0; idx < SYSTIM_CH_CNT; idx++) {
				if (systim_mask & (1 << idx)) {
					systim[idx] = HWREG(SYSTIM_BASE + SYSTIM_CH(idx));
					systim_delta = systim[idx];
					systim_delta -= systim_now << systim_offset[idx];

					if (systim_delta > MAX_SYSTIMER_DELTA) {
						systim_delta = 0;
					}

					systim_delta = systim_delta >> systim_offset[idx];
					systim_next = MIN(systim_next, systim_delta);
				}
			}
		} else {
			systim_next = MAX_SYSTIMER_DELTA;
		}

		rtc_mask = HWREG(RTC_BASE + RTC_O_IMASK);
		if (rtc_mask != 0) {
			rtc_now = HWREG(RTC_BASE + RTC_O_TIME8U);
			if (rtc_mask & 0x1) {
				rtc[0] = HWREG(RTC_BASE + RTC_O_CH0CC8U);
				rtc_delta = (rtc[0] - rtc_now) << 3;
				systim_next = MIN(systim_next, rtc_delta);
			}
			if (rtc_mask & 0x2) {
				rtc[1] = HWREG(RTC_BASE + RTC_O_CH1CC8U);
				rtc_delta = (rtc[1] - rtc_now) << 3;
				systim_next = MIN(systim_next, rtc_delta);
			}
		}

		if (systim_next > PowerCC23X0_TOTALTIMESTANDBY) {
			HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ3SEL) = 0;
			HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ16SEL) =
			      EVTSVT_CPUIRQ16SEL_PUBID_AON_RTC_COMB;
			HwiP_clearInterrupt(INT_CPUIRQ16);
			rtc_now = HWREG(RTC_BASE + RTC_O_TIME8U);
			HWREG(RTC_BASE + RTC_O_CH0CC8U) = RTC_NEXT(systim_next, rtc_now);
#endif /*CONFIG_CC23X0_RTC_TIMER*/
			Power_sleep(PowerLPF3_STANDBY);
#ifndef CONFIG_CC23X0_RTC_TIMER
			pm_cc23x0_systim_standby_restore();
		} else if (idle) {
			__WFI();
		}
	} else if (idle) {
		__WFI();
	}

	HwiP_restore(key);
#endif /*CONFIG_CC23X0_RTC_TIMER*/
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		PowerCC23X0_doWFI();
		break;
	case PM_STATE_STANDBY:
		pm_cc23x0_enter_standby();
		break;
	case PM_STATE_SOFT_OFF:
		Power_shutdown(0, 0);
		break;
	default:
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	HwiP_restore(0);
}

#endif /* CONFIG_PM */

static int power_initialize(void)
{
	Power_init();

	if (DT_HAS_COMPAT_STATUS_OKAY(ti_cc23x0_lf_xosc)) {
		PowerLPF3_selectLFXT();
	}

	PMCTLSetVoltageRegulator(PMCTL_VOLTAGE_REGULATOR_DCDC);

	return 0;
}

SYS_INIT(power_initialize, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
