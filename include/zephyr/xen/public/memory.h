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

#endif /* __XEN_PUBLIC_MEMORY_H__ */
