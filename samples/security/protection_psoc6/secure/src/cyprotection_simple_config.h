/*
 * Copyright (c) 2018, Cypress Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CYPROTECTION_CONFIG_H_
#define _CYPROTECTION_CONFIG_H_

#include "cyprotection.h"

/* Only 7 protection contexts are available in PSoC6-BLE2 */
#define ALL_PROTECTION_CONTEXTS_MASK (CY_PROT_PCMASK1 + CY_PROT_PCMASK2 +\
CY_PROT_PCMASK3 + CY_PROT_PCMASK4 + CY_PROT_PCMASK5 + CY_PROT_PCMASK6 +\
CY_PROT_PCMASK7)
#define SECURE_CONTEXTS_MASK (CY_PROT_PCMASK1 + CY_PROT_PCMASK2 +\
CY_PROT_PCMASK3 + CY_PROT_PCMASK4)

#define ALL_SUBREGIONS (0x0)

const cy_smpu_region_config_t flash_spm_smpu_config[] = {
	{
		.address = (u32_t *) CONFIG_FLASH_BASE_ADDRESS,
		.regionSize = CY_PROT_SIZE_512KB,
		.subregions = 0xC0, /* Only 384kB is used by CM0+ */
		.userPermission = CY_PROT_PERM_RX,
		.privPermission = CY_PROT_PERM_RWX,
		.secure = true,
		.pcMatch = false,
		.pcMask = SECURE_CONTEXTS_MASK,
		.prot_region = PROT_SMPU_SMPU_STRUCT7,
		.userMstPermission = CY_PROT_PERM_R,
		.privMstPermission = CY_PROT_PERM_R,
	}
};

const cy_smpu_region_config_t sram_spm_smpu_config[] = {
	/* SRAM_SPM_PRIV */
	{
		.address = (u32_t *) CONFIG_SRAM_BASE_ADDRESS,
		.regionSize = CY_PROT_SIZE_128KB,
		.subregions = ALL_SUBREGIONS,
		.userPermission = CY_PROT_PERM_DISABLED,
		.privPermission = CY_PROT_PERM_RWX,
		.secure = true,
		.pcMatch = false,
		.pcMask = SECURE_CONTEXTS_MASK,
		.prot_region = PROT_SMPU_SMPU_STRUCT12,
		.userMstPermission = CY_PROT_PERM_R,
		.privMstPermission = CY_PROT_PERM_R,
	}
};

const cy_ppu_fixed_rg_cfg_t fixed_rg_ppu_config[] = {
	{
		.userPermission = CY_PROT_PERM_DISABLED,
		.privPermission = CY_PROT_PERM_RW,
		.secure = true,
		.pcMatch = false,
		.pcMask = SECURE_CONTEXTS_MASK,
		.userMstPermission = CY_PROT_PERM_R,
		.privMstPermission = CY_PROT_PERM_R,
		.secureMst = false,
		.pcMstMask = ALL_PROTECTION_CONTEXTS_MASK,
		.pPpuStr = PERI_GR_PPU_RG_IPC_STRUCT0,
	},
	{
		.userPermission = CY_PROT_PERM_RW,
		.privPermission = CY_PROT_PERM_RW,
		.secure = false,
		.pcMatch = false,
		.pcMask = ALL_PROTECTION_CONTEXTS_MASK - CY_PROT_PCMASK7,
		.userMstPermission = CY_PROT_PERM_R,
		.privMstPermission = CY_PROT_PERM_R,
		.secureMst = false,
		.pcMstMask = ALL_PROTECTION_CONTEXTS_MASK,
		.pPpuStr = PERI_GR_PPU_RG_IPC_STRUCT1,
	}
};

const cy_ppu_fixed_sl_cfg_t fixed_sl_ppu_config[] = {
	{
		.userPermission = CY_PROT_PERM_DISABLED,
		.privPermission = CY_PROT_PERM_RW,
		.secure = true,
		.pcMatch = false,
		.pcMask = SECURE_CONTEXTS_MASK,
		.userMstPermission = CY_PROT_PERM_RW,
		.privMstPermission = CY_PROT_PERM_RW,
		.secureMst = false,
		.pcMstMask = ALL_PROTECTION_CONTEXTS_MASK,
		.pPpuStr = PERI_GR_PPU_SL_SCB2,
	}
};

const cy_ppu_prog_cfg_t prog_ppu_config[] = {
	{
		.address = (u32_t *)0x40010000,
		.regionSize = CY_PROT_SIZE_1KB,/* 0x00400 */
		.subregions = ALL_SUBREGIONS,
		.userPermission = CY_PROT_PERM_DISABLED,
		.privPermission = CY_PROT_PERM_RW,
		.secure = true,
		.pcMatch = false,
		.pcMask = SECURE_CONTEXTS_MASK,
		.userMstPermission = CY_PROT_PERM_R,
		.privMstPermission = CY_PROT_PERM_RW,
		.secureMst = true,
		.pcMstMask = SECURE_CONTEXTS_MASK,
		.pPpuStr = PERI_PPU_PR4,
	}
};

const cy_ppu_fixed_gr_cfg_t fixed_gr_ppu_config[] = {
	{
		.userPermission = CY_PROT_PERM_RW,
		.privPermission = CY_PROT_PERM_RW,
		.secure = false,
		.pcMatch = false,
		.pcMask = ALL_PROTECTION_CONTEXTS_MASK,
		.userMstPermission = CY_PROT_PERM_R,
		.privMstPermission = CY_PROT_PERM_RW,
		.secureMst = true,
		.pcMstMask = SECURE_CONTEXTS_MASK,
		.pPpuStr = PERI_PPU_GR0,
	}
};

const cy_bus_master_config_t bus_masters_config[] = {
	{
		.busMaster = CPUSS_MS_ID_CM4,
		.privileged = true,
		.secure = false,
		.pcMask = CY_PROT_PCMASK6,
		.act_pc = CY_PROT_PC6,
	},
	{
		.busMaster = CPUSS_MS_ID_TC,
		.privileged = false,
		.secure = false,
		.pcMask = CY_PROT_PCMASK1 + CY_PROT_PCMASK6,
		.act_pc = CY_PROT_PC1,
	},
	{
		.busMaster = CPUSS_MS_ID_CRYPTO,
		.privileged = true,
		.secure = true,
		.pcMask = SECURE_CONTEXTS_MASK,
		.act_pc = CY_PROT_PC1,
	},
	{
		.busMaster = CPUSS_MS_ID_CM0,
		.privileged = true,
		.secure = true,
		.pcMask = SECURE_CONTEXTS_MASK,
		.act_pc = CY_PROT_PC1,
	}
};

#endif /* _CYPROTECTION_CONFIG_H_ */
