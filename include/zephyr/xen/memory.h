/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 EPAM Systems
 */

#ifndef ZEPHYR_XEN_MEMORY_H_
#define ZEPHYR_XEN_MEMORY_H_

#include <zephyr/kernel.h>
#include <zephyr/xen/public/memory.h>
#include <zephyr/xen/public/xen.h>

/**
 * Add mapping for specified page frame in Xen domain physmap.
 *
 * @param domid	domain id, where mapping will be added. For unprivileged should
 *		be DOMID_SELF.
 * @param idx	index into space being mapped.
 * @param space	XENMAPSPACE_* mapping space identifier.
 * @param gpfn	page frame where the source mapping page should appear.
 * @return	zero on success, negative errno on error.
 */
int xendom_add_to_physmap(int domid, unsigned long idx, unsigned int space,
			  xen_pfn_t gpfn);

/**
 * Add mapping for specified set of page frames to Xen domain physmap.
 *
 * @param domid		domain id, where mapping will be added. For unprivileged
 *			should be DOMID_SELF.
 * @param foreign_domid	for gmfn_foreign - domain id, whose pages being mapped,
 *			0 for other.
 * @param space		XENMAPSPACE_* mapping space identifier.
 * @param size		number of page frames being mapped.
 * @param idxs		array of indexes into space being mapped.
 * @param gpfns		array of page frames where the mapping should appear.
 * @param errs		array of per-index error codes.
 * @return		zero on success, negative errno on error.
 */
int xendom_add_to_physmap_batch(int domid, int foreign_domid,
				unsigned int space, unsigned int size,
				xen_ulong_t *idxs, xen_pfn_t *gpfns, int *errs);

/**
 * Removes page frame from Xen domain physmap.
 *
 * @param domid	domain id, whose page is going to be removed. For unprivileged
 *		should be DOMID_SELF.
 * @param gpfn	page frame number, that needs to be removed
 * @return	zero on success, negative errno on error.
 */
int xendom_remove_from_physmap(int domid, xen_pfn_t gpfn);

/**
 * Populate specified Xen domain page frames with memory.
 *
 * @param domid		domain id, where mapping will be added. For unprivileged
 *			should be DOMID_SELF.
 * @param extent_order	size/alignment of each extent (size is 2^extent_order),
 *			e.g. 0 for 4K extents, 9 for 2M etc.
 * @param nr_extents	number of page frames being populated.
 * @param mem_flags	N/A, should be 0 for Arm.
 * @param extent_start	page frame bases of extents to populate with memory.
 * @return		number of populated frames success, negative errno on
 *			error.
 */
int xendom_populate_physmap(int domid, unsigned int extent_order,
			    unsigned int nr_extents, unsigned int mem_flags,
			    xen_pfn_t *extent_start);

/**
 * @brief Acquire a resource mapping for the Xen domain.
 *
 * Issues the XENMEM_acquire_resource hypercall to map a resource buffer
 * (e.g., I/O request server, grant table, VM trace buffer) into the
 * specified domain's physmap, or query its total size.
 *
 * @param domid        Target domain identifier. Use DOMID_SELF for the calling domain.
 * @param type         Resource type identifier (e.g., XENMEM_resource_ioreq_server).
 * @param id           Resource-specific identifier (e.g., server ID or table ID).
 * @param frame        Starting frame number for mapping, or ignored if *nr_frames == 0.
 * @param nr_frames    [in,out] On input, number of frames to map; on return,
 *                     number of frames actually mapped (or total frames if queried).
 * @param frame_list   Guest frame list buffer: input GFNs for HVM guests,
 *                     output MFNs for PV guests.
 * @return             Zero on success, or a negative errno code on failure.
 */
int xendom_acquire_resource(domid_t domid, uint16_t type, uint32_t id, uint64_t frame,
			    uint32_t *nr_frames, xen_pfn_t *frame_list);

#endif /* ZEPHYR_XEN_MEMORY_H_ */
