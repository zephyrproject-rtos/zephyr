/* SPDX-License-Identifier: MIT */

/******************************************************************************
 * arch-arm.h
 *
 * Guest OS interface to ARM Xen.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright 2011 (C) Citrix Systems
 */

#ifndef __XEN_PUBLIC_ARCH_ARM_H__
#define __XEN_PUBLIC_ARCH_ARM_H__

#include <zephyr/kernel.h>

/*
 * `incontents 50 arm_abi Hypercall Calling Convention
 *
 * A hypercall is issued using the ARM HVC instruction.
 *
 * A hypercall can take up to 5 arguments. These are passed in
 * registers, the first argument in x0/r0 (for arm64/arm32 guests
 * respectively irrespective of whether the underlying hypervisor is
 * 32- or 64-bit), the second argument in x1/r1, the third in x2/r2,
 * the forth in x3/r3 and the fifth in x4/r4.
 *
 * The hypercall number is passed in r12 (arm) or x16 (arm64). In both
 * cases the relevant ARM procedure calling convention specifies this
 * is an inter-procedure-call scratch register (e.g. for use in linker
 * stubs). This use does not conflict with use during a hypercall.
 *
 * The HVC ISS must contain a Xen specific TAG: XEN_HYPERCALL_TAG.
 *
 * The return value is in x0/r0.
 *
 * The hypercall will clobber x16/r12 and the argument registers used
 * by that hypercall (except r0 which is the return value) i.e. in
 * addition to x16/r12 a 2 argument hypercall will clobber x1/r1 and a
 * 4 argument hypercall will clobber x1/r1, x2/r2 and x3/r3.
 *
 * Parameter structs passed to hypercalls are laid out according to
 * the Procedure Call Standard for the ARM Architecture (AAPCS, AKA
 * EABI) and Procedure Call Standard for the ARM 64-bit Architecture
 * (AAPCS64). Where there is a conflict the 64-bit standard should be
 * used regardless of guest type. Structures which are passed as
 * hypercall arguments are always little endian.
 *
 * All memory which is shared with other entities in the system
 * (including the hypervisor and other guests) must reside in memory
 * which is mapped as Normal Inner Write-Back Outer Write-Back Inner-Shareable.
 * This applies to:
 *  - hypercall arguments passed via a pointer to guest memory.
 *  - memory shared via the grant table mechanism (including PV I/O
 *    rings etc).
 *  - memory shared with the hypervisor (struct shared_info, struct
 *    vcpu_info, the grant table, etc).
 *
 * Any cache allocation hints are acceptable.
 */

/*
 * `incontents 55 arm_hcall Supported Hypercalls
 *
 * Xen on ARM makes extensive use of hardware facilities and therefore
 * only a subset of the potential hypercalls are required.
 *
 * Since ARM uses second stage paging any machine/physical addresses
 * passed to hypercalls are Guest Physical Addresses (Intermediate
 * Physical Addresses) unless otherwise noted.
 *
 * The following hypercalls (and sub operations) are supported on the
 * ARM platform. Other hypercalls should be considered
 * unavailable/unsupported.
 *
 *  HYPERVISOR_memory_op
 *   All generic sub-operations
 *
 *  HYPERVISOR_domctl
 *   All generic sub-operations, with the exception of:
 *    * XEN_DOMCTL_irq_permission (not yet implemented)
 *
 *  HYPERVISOR_sched_op
 *   All generic sub-operations, with the exception of:
 *    * SCHEDOP_block -- prefer wfi hardware instruction
 *
 *  HYPERVISOR_console_io
 *   All generic sub-operations
 *
 *  HYPERVISOR_xen_version
 *   All generic sub-operations
 *
 *  HYPERVISOR_event_channel_op
 *   All generic sub-operations
 *
 *  HYPERVISOR_physdev_op
 *   No sub-operations are currently supported
 *
 *  HYPERVISOR_sysctl
 *   All generic sub-operations, with the exception of:
 *    * XEN_SYSCTL_page_offline_op
 *    * XEN_SYSCTL_get_pmstat
 *    * XEN_SYSCTL_pm_op
 *
 *  HYPERVISOR_hvm_op
 *   Exactly these sub-operations are supported:
 *    * HVMOP_set_param
 *    * HVMOP_get_param
 *
 *  HYPERVISOR_grant_table_op
 *   All generic sub-operations
 *
 *  HYPERVISOR_vcpu_op
 *   Exactly these sub-operations are supported:
 *    * VCPUOP_register_vcpu_info
 *    * VCPUOP_register_runstate_memory_area
 *
 *
 * Other notes on the ARM ABI:
 *
 * - struct start_info is not exported to ARM guests.
 *
 * - struct shared_info is mapped by ARM guests using the
 *   HYPERVISOR_memory_op sub-op XENMEM_add_to_physmap, passing
 *   XENMAPSPACE_shared_info as space parameter.
 *
 * - All the per-cpu struct vcpu_info are mapped by ARM guests using the
 *   HYPERVISOR_vcpu_op sub-op VCPUOP_register_vcpu_info, including cpu0
 *   struct vcpu_info.
 *
 * - The grant table is mapped using the HYPERVISOR_memory_op sub-op
 *   XENMEM_add_to_physmap, passing XENMAPSPACE_grant_table as space
 *   parameter. The memory range specified under the Xen compatible
 *   hypervisor node on device tree can be used as target gpfn for the
 *   mapping.
 *
 * - Xenstore is initialized by using the two hvm_params
 *   HVM_PARAM_STORE_PFN and HVM_PARAM_STORE_EVTCHN. They can be read
 *   with the HYPERVISOR_hvm_op sub-op HVMOP_get_param.
 *
 * - The paravirtualized console is initialized by using the two
 *   hvm_params HVM_PARAM_CONSOLE_PFN and HVM_PARAM_CONSOLE_EVTCHN. They
 *   can be read with the HYPERVISOR_hvm_op sub-op HVMOP_get_param.
 *
 * - Event channel notifications are delivered using the percpu GIC
 *   interrupt specified under the Xen compatible hypervisor node on
 *   device tree.
 *
 * - The device tree Xen compatible node is fully described under Linux
 *   at Documentation/devicetree/bindings/arm/xen.txt.
 */

#define XEN_HYPERCALL_TAG 0XEA1

#define  int64_aligned_t  int64_t __aligned(8)
#define uint64_aligned_t uint64_t __aligned(8)

#ifndef __ASSEMBLY__
#define ___DEFINE_XEN_GUEST_HANDLE(name, type)		\
	typedef union { type *p; unsigned long q; }	\
		__guest_handle_ ## name;		\
	typedef union { type *p; uint64_aligned_t q; }	\
		__guest_handle_64_ ## name

/*
 * XEN_GUEST_HANDLE represents a guest pointer, when passed as a field
 * in a struct in memory. On ARM is always 8 bytes sizes and 8 bytes
 * aligned.
 * XEN_GUEST_HANDLE_PARAM represents a guest pointer, when passed as an
 * hypercall argument. It is 4 bytes on aarch32 and 8 bytes on aarch64.
 */
#define __DEFINE_XEN_GUEST_HANDLE(name, type)	\
	___DEFINE_XEN_GUEST_HANDLE(name, type);	\
	___DEFINE_XEN_GUEST_HANDLE(const_##name, const type)
#define DEFINE_XEN_GUEST_HANDLE(name)		__DEFINE_XEN_GUEST_HANDLE(name, name)
#define __XEN_GUEST_HANDLE(name)		__guest_handle_64_ ## name
#define XEN_GUEST_HANDLE(name)			__XEN_GUEST_HANDLE(name)
#define XEN_GUEST_HANDLE_PARAM(name)		__guest_handle_ ## name
#define set_xen_guest_handle_raw(hnd, val)		\
	do {						\
		__typeof__(&(hnd)) _sxghr_tmp = &(hnd);	\
		_sxghr_tmp->q = 0;			\
		_sxghr_tmp->p = val;			\
	} while (0)
#define set_xen_guest_handle(hnd, val) set_xen_guest_handle_raw(hnd, val)

typedef uint64_t xen_pfn_t;
#define PRI_xen_pfn PRIx64
#define PRIu_xen_pfn PRIu64

typedef uint64_t xen_ulong_t;
#define PRI_xen_ulong PRIx64

/*
 * Maximum number of virtual CPUs in legacy multi-processor guests.
 * Only one. All other VCPUS must use VCPUOP_register_vcpu_info.
 */
#define XEN_LEGACY_MAX_VCPUS 1

struct arch_vcpu_info {
};
typedef struct arch_vcpu_info arch_vcpu_info_t;

struct arch_shared_info {
};
typedef struct arch_shared_info arch_shared_info_t;
typedef uint64_t xen_callback_t;

#endif /* __ASSEMBLY__ */

#endif /* __XEN_PUBLIC_ARCH_ARM_H__ */
