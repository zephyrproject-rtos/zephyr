/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#ifdef CONFIG_REBOOT
#include <zephyr/sys/reboot.h>
#endif

#include <Power_Ip.h>

#ifdef CONFIG_REBOOT
BUILD_ASSERT(POWER_IP_PERFORM_RESET_API == STD_ON, "Power Reset API must be enabled");

/*
 * Overrides default weak implementation of system reboot.
 *
 * SYS_REBOOT_COLD (Destructive Reset):
 * - Leads most parts of the chip, except a few modules, to reset. SRAM content
 *   is lost after this reset event.
 * - Flash is always reset, so an updated value of the option bits is reloaded
 *   in volatile registers outside of the Flash array.
 * - Trimming is lost.
 * - STCU is reset and configured BISTs are executed.
 *
 * SYS_REBOOT_WARM (Functional Reset):
 * - Leads all the communication peripherals and cores to reset. The communication
 *   protocols' sanity is not guaranteed and they are assumed to be reinitialized
 *   after reset. The SRAM content, and the functionality of certain modules, is
 *   preserved across functional reset.
 * - The volatile registers are not reset; in case of a reset event, the
 *   trimming is maintained.
 * - No BISTs are executed after functional reset.
 */
void sys_arch_reboot(int type)
{
	Power_Ip_MC_RGM_ConfigType mc_rgm_cfg = { 0 };

	const Power_Ip_HwIPsConfigType power_cfg = {
		.McRgmConfigPtr = (const Power_Ip_MC_RGM_ConfigType *)&mc_rgm_cfg,
		.PMCConfigPtr = NULL
	};

	switch (type) {
	case SYS_REBOOT_COLD:
		/* Destructive reset */
		mc_rgm_cfg.ResetType = MCU_DEST_RESET;
		Power_Ip_PerformReset(&power_cfg);
		break;
	case SYS_REBOOT_WARM:
		/* Functional reset */
		mc_rgm_cfg.ResetType = MCU_FUNC_RESET;
		Power_Ip_PerformReset(&power_cfg);
		break;
	default:
		/* Do nothing */
		break;
	}
}
#endif /* CONFIG_REBOOT */

static int nxp_s32_power_init(void)
{
	const Power_Ip_MC_RGM_ConfigType mc_rgm_cfg = {
		.FuncResetOpt = 0U, /* All functional reset sources enabled */
		.FesThresholdReset = MC_RGM_FRET_FRET(CONFIG_NXP_S32_FUNC_RESET_THRESHOLD),
		.DesThresholdReset = MC_RGM_DRET_DRET(CONFIG_NXP_S32_DEST_RESET_THRESHOLD)
	};

	const Power_Ip_PMC_ConfigType pmc_cfg = {
#ifdef CONFIG_SOC_SERIES_S32K3
		/* PMC Configuration Register (CONFIG) */
		.ConfigRegister = PMC_CONFIG_LMEN(IS_ENABLED(CONFIG_NXP_S32_PMC_LMEN))
			| PMC_CONFIG_LMBCTLEN(IS_ENABLED(CONFIG_NXP_S32_PMC_LMBCTLEN)),
#else
#error "SoC not supported"
#endif
	};

	const Power_Ip_HwIPsConfigType power_cfg = {
		.McRgmConfigPtr = &mc_rgm_cfg,
		.PMCConfigPtr = &pmc_cfg
	};

	Power_Ip_Init(&power_cfg);

	/* Read and clear the reset reason to avoid persisting it across resets */
	(void)Power_Ip_GetResetReason();

	return 0;
}

SYS_INIT(nxp_s32_power_init, PRE_KERNEL_1, 1);
