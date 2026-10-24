/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <fsl_cmc.h>
#include <fsl_spc.h>

#define MCXW7_CMC_ADDR (CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc))
#define MCXW7_SPC_ADDR (SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc))

#if defined(CONFIG_SOC_MCXW7XX_POWEROFF_DEEP_POWER_DOWN)
#define MCXW7_POWEROFF_MODE kCMC_DeepPowerDown
/* RFMC RF2P4GHZ_CTRL[LP_MODE]: radio domain in Deep Power Down */
#define MCXW7_RADIO_LP_MODE 0xFU
#else
#define MCXW7_POWEROFF_MODE kCMC_PowerDownMode
/* RFMC RF2P4GHZ_CTRL[LP_MODE]: radio domain in Power Down */
#define MCXW7_RADIO_LP_MODE 0x7U
#endif

/* RF_CMC1 RAM_PWR: every radio RAM powered off in low-power modes */
#define MCXW7_RADIO_RAM_PWR_OFF 0x4FFU

void z_sys_poweroff(void)
{
	const cmc_power_domain_config_t cmc_config = {
		.clock_mode = kCMC_GateAllSystemClocksEnterLowPowerMode,
		.main_domain = MCXW7_POWEROFF_MODE,
		.wake_domain = MCXW7_POWEROFF_MODE,
	};

	/*
	 * Stop the radio CPU and put the radio domain in the same mode: the
	 * low-power regulator states only apply when the main and radio domains
	 * both enter it. The SoC resets on wakeup, so a running NBU does not need
	 * to be notified.
	 */
	RFMC->RF2P4GHZ_CTRL |= RFMC_RF2P4GHZ_CTRL_CPU_RST_MASK;
#if defined(CONFIG_SOC_MCXW7XX_POWEROFF_DEEP_POWER_DOWN)
	/* RF_CMC1 is only accessible while the radio domain runs. RFMC keeps
	 * the low-power request across a system off wakeup, so the radio domain
	 * can still be in low power (LP_ACK) with its RAMs as configured then.
	 */
	if ((RFMC->RF2P4GHZ_STAT & RFMC_RF2P4GHZ_STAT_LP_ACK_STAT_MASK) == 0U) {
		RF_CMC1->RAM_PWR = MCXW7_RADIO_RAM_PWR_OFF;
	}
#endif
	RFMC->RF2P4GHZ_CTRL = (RFMC->RF2P4GHZ_CTRL & ~RFMC_RF2P4GHZ_CTRL_LP_MODE_MASK) |
			      RFMC_RF2P4GHZ_CTRL_LP_MODE(MCXW7_RADIO_LP_MODE);
	RFMC->RF2P4GHZ_CTRL |= RFMC_RF2P4GHZ_CTRL_LP_ENTER_MASK;

	/* A non-zero value would send the boot ROM to its low-power wakeup path */
	SPC_SetWakeUpValue(MCXW7_SPC_ADDR, 0U);
	SPC_EnableLowPowerModeCoreVDDInternalVoltageScaling(MCXW7_SPC_ADDR, true);

	/* A debug connection must not keep the power domains up */
	CMC_EnableDebugOperation(MCXW7_CMC_ADDR, false);
	CMC_SetPowerModeProtection(MCXW7_CMC_ADDR, kCMC_AllowAllLowPowerModes);
	CMC_EnterLowPowerMode(MCXW7_CMC_ADDR, &cmc_config);

	CODE_UNREACHABLE;
}
