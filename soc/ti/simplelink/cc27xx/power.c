/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * Copyright (c) 2024 Baylibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

/* Driver header files */
#include <ti/drivers/Power.h>

/* Utilities header files */
#include <ti/drivers/utils/Math.h>

/* DPL header files */
#include <ti/drivers/dpl/HwiP.h>

/* Driverlib header files */
#include <ti/devices/DeviceFamily.h>
/* clang-format off */
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_systim.h)
#include DeviceFamily_constructPath(inc/hw_rtc.h)
/* clang-format on */
#include <cmsis_compiler.h>
/* clang-format off */
#include DeviceFamily_constructPath(driverlib/systick.h)
#include DeviceFamily_constructPath(driverlib/ckmd.h)
/* clang-format on */

/* The range of pins available on this device */
const uint_least8_t GPIO_pinLowerBound;
const uint_least8_t GPIO_pinUpperBound = 28;

#ifdef CONFIG_PM

static void pm_cc27xx_enter_standby(void);
static int power_initialize(void);
extern int_fast16_t PowerCC27XX_notify(uint_fast16_t eventType);

/* Max number of ClockP ticks into the future supported by this ClockP
 * implementation.
 * Under the hood, ClockP uses the SysTimer whose events trigger immediately if
 * the compare value is less than 2^22 systimer ticks in the past
 * (4.194sec at 1us resolution). Therefore, the max number of SysTimer ticks you
 * can schedule into the future is 2^32 - 2^22 - 1 ticks (~= 4290 sec at 1us
 * resolution).
 */
#define MAX_SYSTIMER_DELTA (0xFFBFFFFFU)

/*
 * The RTC timer uses a compare register to generate RTC compare events.
 * The CH0CC8U register is being used to configure the compare time.
 * The CH0CC8U register is a 32-bit register with 8us resolution.
 * The maximum compare event which can be set is:
 * ( ( (2^32) - 1 ) * 8) us = 34359.74 s = 9.54 hours.
 * Since a compare event is generated if the TIME8U register value is
 * one second behind the CH0CC8U register, we have to subtract one second
 * from the maximum value giving a maximum time of 34358.74 s.
 *
 * Maximum delta in microseconds terms is:
 * 8*( (2^32) - 1 ) - (10^6) = 34358738360.
 *
 */
#define MAX_RTC_DELTA_1US (0x7FFF0BDB8ULL)

#define SYSTIMER_CHANNEL_COUNT (5U)

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
 * This function is based on the PowerCC27XX_standbyPolicy() function for nortos
 * from the TI SDK
 */
void pm_cc27xx_enter_standby(void)
{
	uint32_t constraints;
	uint32_t sysTimerDelta;
	uint32_t soonestDelta;
	uint64_t rtcDelta1Us;
	uint32_t rtcTIME8U;
	uint32_t rtcCH0CC8U;
	uint32_t sysTimerIMASK;
	uint32_t sysTimerLoopDelta;
	uint32_t sysTimerCurrTime;
	uint8_t sysTimerIndex;
	uintptr_t key;
	bool standbyAllowed;
	bool idleAllowed;
	bool sysTickEnabled;

	key = __get_PRIMASK();
	__set_PRIMASK(1);

	/* Check state of constraints */
	constraints = Power_getConstraintMask();
	standbyAllowed = (constraints & (1U << PowerLPF3_DISALLOW_STANDBY)) == 0U;
	idleAllowed = (constraints & (1U << PowerLPF3_DISALLOW_IDLE)) == 0U;

	if (standbyAllowed && (PowerLPF3_isLfincFilterAllowingStandby() == false)) {
		/* We cannot enter standby until LFINC filter has settled, we also
		 * cannot enter idle instead of standby because otherwise we could end
		 * up waiting for the next standby wakeup signal from SysTimer or
		 * another wakeup source while we are still in idle. That could be a
		 * very long time. But if standby is currently disallowed from the
		 * constraints, that means we do want to enter idle since something set
		 * that constraint and will lift it again.
		 */
		standbyAllowed = false;
		idleAllowed = false;
	}

	/* Do quick check to see if only WFI allowed; if yes, do it now. */
	if (standbyAllowed) {
		/* If we are allowed to enter standby, check whether the next timeout is
		 * far enough away for it to make sense.
		 */

		/* Get SysTimer IMASK state */
		sysTimerIMASK = HWREG(SYSTIM_BASE + SYSTIM_O_IMASK);

		/* Get current time in 1us resolution */
		sysTimerCurrTime = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);

		rtcTIME8U = HWREG(RTC_BASE + RTC_O_TIME8U);
		rtcCH0CC8U = HWREG(RTC_BASE + RTC_O_CH0CC8U);

		/* We only want to check the SysTimer channels if at least one of them
		 * is active. It may be that no one is using ClockP or RCL in this
		 * application or they have not been initialised yet.
		 */
		if (sysTimerIMASK != 0) {
			/* Set initial SysTimer delta to max possible value. It needs to be
			 * this large since we will shrink it down to the soonest timeout with
			 * Math_MIN() comparisons.
			 */
			sysTimerDelta = 0xFFFFFFFF;

			/* Loop over all SysTimer channels and compute the soonest timeout.
			 * Since the channels have different time bases (1us vs 250ns),
			 * we need to shift all of that to a 1us time base to compare them.
			 * If no channel is active, we will use the max timeout value
			 * supported by the SysTimer.
			 */
			for (sysTimerIndex = 0; sysTimerIndex < SYSTIMER_CHANNEL_COUNT;
			     sysTimerIndex++) {
				if (sysTimerIMASK & (1 << sysTimerIndex)) {
					/* Store current channel timeout in native channel
					 * resolution. Read CHnCCSR to avoid clearing any pending
					 * events as side effect of reading CHnCC.
					 */
					sysTimerLoopDelta =
						HWREG(SYSTIM_BASE + SYSTIM_O_CH0CCSR +
						      (sysTimerIndex * sizeof(uint32_t)));

					/* Convert current time from 1us to native resolution and
					 * subtract from timeout to get delta in native channel
					 * resolution.
					 * We compute the delta in the native resolution
					 * to correctly handle wrapping and underflow at the 32-bit
					 * boundary.
					 * To simplify code paths and SRAM, we shift up the 1us
					 * resolution time stamp instead of reading out and keeping
					 * track of the 250ns time stamp and associating that with
					 * 250ns channels. The loss of resolution for wakeup is not
					 * material as we wake up sufficiently early to handle
					 * timing jitter in the wakeup duration.
					 */
					sysTimerLoopDelta -=
						sysTimerCurrTime
						<< sysTimerResolutionShift[sysTimerIndex];

					/* If sysTimerDelta is larger than MAX_SYSTIMER_DELTA, the
					 * compare event happened in the past and we need to abort
					 * entering standby to handle the timeout instead of waiting
					 * a really long time.
					 */
					if (sysTimerLoopDelta > MAX_SYSTIMER_DELTA) {
						sysTimerLoopDelta = 0;
					}

					/* Convert delta to 1us resolution */
					sysTimerLoopDelta = sysTimerLoopDelta >>
							    sysTimerResolutionShift[sysTimerIndex];

					/* Update the smallest SysTimer delta */
					sysTimerDelta = Math_MIN(sysTimerDelta, sysTimerLoopDelta);
				}
			}
		} else {
			/* None of the SysTimer channels are active. Use the maximum
			 * SysTimer delta instead. That lets us sleep for at least this
			 * long if the OS timeout is even longer.
			 */
			sysTimerDelta = MAX_SYSTIMER_DELTA;
		}

		/* Calculate pending time to RTC compare event. */

		if (HWREG(RTC_BASE + RTC_O_IMASK) & RTC_ARMSET_CH0_SET) {

			rtcDelta1Us = (((uint64_t)(rtcCH0CC8U - rtcTIME8U) * 8ULL)) - 32ULL;

			/* If the RTC delta is more than the maximum delta, the compare event
			 * happened in the past and we need to abort to avoid being in sleep for
			 * a very long time.
			 */

			if (rtcDelta1Us > (uint64_t)MAX_RTC_DELTA_1US) {
				rtcDelta1Us = 0;
			}
		} else {
			rtcDelta1Us = MAX_RTC_DELTA_1US;
		}

		soonestDelta = (uint32_t)Math_MIN(((uint64_t)sysTimerDelta), rtcDelta1Us);

		/* Check sysTimerDelta time vs STANDBY latency */
		if (soonestDelta > PowerCC27XX_TOTALTIMESTANDBY) {
			/* Store SysTick enabled state */
			sysTickEnabled = ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) != 0);

			/* Go to standby mode */
			PowerLPF3_sleep(soonestDelta + sysTimerCurrTime);

			/* If PowerLPF3_sleep() disabled SysTick, it must be re-enabled if
			 * it was enabled before calling PowerLPF3_sleep().
			 */
			if (((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) == 0) && sysTickEnabled) {
				SysTickEnable();
			}
		} else if (idleAllowed) {
			/* If we would be allowed to enter standby but there is not enough
			 * time for it to make sense from an overhead perspective, enter
			 * idle instead.
			 */

			PowerCC27XX_doWFI();
		}
	} else if (idleAllowed) {
		/* We are not allowed to enter standby.
		 * Enter idle instead if it is allowed.
		 */

		PowerCC27XX_doWFI();
	}

	__set_PRIMASK(key);
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Zephyr kernel calls pm_system_suspend() with interrupts disabled using arch_irq_lock().
	 * This sets the BASEPRI to _EXC_IRQ_DEFAULT_PRIO. It is the callee's responsibility to
	 * call irq_unlock() after waking up from the pm state. However, both idle and standby
	 * relies on the WFI instruction, which according to the ARM documentation suspends
	 * execution until one of the following events occurs:
	 *  1. A non-masked interrupt occurs and is taken.
	 *  2. An interrupt masked by PRIMASK becomes pending.
	 *  3. A Debug Entry request, if Debug is enabled.
	 *
	 * See
	 * https://developer.arm.com/documentation/100235/0100/The-Cortex-M33-Instruction-Set/Miscellaneous-instructions/WFI?lang=en
	 *
	 * If PRIMASK is not set, then the CPU would only wake up due to a debug entry request (3)
	 * which will only happen during debugging, or if an interrupt which is not masked by
	 * BASEPRI occurs. We want all enabled interrupts to be able to wake up the CPU, so they
	 * need to be masked by PRIMASK (2)
	 *
	 * So we set the PRIMASK here, call `irq_unlock()` immediately, and clear PRIMASK in
	 * `pm_state_exit_post_ops().`
	 */
	__set_PRIMASK(1);
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		PowerCC27XX_doWFI();
		break;
	case PM_STATE_STANDBY:
		pm_cc27xx_enter_standby();
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

	__set_PRIMASK(0);
}

#ifdef CONFIG_REBOOT

FUNC_NORETURN void sys_arch_reboot(int type)
{
	switch (type) {
	case SYS_REBOOT_WARM:
		Power_reset();
		break;
	case SYS_REBOOT_COLD:
		break;
	}
}

#endif /* CONFIG_REBOOT */

#endif /* CONFIG_PM */

static int power_initialize(void)
{
	unsigned int ret;

	ret = irq_lock();

	/* Set non-default cap array trims */
	CKMDSetInitialCapTrim(33, 33);
	CKMDSetTargetCapTrim(33, 33);

	Power_init();

	if (DT_HAS_COMPAT_STATUS_OKAY(ti_cc27xx_lf_xosc)) {
		PowerLPF3_selectLFXT();
	}

	PMCTLSetVoltageRegulator(PMCTL_VOLTAGE_REGULATOR_DCDC);

	/* HFXT temperature compensation will be performed as long as the measured temperature
	 * exceeds -50 degrees Celsius, and drifts 2 degrees or more.
	 */
	PowerLPF3_enableHFXTCompensation(-50, 2);

#ifdef CONFIG_GPIO
	/* Enable pad power to use GPIOs by setting VDDIOPGIO. This is only done for
	 * CC27XX to support split rails.
	 */
	PMCTLEnableVddioGpioPadPower();

#endif

	irq_unlock(ret);

	return 0;
}

SYS_INIT(power_initialize, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
