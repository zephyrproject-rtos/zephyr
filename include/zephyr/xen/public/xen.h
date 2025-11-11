/* SPDX-License-Identifier: MIT */

/******************************************************************************
 * xen.h
 *
 * Guest OS interface to Xen.
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
 * Copyright (c) 2004, K A Fraser
 */

#ifndef __XEN_PUBLIC_XEN_H__
#define __XEN_PUBLIC_XEN_H__

#if defined(CONFIG_ARM64)
#include "arch-arm.h"
#else
#error "Unsupported architecture"
#endif

#ifndef __ASSEMBLY__
/* Guest handles for primitive C types. */
DEFINE_XEN_GUEST_HANDLE(char);
__DEFINE_XEN_GUEST_HANDLE(uchar, unsigned char);
DEFINE_XEN_GUEST_HANDLE(int);
__DEFINE_XEN_GUEST_HANDLE(uint,  unsigned int);
#if CONFIG_XEN_INTERFACE_VERSION < 0x00040300
DEFINE_XEN_GUEST_HANDLE(long);
__DEFINE_XEN_GUEST_HANDLE(ulong, unsigned long);
#endif
DEFINE_XEN_GUEST_HANDLE(void);

DEFINE_XEN_GUEST_HANDLE(uint8_t);
DEFINE_XEN_GUEST_HANDLE(uint64_t);
DEFINE_XEN_GUEST_HANDLE(xen_pfn_t);
DEFINE_XEN_GUEST_HANDLE(xen_ulong_t);

/* Define a variable length array (depends on compiler). */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define XEN_FLEX_ARRAY_DIM
#elif defined(__GNUC__)
#define XEN_FLEX_ARRAY_DIM 0
#else
#define XEN_FLEX_ARRAY_DIM 1 /* variable size */
#endif

/* Turn a plain number into a C unsigned (long (long)) constant. */
#define __xen_mk_uint(x)	x ## U
#define __xen_mk_ulong(x)	x ## UL
#ifndef __xen_mk_ullong
#define __xen_mk_ullong(x)	x ## ULL
#endif
#define xen_mk_uint(x)		__xen_mk_uint(x)
#define xen_mk_ulong(x)		__xen_mk_ulong(x)
#define xen_mk_ullong(x)	__xen_mk_ullong(x)

#else

/* In assembly code we cannot use C numeric constant suffixes. */
#define xen_mk_uint(x)		x
#define xen_mk_ulong(x)		x
#define xen_mk_ullong(x)	x

#endif

/*
 * HYPERCALLS
 */

/* `incontents 100 hcalls List of hypercalls
 * ` enum hypercall_num { // __HYPERVISOR_* => HYPERVISOR_*()
 */

#define __HYPERVISOR_set_trap_table			0
#define __HYPERVISOR_mmu_update				1
#define __HYPERVISOR_set_gdt				2
#define __HYPERVISOR_stack_switch			3
#define __HYPERVISOR_set_callbacks			4
#define __HYPERVISOR_fpu_taskswitch			5

/* compat since 0x00030101 */
#define __HYPERVISOR_sched_op_compat			6
#define __HYPERVISOR_platform_op			7
#define __HYPERVISOR_set_debugreg			8
#define __HYPERVISOR_get_debugreg			9
#define __HYPERVISOR_update_descriptor			10
#define __HYPERVISOR_memory_op				12
#define __HYPERVISOR_multicall				13
#define __HYPERVISOR_update_va_mapping			14
#define __HYPERVISOR_set_timer_op			15

/* compat since 0x00030202 */
#define __HYPERVISOR_event_channel_op_compat		16
#define __HYPERVISOR_xen_version			17
#define __HYPERVISOR_console_io				18

/* compat since 0x00030202 */
#define __HYPERVISOR_physdev_op_compat			19
#define __HYPERVISOR_grant_table_op			20
#define __HYPERVISOR_vm_assist				21
#define __HYPERVISOR_update_va_mapping_otherdomain	22

/* x86 only */
#define __HYPERVISOR_iret				23
#define __HYPERVISOR_vcpu_op				24

/* x86/64 only */
#define __HYPERVISOR_set_segment_base			25
#define __HYPERVISOR_mmuext_op				26
#define __HYPERVISOR_xsm_op				27
#define __HYPERVISOR_nmi_op				28
#define __HYPERVISOR_sched_op				29
#define __HYPERVISOR_callback_op			30
#define __HYPERVISOR_xenoprof_op			31
#define __HYPERVISOR_event_channel_op			32
#define __HYPERVISOR_physdev_op				33
#define __HYPERVISOR_hvm_op				34
#define __HYPERVISOR_sysctl				35
#define __HYPERVISOR_domctl				36
#define __HYPERVISOR_kexec_op				37
#define __HYPERVISOR_tmem_op				38
#define __HYPERVISOR_argo_op				39
#define __HYPERVISOR_xenpmu_op				40
#define __HYPERVISOR_dm_op				41
#define __HYPERVISOR_hypfs_op				42

/*
 * ` int
 * ` HYPERVISOR_console_io(unsigned int cmd,
 * `					   unsigned int count,
 * `					   char buffer[]);
 *
 * @cmd: Command (see below)
 * @count: Size of the buffer to read/write
 * @buffer: Pointer in the guest memory
 *
 * List of commands:
 *
 *  * CONSOLEIO_write: Write the buffer to Xen console.
 *      For the hardware domain, all the characters in the buffer will
 *      be written. Characters will be printed directly to the console.
 *      For all the other domains, only the printable characters will be
 *      written. Characters may be buffered until a newline (i.e '\n') is
 *      found.
 *      @return 0 on success, otherwise return an error code.
 *  * CONSOLEIO_read: Attempts to read up to @count characters from Xen
 *      console. The maximum buffer size (i.e. @count) supported is 2GB.
 *      @return the number of characters read on success, otherwise return
 *      an error code.
 */
#define CONSOLEIO_write		0
#define CONSOLEIO_read		1

/* Domain ids >= DOMID_FIRST_RESERVED cannot be used for ordinary domains. */
#define DOMID_FIRST_RESERVED	xen_mk_uint(0x7FF0)

/* DOMID_SELF is used in certain contexts to refer to oneself. */
#define DOMID_SELF		xen_mk_uint(0x7FF0)

/*
 * DOMID_IO is used to restrict page-table updates to mapping I/O memory.
 * Although no Foreign Domain need be specified to map I/O pages, DOMID_IO
 * is useful to ensure that no mappings to the OS's own heap are accidentally
 * installed. (e.g., in Linux this could cause havoc as reference counts
 * aren't adjusted on the I/O-mapping code path).
 * This only makes sense as HYPERVISOR_mmu_update()'s and
 * HYPERVISOR_update_va_mapping_otherdomain()'s "foreigndom" argument. For
 * HYPERVISOR_mmu_update() context it can be specified by any calling domain,
 * otherwise it's only permitted if the caller is privileged.
 */
#define DOMID_IO		xen_mk_uint(0x7FF1)

/*
 * DOMID_XEN is used to allow privileged domains to map restricted parts of
 * Xen's heap space (e.g., the machine_to_phys table).
 * This only makes sense as
 * - HYPERVISOR_mmu_update()'s, HYPERVISOR_mmuext_op()'s, or
 *   HYPERVISOR_update_va_mapping_otherdomain()'s "foreigndom" argument,
 * - with XENMAPSPACE_gmfn_foreign,
 * and is only permitted if the caller is privileged.
 */
#define DOMID_XEN		xen_mk_uint(0x7FF2)

/*
 * DOMID_COW is used as the owner of sharable pages.
 */
#define DOMID_COW		xen_mk_uint(0x7FF3)

/* DOMID_INVALID is used to identify pages with unknown owner. */
#define DOMID_INVALID		xen_mk_uint(0x7FF4)

/* Idle domain. */
#define DOMID_IDLE		xen_mk_uint(0x7FFF)

/* Mask for valid domain id values */
#define DOMID_MASK		xen_mk_uint(0x7FFF)

#ifndef __ASSEMBLY__

typedef uint16_t domid_t;

#if CONFIG_XEN_INTERFACE_VERSION < 0x00040400
/*
 * Event channel endpoints per domain (when using the 2-level ABI):
 *  1024 if a long is 32 bits; 4096 if a long is 64 bits.
 */
#define NR_EVENT_CHANNELS EVTCHN_2L_NR_CHANNELS
#endif

struct vcpu_time_info {
	/*
	 * Updates to the following values are preceded and followed by an
	 * increment of 'version'. The guest can therefore detect updates by
	 * looking for changes to 'version'. If the least-significant bit of
	 * the version number is set then an update is in progress and the
	 * guest must wait to read a consistent set of values.
	 * The correct way to interact with the version number is similar to
	 * Linux's seqlock: see the implementations of
	 * read_seqbegin/read_seqretry.
	 */
	uint32_t	version;
	uint32_t	pad0;
	uint64_t	tsc_timestamp;	/* TSC at last update of time vals. */
	uint64_t	system_time;	/* Time, in nanosecs, since boot.*/
	/*
	 * Current system time:
	 *   system_time +
	 *   ((((tsc - tsc_timestamp) << tsc_shift) * tsc_to_system_mul) >> 32)
	 * CPU frequency (Hz):
	 *   ((10^9 << 32) / tsc_to_system_mul) >> tsc_shift
	 */
	uint32_t	tsc_to_system_mul;
	int8_t		tsc_shift;
#if CONFIG_XEN_INTERFACE_VERSION > 0x040600
	uint8_t		flags;
	uint8_t		pad1[2];
#else
	int8_t		pad1[3];
#endif
}; /* 32 bytes */
typedef struct vcpu_time_info vcpu_time_info_t;

#define XEN_PVCLOCK_TSC_STABLE_BIT	(1 << 0)
#define XEN_PVCLOCK_GUEST_STOPPED	(1 << 1)

struct vcpu_info {
	/*
	 * 'evtchn_upcall_pending' is written non-zero by Xen to indicate
	 * a pending notification for a particular VCPU. It is then cleared
	 * by the guest OS /before/ checking for pending work, thus avoiding
	 * a set-and-check race. Note that the mask is only accessed by Xen
	 * on the CPU that is currently hosting the VCPU. This means that the
	 * pending and mask flags can be updated by the guest without special
	 * synchronisation (i.e., no need for the x86 LOCK prefix).
	 * This may seem suboptimal because if the pending flag is set by
	 * a different CPU then an IPI may be scheduled even when the mask
	 * is set. However, note:
	 *  1. The task of 'interrupt holdoff' is covered by the per-event-
	 *     channel mask bits. A 'noisy' event that is continually being
	 *     triggered can be masked at source at this very precise
	 *	 granularity.
	 *  2. The main purpose of the per-VCPU mask is therefore to restrict
	 *     reentrant execution: whether for concurrency control, or to
	 *     prevent unbounded stack usage. Whatever the purpose, we expect
	 *     that the mask will be asserted only for short periods at a time,
	 *	 and so the likelihood of a 'spurious' IPI is suitably small.
	 * The mask is read before making an event upcall to the guest: a
	 * non-zero mask therefore guarantees that the VCPU will not receive
	 * an upcall activation. The mask is cleared when the VCPU requests
	 * to block: this avoids wakeup-waiting races.
	 */
	uint8_t evtchn_upcall_pending;
#ifdef XEN_HAVE_PV_UPCALL_MASK
	uint8_t evtchn_upcall_mask;
#else /* XEN_HAVE_PV_UPCALL_MASK */
	uint8_t pad0;
#endif /* XEN_HAVE_PV_UPCALL_MASK */
	xen_ulong_t evtchn_pending_sel;
	struct arch_vcpu_info arch;
	vcpu_time_info_t time;
}; /* 64 bytes (x86) */
#ifndef __XEN__
typedef struct vcpu_info vcpu_info_t;
#endif

/*
 * `incontents 200 startofday_shared Start-of-day shared data structure
 * Xen/kernel shared data -- pointer provided in start_info.
 *
 * This structure is defined to be both smaller than a page, and the
 * only data on the shared page, but may vary in actual size even within
 * compatible Xen versions; guests should not rely on the size
 * of this structure remaining constant.
 */
struct shared_info {
	struct vcpu_info vcpu_info[XEN_LEGACY_MAX_VCPUS];

	/*
	 * A domain can create "event channels" on which it can send and receive
	 * asynchronous event notifications. There are three classes of event that
	 * are delivered by this mechanism:
	 *  1. Bi-directional inter- and intra-domain connections. Domains must
	 *     arrange out-of-band to set up a connection (usually by allocating
	 *     an unbound 'listener' port and advertising that via a storage service
	 *     such as xenstore).
	 *  2. Physical interrupts. A domain with suitable hardware-access
	 *     privileges can bind an event-channel port to a physical interrupt
	 *     source.
	 *  3. Virtual interrupts ('events'). A domain can bind an event-channel
	 *     port to a virtual interrupt source, such as the virtual-timer
	 *     device or the emergency console.
	 *
	 * Event channels are addressed by a "port index". Each channel is
	 * associated with two bits of information:
	 *  1. PENDING -- notifies the domain that there is a pending notification
	 *     to be processed. This bit is cleared by the guest.
	 *  2. MASK -- if this bit is clear then a 0->1 transition of PENDING
	 *     will cause an asynchronous upcall to be scheduled. This bit is only
	 *     updated by the guest. It is read-only within Xen. If a channel
	 *     becomes pending while the channel is masked then the 'edge' is lost
	 *     (i.e., when the channel is unmasked, the guest must manually handle
	 *     pending notifications as no upcall will be scheduled by Xen).
	 *
	 * To expedite scanning of pending notifications, any 0->1 pending
	 * transition on an unmasked channel causes a corresponding bit in a
	 * per-vcpu selector word to be set. Each bit in the selector covers a
	 * 'C long' in the PENDING bitfield array.
	 */
	xen_ulong_t evtchn_pending[sizeof(xen_ulong_t) * 8];
	xen_ulong_t evtchn_mask[sizeof(xen_ulong_t) * 8];

	/*
	 * Wallclock time: updated by control software or RTC emulation.
	 * Guests should base their gettimeofday() syscall on this
	 * wallclock-base value.
	 * The values of wc_sec and wc_nsec are offsets from the Unix epoch
	 * adjusted by the domain's 'time offset' (in seconds) as set either
	 * by XEN_DOMCTL_settimeoffset, or adjusted via a guest write to the
	 * emulated RTC.
	 */
	uint32_t wc_version;	/* Version counter: see vcpu_time_info_t. */
	uint32_t wc_sec;
	uint32_t wc_nsec;
#if !defined(__i386__)
	uint32_t wc_sec_hi;
# define xen_wc_sec_hi wc_sec_hi
#elif !defined(__XEN__) && !defined(__XEN_TOOLS__)
# define xen_wc_sec_hi arch.wc_sec_hi
#endif

	struct arch_shared_info arch;

};
#ifndef __XEN__
typedef struct shared_info shared_info_t;
#endif

typedef uint8_t xen_domain_handle_t[16];

#ifndef int64_aligned_t
#define int64_aligned_t int64_t
#endif
#ifndef uint64_aligned_t
#define uint64_aligned_t uint64_t
#endif
#ifndef XEN_GUEST_HANDLE_64
#define XEN_GUEST_HANDLE_64(name) XEN_GUEST_HANDLE(name)
#endif

#ifndef __ASSEMBLY__
struct xenctl_bitmap {
	XEN_GUEST_HANDLE_64(uint8_t) bitmap;
	uint32_t nr_bits;
};
typedef struct xenctl_bitmap xenctl_bitmap_t;
#endif

#endif /* !__ASSEMBLY__ */

#endif /* __XEN_PUBLIC_XEN_H__ */
