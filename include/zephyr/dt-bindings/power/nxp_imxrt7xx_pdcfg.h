/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Symbolic bits for the RT700 deep-sleep / deep-sleep-retention keep-alive masks
 * used by "nxp,pdcfg-power" (deep-sleep-config / deep-sleep-retention-config).
 *
 * Each word is an "exclude from power-down" mask for one register: a set bit
 * keeps that block powered. The SoC programs each as (PDSLEEPCFGn & ~excl[n]).
 *
 *   word[0] SLEEPCON0.SLEEPCFG  clocks / oscillators / PLLs      RT7XX_SLEEPCFG_*
 *   word[1] PMC.PDSLEEPCFG0     voltage domains / DSR switches   RT7XX_PDCFG0_*
 *   word[2] PMC.PDSLEEPCFG1     (unused here)
 *   word[3] PMC.PDSLEEPCFG2     main-memory ARRAY                RT7XX_PDCFG_SRAM_*
 *   word[4] PMC.PDSLEEPCFG3     main-memory PERIPHERY            (same SRAM bits)
 *   word[5] PMC.PDSLEEPCFG4     peripheral-memory ARRAY          RT7XX_PDCFG4_*
 *   word[6] PMC.PDSLEEPCFG5     peripheral-memory PERIPHERY      (same CFG4 bits)
 *
 * ARRAY keeps the RAM contents; PERIPHERY keeps the access logic and only
 * matters while the array is kept on.
 *
 * Only bits used by the in-tree configs are named; values mirror the MCUX
 * *_MASK fields 1:1 (see RM 32 for the full maps).
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_NXP_IMXRT7XX_PDCFG_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_NXP_IMXRT7XX_PDCFG_H_

/* word[0]: SLEEPCON0.SLEEPCFG */
#define RT7XX_SLEEPCFG_SENSEP_MAINCLK	0x00000002 /* bit1 */
#define RT7XX_SLEEPCFG_SENSES_MAINCLK	0x00000004 /* bit2 */
#define RT7XX_SLEEPCFG_COMN_MAINCLK	0x00000020 /* bit5 */
#define RT7XX_SLEEPCFG_XTAL		0x00000080 /* bit7 */
#define RT7XX_SLEEPCFG_FRO0		0x00000100 /* bit8 */
#define RT7XX_SLEEPCFG_FRO1		0x00000200 /* bit9 */
#define RT7XX_SLEEPCFG_FRO2		0x00000400 /* bit10 */
#define RT7XX_SLEEPCFG_LPOSC		0x00000800 /* bit11 */
#define RT7XX_SLEEPCFG_PLLANA		0x00001000 /* bit12 main PLL analog */
#define RT7XX_SLEEPCFG_PLLLDO		0x00002000 /* bit13 main PLL LDO */
#define RT7XX_SLEEPCFG_AUDPLLANA	0x00004000 /* bit14 audio PLL analog */
#define RT7XX_SLEEPCFG_AUDPLLLDO	0x00008000 /* bit15 audio PLL LDO */
#define RT7XX_SLEEPCFG_FRO0_GATE	0x20000000 /* bit29 */
#define RT7XX_SLEEPCFG_FRO2_GATE	0x80000000 /* bit31 */
#define RT7XX_SLEEPCFG_PLL_ALL		(RT7XX_SLEEPCFG_PLLANA | RT7XX_SLEEPCFG_PLLLDO |\
					 RT7XX_SLEEPCFG_AUDPLLANA | RT7XX_SLEEPCFG_AUDPLLLDO)

/* word[1]: PMC.PDSLEEPCFG0 -- voltage-domain keep-alive (deep-sleep power-down) bits.
 * Setting one keeps that VDD2/VDDN domain powered during deep sleep. The
 * mode-selector / override bits (FDSR, DPD, FDPD, V2COMP_DSR, PMICMODE) are not
 * keep-alive domains: they define the low-power mode (with RM Table 352 legality
 * rules) and are driven by the SoC per mode, not composed here.
 */
#define RT7XX_PDCFG0_V2NMED_DSR		0x00000040 /* bit6 VDDN_MEDIA + VDD2_MEDIA */
#define RT7XX_PDCFG0_V2COM_DSR		0x00000080 /* bit7 VDD2_COMMON */
#define RT7XX_PDCFG0_VNCOM_DSR		0x00000100 /* bit8 VDDN_COMMON */
#define RT7XX_PDCFG0_V2DSP_PD		0x00000200 /* bit9 VDD2_DSP (HiFi4) */
#define RT7XX_PDCFG0_V2MIPI_PD		0x00000400 /* bit10 VDD2_MIPI (display) */

/* word[3]/word[4]: PMC.PDSLEEPCFG2 (array) / PDSLEEPCFG3 (periphery) -- 30 main-memory
 * partitions SRAM0..SRAM29 (bits 30-31 reserved), split by domain/core per RM 32.
 */
#define RT7XX_PDCFG_SRAM_VDD2_COM	0x0003FFFF /* SRAM0-17  : Compute / CPU0 RAM */
#define RT7XX_PDCFG_SRAM_VDD1_SENSE	0x3FFC0000 /* SRAM18-29 : Sense / CPU1 RAM */
#define RT7XX_PDCFG_SRAM_ALL		(RT7XX_PDCFG_SRAM_VDD2_COM | RT7XX_PDCFG_SRAM_VDD1_SENSE)

/* word[5]/word[6]: PMC.PDSLEEPCFG4 (array) / PDSLEEPCFG5 (periphery) -- per-IP
 * peripheral memory (SDHC/USB/JPEG/GPU/DMA/caches/DSP TCM/NPU/XSPI/LCD/OCOTP, bits 0-22).
 */
#define RT7XX_PDCFG4_CPU0_CCACHE	0x00000400 /* bit10 CPU0 code cache */
#define RT7XX_PDCFG4_CPU0_SCACHE	0x00000800 /* bit11 CPU0 system cache */
#define RT7XX_PDCFG4_OCOTP		0x00400000 /* bit22 OCOTP shadow RAM */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_NXP_IMXRT7XX_PDCFG_H_ */
