/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * Copyright (c) 2018, Xilinx Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/io.h>
#include <openamp/rsc_table_parser.h>

static int handle_dummy_rsc(struct remoteproc *rproc, void *rsc);

/* Resources handler */
rsc_handler rsc_handler_table[] = {
	handle_carve_out_rsc, /**< carved out resource */
	handle_dummy_rsc, /**< IOMMU dev mem resource */
	handle_trace_rsc, /**< trace buffer resource */
	handle_vdev_rsc, /**< virtio resource */
	handle_dummy_rsc, /**< rproc shared memory resource */
	handle_dummy_rsc, /**< firmware checksum resource */
};

int handle_rsc_table(struct remoteproc *rproc,
		     struct resource_table *rsc_table, int size,
		     struct metal_io_region *io)
{
	char *rsc_start;
	unsigned int rsc_type;
	unsigned int idx, offset;
	int status = 0;

	/* Validate rsc table header fields */

	/* Minimum rsc table size */
	if (sizeof(struct resource_table) > (unsigned int)size) {
		return -RPROC_ERR_RSC_TAB_TRUNC;
	}

	/* Supported version */
	if (rsc_table->ver != RSC_TAB_SUPPORTED_VERSION) {
		return -RPROC_ERR_RSC_TAB_VER;
	}

	/* Offset array */
	offset = sizeof(struct resource_table)
		 + rsc_table->num * sizeof(rsc_table->offset[0]);

	if (offset > (unsigned int)size) {
		return -RPROC_ERR_RSC_TAB_TRUNC;
	}

	/* Reserved fields - must be zero */
	if ((rsc_table->reserved[0] != 0 || rsc_table->reserved[1]) != 0) {
		return -RPROC_ERR_RSC_TAB_RSVD;
	}

	/* Loop through the offset array and parse each resource entry */
	for (idx = 0; idx < rsc_table->num; idx++) {
		rsc_start = (char *)rsc_table;
		rsc_start += rsc_table->offset[idx];
		if (io &&
		    metal_io_virt_to_offset(io, rsc_start) == METAL_BAD_OFFSET)
			return -RPROC_ERR_RSC_TAB_TRUNC;
		rsc_type = *((uint32_t *)rsc_start);
		if (rsc_type < RSC_LAST)
			status = rsc_handler_table[rsc_type](rproc,
							     rsc_start);
		else if (rsc_type >= RSC_VENDOR_START &&
			 rsc_type <= RSC_VENDOR_END)
			status = handle_vendor_rsc(rproc, rsc_start);
		if (status == -RPROC_ERR_RSC_TAB_NS) {
			status = 0;
			continue;
		}
		else if (status)
			break;
	}

	return status;
}

/**
 * handle_carve_out_rsc
 *
 * Carveout resource handler.
 *
 * @param rproc - pointer to remote remoteproc
 * @param rsc   - pointer to carveout resource
 *
 * @returns - 0 for success, or negative value for failure
 *
 */
int handle_carve_out_rsc(struct remoteproc *rproc, void *rsc)
{
	struct fw_rsc_carveout *carve_rsc = (struct fw_rsc_carveout *)rsc;
	metal_phys_addr_t da;
	metal_phys_addr_t pa;
	size_t size;
	unsigned int attribute;

	/* Validate resource fields */
	if (!carve_rsc) {
		return -RPROC_ERR_RSC_TAB_NP;
	}

	if (carve_rsc->reserved) {
		return -RPROC_ERR_RSC_TAB_RSVD;
	}
	pa = carve_rsc->pa;
	da = carve_rsc->da;
	size = carve_rsc->len;
	attribute = carve_rsc->flags;
	if (remoteproc_mmap(rproc, &pa, &da, size, attribute, NULL))
		return 0;
	else
		return -RPROC_EINVAL;
}

int handle_vendor_rsc(struct remoteproc *rproc, void *rsc)
{
	if (rproc && rproc->ops->handle_rsc) {
		struct fw_rsc_vendor *vend_rsc = rsc;
		size_t len = vend_rsc->len;

		return rproc->ops->handle_rsc(rproc, rsc, len);
	}
	return -RPROC_ERR_RSC_TAB_NS;
}

int handle_vdev_rsc(struct remoteproc *rproc, void *rsc)
{
	struct fw_rsc_vdev *vdev_rsc = (struct fw_rsc_vdev *)rsc;
	unsigned int notifyid, i, num_vrings;

	/* only assign notification IDs but do not initialize vdev */
	notifyid = vdev_rsc->notifyid;
	if (notifyid == RSC_NOTIFY_ID_ANY) {
		notifyid = remoteproc_allocate_id(rproc,
						  notifyid, notifyid + 1);
		vdev_rsc->notifyid = notifyid;
	}

	num_vrings = vdev_rsc->num_of_vrings;
	for (i = 0; i < num_vrings; i++) {
		struct fw_rsc_vdev_vring *vring_rsc;

		vring_rsc = &vdev_rsc->vring[i];
		notifyid = vring_rsc->notifyid;
		if (notifyid == RSC_NOTIFY_ID_ANY) {
			notifyid = remoteproc_allocate_id(rproc,
							  notifyid,
							  notifyid + 1);
			vdev_rsc->notifyid = notifyid;
		}
	}

	return 0;
}

/**
 * handle_trace_rsc
 *
 * trace resource handler.
 *
 * @param rproc - pointer to remote remoteproc
 * @param rsc   - pointer to trace resource
 *
 * @returns - no service error
 *
 */
int handle_trace_rsc(struct remoteproc *rproc, void *rsc)
{
	struct fw_rsc_trace *vdev_rsc = (struct fw_rsc_trace *)rsc;
	(void)rproc;

	if (vdev_rsc->da != FW_RSC_U32_ADDR_ANY && vdev_rsc->len != 0)
		return 0;
	/* FIXME: master should allocated a memory used by slave */

	return -RPROC_ERR_RSC_TAB_NS;
}

/**
 * handle_dummy_rsc
 *
 * dummy resource handler.
 *
 * @param rproc - pointer to remote remoteproc
 * @param rsc   - pointer to trace resource
 *
 * @returns - no service error
 *
 */
static int handle_dummy_rsc(struct remoteproc *rproc, void *rsc)
{
	(void)rproc;
	(void)rsc;

	return -RPROC_ERR_RSC_TAB_NS;
}

size_t find_rsc(void *rsc_table, unsigned int rsc_type, unsigned int index)
{
	struct resource_table *r_table = rsc_table;
	unsigned int i, rsc_index;
	unsigned int lrsc_type;
	char *rsc_start;

	metal_assert(r_table);
	/* Loop through the offset array and parse each resource entry */
	rsc_index = 0;
	for (i = 0; i < r_table->num; i++) {
		rsc_start = (char *)r_table;
		rsc_start += r_table->offset[i];
		lrsc_type = *((uint32_t *)rsc_start);
		if (lrsc_type == rsc_type) {
			if (rsc_index++ == index)
				return r_table->offset[i];
		}
	}
	return 0;
}
