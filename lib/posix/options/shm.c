/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdio.h>

#include <kernel_arch_interface.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/sys/mman.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/hash_function.h>

#define _page_size COND_CODE_1(CONFIG_MMU, (CONFIG_MMU_PAGE_SIZE), (PAGE_SIZE))

static const struct fd_op_vtable shm_vtable;

static sys_dlist_t shm_list = SYS_DLIST_STATIC_INIT(&shm_list);

struct shm_obj {
	uint8_t *mem;
	sys_dnode_t node;
	size_t refs;
	size_t size;
	uint32_t hash;
	bool unlinked: 1;
	bool mapped: 1;
};

static inline uint32_t hash32(const char *str, size_t n)
{
	/* we need a hasher that is not sensitive to input alignment */
	return sys_hash32_djb2(str, n);
}

static bool shm_obj_name_valid(const char *name, size_t len)
{
	if (name == NULL) {
		return false;
	}

	if (name[0] != '/') {
		return false;
	}

	if (len < 2) {
		return false;
	}

	return true;
}

static struct shm_obj *shm_obj_find(uint32_t key)
{
	struct shm_obj *shm;

	SYS_DLIST_FOR_EACH_CONTAINER(&shm_list, shm, node) {
		if (shm->hash == key) {
			return shm;
		}
	}

	return NULL;
}

static void shm_obj_add(struct shm_obj *shm)
{
	sys_dlist_init(&shm->node);
	sys_dlist_append(&shm_list, &shm->node);
}

static void shm_obj_remove(struct shm_obj *shm)
{
	sys_dlist_remove(&shm->node);
	if (shm->size > 0) {
		if (IS_ENABLED(CONFIG_MMU)) {
			uintptr_t phys = 0;

			if (arch_page_phys_get(shm->mem, &phys) == 0) {
				k_mem_unmap(shm->mem, ROUND_UP(shm->size, _page_size));
			}
		} else {
			k_free(shm->mem);
		}
	}
	k_free(shm);
}

static int shm_fstat(struct shm_obj *shm, struct stat *st)
{
	*st = (struct stat){0};
	st->st_mode = ZVFS_MODE_IFSHM;
	st->st_size = shm->size;

	return 0;
}

static int shm_ftruncate(struct shm_obj *shm, off_t length)
{
	void *virt;

	if (length < 0) {
		errno = EINVAL;
		return -1;
	}

	if (length == 0) {
		if (shm->size != 0) {
			/* only allow resizing this once, for consistence */
			errno = EBUSY;
			return -1;
		}

		return 0;
	}

	if (IS_ENABLED(CONFIG_MMU)) {
		virt = k_mem_map(ROUND_UP(length, _page_size), K_MEM_PERM_RW);
	} else {
		virt = k_calloc(1, length);
	}

	if (virt == NULL) {
		errno = ENOMEM;
		return -1;
	}

	shm->mem = virt;
	shm->size = length;

	return 0;
}

static off_t shm_lseek(struct shm_obj *shm, off_t offset, int whence, size_t cur)
{
	size_t addend;

	switch (whence) {
	case SEEK_SET:
		addend = 0;
		break;
	case SEEK_CUR:
		addend = cur;
		break;
	case SEEK_END:
		addend = shm->size;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if ((INTPTR_MAX - addend) < offset) {
		errno = EOVERFLOW;
		return -1;
	}

	offset += addend;
	if (offset < 0) {
		errno = EINVAL;
		return -1;
	}

	return offset;
}

static int shm_mmap(struct shm_obj *shm, void *addr, size_t len, int prot, int flags, off_t off,
		    void **virt)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(prot);
	__ASSERT_NO_MSG(virt != NULL);

	if ((len == 0) || (off < 0) || ((flags & MAP_FIXED) != 0) ||
	    ((off & (_page_size - 1)) != 0) || ((len + off) > shm->size)) {
		errno = EINVAL;
		return -1;
	}

	if (!IS_ENABLED(CONFIG_MMU)) {
		errno = ENOTSUP;
		return -1;
	}

	if (shm->mem == NULL) {
		errno = ENOMEM;
		return -1;
	}

	/*
	 * Note: due to Zephyr's page mapping algorithm, physical pages can only have 1
	 * mapping, so different file handles will have the same virtual memory address
	 * underneath.
	 */
	*virt = shm->mem + off;

	return 0;
}

static ssize_t shm_rw(struct shm_obj *shm, void *buf, size_t size, bool is_write, size_t offset)
{
	if (offset >= shm->size) {
		size = 0;
	} else {
		size = MIN(size, shm->size - offset);
	}

	if (size > 0) {
		if (is_write) {
			memcpy(&shm->mem[offset], buf, size);
		} else {
			memcpy(buf, &shm->mem[offset], size);
		}
	}

	return size;
}

static ssize_t shm_read(void *obj, void *buf, size_t sz, size_t offset)
{
	return shm_rw((struct shm_obj *)obj, buf, sz, false, offset);
}

static ssize_t shm_write(void *obj, const void *buf, size_t sz, size_t offset)
{
	return shm_rw((struct shm_obj *)obj, (void *)buf, sz, true, offset);
}

static int shm_close(void *obj)
{
	struct shm_obj *shm = obj;

	shm->refs -= (shm->refs > 0) ? 1 : 0;
	if (shm->unlinked && (shm->refs == 0)) {
		shm_obj_remove(shm);
	}

	return 0;
}

static int shm_ioctl(void *obj, unsigned int request, va_list args)
{
	struct shm_obj *shm = obj;

	switch (request) {
	case ZFD_IOCTL_LSEEK: {
		off_t offset = va_arg(args, off_t);
		int whence = va_arg(args, int);
		size_t cur = va_arg(args, size_t);

		return shm_lseek(shm, offset, whence, cur);
	} break;
	case ZFD_IOCTL_MMAP: {
		void *addr = va_arg(args, void *);
		size_t len = va_arg(args, size_t);
		int prot = va_arg(args, int);
		int flags = va_arg(args, int);
		off_t off = va_arg(args, off_t);
		void **maddr = va_arg(args, void **);

		return shm_mmap(shm, addr, len, prot, flags, off, maddr);
	} break;
	case ZFD_IOCTL_SET_LOCK:
		break;
	case ZFD_IOCTL_STAT: {
		struct stat *st = va_arg(args, struct stat *);

		return shm_fstat(shm, st);
	} break;
	case ZFD_IOCTL_TRUNCATE: {
		off_t length = va_arg(args, off_t);

		return shm_ftruncate(shm, length);
	} break;
	default:
		errno = ENOTSUP;
		return -1;
	}

	return 0;
}

static const struct fd_op_vtable shm_vtable = {
	.read_offs = shm_read,
	.write_offs = shm_write,
	.close = shm_close,
	.ioctl = shm_ioctl,
};

int shm_open(const char *name, int oflag, mode_t mode)
{
	int fd;
	uint32_t key;
	struct shm_obj *shm;
	bool rd = (oflag & O_RDONLY) != 0;
	bool rw = (oflag & O_RDWR) != 0;
	bool creat = (oflag & O_CREAT) != 0;
	bool excl = (oflag & O_EXCL) != 0;
	bool trunc = false; /* (oflag & O_TRUNC) != 0 */
	size_t name_len = (name == NULL) ? 0 : strnlen(name, PATH_MAX);

	/* revisit when file-based permissions are available */
	if ((mode & 0777) == 0) {
		errno = EINVAL;
		return -1;
	}

	if (!(rd ^ rw)) {
		errno = EINVAL;
		return -1;
	}

	if (rd && trunc) {
		errno = EINVAL;
		return -1;
	}

	if (!shm_obj_name_valid(name, name_len)) {
		errno = EINVAL;
		return -1;
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		errno = EMFILE;
		return -1;
	}

	key = hash32(name, name_len);
	shm = shm_obj_find(key);
	if ((shm != NULL) && shm->unlinked) {
		/* we cannot open a shm object that has already been unlinked */
		errno = EACCES;
		return -1;
	}

	if (creat) {
		if ((shm != NULL) && excl) {
			zvfs_free_fd(fd);
			errno = EEXIST;
			return -1;
		}

		if (shm == NULL) {
			shm = k_calloc(1, sizeof(*shm));
			if (shm == NULL) {
				zvfs_free_fd(fd);
				errno = ENOSPC;
				return -1;
			}

			shm->hash = key;
			shm_obj_add(shm);
		}
	} else if (shm == NULL) {
		errno = ENOENT;
		return -1;
	}

	++shm->refs;
	zvfs_finalize_typed_fd(fd, shm, &shm_vtable, ZVFS_MODE_IFSHM);

	return fd;
}

int shm_unlink(const char *name)
{
	uint32_t key;
	struct shm_obj *shm;
	size_t name_len = (name == NULL) ? 0 : strnlen(name, PATH_MAX);

	if (!shm_obj_name_valid(name, name_len)) {
		errno = EINVAL;
		return -1;
	}

	key = hash32(name, name_len);
	shm = shm_obj_find(key);
	if ((shm == NULL) || shm->unlinked) {
		errno = ENOENT;
		return -1;
	}

	shm->unlinked = true;
	if (shm->refs == 0) {
		shm_obj_remove(shm);
	}

	return 0;
}
