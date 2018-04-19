/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/sys.h
 * @brief	Linux system primitives for libmetal.
 */

#ifndef __METAL_SYS__H__
#error "Include metal/sys.h instead of metal/linux/sys.h"
#endif

#ifndef __METAL_LINUX_SYS__H__
#define __METAL_LINUX_SYS__H__

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/futex.h>
#include <sysfs/libsysfs.h>
#ifdef HAVE_HUGETLBFS_H
#include <hugetlbfs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define METAL_INVALID_VADDR     NULL
#define MAX_PAGE_SIZES		32

/** Structure of shared page or hugepage sized data. */
struct metal_page_size {
	/** Page size. */
	size_t	page_size;

	/** Page shift. */
	unsigned long page_shift;

	/** Path to hugetlbfs (or tmpfs) mount point. */
	char path[PATH_MAX];

	/** Flags to use for mmap. */
	int mmap_flags;
};

/** Structure of linux specific libmetal runtime state. */
struct metal_state {

	/** Common (system independent) data. */
	struct metal_common_state common;

	/** file descriptor for shared data. */
	int			data_fd;

	/** system page size. */
	unsigned long		page_size;

	/** system page shift. */
	unsigned long		page_shift;

	/** sysfs mount point. */
	const char		*sysfs_path;

	/** sysfs mount point. */
	const char		*tmp_path;

	/** available page sizes. */
	struct metal_page_size	page_sizes[MAX_PAGE_SIZES];

	/** number of available page sizes. */
	int			num_page_sizes;

	/** File descriptor for /proc/self/pagemap (or -1). */
	int			pagemap_fd;
};

#ifdef METAL_INTERNAL
extern int metal_linux_bus_init(void);
extern void metal_linux_bus_finish(void);

extern int metal_open(const char *path, int shm);
extern int metal_open_unlinked(const char *path, int shm);
extern int metal_mktemp(char *template, int fifo);
extern int metal_mktemp_unlinked(char *template);

extern int metal_map(int fd, off_t offset, size_t size, int expand,
		     int flags, void **result);
extern int metal_unmap(void *mem, size_t size);
extern int metal_mlock(void *mem, size_t size);

extern void metal_randomize_string(char *template);
extern void metal_mktemp_template(char template[PATH_MAX],
				  const char *name);
extern int metal_virt2phys(void *addr, unsigned long *phys);

#define metal_for_each_page_size_up(ps)					\
	for ((ps) = &_metal.page_sizes[0];				\
	     (ps) <= &_metal.page_sizes[_metal.num_page_sizes - 1];	\
	     (ps)++)

#define metal_for_each_page_size_down(ps)				\
	for ((ps) = &_metal.page_sizes[_metal.num_page_sizes - 1];	\
	     (ps) >= &_metal.page_sizes[0];				\
	     (ps)--)

#endif

#ifdef __cplusplus
}
#endif

#endif /* __METAL_LINUX_SYS__H__ */
