/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore Core Special Function Register (CSFR) addresses
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_CR_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_CR_H_

/** @cond INTERNAL_HIDDEN */

/* CPU Register */
#define TRICORE_SEGEN             0x1030
#define TRICORE_LCLTEST           0x1040
#define TRICORE_PMA0              0x8100
#define TRICORE_PMA1              0x8104
#define TRICORE_PMA2              0x8108
#define TRICORE_DCON2             0x9000
#define TRICORE_DCON1             0x9008
#define TRICORE_SMACON            0x900C
#define TRICORE_DSTR              0x9010
#define TRICORE_DATR              0x9018
#define TRICORE_DEADD             0x901C
#define TRICORE_DIEAR             0x9020
#define TRICORE_DIETR             0x9024
#define TRICORE_DCON0             0x9040
#define TRICORE_PSTR              0x9200
#define TRICORE_PCON1             0x9204
#define TRICORE_PCON2             0x9208
#define TRICORE_PCON0             0x920C
#define TRICORE_PIEAR             0x9210
#define TRICORE_PIETR             0x9214
#define TRICORE_COMPAT            0x9400
#define TRICORE_FPU_TRAP_CON      0xA000
#define TRICORE_FPU_TRAP_PC       0xA004
#define TRICORE_FPU_TRAP_OPC      0xA008
#define TRICORE_FPU_TRAP_SRC1_L   0xA010
#define TRICORE_FPU_TRAP_SRC1_U   0xA014
#define TRICORE_FPU_TRAP_SRC2_L   0xA018
#define TRICORE_FPU_TRAP_SRC2_U   0xA01C
#define TRICORE_FPU_TRAP_SRC3_L   0xA020
#define TRICORE_FPU_TRAP_SRC3_U   0xA024
#define TRICORE_FPU_SYNC_TRAP_CON 0xA030
#define TRICORE_FPU_SYNC_TRAP_OPC 0xA034

/* Virtualization Control Registers */
#define TRICORE_VCON0        0xB000
#define TRICORE_VCON1        0xB004
#define TRICORE_VCON2        0xB008
#define TRICORE_BHV          0xB010
#define TRICORE_VM0_ICR      0xB100
#define TRICORE_VM1_ICR      0xB104
#define TRICORE_VM2_ICR      0xB108
#define TRICORE_VM3_ICR      0xB10C
#define TRICORE_VM4_ICR      0xB110
#define TRICORE_VM5_ICR      0xB114
#define TRICORE_VM6_ICR      0xB118
#define TRICORE_VM7_ICR      0xB11C
#define TRICORE_VM0_PETHRESH 0xB200
#define TRICORE_VM1_PETHRESH 0xB204
#define TRICORE_VM2_PETHRESH 0xB208
#define TRICORE_VM3_PETHRESH 0xB20C
#define TRICORE_VM4_PETHRESH 0xB210
#define TRICORE_VM5_PETHRESH 0xB214
#define TRICORE_VM6_PETHRESH 0xB218
#define TRICORE_VM7_PETHRESH 0xB21C

/* Temporal Protection System Registers */
#define TRICORE_TPS_CON    0xE400
#define TRICORE_TPS_TIMER0 0xE404
#define TRICORE_TPS_TIMER1 0xE408
#define TRICORE_TPS_TIMER2 0xE40C

/* Trigger Registers */
#define TRICORE_TR0_EVT 0xF000
#define TRICORE_TR0_ADR 0xF004
#define TRICORE_TR1_EVT 0xF008
#define TRICORE_TR1_ADR 0xF00C
#define TRICORE_TR2_EVT 0xF010
#define TRICORE_TR2_ADR 0xF014
#define TRICORE_TR3_EVT 0xF018
#define TRICORE_TR3_ADR 0xF01C
#define TRICORE_TR4_EVT 0xF020
#define TRICORE_TR4_ADR 0xF024
#define TRICORE_TR5_EVT 0xF028
#define TRICORE_TR5_ADR 0xF02C
#define TRICORE_TR6_EVT 0xF030
#define TRICORE_TR6_ADR 0xF034
#define TRICORE_TR7_EVT 0xF038
#define TRICORE_TR7_ADR 0xF03C

/* Performance Counter Registers */
#define TRICORE_CCTRL 0xFC00
#define TRICORE_CCNT  0xFC04
#define TRICORE_ICNT  0xFC08
#define TRICORE_M1CNT 0xFC0C
#define TRICORE_M2CNT 0xFC10
#define TRICORE_M3CNT 0xFC14

/* Debug Registers */
#define TRICORE_DBGSR       0xFD00
#define TRICORE_EXEVT       0xFD08
#define TRICORE_CREVT       0xFD0C
#define TRICORE_SWEVT       0xFD10
#define TRICORE_DBGACT      0xFD14
#define TRICORE_TRIG_ACC    0xFD30
#define TRICORE_DMS         0xFD40
#define TRICORE_DCX         0xFD44
#define TRICORE_DBGTCR      0xFD48
#define TRICORE_DBGCFG      0xFD4C
#define TRICORE_TRCCFG      0xFD50
#define TRICORE_TRCFILT     0xFD54
#define TRICORE_TRCLIM      0xFD58
#define TRICORE_TS16PTCCTRL 0xFD60

/* Core Registers */
#define TRICORE_PCXI    0xFE00
#define TRICORE_PSW     0xFE04
#define TRICORE_PC      0xFE08
#define TRICORE_CORECON 0xFE14
#define TRICORE_CPU_ID  0xFE18
#define TRICORE_CORE_ID 0xFE1C
#define TRICORE_BIV     0xFE20
#define TRICORE_BTV     0xFE24
#define TRICORE_ISP     0xFE28
#define TRICORE_ICR     0xFE2C
#define TRICORE_FCX     0xFE38
#define TRICORE_LCX     0xFE3C
#define TRICORE_SWID    0xFE40
#define TRICORE_CUS_ID  0xFE50
#define TRICORE_BOOTCON 0xFE60
#define TRICORE_LCLCON  0xFE64
#define TRICORE_CCON    0xFE68
#define TRICORE_TCCON   0xFE6C

#define INSERT_FIELD(val, which, fieldval)                                                         \
	(((val) & ~(which)) | ((fieldval) * ((which) & ~((which) - 1))))

#define cr_read(cr)                                                                                \
	({                                                                                         \
		register unsigned long __rv;                                                       \
		__asm__ volatile("mfcr %0, " STRINGIFY(cr) : "=d"(__rv));                          \
		__rv;                                                                              \
	})

#define cr_write(cr, val)                                                                          \
	({                                                                                         \
		unsigned long __wv = (unsigned long)(val);                                         \
		__asm__ volatile("mtcr " STRINGIFY(cr) ", %0\n\tisync" : : "r"(__wv) : "memory");  \
	})

/** @endcond */

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_CR_H_ */
