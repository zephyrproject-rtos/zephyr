/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       remoteproc_loader.h
 *
 * COMPONENT
 *
 *         OpenAMP stack.
 *
 * DESCRIPTION
 *
 *       This file provides definitions for remoteproc loader
 *
 *
 **************************************************************************/
#ifndef REMOTEPROC_LOADER_H_
#define REMOTEPROC_LOADER_H_

#include <metal/io.h>
#include <metal/list.h>
#include <metal/sys.h>
#include <openamp/remoteproc.h>

#if defined __cplusplus
extern "C" {
#endif

/* Loader feature macros */
#define SUPPORT_SEEK 1UL

/* Remoteproc loader any address */
#define RPROC_LOAD_ANYADDR ((metal_phys_addr_t)-1)

/* Remoteproc loader Exectuable Image Parsing States */
/* Remoteproc loader parser intial state */
#define RPROC_LOADER_NOT_READY      0x0UL
/* Remoteproc loader ready to load, even it can be not finish parsing */
#define RPROC_LOADER_READY_TO_LOAD  0x10000UL
/* Remoteproc loader post data load */
#define RPROC_LOADER_POST_DATA_LOAD 0x20000UL
/* Remoteproc loader finished loading */
#define RPROC_LOADER_LOAD_COMPLETE  0x40000UL
/* Remoteproc loader state mask */
#define RPROC_LOADER_MASK           0x00FF0000UL
/* Remoteproc loader private mask */
#define RPROC_LOADER_PRIVATE_MASK   0x0000FFFFUL
/* Remoteproc loader reserved mask */
#define RPROC_LOADER_RESERVED_MASK  0x0F000000UL

/**
 * struct image_store_ops - user defined image store operations
 * @open: user defined callback to open the "firmware" to prepare loading
 * @close: user defined callback to close the "firmware" to clean up
 *         after loading
 * @load: user defined callback to load the firmware contents to target
 *        memory or local memory
 * @features: loader supported features. e.g. seek
 */
struct image_store_ops {
	int (*open)(void *store, const char *path, const void **img_data);
	void (*close)(void *store);
	int (*load)(void *store, size_t offset, size_t size,
		    const void **data,
		    metal_phys_addr_t pa,
		    struct metal_io_region *io, char is_blocking);
	unsigned int features;
};

/**
 * struct loader_ops - loader oeprations
 * @load_header: define how to get the executable headers
 * @load_data: define how to load the target data
 * @locate_rsc_table: define how to get the resource table target address,
 *                    offset to the ELF image file and size of the resource
 *                    table.
 * @release: define how to release the loader
 * @get_entry: get entry address
 * @get_load_state: get load state from the image information
 */
struct loader_ops {
	int (*load_header)(const void *img_data, size_t offset, size_t len,
			   void **img_info, int last_state,
			   size_t *noffset, size_t *nlen);
	int (*load_data)(struct remoteproc *rproc,
			 const void *img_data, size_t offset, size_t len,
			 void **img_info, int last_load_state,
			 metal_phys_addr_t *da,
			 size_t *noffset, size_t *nlen,
			 unsigned char *padding, size_t *nmemsize);
	int (*locate_rsc_table)(void *img_info, metal_phys_addr_t *da,
				size_t *offset, size_t *size);
	void (*release)(void *img_info);
	metal_phys_addr_t (*get_entry)(void *img_info);
	int (*get_load_state)(void *img_info);
};

#if defined __cplusplus
}
#endif

#endif /* REMOTEPROC_LOADER_H_ */
