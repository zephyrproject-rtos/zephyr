/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 EPAM Systems
 */

#include <zephyr/arch/arm64/hypercall.h>
#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/memory.h>
#include <zephyr/xen/public/xen.h>

#include <zephyr/kernel.h>

int xendom_add_to_physmap(int domid, unsigned long idx,
			  unsigned int space, xen_pfn_t gpfn)
{
	struct xen_add_to_physmap xatp = {
		.domid = domid,
		.idx = idx,
		.space = space,
		.gpfn = gpfn,
	};

	return HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xatp);
}

int xendom_add_to_physmap_batch(int domid, int foreign_domid,
				unsigned int space, unsigned int size,
				xen_ulong_t *idxs, xen_pfn_t *gpfns, int *errs)
{
	struct xen_add_to_physmap_batch xatpb = {
		.domid = domid,
		.u.foreign_domid = foreign_domid,
		.space = space,
		.size = size,
	};

	set_xen_guest_handle(xatpb.gpfns, gpfns);
	set_xen_guest_handle(xatpb.idxs, idxs);
	set_xen_guest_handle(xatpb.errs, errs);

	return HYPERVISOR_memory_op(XENMEM_add_to_physmap_batch, &xatpb);
}

int xendom_remove_from_physmap(int domid, xen_pfn_t gpfn)
{
	struct xen_remove_from_physmap xrfp = {
		.domid = domid,
		.gpfn = gpfn,
	};

	return HYPERVISOR_memory_op(XENMEM_remove_from_physmap, &xrfp);
}

int xendom_populate_physmap(int domid, unsigned int extent_order,
			    unsigned int nr_extents, unsigned int mem_flags,
			    xen_pfn_t *extent_start)
{
	struct xen_memory_reservation reservation = {
		.domid = domid,
		.extent_order = extent_order,
		.nr_extents = nr_extents,
		.mem_flags = mem_flags,
	};

	set_xen_guest_handle(reservation.extent_start, extent_start);

	return HYPERVISOR_memory_op(XENMEM_populate_physmap, &reservation);
}

int xendom_acquire_resource(domid_t domid, uint16_t type, uint32_t id, uint64_t frame,
			    uint32_t *nr_frames, xen_pfn_t *frame_list)
{
	struct xen_mem_acquire_resource acquire_res = {
		.domid = domid,
		.type = type,
		.id = id,
		.pad = 0,
		.frame = frame,
		.nr_frames = *nr_frames,
	};
	int ret;

	set_xen_guest_handle(acquire_res.frame_list, frame_list);

	ret = HYPERVISOR_memory_op(XENMEM_acquire_resource, &acquire_res);

	*nr_frames = acquire_res.nr_frames;

	return ret;
}
