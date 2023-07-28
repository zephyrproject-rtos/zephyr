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

void z_sys_reboot(enum sys_reboot_mode mode)
{
	Power_Ip_MC_RGM_ConfigType mc_rgm_cfg = { 0 };

	const Power_Ip_HwIPsConfigType power_cfg = {
		.McRgmConfigPtr = (const Power_Ip_MC_RGM_ConfigType *)&mc_rgm_cfg,
		.PMCConfigPtr = NULL
	};

	switch (mode) {
	case SYS_REBOOT_COLD:
		/* Destructive reset */
		mc_rgm_cfg.ResetType = MCU_DEST_RESET;
		Power_Ip_PerformReset(&power_cfg);
		break;
	default:
		/* Functional reset */
		mc_rgm_cfg.ResetType = MCU_FUNC_RESET;
		Power_Ip_PerformReset(&power_cfg);
		break;
	}

	for (;;) {
		/* wait for reset */
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
#ifdef CONFIG_SOC_PART_NUMBER_S32K3
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

	return 0;
}

SYS_INIT(nxp_s32_power_init, PRE_KERNEL_1, 1);
