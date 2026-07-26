/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_VM_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_VM_H_

/*
 * Hexagon Virtual Machine (HVM) interface definitions.
 *
 * These trap numbers, page table bits, and function wrappers define
 * the ABI between a guest OS and the Hexagon H2 hypervisor.  The
 * interface is documented in the Qualcomm Hexagon Virtual Machine
 * Specification and is stable across H2 versions.
 */

#define HEXAGON_VM_vmversion   0
#define HEXAGON_VM_vmrte       1
#define HEXAGON_VM_vmsetvec    2
#define HEXAGON_VM_vmsetie     3
#define HEXAGON_VM_vmgetie     4
#define HEXAGON_VM_vmintop     5
#define HEXAGON_VM_vmclrmap    10
#define HEXAGON_VM_vmnewmap    11
#define HEXAGON_VM_vmcache     13
#define HEXAGON_VM_vmgettime   14
#define HEXAGON_VM_vmsettime   15
#define HEXAGON_VM_vmwait      16
#define HEXAGON_VM_vmyield     17
#define HEXAGON_VM_vmstart     18
#define HEXAGON_VM_vmstop      19
#define HEXAGON_VM_vmvpid      20
#define HEXAGON_VM_vmsetregs   21
#define HEXAGON_VM_vmgetregs   22
#define HEXAGON_VM_vmtimerop   24
#define HEXAGON_VM_vmpmuconfig 25
#define HEXAGON_VM_vmgetinfo   26

/* Special trap0 numbers */
#define HEXAGON_VM_hwconfig    31

/* Page table entry bits */
#define __HVM_PDE_S_4KB   0
#define __HVM_PDE_S_16KB  1
#define __HVM_PDE_S_64KB  2
#define __HVM_PDE_S_256KB 3
#define __HVM_PDE_S_1MB   4
#define __HVM_PDE_S_4MB   5
#define __HVM_PDE_S_16MB  6

#define __HVM_PTE_R (1 << 9)  /* Read */
#define __HVM_PTE_W (1 << 10) /* Write */
#define __HVM_PTE_X (1 << 11) /* Execute */
#define __HVM_PTE_U (1 << 5)  /* User */
#define __HVM_PTE_SHARED (1 << 3) /* Shared: bypass guestmap translation */

/* vmnewmap translation types (from H2 h2_common_asid.h) */
#define VM_TRANS_TYPE_LINEAR  0
#define VM_TRANS_TYPE_TABLE   1

/* vmnewmap TLB invalidation flags */
#define VM_TLB_INVALIDATE_FALSE 0
#define VM_TLB_INVALIDATE_TRUE  1

/* Cache attributes (bits 6-8) */
#define __HEXAGON_C_WB    0x0 /* Write-back, no L2 */
#define __HEXAGON_C_WT    0x1 /* Write-through, no L2 */
#define __HEXAGON_C_DEV   0x4 /* Device register space */
#define __HEXAGON_C_WT_L2 0x5 /* Write-through, with L2 */
#define __HEXAGON_C_UNC   0x6 /* Uncached memory */
#define __HEXAGON_C_WB_L2 0x7 /* Write-back, with L2 */

#define VM_CLOBBERLIST_ABIV2                                                                       \
	"r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14",   \
		"r15", "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0", "m1", "usr", "p0", "p1",    \
		"p2", "p3"

#define VM_CLOBBERLIST VM_CLOBBERLIST_ABIV2

#ifndef _ASMLANGUAGE
#include <stdint.h>

enum VM_CACHE_OPS {
	hvmc_ickill,
	hvmc_dckill,
	hvmc_l2kill,
	hvmc_dccleaninva,
	hvmc_icinva,
	hvmc_idsync,
	hvmc_fetch_cfg
};

enum VM_INT_OPS {
	hvmi_nop,
	hvmi_globen,
	hvmi_globdis,
	hvmi_locen,
	hvmi_locdis,
	hvmi_affinity,
	hvmi_get,
	hvmi_peek,
	hvmi_status,
	hvmi_post,
	hvmi_clear
};

enum VM_TIMER_OPS {
	hvmt_getfreq,
	hvmt_getres,
	hvmt_gettime,
	hvmt_gettimeout,
	hvmt_settimeout,
	hvmt_deltatimeout
};

enum VM_STOP_STATUS {
	VM_STOP_NONE,
	VM_STOP_POWEROFF,
	VM_STOP_HALT,
	VM_STOP_RESTART,
	VM_STOP_MACHINECHECK
};

enum VM_INFO_TYPE {
	vm_info_build_id,
	vm_info_boot_flags,
	vm_info_stlb,
	vm_info_syscfg,
	vm_info_livelock,
	vm_info_rev,
	vm_info_ssbase,
	vm_info_tlb_free,
	vm_info_tlb_size,
	vm_info_physaddr,
	vm_info_tcm_base,
	vm_info_l2mem_size,
	vm_info_tcm_size,
	vm_info_pgsize,
	vm_info_npages,
	vm_info_l2vic_base,
	vm_info_timer_base,
	vm_info_timer_int,
	vm_info_error,
	vm_info_hthreads,
	vm_info_l2tag_size,
	vm_info_l2cfg_base,
	vm_info_clade_base,
	vm_info_cfgbase,
	vm_info_hvx_vlength,
	vm_info_hvx_contexts,
	vm_info_hvx_switch,
	vm_info_max
};

/*
 * VM functions -- use explicit register bindings so the compiler knows
 * which physical registers are read/written by the trap.  Without this,
 * the compiler may keep live values in r0-r3 across the asm block and
 * silently lose them when the asm overwrites those registers.
 */

static inline int32_t hexagon_vm_cache(enum VM_CACHE_OPS op, uint32_t addr, uint32_t len)
{
	register int32_t r0_val __asm__("r0") = (int32_t)op;
	register uint32_t r1_val __asm__("r1") = addr;
	register uint32_t r2_val __asm__("r2") = len;

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val), "+r"(r1_val), "+r"(r2_val)
			 : [trap_id] "i"(HEXAGON_VM_vmcache)
			 : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10",
			   "r11", "r12", "r13", "r14", "r15", "r28", "r31",
			   "sa0", "lc0", "sa1", "lc1", "m0", "m1", "usr",
			   "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline int32_t hexagon_vm_clrmap(void *addr, uint32_t len)
{
	register int32_t r0_val __asm__("r0") = (int32_t)(uintptr_t)addr;
	register uint32_t r1_val __asm__("r1") = len;

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val), "+r"(r1_val)
			 : [trap_id] "i"(HEXAGON_VM_vmclrmap)
			 : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
			   "r10", "r11", "r12", "r13", "r14", "r15", "r28",
			   "r31", "sa0", "lc0", "sa1", "lc1", "m0", "m1",
			   "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline uint32_t hexagon_vm_getie(void)
{
	register uint32_t r0_val __asm__("r0");

	__asm__ volatile("trap1(#%[trap_id])"
			 : "=r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmgetie)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline uint32_t hexagon_vm_getinfo(uint32_t info_type)
{
	register uint32_t r0_val __asm__("r0") = info_type;

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmgetinfo)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline int32_t hexagon_vm_getregs(void *ptr)
{
	register int32_t r0_val __asm__("r0") = (int32_t)(uintptr_t)ptr;

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmgetregs)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline uint64_t hexagon_vm_gettime(void)
{
	register uint32_t r0_val __asm__("r0");
	register uint32_t r1_val __asm__("r1");

	__asm__ volatile("trap1(#%[trap_id])"
			 : "=r"(r0_val), "=r"(r1_val)
			 : [trap_id] "i"(HEXAGON_VM_vmgettime)
			 : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
			   "r10", "r11", "r12", "r13", "r14", "r15", "r28",
			   "r31", "sa0", "lc0", "sa1", "lc1", "m0", "m1",
			   "usr", "p0", "p1", "p2", "p3");
	return ((uint64_t)r1_val << 32) | r0_val;
}

static inline int32_t hexagon_vm_hwconfig(uint32_t arg0, uint32_t arg1, uint32_t arg2,
					  uint32_t arg3)
{
	register int32_t r0_val __asm__("r0") = (int32_t)arg0;
	register uint32_t r1_val __asm__("r1") = arg1;
	register uint32_t r2_val __asm__("r2") = arg2;
	register uint32_t r3_val __asm__("r3") = arg3;

	__asm__ volatile("trap0(#%[trap_id])"
			 : "+r"(r0_val), "+r"(r1_val), "+r"(r2_val), "+r"(r3_val)
			 : [trap_id] "i"(HEXAGON_VM_hwconfig)
			 : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11",
			   "r12", "r13", "r14", "r15", "r28", "r31",
			   "sa0", "lc0", "sa1", "lc1", "m0", "m1", "usr",
			   "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline int32_t hexagon_vm_intop(enum VM_INT_OPS op, int32_t arg1, int32_t arg2, int32_t arg3,
				       int32_t arg4)
{
	register int32_t result __asm__("r0");
	register int32_t r1_val __asm__("r1") = arg1;
	register int32_t r2_val __asm__("r2") = arg2;
	register int32_t r3_val __asm__("r3") = arg3;
	register int32_t r4_val __asm__("r4") = arg4;

	result = op;
	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(result), "+r"(r1_val), "+r"(r2_val), "+r"(r3_val), "+r"(r4_val)
			 : [trap_id] "i"(HEXAGON_VM_vmintop)
			 : "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14",
			   "r15", "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0", "m1", "usr",
			   "p0", "p1", "p2", "p3", "memory");
	return result;
}

static inline int32_t hexagon_vm_newmap(void *addr, uint32_t type, uint32_t tlb_flush_flag)
{
	register int32_t r0_val __asm__("r0") = (int32_t)(uintptr_t)addr;
	register uint32_t r1_val __asm__("r1") = type;
	register uint32_t r2_val __asm__("r2") = tlb_flush_flag;

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val), "+r"(r1_val), "+r"(r2_val)
			 : [trap_id] "i"(HEXAGON_VM_vmnewmap)
			 : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10",
			   "r11", "r12", "r13", "r14", "r15", "r28", "r31",
			   "sa0", "lc0", "sa1", "lc1", "m0", "m1", "usr",
			   "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline void hexagon_vm_rte(void)
{
	__asm__ volatile("trap1(#%[trap_id]);"
			 : : [trap_id] "i"(HEXAGON_VM_vmrte) : VM_CLOBBERLIST);
}

static inline int32_t hexagon_vm_setie(int32_t enable)
{
	register int32_t r0_val __asm__("r0") = enable;

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmsetie)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline int32_t hexagon_vm_setregs(void *ptr)
{
	register int32_t r0_val __asm__("r0") = (int32_t)(uintptr_t)ptr;

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmsetregs)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline int32_t hexagon_vm_settime(uint64_t time)
{
	register uint32_t r0_val __asm__("r0") = (uint32_t)time;
	register uint32_t r1_val __asm__("r1") = (uint32_t)(time >> 32);

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val), "+r"(r1_val)
			 : [trap_id] "i"(HEXAGON_VM_vmsettime)
			 : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
			   "r10", "r11", "r12", "r13", "r14", "r15", "r28",
			   "r31", "sa0", "lc0", "sa1", "lc1", "m0", "m1",
			   "usr", "p0", "p1", "p2", "p3");
	return (int32_t)r0_val;
}

static inline int32_t hexagon_vm_setvec(void *ptr)
{
	register int32_t r0_val __asm__("r0") = (int32_t)(uintptr_t)ptr;

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmsetvec)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline void hexagon_vm_stop(enum VM_STOP_STATUS status)
{
	register int32_t r0_val __asm__("r0") = (int32_t)status;

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmstop)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
}

static inline uint64_t hexagon_vm_timerop(enum VM_TIMER_OPS op, uint32_t dummy, uint64_t timeout)
{
	register int32_t r0_val __asm__("r0") = (int32_t)op;
	register uint32_t r1_val __asm__("r1") = dummy;
	register uint32_t r2_val __asm__("r2") = (uint32_t)timeout;
	register uint32_t r3_val __asm__("r3") = (uint32_t)(timeout >> 32);

	__asm__ volatile("trap1(#%[trap_id])"
			 : "+r"(r0_val), "+r"(r1_val), "+r"(r2_val), "+r"(r3_val)
			 : [trap_id] "i"(HEXAGON_VM_vmtimerop)
			 : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11",
			   "r12", "r13", "r14", "r15", "r28", "r31",
			   "sa0", "lc0", "sa1", "lc1", "m0", "m1", "usr",
			   "p0", "p1", "p2", "p3");
	return ((uint64_t)r1_val << 32) | (uint32_t)r0_val;
}

static inline uint32_t hexagon_vm_version(void)
{
	register uint32_t r0_val __asm__("r0");

	__asm__ volatile("trap1(#%[trap_id])"
			 : "=r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmversion)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline int32_t hexagon_vm_vpid(void)
{
	register int32_t r0_val __asm__("r0");

	__asm__ volatile("trap1(#%[trap_id])"
			 : "=r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmvpid)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline int32_t hexagon_vm_wait(void)
{
	register int32_t r0_val __asm__("r0");

	__asm__ volatile("trap1(#%[trap_id])"
			 : "=r"(r0_val)
			 : [trap_id] "i"(HEXAGON_VM_vmwait)
			 : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
			   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
			   "r28", "r31", "sa0", "lc0", "sa1", "lc1", "m0",
			   "m1", "usr", "p0", "p1", "p2", "p3");
	return r0_val;
}

static inline void hexagon_vm_yield(void)
{
	__asm__ volatile("trap1(#%[trap_id])"
			 :
			 : [trap_id] "i"(HEXAGON_VM_vmyield)
			 : VM_CLOBBERLIST);
}

/* Cache operation wrappers - alphabetically ordered */
static inline int32_t hexagon_vm_cache_dccleaninva(uint32_t addr, uint32_t len)
{
	return hexagon_vm_cache(hvmc_dccleaninva, addr, len);
}

static inline int32_t hexagon_vm_cache_dckill(void)
{
	return hexagon_vm_cache(hvmc_dckill, 0, 0);
}

static inline int32_t hexagon_vm_cache_fetch_cfg(uint32_t val)
{
	return hexagon_vm_cache(hvmc_fetch_cfg, val, 0);
}

static inline int32_t hexagon_vm_cache_icinva(uint32_t addr, uint32_t len)
{
	return hexagon_vm_cache(hvmc_icinva, addr, len);
}

static inline int32_t hexagon_vm_cache_ickill(void)
{
	return hexagon_vm_cache(hvmc_ickill, 0, 0);
}

static inline int32_t hexagon_vm_cache_idsync(uint32_t addr, uint32_t len)
{
	return hexagon_vm_cache(hvmc_idsync, addr, len);
}

static inline int32_t hexagon_vm_cache_l2kill(void)
{
	return hexagon_vm_cache(hvmc_l2kill, 0, 0);
}

/* Interrupt operation wrappers - alphabetically ordered */
static inline int32_t hexagon_vm_intop_affinity(int32_t i, int32_t cpu)
{
	return hexagon_vm_intop(hvmi_affinity, i, cpu, 0, 0);
}

static inline int32_t hexagon_vm_intop_clear(int32_t i)
{
	return hexagon_vm_intop(hvmi_clear, i, 0, 0, 0);
}

static inline int32_t hexagon_vm_intop_get(void)
{
	return hexagon_vm_intop(hvmi_get, 0, 0, 0, 0);
}

static inline int32_t hexagon_vm_intop_globdis(int32_t i)
{
	return hexagon_vm_intop(hvmi_globdis, i, 0, 0, 0);
}

static inline int32_t hexagon_vm_intop_globen(int32_t i)
{
	return hexagon_vm_intop(hvmi_globen, i, 0, 0, 0);
}

static inline int32_t hexagon_vm_intop_locdis(int32_t i)
{
	return hexagon_vm_intop(hvmi_locdis, i, 0, 0, 0);
}

static inline int32_t hexagon_vm_intop_locen(int32_t i)
{
	return hexagon_vm_intop(hvmi_locen, i, 0, 0, 0);
}

static inline int32_t hexagon_vm_intop_nop(void)
{
	return hexagon_vm_intop(hvmi_nop, 0, 0, 0, 0);
}

static inline int32_t hexagon_vm_intop_peek(void)
{
	return hexagon_vm_intop(hvmi_peek, 0, 0, 0, 0);
}

static inline int32_t hexagon_vm_intop_post(int32_t i, int32_t cpu)
{
	return hexagon_vm_intop(hvmi_post, i, cpu, 0, 0);
}

static inline int32_t hexagon_vm_intop_status(int32_t i)
{
	return hexagon_vm_intop(hvmi_status, i, 0, 0, 0);
}
#endif /* _ASMLANGUAGE */

/*
 * Constants for virtual instruction parameters and return values
 */

/* vmsetie arguments */

#define VM_INT_DISABLE 0
#define VM_INT_ENABLE  1

/* vmsetimask arguments */

#define VM_INT_UNMASK 0
#define VM_INT_MASK   1

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_VM_H_ */
