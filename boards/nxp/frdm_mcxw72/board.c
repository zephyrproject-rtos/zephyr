/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include "fsl_trdc.h"

/* Access-control policy index used by every block opened from this file.
 * Policy 0 is programmed at the top of board_early_init_hook() to permit
 * all access modes (secure / non-secure, user / privileged, R / W / X).
 */
#define BOARD_TRDC_POLICY_FULL_ACCESS    0U

/* MCXW72 routes the host CM33 peripheral windows through MBC2:
 *   slave 0 : PBRIDGE2     (0x4000_0000 .. 0x4007_FFFF, standard peripherals)
 *   slave 1 : Radio Pbridge in Fast Peripheral 1 (0x48A0_xxxx - link layers,
 *             XCVR, radio control, BRIC, packet RAM)
 *   slave 2 : NBU map in Fast Peripheral 1 (0x4880_0000 .. 0x489F_FFFF -
 *             Radio Flash, FRO192M, RF_FMU/CMC, NBU CIU2/SMU2, TPM2, LPTMR2)
 * See MCXW72 RM Rev. 5 section 10 (Table 40) and sections 3.5/3.6 (memory maps).
 */
#define BOARD_TRDC_MBC_INDEX             2U
#define BOARD_TRDC_PBRIDGE2_SLAVE        0U
#define BOARD_TRDC_RADIO_PBRIDGE_SLAVE   1U
#define BOARD_TRDC_NBU_MAP_SLAVE         2U

static uint8_t s_trdc_domain_count;

static void trdc_open_mbc_block(uint8_t slave, uint32_t block)
{
	trdc_mbc_memory_block_config_t cfg = {
		.memoryAccessControlSelect = BOARD_TRDC_POLICY_FULL_ACCESS,
		.nseEnable                 = false,
		.mbcIdx                    = BOARD_TRDC_MBC_INDEX,
		.slaveMemoryIdx            = slave,
		.memoryBlockIdx            = block,
	};

	for (uint8_t d = 0U; d < s_trdc_domain_count; d++) {
		cfg.domainIdx = d;
		TRDC_MbcSetMemoryBlockConfig(TRDC, &cfg);
	}
}

static void trdc_open_all_blocks(uint8_t slave)
{
	trdc_slave_memory_hardware_config_t slv_hw;

	TRDC_GetMbcHardwareConfig(TRDC, &slv_hw, BOARD_TRDC_MBC_INDEX, slave);
	for (uint32_t blk = 0U; blk < slv_hw.blockNum; blk++) {
		trdc_open_mbc_block(slave, blk);
	}
}

#define OPEN_PBRIDGE2(block) \
	trdc_open_mbc_block(BOARD_TRDC_PBRIDGE2_SLAVE, (block))

void board_early_init_hook(void)
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
	s_trdc_domain_count = hw.domainNumber;

	/* Program access-control policy 0 of every MBC to "full access". Each
	 * block we touch below is then bound to policy 0; blocks that stay
	 * untouched keep their reset-time policy and remain inaccessible.
	 */
	for (uint8_t i = 0U; i < hw.mbcNumber; i++) {
		TRDC_MbcSetMemoryAccessConfig(TRDC, &access_all, i,
					      BOARD_TRDC_POLICY_FULL_ACCESS);
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
	OPEN_PBRIDGE2(38U);  /* TRDC0 Manager  - 0x4002_6000 */
	OPEN_PBRIDGE2(39U);  /* TRDC0 MBC0     - 0x4002_7000 */
	OPEN_PBRIDGE2(40U);  /* TRDC0 MBC1     - 0x4002_8000 */
	OPEN_PBRIDGE2(41U);  /* TRDC0 MBC2     - 0x4002_9000 */
	OPEN_PBRIDGE2(42U);  /* TRDC0 MRC0     - 0x4002_A000 */
	OPEN_PBRIDGE2(43U);  /* VBAT0          - 0x4002_B000 */
	OPEN_PBRIDGE2(65U);  /* PORTA pinmux   - 0x4004_1000 */
	OPEN_PBRIDGE2(66U);  /* PORTB pinmux   - 0x4004_2000 */
	OPEN_PBRIDGE2(67U);  /* PORTC pinmux   - 0x4004_3000 */
	OPEN_PBRIDGE2(68U);  /* PORTD pinmux + GPIOD legacy alias - 0x4004_4000 */

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
	OPEN_PBRIDGE2(69U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(vref))
	OPEN_PBRIDGE2(72U);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexcan1))
	OPEN_PBRIDGE2(73U);
#endif

	/* === Radio / NBU (MBC2 SLV1 + MBC2 SLV2) === */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(nbu))
	/* RFMC / DSB0 sits in PBRIDGE2 slot 64 - host bootstraps NBU through it. */
	OPEN_PBRIDGE2(64U);

	/* SLV1: Radio Pbridge @ 0x48A0_xxxx (Zigbee LL, Generic LL, XCVR, radio
	 * control, BRIC, packet RAM). Open every block the hardware exposes.
	 */
	trdc_open_all_blocks(BOARD_TRDC_RADIO_PBRIDGE_SLAVE);

	/* SLV2: NBU map @ 0x4880_0000 .. 0x489F_FFFF (Radio Flash, FRO192M,
	 * RF_FMU/CMC, NBU CIU2/SMU2, TPM2, LPTMR2). Host CM33 needs this for
	 * NBU boot and SMU2 shared-memory access.
	 */
	trdc_open_all_blocks(BOARD_TRDC_NBU_MAP_SLAVE);
#endif

	TRDC_SetMbcGlobalValid(TRDC);

	/* Bind the host processor master to domain 0 (the domain whose policy
	 * we just programmed) and arm domain-access control.
	 */
	trdc_processor_domain_assignment_t pda;

	TRDC_GetDefaultProcessorDomainAssignment(&pda);
	pda.domainId = 0U;
	TRDC_SetProcessorDomainAssignment(TRDC, &pda);

	TRDC_SetDacGlobalValid(TRDC);
}
