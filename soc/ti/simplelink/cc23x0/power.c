/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 Baylibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/reboot.h>
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

static void pm_cc23x0_enter_standby(void);
static int power_initialize(void);
extern int_fast16_t PowerCC23X0_notify(uint_fast16_t eventType);

/* Max number of ClockP ticks into the future supported by this ClockP
 * implementation.
 * Under the hood, ClockP uses the SysTimer whose events trigger immediately if
 * the compare value is less than 2^22 systimer ticks in the past
 * (4.194sec at 1us resolution). Therefore, the max number of SysTimer ticks you
 * can schedule into the future is 2^32 - 2^22 - 1 ticks (~= 4290 sec at 1us
 * resolution).
 */
#define MAX_SYSTIMER_DELTA 0xFFBFFFFFU

/* The RTC has a time base of 8us and the SysTimer uses 1us or 250ns. The
 * conversion factor assumes that the SysTimer values have been converted to 1us
 * already.
 */
#define RTC_TO_SYSTIMER_TICKS 8U

#define SYSTIMER_CHANNEL_COUNT 5U

/* Global to stash the SysTimer timeouts while we enter standby */
static uint32_t sysTimerTimeouts[SYSTIMER_CHANNEL_COUNT];

/* Shift values to convert between the different resolutions of the SysTimer
 * channels. Channel 0 can technically support either 1us or 250ns. Until the
 * channel is actively used, we will hard-code it to 1us resolution to improve
 * runtime.
 */
static const uint8_t sysTimerResolutionShift[SYSTIMER_CHANNEL_COUNT] = {
    0, /* 1us */
    0, /* 1us */
    2, /* 250ns -> 1us */
    2, /* 250ns -> 1us */
    2  /* 250ns -> 1us */
};

/* This function contains the logic required to decide if we enter standby or WFI
 * It considers TI power constraints.
 * This function is based on the PowerCC23X0_standbyPolicy() function for nortos
 * from the TI SDK
 */
static void pm_cc23x0_enter_standby(void)
{
    uint32_t constraints;
    uint32_t sysTimerDelta;
    uint32_t sysTimerIMASK;
    uint32_t sysTimerARMSET;
    uint32_t sysTimerLoopDelta;
    uint32_t sysTimerCurrTime;
    uint8_t sysTimerIndex;
    uint32_t rtcCurrTime;
    uintptr_t key;
    bool standbyAllowed;
    bool idleAllowed;
    bool fltSettled;
    bool lfTick;

    key = HwiP_disable();

    /* Check state of constraints */
    constraints    = Power_getConstraintMask();
    standbyAllowed = (constraints & (1 << PowerLPF3_DISALLOW_STANDBY)) == 0;
    idleAllowed    = (constraints & (1 << PowerLPF3_DISALLOW_IDLE)) == 0;

    /* If we are using LFOSC, we need to wait for the LFINC filter to settle
     * before entering standby. We also cannot enter idle instead of standby
     * because otherwise we could end up waiting for the next standby wakeup
     * signal from the RTC or another wakeup source while we are still in idle.
     * That could be a very long time.
     * But if standby is currently disallowed from the constraints, that means
     * we do want to enter idle since something set that constraint and will
     * lift it again.
     * Additionally, it has been observed a brief period of ~15us occurring
     * ~130us after starting HFXT where FLTSETTLED pulses high. If the idle loop
     * attempts to enter standby while FLTSETTLED pulses high, it may enter
     * standby before the filter is truly settled. To prevent this, we need to
     * wait until the next LFTICK after HFXTGOOD is set before going to sleep.
     * This is achieved by checking against LFTICK before entering standby,
     * which is cleared once AMPSETTLED is set (which occurs after HFXTGOOD is
     * set).
     */
    if ((HWREG(CKMD_BASE + CKMD_O_LFCLKSEL) & CKMD_LFCLKSEL_MAIN_M) == CKMD_LFCLKSEL_MAIN_LFOSC)
    {
        if (standbyAllowed)
        {
            fltSettled = ((HWREG(CKMD_BASE + CKMD_O_LFCLKSTAT) & CKMD_LFCLKSTAT_FLTSETTLED_M) ==
                           CKMD_LFCLKSTAT_FLTSETTLED);
            lfTick     = ((HWREG(CKMD_BASE + CKMD_O_RIS) & CKMD_RIS_LFTICK_M) == CKMD_RIS_LFTICK);
            if (!fltSettled || !lfTick)
            {
                standbyAllowed = false;
                idleAllowed    = false;
            }
        }
    }

    /* Do quick check to see if only WFI allowed; if yes, do it now. */
    if (standbyAllowed)
    {
        /* If we are allowed to enter standby, check whether the next timeout is
         * far enough away for it to make sense.
         */

        /* Save SysTimer IMASK to restore afterwards */
        sysTimerIMASK = HWREG(SYSTIM_BASE + SYSTIM_O_IMASK);

        /* Get current armed status amongst all SysTimer channels */
        sysTimerARMSET = HWREG(SYSTIM_BASE + SYSTIM_O_ARMSET);

        /* Get current time in 1us resolution */
        sysTimerCurrTime = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);

        /* Get current time in 8us resolution */
        rtcCurrTime = HWREG(RTC_BASE + RTC_O_TIME8U);

        /* We only want to check the SysTimer channels if at least one of them
         * is active. It may be that no one is using ClockP or RCL in this
         * application or they have not been initialised yet.
         */
        if (sysTimerIMASK != 0)
        {
            /* Set initial SysTimer delta to max possible value. It needs to be
             * this large since we will shrink it down to the soonest timeout with
             * Math_MIN() comparisons
             */
            sysTimerDelta = 0xFFFFFFFF;

            /* Loop over all SysTimer channels and compute the soonest timeout.
             * Since the channels have different time bases (1us vs 250ns),
             * we need to shift all of that to a 1us time base to compare them.
             * If no channel is active, we will use the max timeout value
             * supported by the SysTimer.
             */
            for (sysTimerIndex = 0; sysTimerIndex < SYSTIMER_CHANNEL_COUNT; sysTimerIndex++)
            {
                if (sysTimerIMASK & (1 << sysTimerIndex))
                {
                    /* Stash SysTimer channel compare value. Read CHnCCSR to
                     * avoid clearing any pending events as side effect of
                     * reading CHnCC.
                     */
                    sysTimerTimeouts[sysTimerIndex] = HWREG(SYSTIM_BASE + SYSTIM_O_CH0CCSR + sysTimerIndex * sizeof(uint32_t));

                    /* Store current channel timeout in native channel
                     * resolution
                     */
                    sysTimerLoopDelta = sysTimerTimeouts[sysTimerIndex];

                    /* Convert current time from 1us to native resolution and
                     * subtract from timeout to get delta in in native channel
                     * resolution.
                     * We compute the delta in the native resolution
                     * to correctly handle wrapping and underflow at the 32-bit
                     * boundary.
                     * To simplify code paths and SRAM, we shift up the 1us
                     * resolution time stamp instead of reading out and keeping
                     * track of the 250ns time stamp and associating that with
                     * channels 2 to 4. The loss of resolution for wakeup is not
                     * material as we wake up sufficiently early to handle
                     * timing jitter in the wakeup duration.
                     */
                    sysTimerLoopDelta -= sysTimerCurrTime << sysTimerResolutionShift[sysTimerIndex];

                    /* If sysTimerDelta is larger than MAX_SYSTIMER_DELTA, the
                     * compare event happened in the past and we need to abort
                     * entering standby to handle the timeout instead of waiting
                     * a really long time.
                     */
                    if (sysTimerLoopDelta > MAX_SYSTIMER_DELTA)
                    {
                        sysTimerLoopDelta = 0;
                    }

                    /* Convert delta to 1us resolution */
                    sysTimerLoopDelta = sysTimerLoopDelta >> sysTimerResolutionShift[sysTimerIndex];

                    /* Update the smallest SysTimer delta */
                    sysTimerDelta = Math_MIN(sysTimerDelta, sysTimerLoopDelta);
                }
            }
        }
        else
        {
            /* None of the SysTimer channels are active. Use the maximum
             * SysTimer delta instead. That lets us sleep for at least this
             * long if the OS timeout is even longer.
             */
            sysTimerDelta = MAX_SYSTIMER_DELTA;
        }

        /* Check sysTimerDelta time vs STANDBY latency */
        if (sysTimerDelta > PowerCC23X0_TOTALTIMESTANDBY)
        {
            /* Switch EVTSVT_O_CPUIRQ16SEL in eventfabric to RTC
             * Since the CC23X0 only has limited interrupt lines, we need
             * to switch the interrupt line from SysTimer to RTC in the
             * event fabric.
             * The triggered interrupt will wake up the device with
             * interrupts disabled. We can consume that interrupt event
             * without vectoring to the ISR and then change the event fabric
             * signal back to the SysTimer.
             * Thus, there is no need to swap out the actual interrupt
             * function of the clockHwi.
             */
            HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ16SEL) = EVTSVT_CPUIRQ16SEL_PUBID_AON_RTC_COMB;

            /* Clear interrupt in case it triggered since we disabled interrupts */
            HwiP_clearInterrupt(INT_CPUIRQ16);

            /* Ensure the device wakes up early enough to reinitialise the
             * HW and take care of housekeeping.
             */
            sysTimerDelta -= PowerCC23X0_WAKEDELAYSTANDBY;

            /* The SysTimer has a time base of 1us while the RTC uses 8us.
             * Divide by 8 to convert from SysTimer to RTC time base
             */
            sysTimerDelta /= RTC_TO_SYSTIMER_TICKS;

            /* RTC channel 0 compare is automatically armed upon writing the
             * compare value. It will automatically be disarmed when it
             * triggers.
             */
            HWREG(RTC_BASE + RTC_O_CH0CC8U) = rtcCurrTime + sysTimerDelta;

			Power_sleep(PowerLPF3_STANDBY);

            /* Disarm RTC compare event in case we woke up from a GPIO or BATMON
             * event. If the RTC times out after clearing RIS and the pending
             * NVIC bit but before we swap event fabric subscribers for the
             * shared interrupt line, we will be left with a pending interrupt
             * in the NVIC that the ClockP callback may not gracefully handle
             * since it did not cause it itself.
             */
            HWREG(RTC_BASE + RTC_O_ARMCLR) = RTC_ARMCLR_CH0_CLR;

            /* Clear the RTC wakeup event */
            HWREG(RTC_BASE + RTC_O_ICLR) = RTC_ICLR_EV0_CLR;

            /* Explicitly read back from ULL domain to guarantee clearing RIS
             * takes effect before clearing the pending NVIC interrupt to avoid
             * the NVIC re-asserting on a set RIS.
             */
            ULLSync();

            /* Clear any pending interrupt in the NVIC */
            HwiP_clearInterrupt(INT_CPUIRQ16);

            /* Switch EVTSVT_O_CPUIRQ16SEL in eventfabric back to SysTimer */
            HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ16SEL) = EVTSVT_CPUIRQ16SEL_PUBID_SYSTIM0;

            /* When waking up from standby, the SysTimer may not have
             * synchronised with the RTC by now. Wait for SysTimer
             * synchronisation with the RTC to complete. This should not take
             * more than one LFCLK period.
             *
             * We need to wait both for RUN to be set and SYNCUP to go low. Any
             * other register state will cause undefined behaviour.
             */
            while (HWREG(SYSTIM_BASE + SYSTIM_O_STATUS) != SYSTIM_STATUS_VAL_RUN) {}

            /* Restore SysTimer timeouts */
            memcpy((void *)(SYSTIM_BASE + SYSTIM_O_CH0CCSR), sysTimerTimeouts, sizeof(sysTimerTimeouts));

            /* Restore SysTimer armed state. This will rearm all previously
             * armed timeouts restored above and cause any that occurred in the
             * past to trigger immediately.
             */
            HWREG(SYSTIM_BASE + SYSTIM_O_ARMSET) = sysTimerARMSET;

            /* Restore SysTimer IMASK */
            HWREG(SYSTIM_BASE + SYSTIM_O_IMASK) = sysTimerIMASK;

            /* Re-configure LRFD clocks */
            LRFDApplyClockDependencies();

            /* Signal clients registered for standby wakeup notification;
             * this should be used to initialize any timing critical or IO
             * dependent hardware.
             * The callback needs to go out after the SysTimer is restored
             * such that notifications can invoke RCL and ClockP APIs if needed.
             */
            PowerCC23X0_notify(PowerLPF3_AWAKE_STANDBY);
            HwiP_restore(key);
		}
        else if (idleAllowed)
        {
            /* If we would be allowed to enter standby but there is not enough
             * time for it to make sense from an overhead perspective, enter idle
             * instead.
             */
            __WFI();
        }
    }
    else if (idleAllowed)
    {
        /* We are not allowed to enter standby.
         * Enter idle instead if it is allowed.
         */
        __WFI();
    }

    HwiP_restore(key);
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

#ifdef CONFIG_REBOOT

void sys_arch_reboot(int type)
{
	switch(type)
	{
		case SYS_REBOOT_WARM:
			Power_reset();
			break;
		case SYS_REBOOT_COLD:
			break;
	}
}

#endif /* CONFIG_REBOOT */

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
