/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * RT118x TRDC setup
 *
 * This implementation is intentionally aligned with the NXP MCUX RT1180 SDK
 * reference (board.c: BOARD_GrantTRDCFullPermissions):
 *   - Request TRDC ownership (AON/MEGA/WAKEUP, with MEGA before WAKEUP)
 *   - Configure TRDC DAC (domain assignments for key masters)
 *   - Grant full access permissions across TRDC regions/blocks (TRDC1/TRDC2)
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <fsl_ele_base_api.h>
#include <fsl_trdc.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Keep values aligned with SDK board.c */
#define ELE_TRDC_AON_ID    0x74
#define ELE_TRDC_WAKEUP_ID 0x78
#define ELE_TRDC_MEGA_ID   0x82
#define ELE_CORE_CM33_ID   0x1
#define ELE_CORE_CM7_ID    0x2

static void imxrt118x_request_trdc_ownership(bool request_aon,
					    bool request_wakeup, bool request_mega)
{
	status_t sts;

	/* Get ELE FW status */
	do {
		uint32_t ele_fw_sts;

		sts = ELE_BaseAPI_GetFwStatus(MU_RT_S3MUA, &ele_fw_sts);
	} while (sts != kStatus_Success);

#if defined(CONFIG_CPU_CORTEX_M33)
	uint8_t ele_core_id = ELE_CORE_CM33_ID;
#elif defined(CONFIG_CPU_CORTEX_M7)
	uint8_t ele_core_id = ELE_CORE_CM7_ID;
#endif

	if (request_aon) {
		sts = ELE_BaseAPI_ReleaseRDC(MU_RT_S3MUA, ELE_TRDC_AON_ID, ele_core_id);
		if (sts != kStatus_Success) {
			LOG_WRN("warning: TRDC AON permission get failed.");
		}
	}

	/*
	 * TRDC MEGA request must be prior to TRDC WAKEUP, as TRDC MEGA access
	 * is controlled by the TRDC WAKEUP.
	 */
	if (request_mega) {
		sts = ELE_BaseAPI_ReleaseRDC(MU_RT_S3MUA, ELE_TRDC_MEGA_ID, ele_core_id);
		if (sts != kStatus_Success) {
			LOG_WRN("warning: TRDC MEGA permission get failed.");
		}
	}

	if (request_wakeup) {
		sts = ELE_BaseAPI_ReleaseRDC(MU_RT_S3MUA, ELE_TRDC_WAKEUP_ID, ele_core_id);
		if (sts != kStatus_Success) {
			LOG_WRN("warning: TRDC WAKEUP permission get failed.");
		}
	}
}

static void app_common_trdc_dac_setting(void)
{
	trdc_processor_domain_assignment_t procAssign = {
		.domainId = 0U,
		.domainIdSelect = kTRDC_DidInput,
		.pidDomainHitConfig = kTRDC_pidDomainHitNone0,
		.pidMask = 0U,
		.secureAttr = kTRDC_ForceSecure,
		.pid = 0U,
		.lock = false,
	};

	trdc_non_processor_domain_assignment_t nonProcAssign = {
		.domainId = 0U,
		.privilegeAttr = kTRDC_ForcePrivilege,
		.secureAttr = kTRDC_ForceSecure,
		.bypassDomainId = true,
		.lock = false,
	};

	/* 1. Set the MDAC Configuration in TRDC1. */
	/* Configure the access control for CM33(master 1 for TRDC1, MDAC_A1). */
	procAssign.domainId = 0x2U;
	TRDC_SetProcessorDomainAssignment(TRDC1, (uint8_t)kTRDC1_MasterCM33, 0U, &procAssign);
	/* Configure the access control for eDMA3(master 2 for TRDC1, MDAC_A2). */
	nonProcAssign.domainId = 0x7U;
	TRDC_SetNonProcessorDomainAssignment(TRDC1, (uint8_t)kTRDC1_MasterDMA3, &nonProcAssign);

	/* 2. Set the MDAC Configuration in TRDC2. */
	/* Configure the access control for CM7 AHBP(master 0 for TRDC2, MDAC_W0). */
	procAssign.domainId = 0x4U;
	TRDC_SetProcessorDomainAssignment(TRDC2, (uint8_t)kTRDC2_MasterCM7AHBP, 0U, &procAssign);
	/* Configure the access control for CM7 AXI(master 1 for TRDC2, MDAC_W1). */
	TRDC_SetProcessorDomainAssignment(TRDC2, (uint8_t)kTRDC2_MasterCM7AXI, 0U, &procAssign);
	/* Configure the access control for DAP AHB_AP_SYS(master 2 for TRDC2, MDAC_W2). */
	nonProcAssign.domainId = 0x9U;
	TRDC_SetNonProcessorDomainAssignment(TRDC2, (uint8_t)kTRDC2_MasterDAP, &nonProcAssign);
	/* Configure the access control for CoreSight(master 3 for TRDC2, MDAC_W3). */
	nonProcAssign.domainId = 0x8U;
	TRDC_SetNonProcessorDomainAssignment(TRDC2, (uint8_t)kTRDC2_MasterCoreSight,
					     &nonProcAssign);
	/* Configure the access control for DMA4(master 4 for TRDC2, MDAC_W4). */
	nonProcAssign.domainId = 0x7U;
	TRDC_SetNonProcessorDomainAssignment(TRDC2, (uint8_t)kTRDC2_MasterDMA4, &nonProcAssign);
	/* Configure the access control for NETC(master 5 for TRDC2, MDAC_W5). */
	procAssign.domainId = 0xAU;
	TRDC_SetProcessorDomainAssignment(TRDC2, (uint8_t)kTRDC2_MasterNETC, 0U, &procAssign);

	/* 3. Set the MDAC Configuration in TRDC3. */
	/* Configure the access control for uSDHC1(master 0 for TRDC3, MDAC_M0). */
	nonProcAssign.domainId = 0x5U;
	TRDC_SetNonProcessorDomainAssignment(TRDC3, (uint8_t)kTRDC3_MasterUSDHC1, &nonProcAssign);
	/* Configure the access control for uSDHC2(master 1 for TRDC3, MDAC_M1). */
	nonProcAssign.domainId = 0x6U;
	TRDC_SetNonProcessorDomainAssignment(TRDC3, (uint8_t)kTRDC3_MasterUSDHC2, &nonProcAssign);
	/* Configure the access control for USB(master 3 for TRDC3, MDAC_M3). */
	nonProcAssign.domainId = 0xBU;
	TRDC_SetNonProcessorDomainAssignment(TRDC3, (uint8_t)kTRDC3_MasterUsb, &nonProcAssign);
	/* Configure the access control for FlexSPI_FLR(master 4 for TRDC3, MDAC_M4). */
	nonProcAssign.domainId = 0xAU;
	TRDC_SetNonProcessorDomainAssignment(TRDC3, (uint8_t)kTRDC3_MasterFlexspiFlr,
					     &nonProcAssign);
}

static bool trdc_is_valid_domain(TRDC_Type *trdc, uint8_t domain)
{
	ARG_UNUSED(trdc);

	/* Copied from SDK board.c */
	return !((domain > 11) || (domain < 2) || (domain == 3));
}

static bool trdc_is_valid_mbc(TRDC_Type *trdc, uint8_t mbc)
{
	if (trdc == TRDC1) {
		return (mbc == 0U) || (mbc == 1U);
	}

	if (trdc == TRDC2) {
		return (mbc == 0U) || (mbc == 1U);
	}

	return false;
}

static uint32_t trdc_get_mbc_mem_num(TRDC_Type *trdc, uint32_t mbc)
{
	if (trdc == TRDC1) {
		/* TRDC1 MBC_A0 has 3 memories, MBC_A1 has 2 memories */
		static const uint8_t mem_num[2] = {3, 2};

		return (mbc < 2U) ? mem_num[mbc] : 0U;
	}

	if (trdc == TRDC2) {
		/* TRDC2 MBC_W0 has 4 memories, MBC_W1 has 4 memories */
		static const uint8_t mem_num[2] = {4, 4};

		return (mbc < 2U) ? mem_num[mbc] : 0U;
	}

	return 0U;
}

static bool trdc_is_valid_mbc_mem(TRDC_Type *trdc, uint8_t mbc, uint8_t mem)
{
	if (trdc == TRDC1) {
		switch (mbc) {
		case 0:
			/* mem1 (Edgelock) intentionally not touched */
			return (mem == 0U) || (mem == 2U);
		case 1:
			return (mem == 0U) || (mem == 1U);
		default:
			return false;
		}
	}

	if (trdc == TRDC2) {
		switch (mbc) {
		case 0:
			return (mem <= 3U);
		case 1:
			return (mem <= 3U);
		default:
			return false;
		}
	}

	return false;
}

static bool trdc_is_valid_mrc(TRDC_Type *trdc, uint8_t mrc)
{
	if (trdc == TRDC1) {
		return (mrc == 0U) || (mrc == 1U);
	}

	if (trdc == TRDC2) {
		return (mrc >= 1U) && (mrc <= 6U);
	}

	return false;
}

static bool trdc_get_mrc_region_addr(TRDC_Type *trdc, uint8_t mrc, uint32_t *start, uint32_t *end)
{
	if (trdc == TRDC1) {
		switch (mrc) {
		case 0:
			*start = 0x00000000UL;
			*end = 0x00027FFFUL;
			return true;
		case 1:
			*start = 0x04000000UL;
			*end = 0x07FFFFFFUL;
			return true;
		default:
			return false;
		}
	}

	if (trdc == TRDC2) {
		switch (mrc) {
		case 1:
			*start = 0x28000000UL;
			*end = 0x2FFFFFFFUL;
			return true;
		case 2:
			*start = 0x203C0000UL;
			*end = 0x2043FFFFUL;
			return true;
		case 3:
			*start = 0x20480000UL;
			*end = 0x204FFFFFUL;
			return true;
		case 4:
			*start = 0x20500000UL;
			*end = 0x2053FFFFUL;
			return true;
		case 5:
			*start = 0x80000000UL;
			*end = 0x8FFFFFFFUL;
			return true;
		case 6:
			*start = 0x60000000UL;
			*end = 0x60FFFFFFUL;
			return true;
		default:
			return false;
		}
	}

	return false;
}

static void trdc_init_memory_access_config(trdc_memory_access_control_config_t *config)
{
	(void)memset(config, 0, sizeof(*config));
	config->nonsecureUsrX = 1U;
	config->nonsecureUsrW = 1U;
	config->nonsecureUsrR = 1U;
	config->nonsecurePrivX = 1U;
	config->nonsecurePrivW = 1U;
	config->nonsecurePrivR = 1U;
	config->secureUsrX = 1U;
	config->secureUsrW = 1U;
	config->secureUsrR = 1U;
	config->securePrivX = 1U;
	config->securePrivW = 1U;
	config->securePrivR = 1U;
}

static void trdc_setup_mrc_access(TRDC_Type *trdc,
				  const trdc_memory_access_control_config_t *memAccessConfig,
				  uint32_t mrc_number)
{
	for (uint32_t mrc = 0U; mrc < mrc_number; mrc++) {
		if (trdc_is_valid_mrc(trdc, (uint8_t)mrc)) {
			for (uint32_t i = 0U; i < 8U; i++) {
				TRDC_MrcSetMemoryAccessConfig(trdc, memAccessConfig, mrc, i);
			}
		}
	}
}

static void trdc_setup_mbc_access(TRDC_Type *trdc,
				  const trdc_memory_access_control_config_t *memAccessConfig,
				  uint32_t mbc_number)
{
	for (uint32_t mbc = 0U; mbc < mbc_number; mbc++) {
		if (trdc_is_valid_mbc(trdc, (uint8_t)mbc)) {
			for (uint32_t i = 0U; i < 8U; i++) {
				TRDC_MbcSetMemoryAccessConfig(trdc, memAccessConfig, mbc, i);
			}
		}
	}
}

static void trdc_configure_mbc_blocks(TRDC_Type *trdc, uint32_t domain, uint32_t mbc,
				      trdc_mbc_memory_block_config_t *mbcBlockConfig)
{
	uint32_t mem_num = trdc_get_mbc_mem_num(trdc, mbc);

	for (uint32_t mem = 0; mem < mem_num; mem++) {
		if (!trdc_is_valid_mbc_mem(trdc, (uint8_t)mbc, (uint8_t)mem)) {
			continue;
		}

		trdc_slave_memory_hardware_config_t mbcHwConfig;

		TRDC_GetMbcHardwareConfig(trdc, &mbcHwConfig, mbc, mem);

		for (uint32_t block = 0; block < mbcHwConfig.blockNum; block++) {
			mbcBlockConfig->domainIdx = domain;
			mbcBlockConfig->mbcIdx = mbc;
			mbcBlockConfig->slaveMemoryIdx = mem;
			mbcBlockConfig->memoryBlockIdx = block;
			TRDC_MbcSetMemoryBlockConfig(trdc, mbcBlockConfig);
		}
	}
}

static void trdc_setup_domain_mbc(TRDC_Type *trdc, uint32_t domain, uint32_t mbc_number,
				  trdc_mbc_memory_block_config_t *mbcBlockConfig)
{
	for (uint32_t mbc = 0U; mbc < mbc_number; mbc++) {
		if (!trdc_is_valid_mbc(trdc, (uint8_t)mbc)) {
			continue;
		}

		trdc_configure_mbc_blocks(trdc, domain, mbc, mbcBlockConfig);
	}
}

static void trdc_setup_domain_mrc(TRDC_Type *trdc, uint32_t domain, uint32_t mrc_number,
				  trdc_mrc_region_descriptor_config_t *mrcRegionConfig)
{
	for (uint32_t mrc = 0U; mrc < mrc_number; mrc++) {
		if (!trdc_is_valid_mrc(trdc, (uint8_t)mrc)) {
			continue;
		}

		uint32_t start_addr, end_addr;

		if (trdc_get_mrc_region_addr(trdc, (uint8_t)mrc, &start_addr, &end_addr)) {
			mrcRegionConfig->startAddr = start_addr;
			mrcRegionConfig->endAddr = end_addr;
			mrcRegionConfig->domainIdx = domain;
			mrcRegionConfig->mrcIdx = mrc;
			TRDC_MrcSetRegionDescriptorConfig(trdc, mrcRegionConfig);
		} else {
			LOG_ERR("TRDC MRC region address invalid: trdc=%p mrc=%u", trdc,
				(unsigned int)mrc);
		}
	}
}

static void app_common_trdc_access_control_setting(TRDC_Type *trdc)
{
	trdc_hardware_config_t hwConfig;
	trdc_memory_access_control_config_t memAccessConfig;
	trdc_mbc_memory_block_config_t mbcBlockConfig;
	trdc_mrc_region_descriptor_config_t mrcRegionConfig;

	TRDC_GetHardwareConfig(trdc, &hwConfig);

	/* Enable all read/write/execute access for MRC/MBC access control. */
	trdc_init_memory_access_config(&memAccessConfig);

	trdc_setup_mrc_access(trdc, &memAccessConfig, hwConfig.mrcNumber);
	trdc_setup_mbc_access(trdc, &memAccessConfig, hwConfig.mbcNumber);

	(void)memset(&mbcBlockConfig, 0, sizeof(mbcBlockConfig));
	mbcBlockConfig.nseEnable = false;
	mbcBlockConfig.memoryAccessControlSelect = 0;

	(void)memset(&mrcRegionConfig, 0, sizeof(mrcRegionConfig));
	mrcRegionConfig.memoryAccessControlSelect = 0U;
	mrcRegionConfig.valid = true;
	mrcRegionConfig.nseEnable = false;
	mrcRegionConfig.regionIdx = 0U;

	for (uint32_t domain = 0; domain < hwConfig.domainNumber; domain++) {
		if (!trdc_is_valid_domain(trdc, (uint8_t)domain)) {
			continue;
		}

		/* Set the configuration for MBC. */
		trdc_setup_domain_mbc(trdc, domain, hwConfig.mbcNumber, &mbcBlockConfig);

		/* Set the configuration for MRC. */
		trdc_setup_domain_mrc(trdc, domain, hwConfig.mrcNumber, &mrcRegionConfig);
	}
}

void imxrt118x_trdc_enable_all_access(void)
{
	/* 1. Request TRDC ownership */
	imxrt118x_request_trdc_ownership(true, true, true);

	/* 2. Config DAC. */
	app_common_trdc_dac_setting();

	/* 3. Enable all access control */
	app_common_trdc_access_control_setting(TRDC1);
	app_common_trdc_access_control_setting(TRDC2);
}
