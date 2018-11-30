/*************************************************************************//**
 * \file cyprotection.h
 * \version 2.10
 *
 * \brief
 *  This file provides the API for SMPU, PPU use.
 *
 ******************************************************************************
 * \copyright
 * Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0



 *****************************************************************************/

#ifndef _CYPROTECTION_H_
#define _CYPROTECTION_H_

#include "cy_prot.h"

#define SMPU_STRUCT_NUMB (16u)

#define MPU_SMPU_SUBREGIONS_NUMB (8u)
/* MPU is not used yet */
#define CPUSS_PROT_MPU_STRUCT_NR (0u)

#define CPUSS_PROT_PPU_GR_STRUCT_NR (16u)
#define CPUSS_PROT_PPU_PROG_STRUCT_NR (16u)
#define CPUSS_PROT_PPU_FX_SL_STRUCT_NR (16u)
#define CPUSS_PROT_PPU_FX_RG_STRUCT_NR (29u)

#define CPUSS_PROT_PPU_FX_RG_START_ADDR (0x40201000UL)

#define PRIVILEGED_MODE (1u)
#define UNPRIVILEGED_MODE (0u)
#define NONSECURE_MODE (1u)
#define SECURE_MODE (0u)

/* PPU Group existing bitmask - 11001011111 */
#define PERI_PPU_GR_MMIO_EXIST_BITMASK 0x65F
/* PPU MMIO1 Group Fixed Region existing bitmask - 10 */
#define PERI_PPU_GR_MMIO1_EXIST_BITMASK 0x2
/* PPU MMIO2 Group Fixed Region existing bitmask - 11001111111111 */
#define PERI_PPU_GR_MMIO2_EXIST_BITMASK 0x33FF
/* PPU MMIO3 Group Fixed Region existing bitmask - 1111101111111 */
#define PERI_PPU_GR_MMIO3_EXIST_BITMASK 0x1F7F
/* PPU MMIO4 Group Fixed Region existing bitmask - 101 */
#define PERI_PPU_GR_MMIO4_EXIST_BITMASK 0x5
/* PPU MMIO6 Group Fixed Region existing bitmask - 1111111111 */
#define PERI_PPU_GR_MMIO6_EXIST_BITMASK 0x3FF
/* PPU MMIO9 Group Fixed Region existing bitmask - 11 */
#define PERI_PPU_GR_MMIO9_EXIST_BITMASK 0x3
/* PPU MMIO10 Group Fixed Region existing bitmask - 111 */
#define PERI_PPU_GR_MMIO10_EXIST_BITMASK 0x7


/* TODO: There is no SWPU configuration part */
typedef struct {
	/**< Base address of the memory region (Only applicable to slave) */
	u32_t *address;
	/**< Size of the memory region (Only applicable to slave) */
	cy_en_prot_size_t regionSize;
	/**< Mask of the 8 subregions to disable (Only applicable to slave) */
	u8_t subregions;
	/**< User permissions for the region */
	cy_en_prot_perm_t userPermission;
	/**< Privileged permissions for the region */
	cy_en_prot_perm_t privPermission;
	/**< Non Secure = 0, Secure = 1 */
	bool secure;
	/**< Access evaluation = 0, Matching = 1 */
	bool pcMatch;
	/**< Mask of allowed protection context(s) */
	u16_t pcMask;
	/* protection region */
	PROT_SMPU_SMPU_STRUCT_Type *prot_region;
	/**< User permissions for the region */
	cy_en_prot_perm_t userMstPermission;
	/**< Privileged permissions for the region */
	cy_en_prot_perm_t privMstPermission;
} cy_smpu_region_config_t;

/*
 * See Cy_Prot_ConfigBusMaster function description for parameters meaning
 *
 * act_pcMask specifies active PC for Cy_Prot_SetActivePC function
 */
typedef struct {
	en_prot_master_t busMaster;
	bool privileged;
	bool secure;
	u32_t pcMask;
	u32_t act_pc;
} cy_bus_master_config_t;

/** Configuration structure for Fixed Group (GR) PPU (PPU_GR) struct
 *  initialization
 */
typedef struct {
	/**< User permissions for the region */
	cy_en_prot_perm_t userPermission;
	/**< Privileged permissions for the region */
	cy_en_prot_perm_t privPermission;
	/**< Non Secure = 0, Secure = 1 */
	bool secure;
	/**< Access evaluation = 0, Matching = 1 */
	bool pcMatch;
	/**< Mask of allowed protection context(s) */
	u16_t pcMask;
	/**< Master User permissions for the region */
	cy_en_prot_perm_t userMstPermission;
	/**< Master Privileged permissions for the region */
	cy_en_prot_perm_t privMstPermission;
	/**< Non Secure = 0, Secure = 1 Master */
	bool secureMst;
	/**< Master Mask of allowed protection context(s) */
	u16_t pcMstMask;
	/**< Ppu structure address */
	PERI_PPU_GR_Type *pPpuStr;
} cy_ppu_fixed_gr_cfg_t;
/** Configuration structure for Fixed Region (RG) PPU (PPU_RG) struct
 *  initialization
 */
typedef struct {
	/**< User permissions for the region */
	cy_en_prot_perm_t userPermission;
	/**< Privileged permissions for the region */
	cy_en_prot_perm_t privPermission;
	/**< Non Secure = 0, Secure = 1 */
	bool secure;
	/**< Access evaluation = 0, Matching = 1 */
	bool pcMatch;
	/**< Mask of allowed protection context(s) */
	u16_t pcMask;
	/**< Master User permissions for the region */
	cy_en_prot_perm_t userMstPermission;
	/**< Master Privileged permissions for the region */
	cy_en_prot_perm_t privMstPermission;
	/**< Non Secure = 0, Secure = 1 Master */
	bool secureMst;
	/**< Master Mask of allowed protection context(s) */
	u16_t pcMstMask;
	/**< Ppu structure address */
	PERI_GR_PPU_RG_Type *pPpuStr;
} cy_ppu_fixed_rg_cfg_t;
/** Configuration structure for Fixed Slave (SL) PPU (PPU_SL) struct
 *  initialization
 */
typedef struct {
	/**< User permissions for the region */
	cy_en_prot_perm_t userPermission;
	/**< Privileged permissions for the region */
	cy_en_prot_perm_t privPermission;
	/**< Non Secure = 0, Secure = 1 */
	bool secure;
	/**< Access evaluation = 0, Matching = 1 */
	bool pcMatch;
	/**< Mask of allowed protection context(s) */
	u16_t pcMask;
	/**< Master User permissions for the region */
	cy_en_prot_perm_t userMstPermission;
	/**< Master Privileged permissions for the region */
	cy_en_prot_perm_t privMstPermission;
	/**< Non Secure = 0, Secure = 1 Master */
	bool secureMst;
	/**< Master Mask of allowed protection context(s) */
	u16_t pcMstMask;
	/**< Ppu structure address */
	PERI_GR_PPU_SL_Type *pPpuStr;
} cy_ppu_fixed_sl_cfg_t;

/** Configuration structure for Programmable (PROG) PPU (PPU_PR) struct
 *  initialization
 */
typedef struct {
	/**< Base address of the memory region (Only applicable to slave) */
	u32_t *address;
	/**< Size of the memory region (Only applicable to slave) */
	cy_en_prot_size_t regionSize;
	/**< Mask of the 8 subregions to disable (Only applicable to slave) */
	u8_t subregions;
	/**< User permissions for the region */
	cy_en_prot_perm_t userPermission;
	/**< Privileged permissions for the region */
	cy_en_prot_perm_t privPermission;
	/**< Non Secure = 0, Secure = 1 */
	bool secure;
	/**< Access evaluation = 0, Matching = 1 */
	bool pcMatch;
	/**< Mask of allowed protection context(s) */
	u16_t pcMask;
	/**< Master User permissions for the region */
	cy_en_prot_perm_t userMstPermission;
	/**< Master Privileged permissions for the region */
	cy_en_prot_perm_t privMstPermission;
	/**< Non Secure = 0, Secure = 1 Master */
	bool secureMst;
	/**< Master Mask of allowed protection context(s) */
	u16_t pcMstMask;
	PERI_PPU_PR_Type *pPpuStr; /**< Ppu structure address */
} cy_ppu_prog_cfg_t;

cy_en_prot_status_t smpu_protect(
	const cy_smpu_region_config_t smpu_config_arr[], u32_t arr_length);
cy_en_prot_status_t smpu_protect_unprotected(
	const cy_stc_smpu_cfg_t *smpu_config);
cy_en_prot_status_t ppu_fixed_rg_protect(
	const cy_ppu_fixed_rg_cfg_t ppu_config_arr[], u32_t arr_length);
cy_en_prot_status_t ppu_fixed_sl_protect(
	const cy_ppu_fixed_sl_cfg_t ppu_config_arr[], u32_t arr_length);
cy_en_prot_status_t ppu_prog_protect(
	const cy_ppu_prog_cfg_t ppu_config_arr[], u32_t arr_length);
cy_en_prot_status_t ppu_fixed_gr_protect(
	const cy_ppu_fixed_gr_cfg_t ppu_config_arr[], u32_t arr_length);
cy_en_prot_status_t bus_masters_protect(
	const cy_bus_master_config_t bus_masters_config_arr[],
	u32_t arr_length);

u8_t isPeripheralAccessAllowed(u32_t perStartAddr, u32_t perSize,
	u8_t privModeFlag, u8_t nsecureFlag, enum cy_en_prot_pc_t protectionCtx,
	cy_en_prot_perm_t accessType);
u8_t isMemoryAccessAllowed(u32_t memStartAddr, u32_t memSize,
	u8_t privModeFlag, u8_t nsecureFlag, enum cy_en_prot_pc_t protectionCtx,
	cy_en_prot_perm_t accessType);

#endif /* _CYPROTECTION_H_ */
