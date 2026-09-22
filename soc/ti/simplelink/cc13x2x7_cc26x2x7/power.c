/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include <driverlib/pwr_ctrl.h>
#include <driverlib/sys_ctrl.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#include <ti/drivers/dpl/ClockP.h>

#include <ti/devices/cc13x2x7_cc26x2x7/driverlib/cpu.h>
#include <ti/devices/cc13x2x7_cc26x2x7/driverlib/vims.h>
#include <ti/devices/cc13x2x7_cc26x2x7/driverlib/sys_ctrl.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

const PowerCC26X2_Config PowerCC26X2_config = {
#if defined(CONFIG_IEEE802154_CC13XX_CC26XX) \
	|| defined(CONFIG_BLE_CC13XX_CC26XX) \
	|| defined(CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ)
	.policyInitFxn      = NULL,
	.policyFxn          = NULL,
	.calibrateFxn       = &PowerCC26XX_calibrate,
	.enablePolicy       = false,
	.calibrateRCOSC_LF  = true,
	.calibrateRCOSC_HF  = true
#else
/* Configuring TI Power module to not use its policy function (we use Zephyr's
 * instead), and disable oscillator calibration functionality for now.
 */
	.policyInitFxn      = NULL,
	.policyFxn          = NULL,
	.calibrateFxn       = NULL,
	.enablePolicy       = false,
	.calibrateRCOSC_LF  = false,
	.calibrateRCOSC_HF  = false
#endif
};

extern PowerCC26X2_ModuleState PowerCC26X2_module;

#ifdef CONFIG_PM
/*
 * Power state mapping:
 * PM_STATE_SUSPEND_TO_IDLE: Idle
 * PM_STATE_STANDBY: Standby
 * PM_STATE_SUSPEND_TO_RAM | PM_STATE_SUSPEND_TO_DISK: Shutdown
 */

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	uint32_t modeVIMS;
	uint32_t constraints;

	LOG_DBG("SoC entering power state %d", state);

	/* Switch to using PRIMASK instead of BASEPRI register, since
	 * we are only able to wake up from standby while using PRIMASK.
	 */
	/* Set PRIMASK */
	CPUcpsid();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		/* query the declared constraints */
		constraints = Power_getConstraintMask();
		/* 1. Get the current VIMS mode */
		do {
			modeVIMS = VIMSModeGet(VIMS_BASE);
		} while (modeVIMS == VIMS_MODE_CHANGING);

		/* 2. Configure flash to remain on in IDLE or not and keep
		 *    VIMS powered on if it is configured as GPRAM
		 * 3. Always keep cache retention ON in IDLE
		 * 4. Turn off the CPU power domain
		 * 5. Ensure any possible outstanding AON writes complete
		 * 6. Enter IDLE
		 */
		if ((constraints & (1 << PowerCC26XX_NEED_FLASH_IN_IDLE)) ||
			(modeVIMS == VIMS_MODE_DISABLED)) {
			SysCtrlIdle(VIMS_ON_BUS_ON_MODE);
		} else {
			SysCtrlIdle(VIMS_ON_CPU_ON_MODE);
		}

		/* 7. Make sure MCU and AON are in sync after wakeup */
		SysCtrlAonUpdate();
		break;

	case PM_STATE_STANDBY:
		/* go to standby mode */
		Power_sleep(PowerCC26XX_STANDBY);
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_DISK:
		__fallthrough;
	case PM_STATE_SOFT_OFF:
		Power_shutdown(0, 0);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	LOG_DBG("SoC leaving power state %d", state);
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	/*
	 * System is now in active mode. Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	CPUcpsie();
}
#endif /* CONFIG_PM */

/* Initialize TI Power module */
static int power_initialize(void)
{
	unsigned int ret;


	ret = irq_lock();
	Power_init();
	irq_unlock(ret);

	return 0;
}

/*
 * Unlatch IO pins after waking up from shutdown
 * This needs to be called during POST_KERNEL in order for "Booting Zephyr"
 * message to show up
 */
static int unlatch_pins(void)
{
	/* Get the reason for reset. */
	uint32_t rSrc = SysCtrlResetSourceGet();

	if (rSrc == RSTSRC_WAKEUP_FROM_SHUTDOWN) {
		PowerCtrlPadSleepDisable();
	}

	return 0;
}

/*
 *  ======== PowerCC26XX_schedulerDisable ========
 */
void PowerCC26XX_schedulerDisable(void)
{
	/*
	 * We are leaving this empty because Zephyr's
	 * scheduler would not get to run with interrupts being disabled
	 * in the context of Power_sleep() in any case.
	 */
}

/*
 *  ======== PowerCC26XX_schedulerRestore ========
 */
void PowerCC26XX_schedulerRestore(void)
{
	/*
	 * We are leaving this empty because Zephyr's
	 * scheduler would not get to run with interrupts being disabled
	 * in the context of Power_sleep() in any case.
	 */
}

SYS_INIT(power_initialize, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
SYS_INIT(unlatch_pins, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
