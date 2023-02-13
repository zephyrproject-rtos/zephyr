/* SPDX-License-Identifier: MIT */

/******************************************************************************
 * grant_table.h
 *
 * Interface for granting foreign access to page frames, and receiving
 * page-ownership transfers.
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

#ifndef __XEN_PUBLIC_GRANT_TABLE_H__
#define __XEN_PUBLIC_GRANT_TABLE_H__

#include "xen.h"

/*
 * `incontents 150 gnttab Grant Tables
 *
 * Xen's grant tables provide a generic mechanism to memory sharing
 * between domains. This shared memory interface underpins the split
 * device drivers for block and network IO.
 *
 * Each domain has its own grant table. This is a data structure that
 * is shared with Xen; it allows the domain to tell Xen what kind of
 * permissions other domains have on its pages. Entries in the grant
 * table are identified by grant references. A grant reference is an
 * integer, which indexes into the grant table. It acts as a
 * capability which the grantee can use to perform operations on the
 * granter's memory.
 *
 * This capability-based system allows shared-memory communications
 * between unprivileged domains. A grant reference also encapsulates
 * the details of a shared page, removing the need for a domain to
 * know the real machine address of a page it is sharing. This makes
 * it possible to share memory correctly with domains running in
 * fully virtualised memory.
 */

/***********************************
 * GRANT TABLE REPRESENTATION
 */

/* Some rough guidelines on accessing and updating grant-table entries
 * in a concurrency-safe manner. For more information, Linux contains a
 * reference implementation for guest OSes (drivers/xen/grant_table.c, see
 * http://git.kernel.org/?p=linux/kernel/git/torvalds/linux.git;a=blob;f=drivers/xen/grant-table.c;hb=HEAD
 *
 * NB. WMB is a no-op on current-generation x86 processors. However, a
 *     compiler barrier will still be required.
 *
 * Introducing a valid entry into the grant table:
 *  1. Write ent->domid.
 *  2. Write ent->frame:
 *      GTF_permit_access:   Frame to which access is permitted.
 *      GTF_accept_transfer: Pseudo-phys frame slot being filled by new
 *                           frame, or zero if none.
 *  3. Write memory barrier (WMB).
 *  4. Write ent->flags, inc. valid type.
 *
 * Invalidating an unused GTF_permit_access entry:
 *  1. flags = ent->flags.
 *  2. Observe that !(flags & (GTF_reading|GTF_writing)).
 *  3. Check result of SMP-safe CMPXCHG(&ent->flags, flags, 0).
 *  NB. No need for WMB as reuse of entry is control-dependent on success of
 *      step 3, and all architectures guarantee ordering of ctrl-dep writes.
 *
 * Invalidating an in-use GTF_permit_access entry:
 *  This cannot be done directly. Request assistance from the domain controller
 *  which can set a timeout on the use of a grant entry and take necessary
 *  action. (NB. This is not yet implemented!).
 *
 * Invalidating an unused GTF_accept_transfer entry:
 *  1. flags = ent->flags.
 *  2. Observe that !(flags & GTF_transfer_committed). [*]
 *  3. Check result of SMP-safe CMPXCHG(&ent->flags, flags, 0).
 *  NB. No need for WMB as reuse of entry is control-dependent on success of
 *      step 3, and all architectures guarantee ordering of ctrl-dep writes.
 *  [*] If GTF_transfer_committed is set then the grant entry is 'committed'.
 *      The guest must /not/ modify the grant entry until the address of the
 *      transferred frame is written. It is safe for the guest to spin waiting
 *      for this to occur (detect by observing GTF_transfer_completed in
 *      ent->flags).
 *
 * Invalidating a committed GTF_accept_transfer entry:
 *  1. Wait for (ent->flags & GTF_transfer_completed).
 *
 * Changing a GTF_permit_access from writable to read-only:
 *  Use SMP-safe CMPXCHG to set GTF_readonly, while checking !GTF_writing.
 *
 * Changing a GTF_permit_access from read-only to writable:
 *  Use SMP-safe bit-setting instruction.
 */

/*
 * Reference to a grant entry in a specified domain's grant table.
 */
typedef uint32_t grant_ref_t;

/*
 * A grant table comprises a packed array of grant entries in one or more
 * page frames shared between Xen and a guest.
 * [XEN]: This field is written by Xen and read by the sharing guest.
 * [GST]: This field is written by the guest and read by Xen.
 */

/*
 * Version 1 of the grant table entry structure is maintained purely
 * for backwards compatibility.  New guests should use version 2.
 */
#if __XEN_INTERFACE_VERSION__ < 0x0003020a
#define grant_entry_v1 grant_entry
#define grant_entry_v1_t grant_entry_t
#endif
struct grant_entry_v1 {
	/* GTF_xxx: various type and flag information.  [XEN,GST] */
	uint16_t flags;
	/* The domain being granted foreign privileges. [GST] */
	domid_t  domid;
	/*
	 * GTF_permit_access: GFN that @domid is allowed to map and access. [GST]
	 * GTF_accept_transfer: GFN that @domid is allowed to transfer into. [GST]
	 * GTF_transfer_completed: MFN whose ownership transferred by @domid
	 * (non-translated guests only). [XEN]
	 */
	uint32_t frame;
};
typedef struct grant_entry_v1 grant_entry_v1_t;

/* The first few grant table entries will be preserved across grant table
 * version changes and may be pre-populated at domain creation by tools.
 */
#define GNTTAB_NR_RESERVED_ENTRIES		8
#define GNTTAB_RESERVED_CONSOLE			0
#define GNTTAB_RESERVED_XENSTORE		1

/*
 * Type of grant entry.
 *  GTF_invalid: This grant entry grants no privileges.
 *  GTF_permit_access: Allow @domid to map/access @frame.
 *  GTF_accept_transfer: Allow @domid to transfer ownership of one page frame
 *                       to this guest. Xen writes the page number to @frame.
 *  GTF_transitive: Allow @domid to transitively access a subrange of
 *                  @trans_grant in @trans_domid.  No mappings are allowed.
 */
#define GTF_invalid				(0U << 0)
#define GTF_permit_access			(1U << 0)
#define GTF_accept_transfer			(2U << 0)
#define GTF_transitive				(3U << 0)
#define GTF_type_mask				(3U << 0)

/*
 * Subflags for GTF_permit_access and GTF_transitive.
 *  GTF_readonly: Restrict @domid to read-only mappings and accesses. [GST]
 *  GTF_reading: Grant entry is currently mapped for reading by @domid. [XEN]
 *  GTF_writing: Grant entry is currently mapped for writing by @domid. [XEN]
 * Further subflags for GTF_permit_access only.
 *  GTF_PAT, GTF_PWT, GTF_PCD: (x86) cache attribute flags to be used for
 *                             mappings of the grant [GST]
 *  GTF_sub_page: Grant access to only a subrange of the page.  @domid
 *                will only be allowed to copy from the grant, and not
 *                map it. [GST]
 */
#define _GTF_readonly				(2)
#define GTF_readonly				(1U << _GTF_readonly)
#define _GTF_reading				(3)
#define GTF_reading				(1U << _GTF_reading)
#define _GTF_writing				(4)
#define GTF_writing				(1U << _GTF_writing)
#define _GTF_PWT				(5)
#define GTF_PWT					(1U << _GTF_PWT)
#define _GTF_PCD				(6)
#define GTF_PCD					(1U << _GTF_PCD)
#define _GTF_PAT				(7)
#define GTF_PAT					(1U << _GTF_PAT)
#define _GTF_sub_page				(8)
#define GTF_sub_page				(1U << _GTF_sub_page)

/*
 * Subflags for GTF_accept_transfer:
 *  GTF_transfer_committed: Xen sets this flag to indicate that it is committed
 *      to transferring ownership of a page frame. When a guest sees this flag
 *      it must /not/ modify the grant entry until GTF_transfer_completed is
 *      set by Xen.
 *  GTF_transfer_completed: It is safe for the guest to spin-wait on this flag
 *      after reading GTF_transfer_committed. Xen will always write the frame
 *      address, followed by ORing this flag, in a timely manner.
 */
#define _GTF_transfer_committed			(2)
#define GTF_transfer_committed			(1U << _GTF_transfer_committed)
#define _GTF_transfer_completed			(3)
#define GTF_transfer_completed			(1U << _GTF_transfer_completed)

/***********************************
 * GRANT TABLE QUERIES AND USES
 */

/* ` enum neg_errnoval
 * ` HYPERVISOR_grant_table_op(enum grant_table_op cmd,
 * `                           void *args,
 * `                           unsigned int count)
 * `
 *
 * @args points to an array of a per-command data structure. The array
 * has @count members
 */

/* ` enum grant_table_op { // GNTTABOP_* => struct gnttab_* */
#define GNTTABOP_map_grant_ref			0
#define GNTTABOP_unmap_grant_ref		1
#define GNTTABOP_setup_table			2
#define GNTTABOP_dump_table			3
#define GNTTABOP_transfer			4
#define GNTTABOP_copy				5
#define GNTTABOP_query_size			6
#define GNTTABOP_unmap_and_replace		7
#if __XEN_INTERFACE_VERSION__ >= 0x0003020a
#define GNTTABOP_set_version			8
#define GNTTABOP_get_status_frames		9
#define GNTTABOP_get_version			10
#define GNTTABOP_swap_grant_ref			11
#define GNTTABOP_cache_flush			12
#endif /* __XEN_INTERFACE_VERSION__ */
/* ` } */

/*
 * Handle to track a mapping created via a grant reference.
 */
typedef uint32_t grant_handle_t;

/*
 * GNTTABOP_map_grant_ref: Map the grant entry (<dom>,<ref>) for access
 * by devices and/or host CPUs. If successful, <handle> is a tracking number
 * that must be presented later to destroy the mapping(s). On error, <status>
 * is a negative status code.
 * NOTES:
 *  1. If GNTMAP_device_map is specified then <dev_bus_addr> is the address
 *     via which I/O devices may access the granted frame.
 *  2. If GNTMAP_host_map is specified then a mapping will be added at
 *     either a host virtual address in the current address space, or at
 *     a PTE at the specified machine address.  The type of mapping to
 *     perform is selected through the GNTMAP_contains_pte flag, and the
 *     address is specified in <host_addr>.
 *  3. Mappings should only be destroyed via GNTTABOP_unmap_grant_ref. If a
 *     host mapping is destroyed by other means then it is *NOT* guaranteed
 *     to be accounted to the correct grant reference!
 */
struct gnttab_map_grant_ref {
	/* IN parameters. */
	uint64_t host_addr;
	uint32_t flags;		/* GNTMAP_* */
	grant_ref_t ref;
	domid_t  dom;
	/* OUT parameters. */
	int16_t  status;	/* => enum grant_status */
	grant_handle_t handle;
	uint64_t dev_bus_addr;
};
typedef struct gnttab_map_grant_ref gnttab_map_grant_ref_t;
DEFINE_XEN_GUEST_HANDLE(gnttab_map_grant_ref_t);

/*
 * GNTTABOP_unmap_grant_ref: Destroy one or more grant-reference mappings
 * tracked by <handle>. If <host_addr> or <dev_bus_addr> is zero, that
 * field is ignored. If non-zero, they must refer to a device/host mapping
 * that is tracked by <handle>
 * NOTES:
 *  1. The call may fail in an undefined manner if either mapping is not
 *     tracked by <handle>.
 *  3. After executing a batch of unmaps, it is guaranteed that no stale
 *     mappings will remain in the device or host TLBs.
 */
struct gnttab_unmap_grant_ref {
	/* IN parameters. */
	uint64_t host_addr;
	uint64_t dev_bus_addr;
	grant_handle_t handle;
	/* OUT parameters. */
	int16_t  status;	/* => enum grant_status */
};
typedef struct gnttab_unmap_grant_ref gnttab_unmap_grant_ref_t;
DEFINE_XEN_GUEST_HANDLE(gnttab_unmap_grant_ref_t);

/*
 * GNTTABOP_setup_table: Set up a grant table for <dom> comprising at least
 * <nr_frames> pages. The frame addresses are written to the <frame_list>.
 * Only <nr_frames> addresses are written, even if the table is larger.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may specify <dom> != DOMID_SELF.
 *  3. Xen may not support more than a single grant-table page per domain.
 */
struct gnttab_setup_table {
	/* IN parameters. */
	domid_t  dom;
	uint32_t nr_frames;

	/* OUT parameters. */
	int16_t status; /* => enum grant_status */
#if __XEN_INTERFACE_VERSION__ < 0x00040300
	XEN_GUEST_HANDLE(ulong) frame_list;
#else
	XEN_GUEST_HANDLE(xen_pfn_t) frame_list;
#endif
};
typedef struct gnttab_setup_table gnttab_setup_table_t;
DEFINE_XEN_GUEST_HANDLE(gnttab_setup_table_t);



/*
 * Bitfield values for gnttab_map_grant_ref.flags.
 */
 /* Map the grant entry for access by I/O devices. */
#define _GNTMAP_device_map	(0)
#define GNTMAP_device_map	(1<<_GNTMAP_device_map)
 /* Map the grant entry for access by host CPUs. */
#define _GNTMAP_host_map	(1)
#define GNTMAP_host_map		(1<<_GNTMAP_host_map)
 /* Accesses to the granted frame will be restricted to read-only access. */
#define _GNTMAP_readonly	(2)
#define GNTMAP_readonly		(1<<_GNTMAP_readonly)
 /*
  * GNTMAP_host_map subflag:
  *  0 => The host mapping is usable only by the guest OS.
  *  1 => The host mapping is usable by guest OS + current application.
  */
#define _GNTMAP_application_map	(3)
#define GNTMAP_application_map	(1<<_GNTMAP_application_map)

 /*
  * GNTMAP_contains_pte subflag:
  *  0 => This map request contains a host virtual address.
  *  1 => This map request contains the machine addess of the PTE to update.
  */
#define _GNTMAP_contains_pte	(4)
#define GNTMAP_contains_pte	(1<<_GNTMAP_contains_pte)

/*
 * Bits to be placed in guest kernel available PTE bits (architecture
 * dependent; only supported when XENFEAT_gnttab_map_avail_bits is set).
 */
#define _GNTMAP_guest_avail0	(16)
#define GNTMAP_guest_avail_mask	((uint32_t)~0 << _GNTMAP_guest_avail0)

/*
 * Values for error status returns. All errors are -ve.
 */
/* ` enum grant_status { */
#define GNTST_okay		(0)  /* Normal return */
#define GNTST_general_error	(-1) /* General undefined error */
#define GNTST_bad_domain	(-2) /* Unrecognsed domain id */
#define GNTST_bad_gntref	(-3) /* Unrecognised or inappropriate gntref */
#define GNTST_bad_handle	(-4) /* Unrecognised or inappropriate handle */
#define GNTST_bad_virt_addr	(-5) /* Inappropriate virtual address to map */
#define GNTST_bad_dev_addr	(-6) /* Inappropriate device address to unmap */
#define GNTST_no_device_space	(-7) /* Out of space in I/O MMU */
#define GNTST_permission_denied	(-8) /* Not enough privilege for operation */
#define GNTST_bad_page		(-9) /* Specified page was invalid for op */
#define GNTST_bad_copy_arg	(-10) /* copy arguments cross page boundary */
#define GNTST_address_too_big	(-11) /* transfer page address too large */
#define GNTST_eagain		(-12) /* Operation not done; try agains */
/* ` } */

#define GNTTABOP_error_msgs {				\
	"okay",						\
	"undefined error",				\
	"unrecognised domain id",			\
	"invalid grant reference",			\
	"invalid mapping handle",			\
	"invalid virtual address",			\
	"invalid device address",			\
	"no spare translation slot in the I/O MMU",	\
	"permission denied",				\
	"bad page",					\
	"copy arguments cross page boundary",		\
	"page address size too large",			\
	"operation not done; try again"			\
}

#endif /* __XEN_PUBLIC_GRANT_TABLE_H__ */
