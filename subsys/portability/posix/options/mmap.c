/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#include <kernel_arch_interface.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/posix/sys/mman.h>
#include <zephyr/posix/unistd.h>

#define _page_size COND_CODE_1(CONFIG_MMU, (CONFIG_MMU_PAGE_SIZE), (CONFIG_POSIX_PAGE_SIZE))

int zvfs_ioctl(int fd, int cmd, va_list args);

static int p2z(int prot, int pflags)
{
	bool rw = (prot & PROT_WRITE) != 0;
	bool ex = (prot & PROT_EXEC) != 0;
	bool fixed = (pflags & MAP_FIXED) != 0;
	bool shared = (pflags & MAP_SHARED) != 0;
	bool private = (pflags & MAP_PRIVATE) != 0;

	if (!(shared ^ private)) {
		return -1;
	}

	return (rw * K_MEM_PERM_RW) | (ex * K_MEM_PERM_EXEC) | (fixed * K_MEM_DIRECT_MAP);
}

static inline int zvfs_ioctl_wrap(int fd, int cmd, ...)
{
	int ret;
	va_list args;

	va_start(args, cmd);
	ret = zvfs_ioctl(fd, cmd, args);
	va_end(args);

	return ret;
}

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	void *virt;
	uintptr_t phys;
	int zflags = p2z(prot, flags);

	if ((len == 0) || (zflags == -1)) {
		errno = EINVAL;
		return MAP_FAILED;
	}

	if ((flags & MAP_ANONYMOUS) != 0) {
		/* force behaviour to be in-line with Linux, fd is ignored */
		fd = -1;
	}

	if (fd > 0) {
		/* non-anonymous mapping */
		virt = NULL;
		if (zvfs_ioctl_wrap(fd, ZFD_IOCTL_MMAP, addr, len, prot, flags, off, &virt) < 0) {
			return MAP_FAILED;
		}

		return virt;
	}

	if (!IS_ENABLED(CONFIG_MMU)) {
		errno = ENOTSUP;
		return MAP_FAILED;
	}

	if ((flags & MAP_FIXED) == 0) {
		/* anonymous mapping */
		virt = k_mem_map(len, zflags);
	} else {
		/* a physical mapping. Care should be taken not to map the same page twice */
		virt = NULL;
		phys = POINTER_TO_UINT(addr);
		k_mem_map_phys_bare((uint8_t **)&virt, phys, (size_t)ROUND_UP(len, _page_size),
				    zflags);
	}

	if (virt == NULL) {
		errno = ENOMEM;
		return MAP_FAILED;
	}

	return virt;
}

int msync(void *addr, size_t length, int flags)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(length);
	ARG_UNUSED(flags);

	return 0;
}

int munmap(void *addr, size_t len)
{
	if (len == 0) {
		errno = EINVAL;
		return -1;
	}

	if (!IS_ENABLED(CONFIG_MMU)) {
		/* cannot munmap without an MPU */
		errno = ENOTSUP;
		return -1;
	}

	uintptr_t phys = 0;

	if (arch_page_phys_get(addr, &phys) == 0) {
		k_mem_unmap(addr, ROUND_UP(len, _page_size));
	}

	return 0;
}
