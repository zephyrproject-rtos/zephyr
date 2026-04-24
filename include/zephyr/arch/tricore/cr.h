/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_CR_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_CR_H_

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

/* Data Protection Registers */
#define TRICORE_DPR0_L  0xC000
#define TRICORE_DPR0_U  0xC004
#define TRICORE_DPR1_L  0xC008
#define TRICORE_DPR1_U  0xC00C
#define TRICORE_DPR2_L  0xC010
#define TRICORE_DPR2_U  0xC014
#define TRICORE_DPR3_L  0xC018
#define TRICORE_DPR3_U  0xC01C
#define TRICORE_DPR4_L  0xC020
#define TRICORE_DPR4_U  0xC024
#define TRICORE_DPR5_L  0xC028
#define TRICORE_DPR5_U  0xC02C
#define TRICORE_DPR6_L  0xC030
#define TRICORE_DPR6_U  0xC034
#define TRICORE_DPR7_L  0xC038
#define TRICORE_DPR7_U  0xC03C
#define TRICORE_DPR8_L  0xC040
#define TRICORE_DPR8_U  0xC044
#define TRICORE_DPR9_L  0xC048
#define TRICORE_DPR9_U  0xC04C
#define TRICORE_DPR10_L 0xC050
#define TRICORE_DPR10_U 0xC054
#define TRICORE_DPR11_L 0xC058
#define TRICORE_DPR11_U 0xC05C
#define TRICORE_DPR12_L 0xC060
#define TRICORE_DPR12_U 0xC064
#define TRICORE_DPR13_L 0xC068
#define TRICORE_DPR13_U 0xC06C
#define TRICORE_DPR14_L 0xC070
#define TRICORE_DPR14_U 0xC074
#define TRICORE_DPR15_L 0xC078
#define TRICORE_DPR15_U 0xC07C
#define TRICORE_DPR16_L 0xC080
#define TRICORE_DPR16_U 0xC084
#define TRICORE_DPR17_L 0xC088
#define TRICORE_DPR17_U 0xC08C
#define TRICORE_DPR18_L 0xC090
#define TRICORE_DPR18_U 0xC094
#define TRICORE_DPR19_L 0xC098
#define TRICORE_DPR19_U 0xC09C
#define TRICORE_DPR20_L 0xC0A0
#define TRICORE_DPR20_U 0xC0A4
#define TRICORE_DPR21_L 0xC0A8
#define TRICORE_DPR21_U 0xC0AC
#define TRICORE_DPR22_L 0xC0B0
#define TRICORE_DPR22_U 0xC0B4
#define TRICORE_DPR23_L 0xC0B8
#define TRICORE_DPR23_U 0xC0BC

/* Code Protection Registers */
#define TRICORE_CPR0_L  0xD000
#define TRICORE_CPR0_U  0xD004
#define TRICORE_CPR1_L  0xD008
#define TRICORE_CPR1_U  0xD00C
#define TRICORE_CPR2_L  0xD010
#define TRICORE_CPR2_U  0xD014
#define TRICORE_CPR3_L  0xD018
#define TRICORE_CPR3_U  0xD01C
#define TRICORE_CPR4_L  0xD020
#define TRICORE_CPR4_U  0xD024
#define TRICORE_CPR5_L  0xD028
#define TRICORE_CPR5_U  0xD02C
#define TRICORE_CPR6_L  0xD030
#define TRICORE_CPR6_U  0xD034
#define TRICORE_CPR7_L  0xD038
#define TRICORE_CPR7_U  0xD03C
#define TRICORE_CPR8_L  0xD040
#define TRICORE_CPR8_U  0xD044
#define TRICORE_CPR9_L  0xD048
#define TRICORE_CPR9_U  0xD04C
#define TRICORE_CPR10_L 0xD050
#define TRICORE_CPR10_U 0xD054
#define TRICORE_CPR11_L 0xD058
#define TRICORE_CPR11_U 0xD05C
#define TRICORE_CPR12_L 0xD060
#define TRICORE_CPR12_U 0xD064
#define TRICORE_CPR13_L 0xD068
#define TRICORE_CPR13_U 0xD06C
#define TRICORE_CPR14_L 0xD070
#define TRICORE_CPR14_U 0xD074
#define TRICORE_CPR15_L 0xD078
#define TRICORE_CPR15_U 0xD07C

/* Protection Enable Registers */
#define TRICORE_CPXE_0 0xE000
#define TRICORE_CPXE_1 0xE004
#define TRICORE_CPXE_2 0xE008
#define TRICORE_CPXE_3 0xE00C
#define TRICORE_DPRE_0 0xE010
#define TRICORE_DPRE_1 0xE014
#define TRICORE_DPRE_2 0xE018
#define TRICORE_DPRE_3 0xE01C
#define TRICORE_DPWE_0 0xE020
#define TRICORE_DPWE_1 0xE024
#define TRICORE_DPWE_2 0xE028
#define TRICORE_DPWE_3 0xE02C
#define TRICORE_CPXE_4 0xE040
#define TRICORE_CPXE_5 0xE044
#define TRICORE_CPXE_6 0xE048
#define TRICORE_CPXE_7 0xE04C
#define TRICORE_DPRE_4 0xE050
#define TRICORE_DPRE_5 0xE054
#define TRICORE_DPRE_6 0xE058
#define TRICORE_DPRE_7 0xE05C
#define TRICORE_DPWE_4 0xE060
#define TRICORE_DPWE_5 0xE064
#define TRICORE_DPWE_6 0xE068
#define TRICORE_DPWE_7 0xE06C

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
#define TRICORE_PPRS    0xFE34
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

#endif /* ZEPHYR_INCLUDE_ARCH_TRICORE_CR_H_ */
