/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <openamp/rsc_table_parser.h>
#include <metal/io.h>

/* Resources handler */
rsc_handler rsc_handler_table[] = {
	handle_carve_out_rsc,
	handle_dev_mem_rsc,
	handle_trace_rsc,
	handle_vdev_rsc,
	handle_rproc_mem_rsc,
	handle_fw_chksum_rsc,
	handle_mmu_rsc
};

/**
 * handle_rsc_table
 *
 * This function parses resource table.
 *
 * @param rproc     - pointer to remote remote_proc
 * @param rsc_table - resource table to parse
 * @param size      -  size of rsc table
 *
 * @returns - execution status
 *
 */
int handle_rsc_table(struct remote_proc *rproc,
		     struct resource_table *rsc_table, int size)
{

	unsigned char *rsc_start;
	unsigned int *rsc_offset;
	unsigned int rsc_type;
	unsigned int idx;
	int status = 0;

	/* Validate rsc table header fields */

	/* Minimum rsc table size */
	if (sizeof(struct resource_table) > (unsigned int)size) {
		return (RPROC_ERR_RSC_TAB_TRUNC);
	}

	/* Supported version */
	if (rsc_table->ver != RSC_TAB_SUPPORTED_VERSION) {
		return (RPROC_ERR_RSC_TAB_VER);
	}

	/* Offset array */
	if (sizeof(struct resource_table)
	    + rsc_table->num * sizeof(rsc_table->offset[0]) > (unsigned int)size) {
		return (RPROC_ERR_RSC_TAB_TRUNC);
	}

	/* Reserved fields - must be zero */
	if ((rsc_table->reserved[0] != 0 || rsc_table->reserved[1]) != 0) {
		return RPROC_ERR_RSC_TAB_RSVD;
	}

	rsc_start = (unsigned char *)rsc_table;

	/* FIX ME: need a clearer solution to set the I/O region for
	 * resource table */
	status = hil_set_rsc(rproc->proc, NULL, NULL,
			(metal_phys_addr_t)((uintptr_t)rsc_table), size);
	if (status)
		return status;

	/* Loop through the offset array and parse each resource entry */
	for (idx = 0; idx < rsc_table->num; idx++) {
		rsc_offset =
		    (unsigned int *)(rsc_start + rsc_table->offset[idx]);
		rsc_type = *rsc_offset;
		status =
		    rsc_handler_table[rsc_type] (rproc, (void *)rsc_offset);
		if (status != RPROC_SUCCESS) {
			break;
		}
	}

	return status;
}

/**
 * handle_carve_out_rsc
 *
 * Carveout resource handler.
 *
 * @param rproc - pointer to remote remote_proc
 * @param rsc   - pointer to carveout resource
 *
 * @returns - execution status
 *
 */
int handle_carve_out_rsc(struct remote_proc *rproc, void *rsc)
{
	struct fw_rsc_carveout *carve_rsc = (struct fw_rsc_carveout *)rsc;

	/* Validate resource fields */
	if (!carve_rsc) {
		return RPROC_ERR_RSC_TAB_NP;
	}

	if (carve_rsc->reserved) {
		return RPROC_ERR_RSC_TAB_RSVD;
	}

	if (rproc->role == RPROC_MASTER) {
		/* FIX ME: TO DO */
		return RPROC_SUCCESS;
	}

	return RPROC_SUCCESS;
}

/**
 * handle_trace_rsc
 *
 * Trace resource handler.
 *
 * @param rproc - pointer to remote remote_proc
 * @param rsc   - pointer to trace resource
 *
 * @returns - execution status
 *
 */
int handle_trace_rsc(struct remote_proc *rproc, void *rsc)
{
	(void)rproc;
	(void)rsc;

	return RPROC_ERR_RSC_TAB_NS;
}

/**
 * handle_dev_mem_rsc
 *
 * Device memory resource handler.
 *
 * @param rproc - pointer to remote remote_proc
 * @param rsc   - pointer to device memory resource
 *
 * @returns - execution status
 *
 */
int handle_dev_mem_rsc(struct remote_proc *rproc, void *rsc)
{
	(void)rproc;
	(void)rsc;

	return RPROC_ERR_RSC_TAB_NS;
}

/**
 * handle_vdev_rsc
 *
 * Virtio device resource handler
 *
 * @param rproc - pointer to remote remote_proc
 * @param rsc   - pointer to virtio device resource
 *
 * @returns - execution status
 *
 */
int handle_vdev_rsc(struct remote_proc *rproc, void *rsc)
{

	struct fw_rsc_vdev *vdev_rsc = (struct fw_rsc_vdev *)rsc;
	struct proc_vdev *vdev;

	if (!vdev_rsc) {
		return RPROC_ERR_RSC_TAB_NP;
	}

	/* Maximum supported vrings per Virtio device */
	if (vdev_rsc->num_of_vrings > RSC_TAB_MAX_VRINGS) {
		return RPROC_ERR_RSC_TAB_VDEV_NRINGS;
	}

	/* Reserved fields - must be zero */
	if (vdev_rsc->reserved[0] || vdev_rsc->reserved[1]) {
		return RPROC_ERR_RSC_TAB_RSVD;
	}

	/* Get the Virtio device from HIL proc */
	vdev = hil_get_vdev_info(rproc->proc);

	/* Initialize HIL Virtio device resources */
	vdev->num_vrings = vdev_rsc->num_of_vrings;
	vdev->dfeatures = vdev_rsc->dfeatures;
	vdev->gfeatures = vdev_rsc->gfeatures;
	vdev->vdev_info = vdev_rsc;

	return RPROC_SUCCESS;
}

/**
 * handle_rproc_mem_rsc
 *
 * This function parses rproc_mem resource.
 * This is the resource for the remote processor
 * to tell the host the memory can be used as
 * shared memory.
 *
 * @param rproc - pointer to remote remote_proc
 * @param rsc   - pointer to mmu resource
 *
 * @returns - execution status
 *
 */
int handle_rproc_mem_rsc(struct remote_proc *rproc, void *rsc)
{
	(void)rproc;
	(void)rsc;

	/* TODO: the firmware side should handle this resource properly
	 * when it is the master or when it is the remote. */
	return RPROC_SUCCESS;
}

/*
 * handle_fw_chksum_rsc
 *
 * This function parses firmware checksum resource.
 *
 * @param rproc - pointer to remote remote_proc
 * @param rsc   - pointer to mmu resource
 *
 * @returns - execution status
 *
 */
int handle_fw_chksum_rsc(struct remote_proc *rproc, void *rsc)
{
	(void)rproc;
	(void)rsc;

	/* TODO: the firmware side should handle this resource properly
	 * when it is the master or when it is the remote. */
	return RPROC_SUCCESS;
}

/**
 * handle_mmu_rsc
 *
 * This function parses mmu resource , requested by the peripheral.
 *
 * @param rproc - pointer to remote remote_proc
 * @param rsc   - pointer to mmu resource
 *
 * @returns - execution status
 *
 */
int handle_mmu_rsc(struct remote_proc *rproc, void *rsc)
{
	(void)rproc;
	(void)rsc;

	return RPROC_ERR_RSC_TAB_NS;
}
