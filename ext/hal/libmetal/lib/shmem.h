/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	shmem.h
 * @brief	Shared memory primitives for libmetal.
 */

#ifndef __METAL_SHMEM__H__
#define __METAL_SHMEM__H__

#include <metal/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup shmem Shared Memory Interfaces
 *  @{ */

/** Generic shared memory data structure. */
struct metal_generic_shmem {
	const char		*name;
	struct metal_io_region	io;
	struct metal_list	node;
};

/**
 * @brief	Open a libmetal shared memory segment.
 *
 * Open a shared memory segment.
 *
 * @param[in]		name	Name of segment to open.
 * @param[in]		size	Size of segment.
 * @param[out]		io	I/O region handle, if successful.
 * @return	0 on success, or -errno on failure.
 *
 * @see metal_shmem_create
 */
extern int metal_shmem_open(const char *name, size_t size,
			    struct metal_io_region **io);

/**
 * @brief	Statically register a generic shared memory region.
 *
 * Shared memory regions may be statically registered at application
 * initialization, or may be dynamically opened.  This interface is used for
 * static registration of regions.  Subsequent calls to metal_shmem_open() look
 * up in this list of pre-registered regions.
 *
 * @param[in]	shmem	Generic shmem structure.
 * @return 0 on success, or -errno on failure.
 */
extern int metal_shmem_register_generic(struct metal_generic_shmem *shmem);

#ifdef METAL_INTERNAL

/**
 * @brief	Open a statically registered shmem segment.
 *
 * This interface is meant for internal libmetal use within system specific
 * shmem implementations.
 *
 * @param[in]		name	Name of segment to open.
 * @param[in]		size	Size of segment.
 * @param[out]		io	I/O region handle, if successful.
 * @return	0 on success, or -errno on failure.
 */
int metal_shmem_open_generic(const char *name, size_t size,
			     struct metal_io_region **result);

#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_SHMEM__H__ */
