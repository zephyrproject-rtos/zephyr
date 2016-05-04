#ifndef __NIOS2_H__
#define __NIOS2_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2008 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
*                                                                             *
******************************************************************************/

/*
 * This header provides processor specific macros for accessing the Nios2
 * control registers.
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Number of available IRQs in internal interrupt controller.
 */
#define NIOS2_NIRQ 32

/*
 * Macros for accessing select Nios II general-purpose registers.
 */

/* ET (Exception Temporary) register */ 
#define NIOS2_READ_ET(et) \
    do { __asm ("mov %0, et" : "=r" (et) ); } while (0)

#define NIOS2_WRITE_ET(et) \
    do { __asm volatile ("mov et, %z0" : : "rM" (et)); } while (0)

/* SP (Stack Pointer) register */ 
#define NIOS2_READ_SP(sp) \
    do { __asm ("mov %0, sp" : "=r" (sp) ); } while (0)

/*
 * Macros for useful processor instructions.
 */
#define NIOS2_BREAK() \
    do { __asm volatile ("break"); } while (0)

#define NIOS2_REPORT_STACK_OVERFLOW() \
    do { __asm volatile("break 3"); } while (0)

/*
 * Macros for accessing Nios II control registers.
 */
#define NIOS2_READ_STATUS(dest) \
    do { dest = __builtin_rdctl(0); } while (0)

#define NIOS2_WRITE_STATUS(src) \
    do { __builtin_wrctl(0, src); } while (0)

#define NIOS2_READ_ESTATUS(dest) \
    do { dest = __builtin_rdctl(1); } while (0)

#define NIOS2_READ_BSTATUS(dest) \
    do { dest = __builtin_rdctl(2); } while (0)

#define NIOS2_READ_IENABLE(dest) \
    do { dest = __builtin_rdctl(3); } while (0)

#define NIOS2_WRITE_IENABLE(src) \
    do { __builtin_wrctl(3, src); } while (0)

#define NIOS2_READ_IPENDING(dest) \
    do { dest = __builtin_rdctl(4); } while (0)

#define NIOS2_READ_CPUID(dest) \
    do { dest = __builtin_rdctl(5); } while (0)

#define NIOS2_READ_EXCEPTION(dest) \
    do { dest = __builtin_rdctl(7); } while (0)

#define NIOS2_READ_PTEADDR(dest) \
    do { dest = __builtin_rdctl(8); } while (0)

#define NIOS2_WRITE_PTEADDR(src) \
    do { __builtin_wrctl(8, src); } while (0)

#define NIOS2_READ_TLBACC(dest) \
    do { dest = __builtin_rdctl(9); } while (0)

#define NIOS2_WRITE_TLBACC(src) \
    do { __builtin_wrctl(9, src); } while (0)

#define NIOS2_READ_TLBMISC(dest) \
    do { dest = __builtin_rdctl(10); } while (0)

#define NIOS2_WRITE_TLBMISC(src) \
    do { __builtin_wrctl(10, src); } while (0)

#define NIOS2_READ_ECCINJ(dest) \
    do { dest = __builtin_rdctl(11); } while (0)

#define NIOS2_WRITE_ECCINJ(src) \
    do { __builtin_wrctl(11, src); } while (0)

#define NIOS2_READ_BADADDR(dest) \
    do { dest = __builtin_rdctl(12); } while (0)

#define NIOS2_WRITE_CONFIG(src) \
    do { __builtin_wrctl(13, src); } while (0)

#define NIOS2_READ_CONFIG(dest) \
    do { dest = __builtin_rdctl(13); } while (0)

#define NIOS2_WRITE_MPUBASE(src) \
    do { __builtin_wrctl(14, src); } while (0)

#define NIOS2_READ_MPUBASE(dest) \
    do { dest = __builtin_rdctl(14); } while (0)

#define NIOS2_WRITE_MPUACC(src) \
    do { __builtin_wrctl(15, src); } while (0)

#define NIOS2_READ_MPUACC(dest) \
    do { dest = __builtin_rdctl(15); } while (0)

/*
 * Nios II control registers that are always present
 */
#define NIOS2_STATUS   status
#define NIOS2_ESTATUS  estatus
#define NIOS2_BSTATUS  bstatus
#define NIOS2_IENABLE  ienable
#define NIOS2_IPENDING ipending
#define NIOS2_CPUID cpuid

/*
 * Bit masks & offsets for Nios II control registers.
 * The presence and size of a field is sometimes dependent on the Nios II
 * configuration.  Bit masks for every possible field and the maximum size of
 * that field are defined.
 *
 * All bit-masks are expressed relative to the position
 * of the data with a register. To read data that is LSB-
 * aligned, the register read data should be masked, then
 * right-shifted by the designated "OFST" macro value. The
 * opposite should be used for register writes when starting
 * with LSB-aligned data.
 */

/* STATUS, ESTATUS, BSTATUS, and SSTATUS registers */
#define NIOS2_STATUS_PIE_MSK  (0x00000001)
#define NIOS2_STATUS_PIE_OFST (0)
#define NIOS2_STATUS_U_MSK    (0x00000002)
#define NIOS2_STATUS_U_OFST   (1)
#define NIOS2_STATUS_EH_MSK   (0x00000004)
#define NIOS2_STATUS_EH_OFST  (2)
#define NIOS2_STATUS_IH_MSK     (0x00000008)
#define NIOS2_STATUS_IH_OFST    (3)
#define NIOS2_STATUS_IL_MSK     (0x000003f0)
#define NIOS2_STATUS_IL_OFST    (4)
#define NIOS2_STATUS_CRS_MSK    (0x0000fc00)
#define NIOS2_STATUS_CRS_OFST   (10)
#define NIOS2_STATUS_PRS_MSK    (0x003f0000)
#define NIOS2_STATUS_PRS_OFST   (16)
#define NIOS2_STATUS_NMI_MSK    (0x00400000)
#define NIOS2_STATUS_NMI_OFST   (22)
#define NIOS2_STATUS_RSIE_MSK   (0x00800000)
#define NIOS2_STATUS_RSIE_OFST  (23)
#define NIOS2_STATUS_SRS_MSK    (0x80000000)
#define NIOS2_STATUS_SRS_OFST   (31)

/* EXCEPTION register */
#define NIOS2_EXCEPTION_REG_CAUSE_MASK (0x0000007c)
#define NIOS2_EXCEPTION_REG_CAUSE_OFST (2)
#define NIOS2_EXCEPTION_REG_ECCFTL_MASK (0x80000000)
#define NIOS2_EXCEPTION_REG_ECCFTL_OFST (31)

/* PTEADDR (Page Table Entry Address) register */
#define NIOS2_PTEADDR_REG_VPN_OFST 2
#define NIOS2_PTEADDR_REG_VPN_MASK 0x3ffffc
#define NIOS2_PTEADDR_REG_PTBASE_OFST 22
#define NIOS2_PTEADDR_REG_PTBASE_MASK 0xffc00000

/* TLBACC (TLB Access) register */
#define NIOS2_TLBACC_REG_PFN_OFST 0
#define NIOS2_TLBACC_REG_PFN_MASK 0xfffff
#define NIOS2_TLBACC_REG_G_OFST 20
#define NIOS2_TLBACC_REG_G_MASK 0x100000
#define NIOS2_TLBACC_REG_X_OFST 21
#define NIOS2_TLBACC_REG_X_MASK 0x200000
#define NIOS2_TLBACC_REG_W_OFST 22
#define NIOS2_TLBACC_REG_W_MASK 0x400000
#define NIOS2_TLBACC_REG_R_OFST 23
#define NIOS2_TLBACC_REG_R_MASK 0x800000
#define NIOS2_TLBACC_REG_C_OFST 24
#define NIOS2_TLBACC_REG_C_MASK 0x1000000
#define NIOS2_TLBACC_REG_IG_OFST 25
#define NIOS2_TLBACC_REG_IG_MASK 0xfe000000

/* TLBMISC (TLB Miscellaneous) register */
#define NIOS2_TLBMISC_REG_D_OFST 0
#define NIOS2_TLBMISC_REG_D_MASK 0x1
#define NIOS2_TLBMISC_REG_PERM_OFST 1
#define NIOS2_TLBMISC_REG_PERM_MASK 0x2
#define NIOS2_TLBMISC_REG_BAD_OFST 2
#define NIOS2_TLBMISC_REG_BAD_MASK 0x4
#define NIOS2_TLBMISC_REG_DBL_OFST 3
#define NIOS2_TLBMISC_REG_DBL_MASK 0x8
#define NIOS2_TLBMISC_REG_PID_OFST 4
#define NIOS2_TLBMISC_REG_PID_MASK 0x3fff0
#define NIOS2_TLBMISC_REG_WE_OFST 18
#define NIOS2_TLBMISC_REG_WE_MASK 0x40000
#define NIOS2_TLBMISC_REG_RD_OFST 19
#define NIOS2_TLBMISC_REG_RD_MASK 0x80000
#define NIOS2_TLBMISC_REG_WAY_OFST 20
#define NIOS2_TLBMISC_REG_WAY_MASK 0xf00000
#define NIOS2_TLBMISC_REG_EE_OFST 24
#define NIOS2_TLBMISC_REG_EE_MASK 0x1000000

/* ECCINJ (ECC Inject) register */
#define NIOS2_ECCINJ_REG_RF_OFST 0
#define NIOS2_ECCINJ_REG_RF_MASK 0x3
#define NIOS2_ECCINJ_REG_ICTAG_OFST 2
#define NIOS2_ECCINJ_REG_ICTAG_MASK 0xc
#define NIOS2_ECCINJ_REG_ICDAT_OFST 4
#define NIOS2_ECCINJ_REG_ICDAT_MASK 0x30
#define NIOS2_ECCINJ_REG_DCTAG_OFST 6
#define NIOS2_ECCINJ_REG_DCTAG_MASK 0xc0
#define NIOS2_ECCINJ_REG_DCDAT_OFST 8
#define NIOS2_ECCINJ_REG_DCDAT_MASK 0x300
#define NIOS2_ECCINJ_REG_TLB_OFST 10
#define NIOS2_ECCINJ_REG_TLB_MASK 0xc00
#define NIOS2_ECCINJ_REG_DTCM0_OFST 12
#define NIOS2_ECCINJ_REG_DTCM0_MASK 0x3000
#define NIOS2_ECCINJ_REG_DTCM1_OFST 14
#define NIOS2_ECCINJ_REG_DTCM1_MASK 0xc000
#define NIOS2_ECCINJ_REG_DTCM2_OFST 16
#define NIOS2_ECCINJ_REG_DTCM2_MASK 0x30000
#define NIOS2_ECCINJ_REG_DTCM3_OFST 18
#define NIOS2_ECCINJ_REG_DTCM3_MASK 0xc0000

/* CONFIG register */
#define NIOS2_CONFIG_REG_PE_MASK (0x00000001)
#define NIOS2_CONFIG_REG_PE_OFST (0)
#define NIOS2_CONFIG_REG_ANI_MASK (0x00000002)
#define NIOS2_CONFIG_REG_ANI_OFST (1)
#define NIOS2_CONFIG_REG_ECCEN_MASK (0x00000004)
#define NIOS2_CONFIG_REG_ECCEN_OFST (2)
#define NIOS2_CONFIG_REG_ECCEXC_MASK (0x00000008)
#define NIOS2_CONFIG_REG_ECCEXC_OFST (3)

/* MPUBASE (MPU Base Address) Register */
#define NIOS2_MPUBASE_D_MASK         (0x00000001)
#define NIOS2_MPUBASE_D_OFST         (0)
#define NIOS2_MPUBASE_INDEX_MASK     (0x0000003e)
#define NIOS2_MPUBASE_INDEX_OFST     (1)
#define NIOS2_MPUBASE_BASE_ADDR_MASK (0xffffffc0)
#define NIOS2_MPUBASE_BASE_ADDR_OFST (6)

/* MPUACC (MPU Access) Register */
#define NIOS2_MPUACC_LIMIT_MASK (0xffffffc0)
#define NIOS2_MPUACC_LIMIT_OFST (6)
#define NIOS2_MPUACC_MASK_MASK  (0xffffffc0)
#define NIOS2_MPUACC_MASK_OFST  (6)
#define NIOS2_MPUACC_C_MASK     (0x00000020)
#define NIOS2_MPUACC_C_OFST     (5)
#define NIOS2_MPUACC_PERM_MASK  (0x0000001c)
#define NIOS2_MPUACC_PERM_OFST  (2)
#define NIOS2_MPUACC_RD_MASK    (0x00000002)
#define NIOS2_MPUACC_RD_OFST    (1)
#define NIOS2_MPUACC_WR_MASK    (0x00000001)
#define NIOS2_MPUACC_WR_OFST    (0)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NIOS2_H__ */
