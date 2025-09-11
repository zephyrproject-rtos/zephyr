/*
 * Copyright (c) 2021 Andes Technology Corporation
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RISCV_ANDES_V5_SOC_V5_H_
#define __RISCV_ANDES_V5_SOC_V5_H_

/* Control and Status Registers (CSRs) available for Andes V5 SoCs */
#define NDS_MMISC_CTL                0x7D0
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

#endif /* __RISCV_ANDES_V5_SOC_V5_H_ */
