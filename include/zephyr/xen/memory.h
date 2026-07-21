/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 EPAM Systems
 */

/**
 * @file
 * @brief Xen memory management operations.
 * @ingroup xen_memory_management
 */

#ifndef ZEPHYR_XEN_MEMORY_H_
#define ZEPHYR_XEN_MEMORY_H_

#include <zephyr/kernel.h>
#include <zephyr/xen/public/memory.h>
#include <zephyr/xen/public/xen.h>

/**
 * @defgroup xen_memory_management Xen memory management
 * @ingroup xen_support
 * @brief Manage domain physmaps and Xen-owned resources.
 * @{
 */

/**
 * @brief Add a single mapping to a domain physmap.
 *
 * @param domid Domain whose physmap is updated. Unprivileged callers must use
 *              ``DOMID_SELF``.
 * @param idx Index within the Xen mapping space identified by @p space.
 * @param space Mapping space selector, typically one of the ``XENMAPSPACE_*``
 *              constants.
 * @param gpfn Guest frame number where the mapping becomes visible.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xendom_add_to_physmap(int domid, unsigned long idx, unsigned int space,
			  xen_pfn_t gpfn);

/**
 * @brief Add multiple mappings to a domain physmap.
 *
 * @param domid Domain whose physmap is updated. Unprivileged callers must use
 *              ``DOMID_SELF``.
 * @param foreign_domid Foreign domain identifier used with
 * ``XENMAPSPACE_gmfn_foreign``. Pass ``0`` for the other mapping spaces.
 * @param space Mapping space selector, typically one of the ``XENMAPSPACE_*``
 *              constants.
 * @param size Number of mappings described by the arrays.
 * @param idxs Array of mapping-space indexes.
 * @param gpfns Array of guest frame numbers that receive the mappings.
 * @param[out] errs Array that receives a per-entry Xen status code.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xendom_add_to_physmap_batch(int domid, int foreign_domid,
				unsigned int space, unsigned int size,
				xen_ulong_t *idxs, xen_pfn_t *gpfns, int *errs);

/**
 * @brief Remove a mapping from a domain physmap.
 *
 * @param domid Domain whose physmap is updated. Unprivileged callers must use
 * ``DOMID_SELF``.
 * @param gpfn Guest frame number to remove.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xendom_remove_from_physmap(int domid, xen_pfn_t gpfn);

/**
 * @brief Populate guest frames with memory.
 *
 * @param domid Domain whose physmap is populated. Unprivileged callers must use
 * ``DOMID_SELF``.
 * @param extent_order Extent order used by Xen, where each extent covers
 *                     ``2^extent_order`` pages.
 * @param nr_extents Number of entries in @p extent_start.
 * @param mem_flags Xen memory flags. Arm guests currently pass ``0``.
 * @param extent_start Array of guest frame numbers that receive the populated
 *                     extents.
 *
 * @return Number of populated extents on success, negative errno value on
 *         failure.
 */
int xendom_populate_physmap(int domid, unsigned int extent_order,
			    unsigned int nr_extents, unsigned int mem_flags,
			    xen_pfn_t *extent_start);

/**
 * @brief Acquire a Xen-managed resource mapping for a domain.
 *
 * @param domid Target domain identifier.
 * @param type Xen resource type, for example ``XENMEM_resource_ioreq_server``.
 * @param id Resource instance identifier interpreted according to @p type.
 * @param frame Starting frame within the Xen resource. Ignored when
 *              ``*nr_frames`` is 0 (size query).
 * @param[in,out] nr_frames On entry, number of frames to acquire. On return,
 * the number of frames that Xen reports for the request.
 * @param[out] frame_list Guest frame list buffer: input GFNs for HVM guests,
 *             output MFNs for PV guests.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xendom_acquire_resource(domid_t domid, uint16_t type, uint32_t id, uint64_t frame,
			    uint32_t *nr_frames, xen_pfn_t *frame_list);

/** @} */

#endif /* ZEPHYR_XEN_MEMORY_H_ */
