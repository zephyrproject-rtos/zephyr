/*
 * Copyright (c) 2019 Nuclei Limited. All rights reserved.
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __RISCV_ENCODING_H__
#define __RISCV_ENCODING_H__

#ifdef __cplusplus
 extern "C" {
#endif

#define WFE_WFE                     (0x1)
#define TXEVT_TXEVT                 (0x1)
#define SLEEPVALUE_SLEEPVALUE       (0x1)

#define MCOUNTINHIBIT_IR            (1<<2)
#define MCOUNTINHIBIT_CY            (1<<0)

#define MILM_CTL_ILM_BPA            (((1ULL<<((__riscv_xlen)-10))-1)<<10)
#define MILM_CTL_ILM_EN             (1<<0)

#define MDLM_CTL_DLM_BPA            (((1ULL<<((__riscv_xlen)-10))-1)<<10)
#define MDLM_CTL_DLM_EN             (1<<0)

#define MSUBM_PTYP                  (0x3<<8)
#define MSUBM_TYP                   (0x3<<6)

#define MDCAUSE_MDCAUSE             (0x3)

#define MMISC_CTL_NMI_CAUSE_FFF     (1<<9)
#define MMISC_CTL_MISALIGN          (1<<6)
#define MMISC_CTL_BPU               (1<<3)

#define MCACHE_CTL_IC_EN            (1<<0)
#define MCACHE_CTL_IC_SCPD_MOD      (1<<1)
#define MCACHE_CTL_DC_EN            (1<<16)

#define MTVT2_MTVT2EN               (1<<0)
#define MTVT2_COMMON_CODE_ENTRY     (((1ULL<<((__riscv_xlen)-2))-1)<<2)

#define MCFG_INFO_TEE               (1<<0)
#define MCFG_INFO_ECC               (1<<1)
#define MCFG_INFO_CLIC              (1<<2)
#define MCFG_INFO_PLIC              (1<<3)
#define MCFG_INFO_FIO               (1<<4)
#define MCFG_INFO_PPI               (1<<5)
#define MCFG_INFO_NICE              (1<<6)
#define MCFG_INFO_ILM               (1<<7)
#define MCFG_INFO_DLM               (1<<8)
#define MCFG_INFO_ICACHE            (1<<9)
#define MCFG_INFO_DCACHE            (1<<10)

#define MICFG_IC_SET                (0xF<<0)
#define MICFG_IC_WAY                (0x7<<4)
#define MICFG_IC_LSIZE              (0x7<<7)
#define MICFG_ILM_SIZE              (0x1F<<16)
#define MICFG_ILM_XONLY             (1<<21)

#define MDCFG_DC_SET                (0xF<<0)
#define MDCFG_DC_WAY                (0x7<<4)
#define MDCFG_DC_LSIZE              (0x7<<7)
#define MDCFG_DLM_SIZE              (0x1F<<16)

#define MPPICFG_INFO_PPI_SIZE       (0x1F<<1)
#define MPPICFG_INFO_PPI_BPA        (((1ULL<<((__riscv_xlen)-10))-1)<<10)

#define MFIOCFG_INFO_FIO_SIZE       (0x1F<<1)
#define MFIOCFG_INFO_FIO_BPA        (((1ULL<<((__riscv_xlen)-10))-1)<<10)

/* === CLIC CSR Registers === */
#define CSR_MTVT                0x307
#define CSR_MNXTI               0x345
#define CSR_MINTSTATUS          0x346
#define CSR_MSCRATCHCSW         0x348
#define CSR_MSCRATCHCSWL        0x349
#define CSR_MCLICBASE           0x350

/* === Nuclei custom CSR Registers === */
#define CSR_MCOUNTINHIBIT       0x320
#define CSR_MILM_CTL            0x7C0
#define CSR_MDLM_CTL            0x7C1
#define CSR_MNVEC               0x7C3
#define CSR_MSUBM               0x7C4
#define CSR_MDCAUSE             0x7C9
#define CSR_MCACHE_CTL          0x7CA
#define CSR_MMISC_CTL           0x7D0
#define CSR_MSAVESTATUS         0x7D6
#define CSR_MSAVEEPC1           0x7D7
#define CSR_MSAVECAUSE1         0x7D8
#define CSR_MSAVEEPC2           0x7D9
#define CSR_MSAVECAUSE2         0x7DA
#define CSR_MSAVEDCAUSE1        0x7DB
#define CSR_MSAVEDCAUSE2        0x7DC
#define CSR_PUSHMSUBM           0x7EB
#define CSR_MTVT2               0x7EC
#define CSR_JALMNXTI            0x7ED
#define CSR_PUSHMCAUSE          0x7EE
#define CSR_PUSHMEPC            0x7EF
#define CSR_MPPICFG_INFO        0x7F0
#define CSR_MFIOCFG_INFO        0x7F1
#define CSR_SLEEPVALUE          0x811
#define CSR_TXEVT               0x812
#define CSR_WFE                 0x810
#define CSR_MICFG_INFO          0xFC0
#define CSR_MDCFG_INFO          0xFC1
#define CSR_MCFG_INFO           0xFC2

#ifdef __cplusplus
}
#endif
#endif /* __RISCV_ENCODING_H__ */
