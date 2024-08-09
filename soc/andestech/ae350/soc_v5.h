/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RISCV_ANDES_V5_SOC_V5_H_
#define __RISCV_ANDES_V5_SOC_V5_H_

/* Control and Status Registers (CSRs) available for Andes V5 SoCs */
#define NDS_MMISC_CTL                0x7D0
#define NDS_MSAVESTATUS              0x7D6
#define NDS_MCACHE_CTL               0x7CA
#define NDS_MXSTATUS                 0x7C4
#define NDS_MCCTLBEGINADDR           0x7CB
#define NDS_MCCTLCOMMAND             0x7CC
#define NDS_MCCTLDATA                0x7CD
#define NDS_UITB                     0x800
#define NDS_UCODE                    0x801
#define NDS_UCCTLBEGINADDR           0x80B
#define NDS_UCCTLCOMMAND             0x80C
#define NDS_MICM_CFG                 0xFC0
#define NDS_MDCM_CFG                 0xFC1
#define NDS_MMSC_CFG                 0xFC2
#define NDS_MMSC_CFG2                0xFC3
#define NDS_MRVARCH_CFG              0xFCA

/* Control and Status Registers (CSRs) available for Andes V5 PMA */
#define NDS_PMACFG0                  0xBC0
#define NDS_PMACFG1                  0xBC1
#define NDS_PMACFG2                  0xBC2
#define NDS_PMACFG3                  0xBC3
#define NDS_PMAADDR0                 0xBD0
#define NDS_PMAADDR1                 0xBD1
#define NDS_PMAADDR2                 0xBD2
#define NDS_PMAADDR3                 0xBD3
#define NDS_PMAADDR4                 0xBD4
#define NDS_PMAADDR5                 0xBD5
#define NDS_PMAADDR6                 0xBD6
#define NDS_PMAADDR7                 0xBD7
#define NDS_PMAADDR8                 0xBD8
#define NDS_PMAADDR9                 0xBD9
#define NDS_PMAADDR10                0xBDA
#define NDS_PMAADDR11                0xBDB
#define NDS_PMAADDR12                0xBDC
#define NDS_PMAADDR13                0xBDD
#define NDS_PMAADDR14                0xBDE
#define NDS_PMAADDR15                0xBDF

#endif /* __RISCV_ANDES_V5_SOC_V5_H_ */
