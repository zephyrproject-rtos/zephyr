/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2015 Xilinx, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <openamp/remoteproc.h>
#include <openamp/remoteproc_loader.h>
#include <openamp/rsc_table_parser.h>
#include <openamp/hil.h>
#include <metal/sys.h>
#include <metal/alloc.h>
#include <metal/sleep.h>

/**
 * remoteproc_resource_init
 *
 * Initializes resources for remoteproc remote configuration. Only
 * remoteproc remote applications are allowed to call this function.
 *
 * @param rsc_info          - pointer to resource table info control
 * 							  block
 * @param proc              - pointer to the hil_proc
 * @param channel_created   - callback function for channel creation
 * @param channel_destroyed - callback function for channel deletion
 * @param default_cb        - default callback for channel I/O
 * @param rproc_handle      - pointer to new remoteproc instance
 * @param rpmsg_role        - 1 for rpmsg master, or 0 for rpmsg slave
 *
 * @param returns - status of function execution
 *
 */
int remoteproc_resource_init(struct rsc_table_info *rsc_info,
			     struct hil_proc *proc,
			     rpmsg_chnl_cb_t channel_created,
			     rpmsg_chnl_cb_t channel_destroyed,
			     rpmsg_rx_cb_t default_cb,
			     struct remote_proc **rproc_handle,
			     int rpmsg_role)
{

	struct remote_proc *rproc;
	int status;
	int remote_rpmsg_role;

	if (!rsc_info || !proc) {
		return RPROC_ERR_PARAM;
	}

	rproc = metal_allocate_memory(sizeof(struct remote_proc));
	if (rproc) {
		memset(rproc, 0x00, sizeof(struct remote_proc));
		/* There can be only one master for remote configuration so use the
		 * rsvd cpu id for creating hil proc */
		rproc->proc = proc;
		status = hil_init_proc(proc);
		if (!status) {
			/* Parse resource table */
			status =
			    handle_rsc_table(rproc, rsc_info->rsc_tab,
					     rsc_info->size);
			if (status == RPROC_SUCCESS) {
				/* Initialize RPMSG "messaging" component */
				*rproc_handle = rproc;
				remote_rpmsg_role = (rpmsg_role == RPMSG_MASTER?
						RPMSG_REMOTE : RPMSG_MASTER);
				status =
				    rpmsg_init(proc,
					       &rproc->rdev, channel_created,
					       channel_destroyed, default_cb,
					       remote_rpmsg_role);
			} else {
				status = RPROC_ERR_NO_RSC_TABLE;
			}
		} else {
			status = RPROC_ERR_CPU_INIT;
		}
	} else {
		status = RPROC_ERR_NO_MEM;
	}

	/* Cleanup in case of error */
	if (status != RPROC_SUCCESS) {
		*rproc_handle = 0;
		(void)remoteproc_resource_deinit(rproc);
		return status;
	}
	return status;
}

/**
 * remoteproc_resource_deinit
 *
 * Uninitializes resources for remoteproc "remote" configuration.
 *
 * @param rproc - pointer to rproc instance
 *
 * @param returns - status of function execution
 *
 */

int remoteproc_resource_deinit(struct remote_proc *rproc)
{
	if (rproc) {
		if (rproc->rdev) {
			rpmsg_deinit(rproc->rdev);
		}
		if (rproc->proc) {
			hil_delete_proc(rproc->proc);
			rproc->proc = NULL;
		}
		metal_free_memory(rproc);
	}

	return RPROC_SUCCESS;
}

/**
 * remoteproc_init
 *
 * Initializes resources for remoteproc master configuration. Only
 * remoteproc master applications are allowed to call this function.
 *
 * @param fw_name           - name of frimware
 * @param proc              - pointer to hil_proc
 * @param channel_created   - callback function for channel creation
 * @param channel_destroyed - callback function for channel deletion
 * @param default_cb        - default callback for channel I/O
 * @param rproc_handle      - pointer to new remoteproc instance
 *
 * @param returns - status of function execution
 *
 */
int remoteproc_init(char *fw_name, struct hil_proc *proc,
		    rpmsg_chnl_cb_t channel_created,
		    rpmsg_chnl_cb_t channel_destroyed, rpmsg_rx_cb_t default_cb,
		    struct remote_proc **rproc_handle)
{

	struct remote_proc *rproc;
	struct resource_table *rsc_table;
	unsigned int fw_size, rsc_size;
	uintptr_t fw_addr;
	int status;

	if (!fw_name) {
		return RPROC_ERR_PARAM;
	}

	rproc = metal_allocate_memory(sizeof(struct remote_proc));
	if (rproc) {
		memset((void *)rproc, 0x00, sizeof(struct remote_proc));
		/* Create proc instance */
		rproc->proc = proc;
		status = hil_init_proc(proc);
		if (!status) {
			/* Retrieve firmware attributes */
			status =
			    hil_get_firmware(fw_name, &fw_addr,
					     &fw_size);
			if (!status) {
				/* Initialize ELF loader - currently only ELF format is supported */
				rproc->loader =
				    remoteproc_loader_init(ELF_LOADER);
				if (rproc->loader) {
					/* Attach the given firmware with the ELF parser/loader */
					status =
					    remoteproc_loader_attach_firmware
					    (rproc->loader,
					     (void *)fw_addr);
				} else {
					status = RPROC_ERR_LOADER;
				}
			}
		} else {
			status = RPROC_ERR_CPU_INIT;
		}
	} else {
		status = RPROC_ERR_NO_MEM;
	}

	if (!status) {
		rproc->role = RPROC_MASTER;

		/* Get resource table from firmware */
		rsc_table =
		    remoteproc_loader_retrieve_resource_section(rproc->loader,
								&rsc_size);
		if (rsc_table) {
			/* Parse resource table */
			status = handle_rsc_table(rproc, rsc_table, rsc_size);
		} else {
			status = RPROC_ERR_NO_RSC_TABLE;
		}
	}

	/* Cleanup in case of error */
	if (status != RPROC_SUCCESS) {
		(void)remoteproc_deinit(rproc);
		return status;
	}

	rproc->channel_created = channel_created;
	rproc->channel_destroyed = channel_destroyed;
	rproc->default_cb = default_cb;

	*rproc_handle = rproc;

	return status;
}

/**
 * remoteproc_deinit
 *
 * Uninitializes resources for remoteproc "master" configuration.
 *
 * @param rproc - pointer to remote proc instance
 *
 * @param returns - status of function execution
 *
 */
int remoteproc_deinit(struct remote_proc *rproc)
{

	if (rproc) {
		if (rproc->loader) {
			(void)remoteproc_loader_delete(rproc->loader);
			rproc->loader = RPROC_NULL;
		}
		if (rproc->proc) {
			hil_delete_proc(rproc->proc);
			rproc->proc = RPROC_NULL;
		}
		metal_free_memory(rproc);
	}

	return RPROC_SUCCESS;
}

/**
 * remoteproc_boot
 *
 * This function loads the image on the remote processor and starts
 * its execution from image load address.
 *
 * @param rproc - pointer to remoteproc instance to boot
 *
 * @param returns - status of function execution
 */
int remoteproc_boot(struct remote_proc *rproc)
{

	void *load_addr;
	int status;

	if (!rproc) {
		return RPROC_ERR_PARAM;
	}

	/* Stop the remote CPU */
	hil_shutdown_cpu(rproc->proc);

	/* Load the firmware */
	status = remoteproc_loader_load_remote_firmware(rproc->loader);
	if (status == RPROC_SUCCESS) {
		load_addr = remoteproc_get_load_address(rproc->loader);
		if (load_addr != RPROC_ERR_PTR) {
			/* Start the remote cpu */
			status = hil_boot_cpu(rproc->proc,
					      (uintptr_t)load_addr);
			if (status == RPROC_SUCCESS) {
				/* Wait for remote side to come up. This delay is arbitrary and may
				 * need adjustment for different configuration of remote systems */
				metal_sleep_usec(RPROC_BOOT_DELAY);

				/* Initialize RPMSG "messaging" component */

				/* It is a work-around to work with remote Linux context. 
				   Since the upstream Linux rpmsg implementation always 
				   assumes itself to be an rpmsg master, we initialize
				   the remote device as an rpmsg master for remote Linux
				   configuration only. */
#if defined (OPENAMP_REMOTE_LINUX_ENABLE)
				status =
				    rpmsg_init(rproc->proc,
					       &rproc->rdev,
					       rproc->channel_created,
					       rproc->channel_destroyed,
					       rproc->default_cb, RPMSG_MASTER);
#else
				status =
				    rpmsg_init(rproc->proc,
					       &rproc->rdev,
					       rproc->channel_created,
					       rproc->channel_destroyed,
					       rproc->default_cb, RPMSG_REMOTE);
#endif
			}
		} else {
			status = RPROC_ERR_LOADER;
		}
	} else {
		status = RPROC_ERR_LOADER;
	}

	return status;
}

/**
 * remoteproc_shutdown
 *
 * This function shutdowns the remote execution context
 *
 * @param rproc - pointer to remote proc instance to shutdown
 *
 * @param returns - status of function execution
 */
int remoteproc_shutdown(struct remote_proc *rproc)
{

	if (rproc) {
		if (rproc->proc) {
			hil_shutdown_cpu(rproc->proc);
		}
		if (rproc->rdev) {
			rpmsg_deinit(rproc->rdev);
			rproc->rdev = RPROC_NULL;
		}
	}

	return RPROC_SUCCESS;
}
