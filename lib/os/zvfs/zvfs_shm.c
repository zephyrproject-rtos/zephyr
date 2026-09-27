/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Shared memory objects: named, system-wide, reached through descriptors.
 *
 * An object owns its pages, pinned k_mem_map() pages with an MMU and a block
 * of the kernel heap otherwise, sized once by ZFD_IOCTL_TRUNCATE. It answers
 * ZFD_IOCTL_MMAP with the address of those pages, so zvfs_mmap() maps it in
 * place and every mapping aliases the same memory. The name is a lookup key
 * only: zvfs_shm_unlink() drops it, and the object goes when its descriptors
 * and mappings are gone.
 *
 * Every zvfs_shm_open() hands out its own open description: the fd table
 * shares a file position between descriptors of one I/O object, which is
 * right for duplicated descriptors and wrong for independent opens of the
 * same object. The description holds a reference on the object and is
 * released, with zvfs_fd_obj_refcount() counting its descriptors and its
 * mappings together, when the last of them goes.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/elastipool.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include <zephyr/zvfs/shm.h>

#define SHM_PAGE       CONFIG_ZVFS_MMAP_PAGE_SIZE
#define SHM_FLAGS_MASK (ZVFS_O_ACCMODE | ZVFS_O_CREAT | ZVFS_O_EXCL | ZVFS_O_TRUNC)

struct zvfs_shm {
	sys_dnode_t node;
	uint8_t *mem;
	size_t size;
	/* open descriptions referring to the object */
	int refs;
	int mode;
	bool unlinked;
	char name[CONFIG_ZVFS_SHM_NAME_MAX];
};

/* one per zvfs_shm_open(): the I/O object behind the descriptors and mappings */
struct zvfs_shm_desc {
	struct zvfs_shm *shm;
};

static struct zvfs_shm shm_storage[CONFIG_ZVFS_SHM_MAX];
SYS_ELASTIPOOL_DEFINE_ADVANCED(shm_pool, sizeof(struct zvfs_shm), __alignof(struct zvfs_shm),
			       CONFIG_ZVFS_SHM_MAX, CONFIG_ZVFS_SHM_MAX, NULL, shm_storage, static);

static struct zvfs_shm_desc desc_storage[CONFIG_ZVFS_SHM_DESC_MAX];
SYS_ELASTIPOOL_DEFINE_ADVANCED(desc_pool, sizeof(struct zvfs_shm_desc),
			       __alignof(struct zvfs_shm_desc), CONFIG_ZVFS_SHM_DESC_MAX,
			       CONFIG_ZVFS_SHM_DESC_MAX, NULL, desc_storage, static);

/* the named objects; an unlinked object is off the list until it is freed */
static K_MUTEX_DEFINE(shm_lock);
static sys_dlist_t shm_named = SYS_DLIST_STATIC_INIT(&shm_named);

static const struct fd_op_vtable shm_vtable;

static struct zvfs_shm *shm_find_locked(const char *name)
{
	struct zvfs_shm *shm;

	SYS_DLIST_FOR_EACH_CONTAINER(&shm_named, shm, node) {
		if (strcmp(shm->name, name) == 0) {
			return shm;
		}
	}

	return NULL;
}

static void shm_free(struct zvfs_shm *shm)
{
	if (shm->mem != NULL) {
#ifdef CONFIG_MMU
		k_mem_unmap(shm->mem, ROUND_UP(shm->size, SHM_PAGE));
#else
		k_free(shm->mem);
#endif
	}
	(void)sys_elastipool_free(&shm_pool, shm);
}

/* an object is sized once: its pages may be mapped */
static int shm_truncate(struct zvfs_shm *shm, size_t length)
{
	void *mem;
	size_t size;

	if (shm->size != 0) {
		if (length == shm->size) {
			return 0;
		}
		errno = EBUSY;
		return -1;
	}
	if (length == 0) {
		return 0;
	}

	if (size_add_overflow(length, SHM_PAGE - 1, &size)) {
		errno = EINVAL;
		return -1;
	}
	size &= ~(size_t)(SHM_PAGE - 1);

#ifdef CONFIG_MMU
	/* pinned: the pages are present in every domain that maps them */
	mem = k_mem_map(size, K_MEM_PERM_RW | K_MEM_MAP_LOCK);
#else
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		/* one memory-protection region: a power of two, aligned to its size */
		size = BIT(32 - u32_count_leading_zeros(size - 1));
	}
	mem = k_aligned_alloc(IS_ENABLED(CONFIG_USERSPACE) ? size : SHM_PAGE, size);
	if (mem != NULL) {
		memset(mem, 0, size);
	}
#endif
	if (mem == NULL) {
		errno = ENOMEM;
		return -1;
	}

	shm->mem = mem;
	shm->size = length;

	return 0;
}

static ssize_t shm_rw(struct zvfs_shm *shm, void *buf, size_t sz, bool is_write, size_t offset)
{
	if (offset >= shm->size) {
		return 0;
	}
	sz = MIN(sz, shm->size - offset);
	if (is_write) {
		memcpy(&shm->mem[offset], buf, sz);
	} else {
		memcpy(buf, &shm->mem[offset], sz);
	}

	return sz;
}

static ssize_t shm_read(void *obj, void *buf, size_t sz, size_t offset)
{
	return shm_rw(((struct zvfs_shm_desc *)obj)->shm, buf, sz, false, offset);
}

static ssize_t shm_write(void *obj, const void *buf, size_t sz, size_t offset)
{
	return shm_rw(((struct zvfs_shm_desc *)obj)->shm, (void *)buf, sz, true, offset);
}

/* the description's last descriptor and last mapping are gone */
static int shm_close(void *obj)
{
	struct zvfs_shm_desc *desc = obj;
	struct zvfs_shm *shm = desc->shm;

	(void)k_mutex_lock(&shm_lock, K_FOREVER);
	(void)sys_elastipool_free(&desc_pool, desc);
	shm->refs--;
	if (shm->unlinked && (shm->refs == 0)) {
		shm_free(shm);
	}
	k_mutex_unlock(&shm_lock);

	return 0;
}

static int shm_lseek(struct zvfs_shm *shm, off_t offset, int whence, size_t cur)
{
	size_t base;

	switch (whence) {
	case FS_SEEK_SET:
		base = 0;
		break;
	case FS_SEEK_CUR:
		base = cur;
		break;
	case FS_SEEK_END:
		base = shm->size;
		break;
	default:
		errno = EINVAL;
		return -1;
	}
	if ((offset < 0) && ((size_t)-offset > base)) {
		errno = EINVAL;
		return -1;
	}
	if ((offset > 0) && ((size_t)offset > (size_t)INT_MAX - base)) {
		errno = EOVERFLOW;
		return -1;
	}

	return (int)(base + offset);
}

static int shm_ioctl(void *obj, unsigned int request, va_list args)
{
	struct zvfs_shm *shm = ((struct zvfs_shm_desc *)obj)->shm;

	switch (request) {
	case ZFD_IOCTL_LSEEK: {
		off_t offset = va_arg(args, off_t);
		int whence = va_arg(args, int);
		size_t cur = va_arg(args, size_t);

		return shm_lseek(shm, offset, whence, cur);
	}
	case ZFD_IOCTL_STAT: {
		struct zvfs_stat *st = va_arg(args, struct zvfs_stat *);

		*st = (struct zvfs_stat){.size = shm->size, .mode = ZVFS_MODE_IFSHM | shm->mode};
		return 0;
	}
	case ZFD_IOCTL_TRUNCATE: {
		off_t length = va_arg(args, off_t);
		int ret;

		if (length < 0) {
			errno = EINVAL;
			return -1;
		}
		(void)k_mutex_lock(&shm_lock, K_FOREVER);
		ret = shm_truncate(shm, (size_t)length);
		k_mutex_unlock(&shm_lock);
		return ret;
	}
	case ZFD_IOCTL_MMAP: {
		size_t off = va_arg(args, size_t);
		size_t len = va_arg(args, size_t);
		void **addr = va_arg(args, void **);

		/* the object's own pages back every mapping */
		if ((len == 0) || (off > shm->size) || (len > shm->size - off) ||
		    (shm->mem == NULL)) {
			errno = ENXIO;
			return -1;
		}
		*addr = shm->mem + off;
		return 0;
	}
	case ZFD_IOCTL_FSYNC:
	case ZFD_IOCTL_SET_LOCK:
		return 0;
	default:
		errno = EOPNOTSUPP;
		return -1;
	}
}

static const struct fd_op_vtable shm_vtable = {
	.read_offs = shm_read,
	.write_offs = shm_write,
	.close = shm_close,
	.ioctl = shm_ioctl,
};

int z_impl_zvfs_shm_open(const char *name, int flags, int mode)
{
	struct zvfs_shm *shm;
	struct zvfs_shm_desc *desc;
	void *obj;
	int acc = flags & ZVFS_O_ACCMODE;
	bool creat = (flags & ZVFS_O_CREAT) != 0;
	int fd;

	if ((name == NULL) || (name[0] == '\0') || ((flags & ~SHM_FLAGS_MASK) != 0) ||
	    ((acc != ZVFS_O_RDONLY) && (acc != ZVFS_O_RDWR)) ||
	    ((acc == ZVFS_O_RDONLY) && ((flags & ZVFS_O_TRUNC) != 0))) {
		errno = EINVAL;
		return -1;
	}
	if (strlen(name) >= CONFIG_ZVFS_SHM_NAME_MAX) {
		errno = ENAMETOOLONG;
		return -1;
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	(void)k_mutex_lock(&shm_lock, K_FOREVER);

	if (sys_elastipool_alloc(&desc_pool, &obj) != 0) {
		errno = EMFILE;
		goto err;
	}
	desc = obj;

	shm = shm_find_locked(name);
	if (shm != NULL) {
		if (creat && ((flags & ZVFS_O_EXCL) != 0)) {
			errno = EEXIST;
			goto err_desc;
		}
		if (((flags & ZVFS_O_TRUNC) != 0) && (shm->size != 0)) {
			/* the object's pages may be mapped: it keeps its size */
			errno = EBUSY;
			goto err_desc;
		}
	} else {
		if (!creat) {
			errno = ENOENT;
			goto err_desc;
		}
		if (sys_elastipool_alloc(&shm_pool, &obj) != 0) {
			errno = ENOSPC;
			goto err_desc;
		}
		shm = obj;
		*shm = (struct zvfs_shm){.mode = mode};
		strcpy(shm->name, name);
		sys_dlist_append(&shm_named, &shm->node);
	}

	desc->shm = shm;
	shm->refs++;
	zvfs_finalize_typed_fd(fd, desc, &shm_vtable, ZVFS_MODE_IFSHM);
	k_mutex_unlock(&shm_lock);

	return fd;

err_desc:
	(void)sys_elastipool_free(&desc_pool, desc);
err:
	k_mutex_unlock(&shm_lock);
	zvfs_free_fd(fd);

	return -1;
}

int z_impl_zvfs_shm_unlink(const char *name)
{
	struct zvfs_shm *shm;

	if ((name == NULL) || (name[0] == '\0')) {
		errno = EINVAL;
		return -1;
	}
	if (strlen(name) >= CONFIG_ZVFS_SHM_NAME_MAX) {
		errno = ENAMETOOLONG;
		return -1;
	}

	(void)k_mutex_lock(&shm_lock, K_FOREVER);

	shm = shm_find_locked(name);
	if (shm == NULL) {
		k_mutex_unlock(&shm_lock);
		errno = ENOENT;
		return -1;
	}

	/* the name goes now; the object goes with its last open description */
	sys_dlist_remove(&shm->node);
	shm->unlinked = true;
	if (shm->refs == 0) {
		shm_free(shm);
	}

	k_mutex_unlock(&shm_lock);

	return 0;
}

#ifdef CONFIG_USERSPACE
static int copy_user_name(char *dst, const char *src)
{
	int err;

	if (src == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (k_usermode_string_nlen(src, CONFIG_ZVFS_SHM_NAME_MAX, &err) >=
	    CONFIG_ZVFS_SHM_NAME_MAX) {
		K_OOPS(err != 0);
		errno = ENAMETOOLONG;
		return -1;
	}
	K_OOPS(err != 0);
	K_OOPS(k_usermode_string_copy(dst, src, CONFIG_ZVFS_SHM_NAME_MAX));

	return 0;
}

static int z_vrfy_zvfs_shm_open(const char *name, int flags, int mode)
{
	char kname[CONFIG_ZVFS_SHM_NAME_MAX];

	if (copy_user_name(kname, name) < 0) {
		return -1;
	}

	return z_impl_zvfs_shm_open(kname, flags, mode);
}
#include <zephyr/syscalls/zvfs_shm_open_mrsh.c>

static int z_vrfy_zvfs_shm_unlink(const char *name)
{
	char kname[CONFIG_ZVFS_SHM_NAME_MAX];

	if (copy_user_name(kname, name) < 0) {
		return -1;
	}

	return z_impl_zvfs_shm_unlink(kname);
}
#include <zephyr/syscalls/zvfs_shm_unlink_mrsh.c>
#endif /* CONFIG_USERSPACE */
