/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	utils.c
 * @brief	Linux libmetal utility functions.
 */

#include <metal/utilities.h>
#include <metal/sys.h>

/**
 * @brief	Open (or create) a file.
 *
 * This function opens or creates a file with read/write permissions and the
 * O_CLOEXEC flag set.
 *
 * @param[in]	path	File path to open.
 * @param[in]	shm	Open shared memory (via shm_open) if non-zero.
 * @return	File descriptor.
 */
int metal_open(const char *path, int shm)
{
	const int flags = O_RDWR | O_CREAT | O_CLOEXEC;
	const int mode = S_IRUSR | S_IWUSR;
	int fd;

	if (!path || !strlen(path))
		return -EINVAL;

	fd = shm ? shm_open(path, flags, mode) : open(path, flags, mode);
	return fd < 0 ? -errno : fd;
}

/**
 * @brief	Open (or create) and unlink a file.
 *
 * This function opens or creates a file with read/write permissions and the
 * O_CLOEXEC flag set.  This file is then unlinked to ensure that it is removed
 * when the last reference to the file is dropped.  This ensures that libmetal
 * shared maps are released appropriately once all users of the shared data
 * exit.
 *
 * @param[in]	path	File path to open.
 * @param[in]	shm	Open shared memory (via shm_open) if non-zero.
 * @return	File descriptor.
 */
int metal_open_unlinked(const char *path, int shm)
{
	int fd, error;

	fd = metal_open(path, shm);
	if (fd < 0)
		return fd;

	error = shm ? shm_unlink(path) : unlink(path);
	error = error < 0 ? -errno : 0;
	if (error)
		close(fd);

	return error ? error : fd;
}

/**
 * @brief	Randomize a string.
 *
 * This function randomizes the contents of a string.
 *
 * @param[in]	template	String to be randomized.
 */
void metal_randomize_string(char *template)
{
	const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			    "abcdefghijklmnopqrstuvwxyz"
			    "0123456789-=";
	while (*template) {
		*template++ = chars[rand() % 64];
	}
}

/**
 * @brief	Create a file name template suitable for use with metal_mktemp.
 *
 * @param[out]	template	Template string to be filled in.
 * @param[in]	name		Module/user identifier used as part of the
 *				generated template.
 */
void metal_mktemp_template(char template[PATH_MAX], const char *name)
{
	snprintf(template, PATH_MAX, "%s/metal-%s-XXXXXX",
		 _metal.tmp_path, name);
}

/**
 * @brief	Create a temporary file or fifo.
 *
 * This function creates a temporary file or fifo, and sets the O_CLOEXEC flag
 * on it.
 *
 * @param[in]	template	File name template (the last 6 characters must
 *				be XXXXXX per mkstemp requirements).
 * @param[in]	fifo		Non-zero to create a FIFO instead of a regular
 *				file.
 * @return	File descriptor.
 */
int metal_mktemp(char *template, int fifo)
{
	const int mode = S_IRUSR | S_IWUSR;
	int result, len, flags;
	char *suffix;

	if (!template)
		return -EINVAL;

	len = strlen(template);
	suffix = &template[len - 6];
	if (len < 6 || strcmp(suffix, "XXXXXX")) {
		metal_log(METAL_LOG_ERROR, "template %s has no trailing pattern\n",
			  template);
		return -EINVAL;
	}

	flags = (fifo
		 ? O_RDONLY | O_NONBLOCK | O_CLOEXEC
		 : O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC);

	for (;;) {
		metal_randomize_string(suffix);
		if (fifo) {
			result = mkfifo(template, mode);
			if (result < 0) {
			       if (errno == EEXIST)
				       continue;
			       metal_log(METAL_LOG_ERROR, "mkfifo(%s) failed (%s)\n",
					 template, strerror(errno));
			       return -errno;
			}
		}

		result = open(template, flags, mode);
		if (result < 0) {
			if (fifo)
				unlink(template);
			if (errno == EEXIST)
				continue;
			metal_log(METAL_LOG_ERROR, "open() failed (%s)\n",
				  strerror(errno));
			return -errno;
		}

		return result;
	}
}

/**
 * @brief	Open (or create) and unlink a temporary file.
 *
 * This function creates a temporary file, and sets the O_CLOEXEC flag on it.
 * O_CLOEXEC flag set.  This file is then unlinked to ensure that it is removed
 * when the last reference to the file is dropped.  This ensures that libmetal
 * shared maps are released appropriately once all users of the shared data
 * exit.
 *
 * @param[in]	template	File name template (the last 6 characters must
 *				be XXXXXX per mkstemp requirements).
 * @return	File descriptor.
 */
int metal_mktemp_unlinked(char *template)
{
	int fd, error;

	fd = metal_mktemp(template, 0);
	if (fd < 0)
		return fd;

	error = unlink(template) < 0 ? -errno : 0;
	if (error)
		close(fd);

	return error ? error : fd;
}

/**
 * @brief	Map a segment of a file/device.
 *
 * This function maps a segment of a file or device into the process address
 * space, after optionally expanding the file if necessary.  If required, the
 * file is expanded to hold the requested map area.  This is done under and
 * advisory lock, and therefore the called must not have an advisory lock on
 * the file being mmapped.
 *
 * @param[in]	fd	File descriptor to map.
 * @param[in]	offset	Offset in file to map.
 * @param[in]	size	Size of region to map.
 * @param[in]	expand	Allow file expansion via ftruncate if non-zero.
 * @param[in]	flags	Flags for mmap(), MAP_SHARED included implicitly.
 * @param[out]	result	Returned pointer to new memory map.
 * @return	0 on success, or -errno on error.
 */
int metal_map(int fd, off_t offset, size_t size, int expand, int flags,
	      void **result)
{
	int prot = PROT_READ | PROT_WRITE, error;
	void *mem;

	flags |= MAP_SHARED;

	if (fd < 0) {
		fd = -1;
		flags = MAP_PRIVATE | MAP_ANONYMOUS;
	} else if (expand) {
		off_t reqsize = offset + size;
		struct stat stat;

		error = flock(fd, LOCK_EX) < 0 ? -errno : 0;
		if (!error)
			error = fstat(fd, &stat);
		if (!error && stat.st_size < reqsize)
			error = ftruncate(fd, reqsize);
		if (!error)
			flock(fd, LOCK_UN);
		if (error)
			return -errno;
	}

	mem = mmap(NULL, size, prot, flags, fd, offset);
	if (mem == MAP_FAILED)
		return -errno;
	*result = mem;
	return 0;
}

/**
 * @brief	Unmap a segment of the process address space.
 *
 * This function unmaps a segment of the process address space.
 *
 * @param[in]	mem	Segment to unmap.
 * @param[in]	size	Size of region to unmap.
 * @return	0 on success, or -errno on error.
 */
int metal_unmap(void *mem, size_t size)
{
	return munmap(mem, size) < 0 ? -errno : 0;
}

/**
 * @brief	Lock in a region of the process address space.
 *
 * @param[in]	mem	Pointer to start of region.
 * @param[in]	size	Size of region.
 * @return	0 on success, or -errno on error.
 */
int metal_mlock(void *mem, size_t size)
{
	return mlock(mem, size) ? -errno : 0;
}

int metal_virt2phys(void *addr, unsigned long *phys)
{
	off_t offset;
	uint64_t entry;
	int error;

	if (_metal.pagemap_fd < 0)
		return -ENOSYS;

	offset = ((uintptr_t)addr >> _metal.page_shift) * sizeof(entry);
	error = pread(_metal.pagemap_fd, &entry, sizeof(entry), offset);
	if (error < 0) {
		metal_log(METAL_LOG_ERROR, "failed pagemap pread (offset %llx) - %s\n",
			  (unsigned long long)offset, strerror(errno));
		return -errno;
	}

	/* Check page present and not swapped. */
	if ((entry >> 62) != 2) {
		metal_log(METAL_LOG_ERROR, "pagemap page not present, %llx -> %llx\n",
			  (unsigned long long)offset, (unsigned long long)entry);
		return -ENOENT;
	}

	*phys = (entry & ((1ULL << 54) - 1)) << _metal.page_shift;
	return 0;
}
