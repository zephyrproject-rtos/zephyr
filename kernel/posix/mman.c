/*
 * Copyright (c) 2018 Juan Manuel Torres Palma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/mman.h>
#include <string.h>

/* Adding assert in here can help debug */
#define SET_ERRNO_AND_RET(_code)      \
	do {                          \
		errno = (_code);      \
		goto error;           \
	} while (0)

/* Syntactic sugar to check flags */
#define fchk(_flag, _val) ((_flag) & (_val))

/*
 * Shared space is implemented with a memory slab and several temporary buffers
 * for special cases.
 */
#define GFT_ENTRIES        (16) /* Should be app defined */
#define REGION_SIZE        (64) /* Bytes size of every page */

#define SLAB_BLOCKS        (32) /* Always >= GFT_REGIONS */

#define FDT_ENTRIES        (16) /* System file descriptors limit */
#define FDS_PER_PROC        (4) /* Process file descriptors limit */

#define MT_ENTRIES (16)

K_MEM_SLAB_DEFINE(mman_slab, REGION_SIZE, SLAB_BLOCKS, 4);

/*
 * For each region we need to track name, processes that have opened it,
 * for now, everything static.
 */

/*
 * Global file table or in this case memory objects table.
 */
struct _gfte {
	const char *name;
	void *buff;
	mode_t perm;
	bool to_unlink;
};

/*
 * File descriptor table entry, keeps track on how every file was opened and
 * how to close it. Ideally this would be in the k_thread, but this is a
 * workaround.
 */
struct _fdte {
	k_tid_t proc;
	int fd;
	int oflag;
	struct _gfte *file;
};

/*
 * Mappings table, tracking how a file descriptor was mapped and which pages it
 * allocated.
 */
struct _mte {
	k_tid_t proc;
	struct _fdte *fd;
	void *reg_start;
	void *reg_end;
	int map_type;
	int prot;
};

static struct _gfte gft[GFT_ENTRIES];
static struct _fdte fdt[FDT_ENTRIES];
static struct _mte mt[MT_ENTRIES];

/*
 * Operations on global file table
 */
static struct _gfte *file_create(const char *name, mode_t perm);
static int file_delete(struct _gfte *fe);
static struct _gfte *file_find_by_name(const char *name);
static int file_unlink(struct _gfte *fe);

/*
 * Operations on descriptors table
 */
static int fds_create(struct _gfte *gf, int oflags, struct _fdte **fde);
static int fds_delete(struct _fdte *fde);
static struct _fdte *fds_find_by_fd(int fd);
static int fds_count_references(struct _gfte *fe);

/*
 * Operation on mappings table
 */
static struct _mte *map_create(struct _fdte *fde, int prot, int map_type);
static int map_delete(struct _mte *me);
static struct _mte *map_find_by_tid_addr(k_tid_t proc, void *addr);

/*
 * NULL if not found, pointer to the object if found.
 */
inline struct _gfte *file_find_by_name(const char *name)
{
	struct _gfte *it, *lmt, *fe = NULL;

	if (name == NULL)
		return NULL;

	it = gft;
	lmt = gft + GFT_ENTRIES;

	while (it != lmt && !fe) {
		if (!strncmp(name, it->name, PATH_MAX))
			fe = it;
		++it;
	}

	return fe;
}

/*
 * No check for NULL
 * Fails if no memory to allocate in the slab or GFT is full.
 */
inline struct _gfte *file_create(const char *name, mode_t perm)
{
	struct _gfte *it, *lmt, *fe = NULL;
	void *data_ptr;

	if (k_mem_slab_alloc(&mman_slab, &data_ptr, K_NO_WAIT))
		return NULL;

	it = gft;
	lmt = gft + GFT_ENTRIES;

	while (it != lmt && !fe) {
		if (it->name == NULL && !it->to_unlink) {
			fe = it;
			fe->name = name;
			fe->perm = perm;
			fe->buff = data_ptr;
			fe->to_unlink = false;
		}
		++it;
	}
	return fe;
}

inline int file_delete(struct _gfte *fe)
{
	if (!fe)
		return -1;

	k_mem_slab_free(&mman_slab, &fe->buff);
	memset(fe, 0, sizeof(struct _gfte));

	return 0;
}

inline int file_unlink(struct _gfte *fe)
{
	if (!fe)
		return -1;

	fe->name = NULL;
	fe->to_unlink = true;
	return 0;
}

/*
 * TODO: consider that 0, 1, and 2 are reserved for stderr, stdin, stdout
 */
#define fds_valid(_fd) ((_fd) < FDS_PER_PROC)

/*
 * Needs to find an empty spot in the table, and still generate a valid fd,
 * being the lowest possible.
 */
inline int fds_create(struct _gfte *gf, int oflags, struct _fdte **fde)
{
	struct _fdte *it, *lmt;
	char used_fd[FDS_PER_PROC];
	k_tid_t proc;
	int fd = -1;

	proc = k_current_get();
	memset(used_fd, 0, FDS_PER_PROC);

	it = fdt;
	lmt = fdt + FDT_ENTRIES;
	*fde = NULL;

	/* Go through the whole FDT */
	while (it < lmt) {
		if (it->proc == NULL)
			*fde = it;
		else if (it->proc == proc)
			used_fd[it->fd] = 1;
		++it;
	}

	/* Too many shared objects open in FDT */
	if (!(*fde))
		return ENFILE;

	for (int i = 0; i < FDS_PER_PROC; ++i)
		if (used_fd[i] == 0) {
			fd = i;
			break;
		}

	/* All file descriptors for this project */
	if (fd == -1)
		return EMFILE;

	(*fde)->proc = proc;
	(*fde)->fd = fd;
	(*fde)->oflag = oflags;
	(*fde)->file = gf;

	return 0;
}

inline int fds_delete(struct _fdte *fde)
{
	if (!fde)
		return -1;

	memset(fde, 0, sizeof(struct _fdte));
	return 0;
}

inline struct _fdte *fds_find_by_fd(int fd)
{
	struct _fdte *it, *lmt, *fe = NULL;
	k_tid_t proc;

	if (!fds_valid(fd))
		return NULL;

	proc = k_current_get();

	it = fdt;
	lmt = fdt + FDT_ENTRIES;

	while (it < lmt && !fe) {
		if (it->proc == proc && it->fd == fd)
			fe = it;
		++it;
	}

	return fe;
}

inline int fds_count_references(struct _gfte *fe)
{
	int c = 0;
	struct _fdte *it, *lmt;

	if (!fe)
		return -1;

	it = fdt;
	lmt = fdt + FDT_ENTRIES;

	while (it < lmt) {
		if (it->file == fe)
			++c;
		++it;
	}

	return c;
}

/*
 * Here we only care of allocating, no permissions or parameters check.
 * Fails if no regions available in the slab or no more entries available in
 * the mappings table.
 */
inline struct _mte *map_create(struct _fdte *fde, int prot, int map_type)
{
	struct _mte *me = NULL;
	struct _mte *it, *lmt;
	void *buff;

	it = mt;
	lmt = mt + MT_ENTRIES;

	/* Find an available spot */
	while (it < lmt && !me) {
		if (it->fd == NULL)
			me = it;
		++it;
	}

	/* No slot available in MT */
	if (!me)
		return NULL;

	if (fchk(map_type, MAP_PRIVATE)) {
		/* No more slots in slab */
		if (!k_mem_slab_alloc(&mman_slab, &buff, K_NO_WAIT))
			return NULL;
		memcpy(buff, fde->file->buff, REGION_SIZE);
	} else {
		buff = fde->file->buff;
	}

	me->proc = k_current_get(); /* XXX: can lead to inconsistencies */
	me->fd = fde;
	me->reg_start = buff;
	me->reg_end = buff + REGION_SIZE;
	me->map_type = map_type;
	me->prot = prot;

	return me;
}

inline int map_delete(struct _mte *me)
{
	if (!me)
		return -1;

	if (fchk(me->map_type, MAP_PRIVATE))
		k_mem_slab_free(&mman_slab, &me->reg_start);

	memset(me, 0, sizeof(struct _mte));
	return 0;
}

#define map_addr_in_range(_p, _addr) \
		((_p)->reg_start <= (_addr) && (_p)->reg_end > (_addr))

inline struct _mte *map_find_by_tid_addr(k_tid_t proc, void *addr)
{
	struct _mte *me = NULL;
	struct _mte *it, *lmt;

	it = mt;
	lmt = mt + MT_ENTRIES;

	while (it < lmt && !me) {
		if (it->proc == proc && map_addr_in_range(it, addr))
			me = it;
		++it;
	}

	return me;
}

/*
 * Checks if the file exists, if it does, simply assign a FD to it, adding it
 * to the FDT.
 * If it does not, creates it and the FD.
 */
int shm_open(const char *name, int oflag, mode_t mode)
{
	struct _gfte *gf;
	struct _fdte *fde;
	unsigned int key;
	bool can_write, can_read, created_file = false;
	int used_flag, ret;

	key = irq_lock();

	/* This check avoids checking internally */
	if (name == NULL)
		SET_ERRNO_AND_RET(EINVAL);

	used_flag = (fchk(oflag, O_RDONLY)) ? O_RDONLY : O_RDWR;

	gf = file_find_by_name(name);
	if (gf) {
		if (fchk(oflag, O_EXCL))
			SET_ERRNO_AND_RET(EEXIST);

		/*
		 * Assume there is only one user in the system
		 */
		can_read = fchk(gf->perm, S_IRUSR);
		can_write = fchk(gf->perm, S_IWUSR);

		if (!can_read || (!can_write && used_flag == O_RDWR))
			SET_ERRNO_AND_RET(EACCES);

		/* Truncate file */
		if (fchk(oflag, O_TRUNC) && can_write)
			memset(gf->buff, REGION_SIZE, 0);

	} else {
		if (!fchk(oflag, O_CREAT))
			SET_ERRNO_AND_RET(ENOENT);

		gf = file_create(name, mode);
		if (!gf)
			SET_ERRNO_AND_RET(ENOSPC);
		created_file = true;
	}

	/* If this fails we need to cleanup */
	ret = fds_create(gf, used_flag, &fde);
	if (ret != 0) {
		if (created_file)
			file_delete(gf);
		SET_ERRNO_AND_RET(ret);
	}

	irq_unlock(key);
	return fde->fd;

error:
	irq_unlock(key);
	return -1;
}

/*
 * Marks memory region to release or frees it
 */
int shm_unlink(const char *name)
{
	struct _gfte *gf;
	unsigned int key;

	key = irq_lock();
	gf = file_find_by_name(name);

	if (!gf)
		SET_ERRNO_AND_RET(ENOENT);

	if (!fds_count_references(gf))
		file_delete(gf);
	else
		file_unlink(gf);

	irq_unlock(key);
	return 0;

error:
	irq_unlock(key);
	return -1;
}

/*
 * prot is made to set page permissions in MMU, but in Zephyr we do not have
 */
void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
	struct _fdte *fde;
	struct _mte *mapping;
	unsigned int key;

	key = irq_lock();

	/* We don't support this */
	if (fchk(flags, MAP_FIXED) || addr != NULL || !len ||
	    (!fchk(flags, MAP_SHARED) && !fchk(flags, MAP_PRIVATE)) || off < 0)
		SET_ERRNO_AND_RET(EINVAL);

	fde = fds_find_by_fd(fildes);
	if (!fde)
		SET_ERRNO_AND_RET(EBADF);

	/* TODO: Compare file descriptor permissions with protection bits */

	/* Check bounds of mapping */
	if (off + len > REGION_SIZE)
		SET_ERRNO_AND_RET(ENOMEM);

	mapping = map_create(fde, prot, flags);
	if (!mapping)
		SET_ERRNO_AND_RET(ENOMEM);

	irq_unlock(key);
	return mapping->reg_start + off;

error:
	irq_unlock(key);
	return MAP_FAILED;
}

int mlock(const void *addr, size_t len)
{
	return 0;
}

int mlockall(int flags)
{
	return 0;
}

int munlock(const void *addr, size_t len)
{
	return 0;
}

int munlockall(void)
{
	return 0;
}

int mprotect(void *addr, size_t len, int prot)
{
	return 0;
}

int posix_madvise(void *addr, size_t len, int advice)
{
	return 0;
}

int posix_mem_offset(const void *restrict addr, size_t len,
		     off_t *restrict off, size_t *restrict contig_len,
		     int *restrict fildes)
{
	return 0;
}

int posix_typed_mem_get_info(int fildes, struct posix_typed_mem_info *info)
{
	return 0;
}

int posix_typed_mem_open(const char *name, int oflag, int tflag)
{
	return 0;
}

/*
 */
int munmap(void *addr, size_t len)
{
	struct _mte *me = NULL;
	k_tid_t proc;
	unsigned int key;

	key = irq_lock();

	/*
	 * FIXME: properly check the range
	 */
	if (len > REGION_SIZE)
		SET_ERRNO_AND_RET(EINVAL);

	proc = k_current_get();

	while ((me = map_find_by_tid_addr(proc, addr)))
		map_delete(me);

	irq_unlock(key);
	return 0;

error:
	irq_unlock(key);
	return -1;
}

int msync(void *addr, size_t len, int flags)
{
	return 0;
}

/*
 * This should be declared once only for all posix named objects, and then
 * probably based in pointers select the appropriate *_close() function.
 */
int close(int fd)
{
	struct _fdte *fde;
	struct _gfte *gf;
	int c;
	unsigned int key;

	key = irq_lock();

	fde = fds_find_by_fd(fd);
	if (!fde)
		SET_ERRNO_AND_RET(ENOENT);

	/* If there is only one reference to that file, it's this */
	gf = fde->file;
	c = fds_count_references(gf);

	if (c == 1 && gf->to_unlink)
		file_delete(gf);
	fds_delete(fde);

	irq_unlock(key);
	return 0;

error:
	irq_unlock(key);
	return -1;
}
