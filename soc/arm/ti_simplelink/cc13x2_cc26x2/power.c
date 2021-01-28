/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <init.h>
#include <power/power.h>

#include <driverlib/pwr_ctrl.h>
#include <driverlib/sys_ctrl.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#include <ti/drivers/dpl/ClockP.h>

#include <ti/devices/cc13x2_cc26x2/driverlib/cpu.h>
#include <ti/devices/cc13x2_cc26x2/driverlib/vims.h>
#include <ti/devices/cc13x2_cc26x2/driverlib/sys_ctrl.h>

#include <logging/log.h>
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

/*
 * Power state mapping:
 * PM_STATE_SUSPEND_TO_IDLE: Idle
 * PM_STATE_STANDBY: Standby
 * PM_STATE_SUSPEND_TO_RAM | PM_STATE_SUSPEND_TO_DISK: Shutdown
 */

/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(struct pm_state_info info)
{
	uint32_t modeVIMS;
	uint32_t constraints;

	LOG_DBG("SoC entering power state %d", info.state);

	/* Switch to using PRIMASK instead of BASEPRI register, since
	 * we are only able to wake up from standby while using PRIMASK.
	 */
	/* Set PRIMASK */
	CPUcpsid();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (info.state) {
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
		/* schedule the wakeup event */
		ClockP_start(ClockP_handle((ClockP_Struct *)
			&PowerCC26X2_module.clockObj));

		/* go to standby mode */
		Power_sleep(PowerCC26XX_STANDBY);
		ClockP_stop(ClockP_handle((ClockP_Struct *)
			&PowerCC26X2_module.clockObj));
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_DISK:
		__fallthrough;
	case PM_STATE_SOFT_OFF:
		Power_shutdown(0, 0);
		break;
	default:
		LOG_DBG("Unsupported power state %u", info.state);
		break;
	}

	LOG_DBG("SoC leaving power state %d", info.state);
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	/*
	 * System is now in active mode. Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	CPUcpsie();
}

/* Initialize TI Power module */
static int power_initialize(const struct device *dev)
{
	unsigned int ret;

	ARG_UNUSED(dev);

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
static int unlatch_pins(const struct device *dev)
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
