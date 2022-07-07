/*
 * Copyright (c) 2019 Nuclei Limited. All rights reserved.
 * Copyright (c) 2021 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Define Nuclei specific CSR and related definitions.
 *
 * This header contains Nuclei specific definitions.
 * Use arch/riscv/csr.h for RISC-V standard CSR and definitions.
 */

#ifndef ZEPHYR_SOC_RISCV_RISCV_PRIVILEGE_COMMON_NUCLEI_NUCLEI_CSR_H_
#define ZEPHYR_SOC_RISCV_RISCV_PRIVILEGE_COMMON_NUCLEI_NUCLEI_CSR_H_

#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UCODE_OV                    (0x1U)

#define WFE_WFE                     (0x1U)
#define TXEVT_TXEVT                 (0x1U)
#define SLEEPVALUE_SLEEPVALUE       (0x1U)

#define MCOUNTINHIBIT_IR            BIT(2U)
#define MCOUNTINHIBIT_CY            BIT(0U)

#define MILM_CTL_ILM_BPA            (((1ULL << ((__riscv_xlen) - 10U)) - 1U) << 10U)
#define MILM_CTL_ILM_RWECC          BIT(3U)
#define MILM_CTL_ILM_ECC_EXCP_EN    BIT(2U)
#define MILM_CTL_ILM_ECC_EN         BIT(1U)
#define MILM_CTL_ILM_EN             BIT(0U)

#define MDLM_CTL_DLM_BPA            (((1ULL << ((__riscv_xlen) - 10U)) - 1U) << 10U)
#define MDLM_CTL_DLM_RWECC          BIT(3U)
#define MDLM_CTL_DLM_ECC_EXCP_EN    BIT(2U)
#define MDLM_CTL_DLM_ECC_EN         BIT(1U)
#define MDLM_CTL_DLM_EN             BIT(0U)

#define MSUBM_PTYP                  (0x3U << 8U)
#define MSUBM_TYP                   (0x3U << 6U)

#define MDCAUSE_MDCAUSE             (0x3U)

#define MMISC_CTL_NMI_CAUSE_FFF     BIT(9U)
#define MMISC_CTL_MISALIGN          BIT(6U)
#define MMISC_CTL_BPu               BIT(3U)

#define MCACHE_CTL_IC_EN            BIT(0U)
#define MCACHE_CTL_IC_SCPD_MOD      BIT(1U)
#define MCACHE_CTL_IC_ECC_EN        BIT(2U)
#define MCACHE_CTL_IC_ECC_EXCP_EN   BIT(3U)
#define MCACHE_CTL_IC_RWTECC        BIT(4U)
#define MCACHE_CTL_IC_RWDECC        BIT(5U)
#define MCACHE_CTL_DC_EN            BIT(16U)
#define MCACHE_CTL_DC_ECC_EN        BIT(17U)
#define MCACHE_CTL_DC_ECC_EXCP_EN   BIT(18U)
#define MCACHE_CTL_DC_RWTECC        BIT(19U)
#define MCACHE_CTL_DC_RWDECC        BIT(20U)

#define MTVT2_MTVT2EN               BIT(0U)
#define MTVT2_COMMON_CODE_ENTRY     (((1ULL << ((__riscv_xlen) - 2U)) - 1U) << 2U)

#define MCFG_INFO_TEE               BIT(0U)
#define MCFG_INFO_ECC               BIT(1U)
#define MCFG_INFO_CLIC              BIT(2U)
#define MCFG_INFO_PLIC              BIT(3U)
#define MCFG_INFO_FIO               BIT(4U)
#define MCFG_INFO_PPI               BIT(5U)
#define MCFG_INFO_NICE              BIT(6U)
#define MCFG_INFO_ILM               BIT(7U)
#define MCFG_INFO_DLM               BIT(8U)
#define MCFG_INFO_ICACHE            BIT(9U)
#define MCFG_INFO_DCACHE            BIT(10U)

#define MICFG_IC_SET                (0xFU  << 0U)
#define MICFG_IC_WAY                (0x7U  << 4U)
#define MICFG_IC_LSIZE              (0x7U  << 7U)
#define MICFG_IC_ECC                (0x1U  << 10U)
#define MICFG_ILM_SIZE              (0x1FU << 16U)
#define MICFG_ILM_XONLY             (0x1U  << 21U)
#define MICFG_ILM_ECC               (0x1U  << 22U)

#define MDCFG_DC_SET                (0xFU  << 0U)
#define MDCFG_DC_WAY                (0x7U  << 4U)
#define MDCFG_DC_LSIZE              (0x7U  << 7U)
#define MDCFG_DC_ECC                (0x1U  << 10U)
#define MDCFG_DLM_SIZE              (0x1FU << 16U)
#define MDCFG_DLM_ECC               (0x1U  << 21U)

#define MPPICFG_INFO_PPI_SIZE       (0x1FU << 1U)
#define MPPICFG_INFO_PPI_BPA        (((1ULL << ((__riscv_xlen) - 10U)) - 1U) << 10U)

#define MFIOCFG_INFO_FIO_SIZE       (0x1FU << 1)
#define MFIOCFG_INFO_FIO_BPA        (((1ULL << ((__riscv_xlen) - 10U)) - 1U) << 10U)

#define MECC_LOCK_ECC_LOCK          (0x1U)

#define MECC_CODE_CODE              (0x1FFU)
#define MECC_CODE_RAMID             (0x1FU << 16U)
#define MECC_CODE_SRAMID            (0x1FU << 24U)

#define CCM_SUEN_SUEN               (0x1U << 0U)
#define CCM_DATA_DATA               (0x7U << 0U)
#define CCM_COMMAND_COMMAND         (0x1FU << 0U)

#define FRM_RNDMODE_RNE             0x0U
#define FRM_RNDMODE_RTZ             0x1U
#define FRM_RNDMODE_RDN             0x2U
#define FRM_RNDMODE_RUP             0x3U
#define FRM_RNDMODE_RMM             0x4U
#define FRM_RNDMODE_DYN             0x7U

#define FFLAGS_AE_NX                BIT(0U)
#define FFLAGS_AE_UF                BIT(1U)
#define FFLAGS_AE_OF                BIT(2U)
#define FFLAGS_AE_DZ                BIT(3U)
#define FFLAGS_AE_NV                BIT(4U)

#define MSTATUS_SD MSTATUS32_SD
#define SSTATUS_SD SSTATUS32_SD
#define RISCV_PGLEVEL_BITS          10

#define RISCV_PGSHIFT               12U
#define RISCV_PGSIZE                (1U << RISCV_PGSHIFT)

#define CSR_SPMPCFG0                0x1A0U
#define CSR_SPMPCFG1                0x1A1U
#define CSR_SPMPCFG2                0x1A2U
#define CSR_SPMPCFG3                0x1A3U
#define CSR_SPMPADDR0               0x1B0U
#define CSR_SPMPADDR1               0x1B1U
#define CSR_SPMPADDR2               0x1B2U
#define CSR_SPMPADDR3               0x1B3U
#define CSR_SPMPADDR4               0x1B4U
#define CSR_SPMPADDR5               0x1B5U
#define CSR_SPMPADDR6               0x1B6U
#define CSR_SPMPADDR7               0x1B7U
#define CSR_SPMPADDR8               0x1B8U
#define CSR_SPMPADDR9               0x1B9U
#define CSR_SPMPADDR10              0x1BAU
#define CSR_SPMPADDR11              0x1BBU
#define CSR_SPMPADDR12              0x1BCU
#define CSR_SPMPADDR13              0x1BDU
#define CSR_SPMPADDR14              0x1BEU
#define CSR_SPMPADDR15              0x1BFU

#define CSR_JALSNXTI                0x947U
#define CSR_STVT2                   0x948U
#define CSR_PUSHSCAUSE              0x949U
#define CSR_PUSHSEPC                0x94AU

#define CSR_MTVT                    0x307U
#define CSR_MNXTI                   0x345U
#define CSR_MINTSTATUS              0x346U
#define CSR_MSCRATCHCSW             0x348U
#define CSR_MSCRATCHCSWL            0x349U
#define CSR_MCLICBASE               0x350U

#define CSR_UCODE                   0x801U

#define CSR_MILM_CTL                0x7C0U
#define CSR_MDLM_CTL                0x7C1U
#define CSR_MECC_CODE               0x7C2U
#define CSR_MNVEC                   0x7C3U
#define CSR_MSUBM                   0x7C4U
#define CSR_MDCAUSE                 0x7C9U
#define CSR_MCACHE_CTL              0x7CAU
#define CSR_MMISC_CTL               0x7D0U
#define CSR_MSAVESTATUS             0x7D6U
#define CSR_MSAVEEPC1               0x7D7U
#define CSR_MSAVECAUSE1             0x7D8U
#define CSR_MSAVEEPC2               0x7D9U
#define CSR_MSAVECAUSE2             0x7DAU
#define CSR_MSAVEDCAUSE1            0x7DBU
#define CSR_MSAVEDCAUSE2            0x7DCU
#define CSR_MTLB_CTL                0x7DDU
#define CSR_MECC_LOCK               0x7DEU
#define CSR_MFP16MODE               0x7E2U
#define CSR_LSTEPFORC               0x7E9U
#define CSR_PUSHMSUBM               0x7EBU
#define CSR_MTVT2                   0x7ECU
#define CSR_JALMNXTI                0x7EDU
#define CSR_PUSHMCAUSE              0x7EEU
#define CSR_PUSHMEPC                0x7EFU
#define CSR_MPPICFG_INFO            0x7F0U
#define CSR_MFIOCFG_INFO            0x7F1U
#define CSR_MSMPCFG_INFO            0x7F7U
#define CSR_SLEEPVALUE              0x811U
#define CSR_TXEVT                   0x812U
#define CSR_WFE                     0x810U
#define CSR_MICFG_INFO              0xFC0U
#define CSR_MDCFG_INFO              0xFC1U
#define CSR_MCFG_INFO               0xFC2U
#define CSR_MTLBCFG_INFO            0xFC3U

#define CSR_CCM_MBEGINADDR          0x7CBU
#define CSR_CCM_MCOMMAND            0x7CCU
#define CSR_CCM_MDATA               0x7CDU
#define CSR_CCM_SUEN                0x7CEU
#define CSR_CCM_SBEGINADDR          0x5CBU
#define CSR_CCM_SCOMMAND            0x5CCU
#define CSR_CCM_SDATA               0x5CDU
#define CSR_CCM_UBEGINADDR          0x4CBU
#define CSR_CCM_UCOMMAND            0x4CCU
#define CSR_CCM_UDATA               0x4CDU
#define CSR_CCM_FPIPE               0x4CFU

#define CAUSE_MISALIGNED_FETCH      0x0U
#define CAUSE_FAULT_FETCH           0x1U
#define CAUSE_ILLEGAL_INSTRUCTION   0x2U
#define CAUSE_BREAKPOINT            0x3U
#define CAUSE_MISALIGNED_LOAD       0x4U
#define CAUSE_FAULT_LOAD            0x5U
#define CAUSE_MISALIGNED_STORE      0x6U
#define CAUSE_FAULT_STORE           0x7U
#define CAUSE_USER_ECALL            0x8U
#define CAUSE_SUPERVISOR_ECALL      0x9U
#define CAUSE_HYPERVISOR_ECALL      0xaU
#define CAUSE_MACHINE_ECALL         0xbU

#define DCAUSE_FAULT_FETCH_PMP      0x1U
#define DCAUSE_FAULT_FETCH_INST     0x2U

#define DCAUSE_FAULT_LOAD_PMP       0x1U
#define DCAUSE_FAULT_LOAD_INST      0x2U
#define DCAUSE_FAULT_LOAD_NICE      0x3U

#define DCAUSE_FAULT_STORE_PMP      0x1U
#define DCAUSE_FAULT_STORE_INST     0x2U

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RISCV_RISCV_PRIVILEGE_COMMON_NUCLEI_NUCLEI_CSR_H_ */
