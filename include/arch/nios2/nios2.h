#ifndef ZEPHYR_INCLUDE_ARCH_NIOS2_NIOS2_H_
#define ZEPHYR_INCLUDE_ARCH_NIOS2_NIOS2_H_

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

/* Size in bits of registers */
#define SYSTEM_BUS_WIDTH 32

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <arch/cpu.h>
#include <sys_io.h>

/*
 * Functions for accessing select Nios II general-purpose registers.
 */

/* ET (Exception Temporary) register */ 
static inline u32_t _nios2_read_et(void)
{
	u32_t et;

	__asm__("mov %0, et" : "=r" (et));
	return et;
}

static inline void _nios2_write_et(u32_t et)
{
	__asm__ volatile("mov et, %z0" : : "rM" (et));
}

static inline u32_t _nios2_read_sp(void)
{
	u32_t sp;

	__asm__("mov %0, sp" : "=r" (sp));
	return sp;
}

/*
 * Functions for useful processor instructions.
 */
static inline void _nios2_break(void)
{
	__asm__ volatile("break");
}

static inline void _nios2_report_stack_overflow(void)
{
	__asm__ volatile("break 3");
}

/*
 * Low-level cache management functions
 */
static inline void _nios2_dcache_addr_flush(void *addr)
{
	__asm__ volatile ("flushda (%0)" :: "r" (addr));
}

static inline void _nios2_dcache_flush(u32_t offset)
{
	__asm__ volatile ("flushd (%0)" :: "r" (offset));
}

static inline void _nios2_icache_flush(u32_t offset)
{
	__asm__ volatile ("flushi %0" :: "r" (offset));
}

static inline void _nios2_pipeline_flush(void)
{
	__asm__ volatile ("flushp");
}

/*
 * Functions for reading/writing control registers
 */

enum nios2_creg {
	NIOS2_CR_STATUS = 0,
	NIOS2_CR_ESTATUS = 1,
	NIOS2_CR_BSTATUS = 2,
	NIOS2_CR_IENABLE = 3,
	NIOS2_CR_IPENDING = 4,
	NIOS2_CR_CPUID = 5,
	/* 6 is reserved */
	NIOS2_CR_EXCEPTION = 7,
	NIOS2_CR_PTEADDR = 8,
	NIOS2_CR_TLBACC = 9,
	NIOS2_CR_TLBMISC = 10,
	NIOS2_CR_ECCINJ = 11,
	NIOS2_CR_BADADDR = 12,
	NIOS2_CR_CONFIG = 13,
	NIOS2_CR_MPUBASE = 14,
	NIOS2_CR_MPUACC = 15
};

/* XXX I would prefer to define these as static inline functions for
 * type checking purposes. However if -O0 is used (i.e. CONFIG_DEBUG is on)
 * we get errors "Control register number must be in range 0-31 for
 * __builtin_rdctl" with the following code:
 *
 * static inline u32_t _nios2_creg_read(enum nios2_creg reg)
 * {
 *          return __builtin_rdctl(reg);
 * }
 *
 * This compiles just fine with -Os.
 */
#define _nios2_creg_read(reg) __builtin_rdctl(reg)
#define _nios2_creg_write(reg, val) __builtin_wrctl(reg, val)

#define _nios2_get_register_address(base, regnum) \
	((void *)(((u8_t *)base) + ((regnum) * (SYSTEM_BUS_WIDTH / 8))))

static inline void _nios2_reg_write(void *base, int regnum, u32_t data)
{
	sys_write32(data, (mm_reg_t)_nios2_get_register_address(base, regnum));
}

static inline u32_t _nios2_reg_read(void *base, int regnum)
{
	return sys_read32((mm_reg_t)_nios2_get_register_address(base, regnum));
}

#endif /* _ASMLANGUAGE */

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

#endif /* ZEPHYR_INCLUDE_ARCH_NIOS2_NIOS2_H_ */
