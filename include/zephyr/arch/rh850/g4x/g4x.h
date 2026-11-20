/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _G4X_CPU_H
#define _G4X_CPU_H

/* Basic System Registers ID (regID) */
/* selID = 0 */
#define CPU_REG_EIPC   0
#define CPU_REG_EIPSW  1
#define CPU_REG_FEPC   2
#define CPU_REG_FEPSW  3
#define CPU_REG_PSW    5
#define CPU_REG_FPSR   6
#define CPU_REG_FPEPC  7
#define CPU_REG_FPST   8
#define CPU_REG_FPCC   9
#define CPU_REG_FPCFG  10
#define CPU_REG_EIIC   13
#define CPU_REG_FEIC   14
#define CPU_REG_CTPC   16
#define CPU_REG_CTPSW  17
#define CPU_REG_CTBP   20
#define CPU_REG_SNZCFG 21
#define CPU_REG_EIWR   28
#define CPU_REG_FEWR   29

/* selID = 1 */
#define CPU_REG_SPID     0
#define CPU_REG_SPIDLIST 1
#define CPU_REG_RBASE    2
#define CPU_REG_EBASE    3
#define CPU_REG_INTBP    4
#define CPU_REG_MCTL     5
#define CPU_REG_PID      6
#define CPU_REG_SVLOCK   8
#define CPU_REG_SCCFG    11
#define CPU_REG_SCBP     12

/* selID = 2 */
#define CPU_REG_PEID  0
#define CPU_REG_BMID  1
#define CPU_REG_MEA   6
#define CPU_REG_MEI   8
#define CPU_REG_RBCR0 15
#define CPU_REG_RBCR1 16
#define CPU_REG_RBNR  17
#define CPU_REG_RBIP  18

/* selID = 10 */
#define CPU_REG_FXSR 6
#define CPU_REG_FXXC 12

/* Basic System Registers Selector ID (selID) */
#define CPU_SEL_EIPC   0
#define CPU_SEL_EIPSW  0
#define CPU_SEL_FEPC   0
#define CPU_SEL_FEPSW  0
#define CPU_SEL_PSW    0
#define CPU_SEL_FPSR   0
#define CPU_SEL_FPEPC  0
#define CPU_SEL_FPST   0
#define CPU_SEL_FPCC   0
#define CPU_SEL_FPCFG  0
#define CPU_SEL_EIIC   0
#define CPU_SEL_FEIC   0
#define CPU_SEL_CTPC   0
#define CPU_SEL_CTPSW  0
#define CPU_SEL_CTBP   0
#define CPU_SEL_SNZCFG 0
#define CPU_SEL_EIWR   0
#define CPU_SEL_FEWR   0

#define CPU_SEL_SPID     1
#define CPU_SEL_SPIDLIST 1
#define CPU_SEL_RBASE    1
#define CPU_SEL_EBASE    1
#define CPU_SEL_INTBP    1
#define CPU_SEL_MCTL     1
#define CPU_SEL_PID      1
#define CPU_SEL_SVLOCK   1
#define CPU_SEL_SCCFG    1
#define CPU_SEL_SCBP     1

#define CPU_SEL_PEID  2
#define CPU_SEL_BMID  2
#define CPU_SEL_MEA   2
#define CPU_SEL_MEI   2
#define CPU_SEL_RBCR0 2
#define CPU_SEL_RBCR1 2
#define CPU_SEL_RBNR  2
#define CPU_SEL_RBIP  2

#define CPU_SEL_FXSR 10
#define CPU_SEL_FXXC 10

/* PSW Register Bit Definitions */
/* Bit positions */
#define PSW_Z_Pos   0U  /* Zero flag */
#define PSW_S_Pos   1U  /* Sign flag */
#define PSW_OV_Pos  2U  /* Overflow flag */
#define PSW_CY_Pos  3U  /* Carry flag */
#define PSW_SAT_Pos 4U  /* Saturated flag */
#define PSW_ID_Pos  5U  /* Interrupt enable (0: enabled) */
#define PSW_EP_Pos  6U  /* Exception in process (0: interrupt processed) */
#define PSW_NP_Pos  7U  /* FE exception enable (0: enabled) */
#define PSW_DBG_Pos 9U  /* Debug, bits 9–11 */
#define PSW_EBV_Pos 15U /* Exception base (0: RBASE, 1: EBASE) */
#define PSW_CU0_Pos 16U /* FPU / coprocessor 0 */
#define PSW_CU1_Pos 17U /* Reserved / coprocessor 1 */
#define PSW_CU2_Pos 18U /* Reserved / coprocessor 2 */
#define PSW_UM_Pos  30U /* User mode (0: supervisor, 1: user) */

/* Bit masks */
#define PSW_Z   (1UL << PSW_Z_Pos)
#define PSW_S   (1UL << PSW_S_Pos)
#define PSW_OV  (1UL << PSW_OV_Pos)
#define PSW_CY  (1UL << PSW_CY_Pos)
#define PSW_SAT (1UL << PSW_SAT_Pos)
#define PSW_ID  (1UL << PSW_ID_Pos)
#define PSW_EP  (1UL << PSW_EP_Pos)
#define PSW_NP  (1UL << PSW_NP_Pos)
#define PSW_DBG (0x7UL << PSW_DBG_Pos)
#define PSW_EBV (1UL << PSW_EBV_Pos)
#define PSW_CU0 (1UL << PSW_CU0_Pos)
#define PSW_CU1 (1UL << PSW_CU1_Pos)
#define PSW_CU2 (1UL << PSW_CU2_Pos)
#define PSW_UM  (1UL << PSW_UM_Pos)

/* FPU Register Bit Definitions */
#define FPSR_FS_Pos 17U
#define FPSR_FS     (1UL << FPSR_FS_Pos)

/* FXU Register Bit Definitions */
#define FXSR_FS_Pos 17U
#define FXSR_FS     (1UL << FXSR_FS_Pos)

#endif /* _G4X_CPU_H */
