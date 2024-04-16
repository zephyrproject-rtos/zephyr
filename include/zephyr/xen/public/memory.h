/* SPDX-License-Identifier: MIT */

/******************************************************************************
 * memory.h
 *
 * Memory reservation and information.
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
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */

#ifndef __XEN_PUBLIC_MEMORY_H__
#define __XEN_PUBLIC_MEMORY_H__

#include "xen.h"

#define XENMEM_populate_physmap		6

struct xen_memory_reservation {

	/*
	 * XENMEM_increase_reservation:
	 *   OUT: MFN (*not* GMFN) bases of extents that were allocated
	 * XENMEM_decrease_reservation:
	 *   IN:  GMFN bases of extents to free
	 * XENMEM_populate_physmap:
	 *   IN:  GPFN bases of extents to populate with memory
	 *   OUT: GMFN bases of extents that were allocated
	 *   (NB. This command also updates the mach_to_phys translation table)
	 * XENMEM_claim_pages:
	 *   IN: must be zero
	 */
	XEN_GUEST_HANDLE(xen_pfn_t) extent_start;

	/* Number of extents, and size/alignment of each (2^extent_order pages). */
	xen_ulong_t	nr_extents;
	unsigned int	extent_order;

#if __XEN_INTERFACE_VERSION__ >= 0x00030209
	/* XENMEMF flags. */
	unsigned int	mem_flags;
#else
	unsigned int	address_bits;
#endif

	/*
	 * Domain whose reservation is being changed.
	 * Unprivileged domains can specify only DOMID_SELF.
	 */
	domid_t		domid;
};
typedef struct xen_memory_reservation xen_memory_reservation_t;
DEFINE_XEN_GUEST_HANDLE(xen_memory_reservation_t);

/* A batched version of add_to_physmap. */
#define XENMEM_add_to_physmap_batch 23
struct xen_add_to_physmap_batch {
	/* IN */
	/* Which domain to change the mapping for. */
	domid_t domid;
	uint16_t space; /* => enum phys_map_space */

	/* Number of pages to go through */
	uint16_t size;

#if __XEN_INTERFACE_VERSION__ < 0x00040700
	domid_t foreign_domid; /* IFF gmfn_foreign. Should be 0 for other spaces. */
#else
	union xen_add_to_physmap_batch_extra {
		domid_t foreign_domid; /* gmfn_foreign */
		uint16_t res0;  /* All the other spaces. Should be 0 */
	} u;
#endif

	/* Indexes into space being mapped. */
	XEN_GUEST_HANDLE(xen_ulong_t) idxs;

	/* GPFN in domid where the source mapping page should appear. */
	XEN_GUEST_HANDLE(xen_pfn_t) gpfns;

	/* OUT */
	/* Per index error code. */
	XEN_GUEST_HANDLE(int) errs;
};
typedef struct xen_add_to_physmap_batch xen_add_to_physmap_batch_t;
DEFINE_XEN_GUEST_HANDLE(xen_add_to_physmap_batch_t);


#define XENMAPSPACE_shared_info		0	/* shared info page */
#define XENMAPSPACE_grant_table		1	/* grant table page */
#define XENMAPSPACE_gmfn		2	/* GMFN */

/* GMFN range, XENMEM_add_to_physmap only.*/
#define XENMAPSPACE_gmfn_range		3

/* GMFN from another dom, XENMEM_add_to_physmap_batch only. */
#define XENMAPSPACE_gmfn_foreign	4

/*
 * Device mmio region ARM only; the region is mapped in Stage-2 using the
 * Normal Memory Inner/Outer Write-Back Cacheable memory attribute.
 */
#define XENMAPSPACE_dev_mmio		5

/*
 * Sets the GPFN at which a particular page appears in the specified guest's
 * physical address space (translated guests only).
 * arg == addr of xen_add_to_physmap_t.
 */
#define XENMEM_add_to_physmap		7
struct xen_add_to_physmap {
	/* Which domain to change the mapping for. */
	domid_t domid;

	/* Number of pages to go through for gmfn_range */
	uint16_t size;

	unsigned int space; /* => enum phys_map_space */

#define XENMAPIDX_grant_table_status 0x80000000

	/* Index into space being mapped. */
	xen_ulong_t idx;

	/* GPFN in domid where the source mapping page should appear. */
	xen_pfn_t gpfn;
};
typedef struct xen_add_to_physmap xen_add_to_physmap_t;
DEFINE_XEN_GUEST_HANDLE(xen_add_to_physmap_t);

/*
 * Unmaps the page appearing at a particular GPFN from the specified guest's
 * physical address space (translated guests only).
 * arg == addr of xen_remove_from_physmap_t.
 */
#define XENMEM_remove_from_physmap	15
struct xen_remove_from_physmap {
	/* Which domain to change the mapping for. */
	domid_t domid;

	/* GPFN of the current mapping of the page. */
	xen_pfn_t gpfn;
};
typedef struct xen_remove_from_physmap xen_remove_from_physmap_t;
DEFINE_XEN_GUEST_HANDLE(xen_remove_from_physmap_t);

#endif /* __XEN_PUBLIC_MEMORY_H__ */
