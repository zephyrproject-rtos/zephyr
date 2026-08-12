/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <soc.h>
#include <fsl_trdc.h>

/* Access-control policy index used by every block opened from this file.
 * Policy 0 is programmed at the top of nxp_mcxw72_trdc_init() to permit
 * all access modes (secure / non-secure, user / privileged, R / W / X).
 */
#define MCXW72_TRDC_POLICY_FULL_ACCESS    0U

/* MCXW72 routes the host CM33 peripheral windows through MBC2:
 *   slave 0 : PBRIDGE2 (0x4000_0000 .. 0x4007_FFFF, standard peripherals)
 *   slave 1 : Radio Pbridge in Fast Peripheral 1 (0x4800_0000 .. 0x487F_FFFF)
 *   slave 2 : NBU map in Fast Peripheral 1 (0x48A0_0000 .. 0x48A0_FFFF -
 *             Zigbee / Generic link layers, radio control, XCVR, packet RAM)
 *
 * A fourth radio window, the NBU map at 0x4880_0000 .. 0x489F_FFFF (Radio
 * Flash/IFR, NBU CIU2, FRO192M, RF FMU/CMC, LPTPM2, NBU SMU2), is guarded by
 * the region checker MRC0.
 * See MCXW72 RM Rev. 5 section 10 (Table 40) and sections 3.5/3.6 (memory maps).
 */
#define MCXW72_TRDC_MBC_INDEX             2U
#define MCXW72_TRDC_PBRIDGE2_SLAVE        0U
#define MCXW72_TRDC_RADIO_PBRIDGE_SLAVE   1U
#define MCXW72_TRDC_NBU_MAP_SLAVE         2U

/* MRC0 guards the NBU map window; program it as a single full-access region. */
#define MCXW72_TRDC_MRC0_INDEX            0U
#define MCXW72_TRDC_NBU_MAP_MRC_START     0x48800000U
#define MCXW72_TRDC_NBU_MAP_MRC_END       0x489FFFFFU

/* Bus masters and their TRDC domain IDs (DIDs).
 * The masters are CM33 (MDAC0), DMA3 (MDAC1), Data stream buffer (MDAC2),
 * Radio NBU (MDAC3) and LCE (MDAC4). The device implements 3 domains
 * (DID 0-2, TRDC_HWCFG0[NDID]).
 *
 * A TRDC-locked boot ROM (Sentinel) may already program the master domain
 * assignment (MDA) registers. This file therefore reads each MDA register and
 * preserves a valid assignment; the DID_*_DEFAULT values below are fallbacks
 * used only for masters the boot ROM left unassigned. Each master is kept in
 * its own domain so the domains can carry different policy - the peripheral
 * grants below are programmed for the CM33 host domain only, and the other
 * masters' domains are opened separately when a driver needs them.
 */
#define MCXW72_TRDC_DID_HOST_DEFAULT      0U   /* CM33 fallback domain. */
#define MCXW72_TRDC_DID_DSB_DEFAULT       1U   /* Data stream buffer fallback. */
#define MCXW72_TRDC_DID_NBU_DEFAULT       2U   /* Radio NBU fallback. */

/* Domain that carries the CM33 host peripheral grants. Resolved at runtime
 * from the CM33 MDA register: the boot-ROM value when it is already valid,
 * otherwise MCXW72_TRDC_DID_HOST_DEFAULT.
 */
static uint8_t s_host_domain;

/* Reads a master's current domain assignment. Returns true and fills
 * domain_id when the master's MDA register is already valid (e.g. programmed
 * by the boot ROM), false when the master is still unassigned.
 */
static bool trdc_master_domain_read(uint8_t master, uint8_t *domain_id)
{
	uint32_t reg;
	uint32_t vld_mask;
	uint32_t did_mask;
	uint32_t did_shift;

	if (master == (uint8_t)kTRDC_MasterCM33) {
		reg       = TRDC->MDA_W0_0_DFMT0;
		vld_mask  = TRDC_MDA_W0_0_DFMT0_VLD_MASK;
		did_mask  = TRDC_MDA_W0_0_DFMT0_DID_MASK;
		did_shift = TRDC_MDA_W0_0_DFMT0_DID_SHIFT;
	} else {
		reg       = TRDC->MDA_W0_DFMT1[master - 1U].MDA_W0_x_DFMT1;
		vld_mask  = TRDC_MDA_W0_x_DFMT1_VLD_MASK;
		did_mask  = TRDC_MDA_W0_x_DFMT1_DID_MASK;
		did_shift = TRDC_MDA_W0_x_DFMT1_DID_SHIFT;
	}

	if ((reg & vld_mask) == 0U) {
		return false;
	}

	*domain_id = (uint8_t)((reg & did_mask) >> did_shift);
	return true;
}

static void trdc_open_mbc_block(uint8_t slave, uint32_t block)
{
	trdc_mbc_memory_block_config_t cfg = {
		.memoryAccessControlSelect = MCXW72_TRDC_POLICY_FULL_ACCESS,
		.nseEnable                 = false,
		.mbcIdx                    = MCXW72_TRDC_MBC_INDEX,
		.slaveMemoryIdx            = slave,
		.memoryBlockIdx            = block,
		.domainIdx                 = s_host_domain,
	};

	TRDC_MbcSetMemoryBlockConfig(TRDC, &cfg);
}

static void trdc_open_all_blocks(uint8_t slave)
{
	trdc_slave_memory_hardware_config_t slv_hw;

	TRDC_GetMbcHardwareConfig(TRDC, &slv_hw, MCXW72_TRDC_MBC_INDEX, slave);
	for (uint32_t blk = 0U; blk < slv_hw.blockNum; blk++) {
		trdc_open_mbc_block(slave, blk);
	}
}

static void trdc_open_mrc_region(uint8_t mrc, uint8_t region, uint32_t start_addr,
				 uint32_t end_addr)
{
	trdc_mrc_region_descriptor_config_t cfg = {
		.memoryAccessControlSelect = MCXW72_TRDC_POLICY_FULL_ACCESS,
		.startAddr                 = start_addr,
		.endAddr                   = end_addr,
		.valid                     = true,
		.nseEnable                 = false,
		.mrcIdx                    = mrc,
		.regionIdx                 = region,
		.domainIdx                 = s_host_domain,
	};

	TRDC_MrcSetRegionDescriptorConfig(TRDC, &cfg);
}

#define OPEN_PBRIDGE2(block) \
	trdc_open_mbc_block(MCXW72_TRDC_PBRIDGE2_SLAVE, (block))

void nxp_mcxw72_trdc_init(void)
{
	trdc_memory_access_control_config_t access_all = {
		.nonsecureUsrX  = 1U, .nonsecureUsrW  = 1U, .nonsecureUsrR  = 1U,
		.nonsecurePrivX = 1U, .nonsecurePrivW = 1U, .nonsecurePrivR = 1U,
		.secureUsrX     = 1U, .secureUsrW     = 1U, .secureUsrR     = 1U,
		.securePrivX    = 1U, .securePrivW    = 1U, .securePrivR    = 1U,
	};
	trdc_hardware_config_t hw;

	TRDC_Init(TRDC);

	TRDC_GetHardwareConfig(TRDC, &hw);

	/* Resolve the CM33 host domain before opening any block: keep the
	 * boot-ROM (Sentinel) assignment when the CM33 MDA register is already
	 * valid, otherwise fall back to the default host DID. Every block and
	 * region grant below targets this domain.
	 */
	if (!trdc_master_domain_read((uint8_t)kTRDC_MasterCM33, &s_host_domain)) {
		s_host_domain = MCXW72_TRDC_DID_HOST_DEFAULT;
	}

	/* Program access-control policy 0 of every MBC to "full access". Each
	 * block we touch below is then bound to policy 0.
	 */
	for (uint8_t i = 0U; i < hw.mbcNumber; i++) {
		TRDC_MbcSetMemoryAccessConfig(TRDC, &access_all, i,
					      MCXW72_TRDC_POLICY_FULL_ACCESS);
	}

	/* Mirror the same full-access policy 0 into every MRC so the region
	 * descriptor opened for the NBU map below can select it.
	 */
	for (uint8_t i = 0U; i < hw.mrcNumber; i++) {
		TRDC_MrcSetMemoryAccessConfig(TRDC, &access_all, i,
					      MCXW72_TRDC_POLICY_FULL_ACCESS);
	}

	/* Always-required PBRIDGE2 blocks for Zephyr boot, regardless of DT.
	 * Most of these have no device-tree nodes (system control / clock /
	 * TRDC programming model itself) but their registers must be reachable
	 * for the SoC layer to bring the kernel up.
	 */
	OPEN_PBRIDGE2(1U);   /* CMC0           - 0x4000_1000 */
	OPEN_PBRIDGE2(20U);  /* MSCM0          - 0x4001_4000 */
	OPEN_PBRIDGE2(21U);  /* SMSCM0         - 0x4001_5000 */
	OPEN_PBRIDGE2(22U);  /* SPC0           - 0x4001_6000 */
	OPEN_PBRIDGE2(24U);  /* TRGMUX0        - 0x4001_8000 */
	OPEN_PBRIDGE2(25U);  /* WUU0           - 0x4001_9000 */
	OPEN_PBRIDGE2(28U);  /* PCC0           - 0x4001_C000 */
	OPEN_PBRIDGE2(30U);  /* SCG0           - 0x4001_E000 */
	OPEN_PBRIDGE2(31U);  /* CCM32K / RTCD0 - 0x4001_F000 */
	OPEN_PBRIDGE2(38U);  /* TRDC0 Manager  - 0x4002_6000 */
	OPEN_PBRIDGE2(39U);  /* TRDC0 MBC0     - 0x4002_7000 */
	OPEN_PBRIDGE2(40U);  /* TRDC0 MBC1     - 0x4002_8000 */
	OPEN_PBRIDGE2(41U);  /* TRDC0 MBC2     - 0x4002_9000 */
	OPEN_PBRIDGE2(42U);  /* TRDC0 MRC0     - 0x4002_A000 */
	OPEN_PBRIDGE2(43U);  /* VBAT0          - 0x4002_B000 */
	OPEN_PBRIDGE2(64U);  /* RFMC           - 0x4004_0000 (host bootstraps NBU) */
	OPEN_PBRIDGE2(65U);  /* DSB0           - 0x4004_1000 */
	OPEN_PBRIDGE2(66U);  /* PORTA pinmux   - 0x4004_2000 */
	OPEN_PBRIDGE2(67U);  /* PORTB pinmux   - 0x4004_3000 */
	OPEN_PBRIDGE2(68U);  /* PORTC pinmux   - 0x4004_4000 */
	OPEN_PBRIDGE2(69U);  /* PORTD pinmux   - 0x4004_5000 */
	OPEN_PBRIDGE2(70U);  /* GPIOD          - 0x4004_6000 */

	/* === Device-tree gated PBRIDGE2 peripherals === */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(edma))
	/* DMA0 management page + 16 channels: slots 2..18 */
	for (uint32_t blk = 2U; blk <= 18U; blk++) {
		OPEN_PBRIDGE2(blk);
	}
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ewm0))
	OPEN_PBRIDGE2(19U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wdog0))
	OPEN_PBRIDGE2(26U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wdog1))
	OPEN_PBRIDGE2(27U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(fmu))
	OPEN_PBRIDGE2(32U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(crc))
	OPEN_PBRIDGE2(35U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rtc))
	OPEN_PBRIDGE2(44U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lptmr0))
	OPEN_PBRIDGE2(45U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lptmr1))
	OPEN_PBRIDGE2(46U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpit0))
	OPEN_PBRIDGE2(47U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(tpm0))
	OPEN_PBRIDGE2(49U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(tpm1))
	OPEN_PBRIDGE2(50U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c0))
	OPEN_PBRIDGE2(51U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c1))
	OPEN_PBRIDGE2(52U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(i3c0))
	OPEN_PBRIDGE2(53U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpspi0))
	OPEN_PBRIDGE2(54U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpspi1))
	OPEN_PBRIDGE2(55U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart0))
	OPEN_PBRIDGE2(56U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart1))
	OPEN_PBRIDGE2(57U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexio))
	OPEN_PBRIDGE2(58U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan0))
	/* FlexCAN0 occupies 4 contiguous PBRIDGE2 slots (59..62). */
	OPEN_PBRIDGE2(59U);
	OPEN_PBRIDGE2(60U);
	OPEN_PBRIDGE2(61U);
	OPEN_PBRIDGE2(62U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(adc0))
	OPEN_PBRIDGE2(71U);  /* ADC0 - 0x4004_7000 */
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(vref))
	OPEN_PBRIDGE2(74U);  /* VREF0 - 0x4004_A000 */
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan1))
	OPEN_PBRIDGE2(79U);  /* CAN1 - 0x4004_F000 */
	OPEN_PBRIDGE2(80U);
	OPEN_PBRIDGE2(81U);
	OPEN_PBRIDGE2(82U);
#endif

	/* === Radio window (MBC2 SLV1) — required regardless of NBU ===
	 *
	 * MBC2 SLV1 is the Radio Pbridge in Fast Peripheral 1
	 * (0x4800_0000 .. 0x487F_FFFF). This window also hosts GPIOA/B/C
	 * (0x5801_0000 / 0x5802_0000 / 0x5803_0000), which the host needs
	 * regardless of NBU, so open it unconditionally rather than under the
	 * nbu gate. Open every block the hardware exposes.
	 */
	trdc_open_all_blocks(MCXW72_TRDC_RADIO_PBRIDGE_SLAVE);

	/* === Radio / NBU (MBC2 SLV2, MRC0) === */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(nbu))
	/* MBC2 SLV2: NBU map in Fast Peripheral 1 (0x48A0_0000 .. 0x48A0_FFFF -
	 * Zigbee / Generic link layers, radio control, XCVR, packet RAM). Open
	 * every block the hardware exposes.
	 */
	trdc_open_all_blocks(MCXW72_TRDC_NBU_MAP_SLAVE);

	/* NBU map at 0x4880_0000 .. 0x489F_FFFF (Radio Flash/IFR, NBU CIU2,
	 * FRO192M, RF FMU/CMC, LPTPM2, NBU SMU2) is guarded by MRC0.
	 * The host CM33 needs it for NBU boot and SMU2 shared-memory
	 * access, so grant the whole window as one full-access MRC0 region.
	 */
	trdc_open_mrc_region(MCXW72_TRDC_MRC0_INDEX, 0U,
			     MCXW72_TRDC_NBU_MAP_MRC_START,
			     MCXW72_TRDC_NBU_MAP_MRC_END);
	TRDC_SetMrcGlobalValid(TRDC);
#endif

	TRDC_SetMbcGlobalValid(TRDC);

	/* === Domain Assignment Controller (DAC) ===
	 *
	 * Program each master's domain, but never override a valid boot-ROM
	 * (Sentinel) assignment - only masters the ROM left unassigned get a
	 * fallback DID.
	 */
	uint8_t did;

	/* CM33 host processor (MDAC0). */
	if (!trdc_master_domain_read((uint8_t)kTRDC_MasterCM33, &did)) {
		trdc_processor_domain_assignment_t cpu_assign;

		TRDC_GetDefaultProcessorDomainAssignment(&cpu_assign);
		cpu_assign.domainId = s_host_domain;
		TRDC_SetProcessorDomainAssignment(TRDC, &cpu_assign);
	}

	/* Non-processor masters (MDAC1..3). Keep each master's own secure and
	 * privileged attributes. eDMA masquerades as the initiator that
	 * programmed the transfer (DID bypass), so a CM33-initiated transfer
	 * runs in the host domain; the data stream buffer and Radio NBU each
	 * get their own radio-side domain.
	 */
	trdc_non_processor_domain_assignment_t np_assign;

	TRDC_GetDefaultNonProcessorDomainAssignment(&np_assign);
	np_assign.privilegeAttr = kTRDC_MasterPrivilege;
	np_assign.secureAttr    = kTRDC_MasterSecure;

	if (!trdc_master_domain_read((uint8_t)kTRDC_MasterDMA3, &did)) {
		np_assign.bypassDomainId = true;
		np_assign.domainId       = s_host_domain;
		TRDC_SetNonProcessorDomainAssignment(TRDC, (uint8_t)kTRDC_MasterDMA3,
						     &np_assign);
	}

	if (!trdc_master_domain_read((uint8_t)kTRDC_MasterDataSteamBuffer, &did)) {
		np_assign.bypassDomainId = false;
		np_assign.domainId       = MCXW72_TRDC_DID_DSB_DEFAULT;
		TRDC_SetNonProcessorDomainAssignment(TRDC, (uint8_t)kTRDC_MasterDataSteamBuffer,
						     &np_assign);
	}

	if (!trdc_master_domain_read((uint8_t)kTRDC_MasterRadioNBU, &did)) {
		np_assign.bypassDomainId = false;
		np_assign.domainId       = MCXW72_TRDC_DID_NBU_DEFAULT;
		TRDC_SetNonProcessorDomainAssignment(TRDC, (uint8_t)kTRDC_MasterRadioNBU,
						     &np_assign);
	}

	TRDC_SetDacGlobalValid(TRDC);
}
