/*
 * Copyright (c) 2023 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_PMU_H
#define ENE_KB1200_PMU_H

/**
 *  Structure type to access Power Management Unit (PMU).
 */
struct pmu_regs {
	volatile uint8_t PMUIDLE;      /*IDLE wakeup by Interrupt Register */
	volatile uint8_t Reserved0[3]; /*Reserved */
	volatile uint32_t PMUSTOP;     /*STOP Wakeup Source Register */
	volatile uint8_t PMUSTOPC;     /*STOP Control Register */
	volatile uint8_t Reserved1[3]; /*Reserved */
	volatile uint8_t PMUCTRL;      /*Control Register */
	volatile uint8_t Reserved2[3]; /*Reserved */
	volatile uint8_t PMUSTAF;      /*Status Flag */
	volatile uint8_t Reserved3[3]; /*Reserved */
};

/* STOP Wakeup Source */
#define PMU_STOP_WU_GPTD   0x00000001
#define PMU_STOP_WU_VC0    0x00000002
#define PMU_STOP_WU_VC1    0x00000004
#define PMU_STOP_WU_IKB    0x00000010
#define PMU_STOP_WU_WDT    0x00000100
#define PMU_STOP_WU_HIBTMR 0x00000400
#define PMU_STOP_WU_eSPI   0x00010000
#define PMU_STOP_WU_SPIS   0x00010000
#define PMU_STOP_WU_I2CD32 0x00020000
#define PMU_STOP_WU_EDI32  0x00040000
#define PMU_STOP_WU_SWD    0x00080000
#define PMU_STOP_WU_ITIM   0x00100000
#define PMU_STOP_WU_I2CS0  0x01000000
#define PMU_STOP_WU_I2CS1  0x02000000
#define PMU_STOP_WU_I2CS2  0x04000000
#define PMU_STOP_WU_I2CS3  0x08000000

#define PMU_IDLE_WU_ENABLE 0x00000001

#endif /* ENE_KB1200_PMU_H */
