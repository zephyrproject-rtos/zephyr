/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/thread.h>
#include <zephyr/sys/zvfs.h>
#include <zephyr/sys/zvfs_fs.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define MNT   "/RAM:"
#define TFILE MNT "/mapped.bin"
#define PAGE  CONFIG_ZVFS_MMAP_PAGE_SIZE

/* the fixture file holds this many full pages, plus half a page */
#define FILE_PAGES 3
#define FILE_SIZE  (FILE_PAGES * PAGE + PAGE / 2)
#define FILE_SPAN  ((FILE_PAGES + 1) * PAGE)

#define RW (ZVFS_PROT_READ | ZVFS_PROT_WRITE)

static FATFS fat_fs;
static struct fs_mount_t fs_mnt = {
	.type = FS_FATFS,
	.mnt_point = MNT,
	.fs_data = &fat_fs,
};

static inline uint8_t file_byte(size_t i)
{
	return (uint8_t)(i * 7 + (i >> 8));
}

static void expect_file_byte(int fd, size_t off, uint8_t expected)
{
	uint8_t b = ~expected;

	zassert_equal(zvfs_read_offset(fd, &b, 1, &off), 1, "read at %zu failed: %d", off, errno);
	zassert_equal(b, expected, "file[%zu] is 0x%02x, expected 0x%02x", off, b, expected);
}

static void write_file_byte(int fd, size_t off, uint8_t b)
{
	zassert_equal(zvfs_write_offset(fd, &b, 1, &off), 1, "write at %zu failed: %d", off, errno);
}

static void expect_mapped_file(const uint8_t *p, size_t off, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		zassert_equal(p[i], file_byte(off + i), "mapping[%zu] is 0x%02x, expected 0x%02x", i,
			      p[i], file_byte(off + i));
	}
}

static int open_file(int flags)
{
	int fd = zvfs_open(TFILE, flags, 0600);

	zassert_true(fd >= 0, "zvfs_open(%s, %#x) failed: %d", TFILE, flags, errno);

	return fd;
}

static void *map(size_t len, int prot, int flags, int fd, size_t off)
{
	void *p = NULL;

	zassert_ok(zvfs_mmap(len, prot, flags, fd, off, &p), "zvfs_mmap() failed: %d", errno);
	zassert_not_null(p);
	zassert_true(((uintptr_t)p % PAGE) == 0, "%p is not page aligned", p);

	return p;
}

static void expect_mmap_error(size_t len, int prot, int flags, int fd, size_t off, int err)
{
	void *p = NULL;

	errno = 0;
	zassert_equal(zvfs_mmap(len, prot, flags, fd, off, &p), -1);
	zassert_equal(errno, err, "errno %d, expected %d", errno, err);
}

ZTEST_USER(zvfs_mmap, test_zvfs_mmap)
{
	int fd = open_file(ZVFS_O_RDWR);
	int other;
	uint8_t *p;
	uint8_t *q;

	/* arguments */
	expect_mmap_error(0, ZVFS_PROT_READ, ZVFS_MAP_PRIVATE, fd, 0, EINVAL);
	expect_mmap_error(PAGE, ZVFS_PROT_READ, ZVFS_MAP_PRIVATE | ZVFS_MAP_SHARED, fd, 0, EINVAL);
	expect_mmap_error(PAGE, ZVFS_PROT_READ, 0, fd, 0, EINVAL);
	expect_mmap_error(PAGE, ZVFS_PROT_READ, ZVFS_MAP_PRIVATE, fd, PAGE + 1, EINVAL);
	expect_mmap_error(PAGE, 0x80, ZVFS_MAP_PRIVATE, fd, 0, EINVAL);
	expect_mmap_error(PAGE, ZVFS_PROT_WRITE | ZVFS_PROT_EXEC, ZVFS_MAP_PRIVATE, fd, 0, ENOTSUP);

	/* the descriptor */
	other = open_file(ZVFS_O_RDONLY);
	zassert_ok(zvfs_close(other));
	expect_mmap_error(PAGE, ZVFS_PROT_READ, ZVFS_MAP_PRIVATE, other, 0, EBADF);
	other = zvfs_open(MNT, ZVFS_O_RDONLY | ZVFS_O_DIRECTORY, 0);
	zassert_true(other >= 0, "opening the directory failed: %d", errno);
	expect_mmap_error(PAGE, ZVFS_PROT_READ, ZVFS_MAP_PRIVATE, other, 0, ENODEV);
	zassert_ok(zvfs_close(other));
	other = open_file(ZVFS_O_WRONLY);
	expect_mmap_error(PAGE, ZVFS_PROT_READ, ZVFS_MAP_PRIVATE, other, 0, EACCES);
	zassert_ok(zvfs_close(other));
	other = open_file(ZVFS_O_RDONLY);
	expect_mmap_error(PAGE, RW, ZVFS_MAP_SHARED, other, 0, EACCES);
	p = map(PAGE, RW, ZVFS_MAP_PRIVATE, other, 0);
	zassert_ok(zvfs_munmap(p, PAGE));
	zassert_ok(zvfs_close(other));

	/* anonymous: zero-filled, writable, independent */
	p = map(2 * PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);
	q = map(PAGE, RW, ZVFS_MAP_SHARED | ZVFS_MAP_ANONYMOUS, -1, 0);
	for (size_t i = 0; i < 2 * PAGE; ++i) {
		zassert_equal(p[i], 0, "anonymous page not zero at %zu", i);
	}
	p[0] = 0x11;
	p[2 * PAGE - 1] = 0x22;
	q[0] = 0x33;
	zassert_equal(p[0], 0x11);
	zassert_equal(p[2 * PAGE - 1], 0x22);
	zassert_equal(q[0], 0x33);
	zassert_ok(zvfs_munmap(q, PAGE));
	zassert_ok(zvfs_munmap(p, 2 * PAGE));

	/* private file mapping: the file's contents, modifications stay private */
	p = map(FILE_SPAN, RW, ZVFS_MAP_PRIVATE, fd, 0);
	expect_mapped_file(p, 0, FILE_SIZE);
	for (size_t i = FILE_SIZE; i < FILE_SPAN; ++i) {
		zassert_equal(p[i], 0, "byte %zu past the end of the file is 0x%02x", i, p[i]);
	}
	p[0] = (uint8_t)~file_byte(0);
	zassert_ok(zvfs_msync(p, FILE_SPAN, ZVFS_MS_SYNC));
	expect_file_byte(fd, 0, file_byte(0));
	zassert_ok(zvfs_munmap(p, FILE_SPAN));
	expect_file_byte(fd, 0, file_byte(0));

	/* shared file mapping: modifications reach the file when synchronized */
	p = map(FILE_SPAN, RW, ZVFS_MAP_SHARED, fd, 0);
	expect_mapped_file(p, 0, FILE_SIZE);
	p[0] = (uint8_t)~file_byte(0);
	p[FILE_SIZE - 1] = (uint8_t)~file_byte(FILE_SIZE - 1);
	p[FILE_SIZE] = 0x5a;
	zassert_ok(zvfs_msync(p, FILE_SPAN, ZVFS_MS_SYNC));
	expect_file_byte(fd, 0, (uint8_t)~file_byte(0));
	expect_file_byte(fd, FILE_SIZE - 1, (uint8_t)~file_byte(FILE_SIZE - 1));
	expect_file_byte(fd, 1, file_byte(1));
	{
		struct zvfs_stat st;

		/* writes past the end of the file do not grow it */
		zassert_ok(zvfs_fstat(fd, &st));
		zassert_equal((size_t)st.size, FILE_SIZE);
	}
	p[0] = file_byte(0);
	p[FILE_SIZE - 1] = file_byte(FILE_SIZE - 1);
	zassert_ok(zvfs_munmap(p, FILE_SPAN));
	expect_file_byte(fd, 0, file_byte(0));

	/* an offset */
	p = map(PAGE, ZVFS_PROT_READ, ZVFS_MAP_SHARED, fd, PAGE);
	expect_mapped_file(p, PAGE, PAGE);
	zassert_ok(zvfs_munmap(p, PAGE));

	/* the mapping keeps the file open after its descriptor is closed */
	other = open_file(ZVFS_O_RDWR);
	p = map(PAGE, RW, ZVFS_MAP_SHARED, other, 0);
	zassert_ok(zvfs_close(other));
	expect_mapped_file(p, 0, PAGE);
	p[2] = (uint8_t)~file_byte(2);
	zassert_ok(zvfs_msync(p, PAGE, ZVFS_MS_SYNC));
	zassert_ok(zvfs_munmap(p, PAGE));
	expect_file_byte(fd, 2, (uint8_t)~file_byte(2));
	write_file_byte(fd, 2, file_byte(2));

	zassert_ok(zvfs_close(fd));
}

/*
 * An object that owns its pages: mapped in place, so every mapping of it
 * aliases the same memory, and kept open by its mappings.
 */
struct page_obj {
	uint8_t mem[PAGE];
	int closed;
};

static struct page_obj pobj __aligned(PAGE);

static ssize_t pobj_read(void *obj, void *buf, size_t sz, size_t off)
{
	struct page_obj *o = obj;

	sz = (off >= sizeof(o->mem)) ? 0 : MIN(sz, sizeof(o->mem) - off);
	memcpy(buf, &o->mem[off], sz);

	return sz;
}

static ssize_t pobj_write(void *obj, const void *buf, size_t sz, size_t off)
{
	struct page_obj *o = obj;

	sz = (off >= sizeof(o->mem)) ? 0 : MIN(sz, sizeof(o->mem) - off);
	memcpy(&o->mem[off], buf, sz);

	return sz;
}

static int pobj_close(void *obj)
{
	((struct page_obj *)obj)->closed++;

	return 0;
}

static int pobj_ioctl(void *obj, unsigned int request, va_list args)
{
	struct page_obj *o = obj;

	switch (request) {
	case ZFD_IOCTL_MMAP: {
		size_t off = va_arg(args, size_t);
		size_t len = va_arg(args, size_t);
		void **addr = va_arg(args, void **);

		if ((off > sizeof(o->mem)) || (len > sizeof(o->mem) - off)) {
			errno = ENXIO;
			return -1;
		}
		*addr = &o->mem[off];
		return 0;
	}
	case ZFD_IOCTL_STAT: {
		struct zvfs_stat *st = va_arg(args, struct zvfs_stat *);

		*st = (struct zvfs_stat){.size = sizeof(o->mem), .mode = ZVFS_MODE_IFSHM};
		return 0;
	}
	default:
		errno = EOPNOTSUPP;
		return -1;
	}
}

static const struct fd_op_vtable pobj_vtable = {
	.read_offs = pobj_read,
	.write_offs = pobj_write,
	.close = pobj_close,
	.ioctl = pobj_ioctl,
};

ZTEST(zvfs_mmap, test_zvfs_mmap_object)
{
	int fd;
	uint8_t *p;
	uint8_t *q;

	memset(&pobj, 0, sizeof(pobj));
	fd = zvfs_reserve_fd();
	zassert_true(fd >= 0);
	zvfs_finalize_typed_fd(fd, &pobj, &pobj_vtable, ZVFS_MODE_IFSHM);

	/* out of the object's range */
	expect_mmap_error(PAGE, RW, ZVFS_MAP_SHARED, fd, PAGE, ENXIO);

	/* two mappings of the object alias its page */
	p = map(PAGE, RW, ZVFS_MAP_SHARED, fd, 0);
	q = map(PAGE, RW, ZVFS_MAP_SHARED, fd, 0);
	zassert_equal(p, pobj.mem, "not mapped in place");
	zassert_equal(q, pobj.mem, "not mapped in place");
	p[1] = 0x77;
	zassert_equal(q[1], 0x77);
	zassert_equal(pobj.mem[1], 0x77);

	/* the object stays open while mapped */
	zassert_ok(zvfs_close(fd));
	zassert_equal(pobj.closed, 0, "closed while mapped");

	/* both mappings share the address, so unmapping it releases both and closes the object */
	zassert_ok(zvfs_munmap(p, PAGE));
	zassert_equal(pobj.closed, 1, "not closed with the last mapping");
	zassert_ok(zvfs_munmap(q, PAGE));
	zassert_equal(pobj.closed, 1);
}

ZTEST_USER(zvfs_mmap, test_zvfs_munmap)
{
	int fd = open_file(ZVFS_O_RDWR);
	uint8_t *p;

	p = map(2 * PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);

	errno = 0;
	zassert_equal(zvfs_munmap(p, 0), -1);
	zassert_equal(errno, EINVAL);
	errno = 0;
	zassert_equal(zvfs_munmap(p + 1, PAGE), -1);
	zassert_equal(errno, EINVAL);

	if (!IS_ENABLED(CONFIG_MMU) && IS_ENABLED(CONFIG_USERSPACE)) {
		/* a memory-protection region covers a user mapping whole */
		errno = 0;
		zassert_equal(zvfs_munmap(p, PAGE), -1);
		zassert_equal(errno, EINVAL);
		zassert_ok(zvfs_munmap(p, 2 * PAGE));
	} else {
		/* unmapping the front of a mapping keeps the rest */
		p[PAGE] = 1;
		zassert_ok(zvfs_munmap(p, PAGE));
		zassert_equal(p[PAGE], 1);
		zassert_ok(zvfs_munmap(p + PAGE, PAGE));
		/* a range with no mapping in it is not an error */
		zassert_ok(zvfs_munmap(p, 2 * PAGE));

		/* unmapping the middle splits a mapping; the pieces are released on their own */
		p = map(3 * PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);
		p[0] = 5;
		p[2 * PAGE] = 7;
		zassert_ok(zvfs_munmap(p + PAGE, PAGE));
		zassert_equal(p[0], 5);
		zassert_equal(p[2 * PAGE], 7);
		errno = 0;
		zassert_equal(zvfs_msync(p + PAGE, PAGE, ZVFS_MS_SYNC), -1);
		zassert_equal(errno, ENOMEM);
		zassert_ok(zvfs_munmap(p + 2 * PAGE, PAGE));
		zassert_equal(p[0], 5);
		zassert_ok(zvfs_munmap(p, PAGE));

		/* each piece of a shared file mapping writes its own range back */
		p = map(3 * PAGE, RW, ZVFS_MAP_SHARED, fd, 0);
		p[1] = (uint8_t)~file_byte(1);
		p[PAGE + 2] = (uint8_t)~file_byte(PAGE + 2);
		p[2 * PAGE + 3] = (uint8_t)~file_byte(2 * PAGE + 3);
		zassert_ok(zvfs_munmap(p + PAGE, PAGE));
		expect_file_byte(fd, PAGE + 2, (uint8_t)~file_byte(PAGE + 2));
		zassert_ok(zvfs_munmap(p + 2 * PAGE, PAGE));
		expect_file_byte(fd, 2 * PAGE + 3, (uint8_t)~file_byte(2 * PAGE + 3));
		zassert_ok(zvfs_munmap(p, PAGE));
		expect_file_byte(fd, 1, (uint8_t)~file_byte(1));
		write_file_byte(fd, 1, file_byte(1));
		write_file_byte(fd, PAGE + 2, file_byte(PAGE + 2));
		write_file_byte(fd, 2 * PAGE + 3, file_byte(2 * PAGE + 3));
	}

	/* unmapping a shared mapping writes its modifications back */
	p = map(PAGE, RW, ZVFS_MAP_SHARED, fd, 0);
	p[3] = (uint8_t)~file_byte(3);
	zassert_ok(zvfs_munmap(p, PAGE));
	expect_file_byte(fd, 3, (uint8_t)~file_byte(3));
	write_file_byte(fd, 3, file_byte(3));

	/* released mappings free their records */
	for (int i = 0; i < 2 * CONFIG_ZVFS_MMAP_MAX; ++i) {
		p = map(PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);
		p[0] = (uint8_t)i;
		zassert_ok(zvfs_munmap(p, PAGE));
	}

	zassert_ok(zvfs_close(fd));
}

ZTEST_USER(zvfs_mmap, test_zvfs_msync)
{
	int fd = open_file(ZVFS_O_RDWR);
	uint8_t *p;
	uint8_t b;

	p = map(PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);
	errno = 0;
	zassert_equal(zvfs_msync(p, PAGE, 0x80), -1);
	zassert_equal(errno, EINVAL);
	errno = 0;
	zassert_equal(zvfs_msync(p, PAGE, ZVFS_MS_SYNC | ZVFS_MS_ASYNC), -1);
	zassert_equal(errno, EINVAL);
	errno = 0;
	zassert_equal(zvfs_msync(p + 1, PAGE, ZVFS_MS_SYNC), -1);
	zassert_equal(errno, EINVAL);
	zassert_ok(zvfs_msync(p, 0, ZVFS_MS_SYNC));
	zassert_ok(zvfs_munmap(p, PAGE));
	errno = 0;
	zassert_equal(zvfs_msync(p, PAGE, ZVFS_MS_SYNC), -1);
	zassert_equal(errno, ENOMEM);

	/* synchronous and asynchronous write-back of the requested range */
	p = map(2 * PAGE, RW, ZVFS_MAP_SHARED, fd, 0);
	p[PAGE + 7] = (uint8_t)~file_byte(PAGE + 7);
	zassert_ok(zvfs_msync(p + PAGE, PAGE, ZVFS_MS_SYNC));
	expect_file_byte(fd, PAGE + 7, (uint8_t)~file_byte(PAGE + 7));
	p[PAGE + 7] = file_byte(PAGE + 7);
	zassert_ok(zvfs_msync(p, 2 * PAGE, ZVFS_MS_ASYNC));
	expect_file_byte(fd, PAGE + 7, file_byte(PAGE + 7));

	if (IS_ENABLED(CONFIG_ZVFS_MMAP_DIRTY_TRACK)) {
		/* only the page modified through the mapping is written back */
		b = (uint8_t)~file_byte(PAGE + 3);
		write_file_byte(fd, PAGE + 3, b);
		p[5] = (uint8_t)~file_byte(5);
		zassert_ok(zvfs_msync(p, 2 * PAGE, ZVFS_MS_SYNC));
		expect_file_byte(fd, 5, (uint8_t)~file_byte(5));
		expect_file_byte(fd, PAGE + 3, b);

		/* a page written back is tracked again */
		p[5] = file_byte(5);
		p[PAGE + 9] = (uint8_t)~file_byte(PAGE + 9);
		zassert_ok(zvfs_msync(p, 2 * PAGE, ZVFS_MS_SYNC));
		expect_file_byte(fd, 5, file_byte(5));
		expect_file_byte(fd, PAGE + 9, (uint8_t)~file_byte(PAGE + 9));
		p[PAGE + 9] = file_byte(PAGE + 9);
		zassert_ok(zvfs_msync(p, 2 * PAGE, ZVFS_MS_SYNC));
		write_file_byte(fd, PAGE + 3, file_byte(PAGE + 3));
	}
	zassert_ok(zvfs_munmap(p, 2 * PAGE));

	/* invalidation re-reads the pages from the file */
	p = map(PAGE, ZVFS_PROT_READ, ZVFS_MAP_SHARED, fd, 0);
	zassert_equal(p[9], file_byte(9));
	b = (uint8_t)~file_byte(9);
	write_file_byte(fd, 9, b);
	zassert_ok(zvfs_msync(p, PAGE, ZVFS_MS_INVALIDATE));
	zassert_equal(p[9], b);
	zassert_ok(zvfs_msync(p, PAGE, ZVFS_MS_SYNC | ZVFS_MS_INVALIDATE));
	zassert_equal(p[8], file_byte(8));
	zassert_ok(zvfs_munmap(p, PAGE));
	write_file_byte(fd, 9, file_byte(9));

	zassert_ok(zvfs_close(fd));
}

ZTEST_USER(zvfs_mmap, test_zvfs_mprotect)
{
	int fd = open_file(ZVFS_O_RDONLY);
	uint8_t *p;

	p = map(2 * PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);
	errno = 0;
	zassert_equal(zvfs_mprotect(p + 1, PAGE, ZVFS_PROT_READ), -1);
	zassert_equal(errno, EINVAL);
	errno = 0;
	zassert_equal(zvfs_mprotect(p, PAGE, 0x80), -1);
	zassert_equal(errno, EINVAL);
	errno = 0;
	zassert_equal(zvfs_mprotect(p, 2 * PAGE, ZVFS_PROT_WRITE | ZVFS_PROT_EXEC), -1);
	zassert_equal(errno, ENOTSUP);
	/* part of a mapping is not supported */
	errno = 0;
	zassert_equal(zvfs_mprotect(p, PAGE, ZVFS_PROT_READ), -1);
	zassert_equal(errno, ENOTSUP);

	/* transitions */
	p[0] = 0x11;
	zassert_ok(zvfs_mprotect(p, 2 * PAGE, ZVFS_PROT_READ));
	zassert_equal(p[0], 0x11);
	zassert_ok(zvfs_mprotect(p, 2 * PAGE, ZVFS_PROT_NONE));
	zassert_ok(zvfs_mprotect(p, 2 * PAGE, RW));
	p[0] = 0x22;
	zassert_equal(p[0], 0x22);
	zassert_ok(zvfs_munmap(p, 2 * PAGE));

	errno = 0;
	zassert_equal(zvfs_mprotect(p, 2 * PAGE, ZVFS_PROT_READ), -1);
	zassert_equal(errno, ENOMEM);

	/* a shared mapping of a read-only file cannot become writable */
	p = map(PAGE, ZVFS_PROT_READ, ZVFS_MAP_SHARED, fd, 0);
	errno = 0;
	zassert_equal(zvfs_mprotect(p, PAGE, RW), -1);
	zassert_equal(errno, EACCES);
	zassert_ok(zvfs_munmap(p, PAGE));

	zassert_ok(zvfs_close(fd));
}

/* results of the mapper thread, checked by the test: a non-test thread must not assert */
static ZTEST_BMEM int mapper_mapped;
static ZTEST_BMEM int mapper_extra_ret;
static ZTEST_BMEM int mapper_extra_errno;
static ZTEST_BMEM void *mapper_addrs[CONFIG_ZVFS_MMAP_MAX];

static void mapper(void *p1, void *p2, void *p3)
{
	void *p = NULL;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* take every record, then exit without unmapping */
	mapper_mapped = 0;
	for (int i = 0; i < CONFIG_ZVFS_MMAP_MAX; ++i) {
		if (zvfs_mmap(PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0, &p) != 0) {
			break;
		}
		((uint8_t *)p)[0] = (uint8_t)i;
		mapper_addrs[mapper_mapped++] = p;
	}
	errno = 0;
	mapper_extra_ret = zvfs_mmap(PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0, &p);
	mapper_extra_errno = errno;
}

static K_THREAD_STACK_DEFINE(mapper_stack, 4096);
static struct k_thread mapper_thread;

/* the mappings of a thread are released when it exits; with processes they outlive it */
ZTEST(zvfs_mmap, test_zvfs_mmap_owner_exit)
{
	uint32_t opts = IS_ENABLED(CONFIG_USERSPACE) ? (K_USER | K_INHERIT_PERMS) : 0;
	uint8_t *p;
	int ret;

	k_thread_create(&mapper_thread, mapper_stack, K_THREAD_STACK_SIZEOF(mapper_stack), mapper,
			NULL, NULL, NULL, K_PRIO_PREEMPT(0), opts, K_NO_WAIT);
	ret = k_thread_join(&mapper_thread, K_SECONDS(5));
	zassert_ok(ret, "joining the mapping thread failed: %d", ret);
	/* the whole pool, less what a faulted test thread may have left to its process */
	zassert_true(mapper_mapped >= 1, "no mapping at all");
	zassert_equal(mapper_extra_ret, -1, "a mapping beyond the pool succeeded");
	zassert_equal(mapper_extra_errno, ENOMEM);

	if (IS_ENABLED(CONFIG_PROCESS)) {
		/* a member thread's mappings belong to the process: still there, still full */
		expect_mmap_error(PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0, ENOMEM);
		for (int i = 0; i < mapper_mapped; ++i) {
			zassert_equal(((uint8_t *)mapper_addrs[i])[0], (uint8_t)i);
			zassert_ok(zvfs_munmap(mapper_addrs[i], PAGE));
		}
	}

	/* otherwise the next mapping operation releases them */
	p = map(PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);
	zassert_equal(p[0], 0);
	zassert_ok(zvfs_munmap(p, PAGE));
}

#ifdef CONFIG_PROCESS
#define PROCESS_MARK 0xA5

static K_THREAD_STACK_DEFINE(child_stack, 4096);
static struct k_thread child_thread;

/* map the file, fill the pool with anonymous mappings, then leave without unmapping */
static void process_mapper(void *p1, void *p2, void *p3)
{
	int fd = (int)(intptr_t)p1;
	bool call_exit = p2 != NULL;
	void *p = NULL;
	int n = 0;

	ARG_UNUSED(p3);

	if (zvfs_mmap(PAGE, RW, ZVFS_MAP_SHARED, fd, 0, &p) == 0) {
		((uint8_t *)p)[0] = PROCESS_MARK;
		n++;
	}
	while (zvfs_mmap(PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0, &p) == 0) {
		n++;
	}
	if (call_exit) {
		k_exit(n);
	}
	/* the leader returning ends the process as well */
}

static int spawn_mapper(int fd, bool call_exit)
{
	k_pid_t child = NULL;
	int wstatus = -1;
	struct sys_clone_args args = {
		.entry = process_mapper,
		.p1 = (void *)(intptr_t)fd,
		.p2 = call_exit ? &child : NULL,
		.thread = &child_thread,
		.stack = child_stack,
		.stack_size = K_THREAD_STACK_SIZEOF(child_stack),
		.prio = K_PRIO_PREEMPT(0),
	};

	int ret = sys_clone(&args, &child);

	zassert_ok(ret, "sys_clone failed: %d", ret);
	ret = k_waitpid(child, NULL, &wstatus, 0, K_FOREVER);
	zassert_ok(ret, "k_waitpid failed: %d", ret);
	zassert_true(K_WIFEXITED(wstatus), "wait status 0x%x", wstatus);

	return K_WEXITSTATUS(wstatus);
}

/* @a n mappings can be made: the ones a child made and left are gone */
static void expect_pool_free(int n)
{
	uint8_t *p[CONFIG_ZVFS_MMAP_MAX];

	zassert_true((n >= 1) && (n <= CONFIG_ZVFS_MMAP_MAX), "%d mappings", n);
	for (int i = 0; i < n; ++i) {
		p[i] = map(PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);
	}
	for (int i = 0; i < n; ++i) {
		zassert_ok(zvfs_munmap(p[i], PAGE));
	}
}

/* the mappings of a process are released, written back, when it exits */
ZTEST(zvfs_mmap, test_zvfs_mmap_process_exit)
{
	int fd = zvfs_open(TFILE, ZVFS_O_RDWR, 0);
	int n;

	zassert_true(fd >= 0, "opening %s failed: %d", TFILE, errno);

	/* k_exit(): released in the child's own context, so the file is current at the reap */
	n = spawn_mapper(fd, true);
	expect_file_byte(fd, 0, PROCESS_MARK);
	expect_pool_free(n);

	/* the leader returning: released by the next mapping operation after the reap */
	write_file_byte(fd, 0, file_byte(0));
	zassert_equal(spawn_mapper(fd, false), 0);
	expect_file_byte(fd, 0, file_byte(0));
	expect_pool_free(n);
	expect_file_byte(fd, 0, PROCESS_MARK);

	zassert_ok(zvfs_close(fd));
}
#endif /* CONFIG_PROCESS */

#ifdef CONFIG_PROCESS_VM
static void fork_child(void *p1, void *p2, void *p3)
{
	uint8_t *shared = p1;
	uint8_t *priv = p2;
	uint8_t *file = p3;
	int seen = 0;

	/* the parent's values, through the copied address space */
	seen += (shared[0] == 1);
	seen += (priv[0] == 2);
	seen += (file[0] == file_byte(0));
	shared[0] = 0x11;
	priv[0] = 0x22;
	file[0] = 0x33;
	k_exit(seen);
}

/* a process cloned with its address space inherits the mappings */
ZTEST_USER(zvfs_mmap, test_zvfs_mmap_fork)
{
	int fd = zvfs_open(TFILE, ZVFS_O_RDWR, 0);
	k_pid_t child = NULL;
	int wstatus = -1;
	int ret;
	uint8_t *shared, *priv, *file;
	struct sys_clone_args args = {
		.flags = SYS_CLONE_VM_COPY,
		.entry = fork_child,
		.stack_size = 4096,
		.prio = k_thread_priority_get(k_current_get()),
		.options = K_USER,
	};

	zassert_true(fd >= 0, "opening %s failed: %d", TFILE, errno);
	shared = map(PAGE, RW, ZVFS_MAP_SHARED | ZVFS_MAP_ANONYMOUS, -1, 0);
	priv = map(PAGE, RW, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);
	file = map(PAGE, RW, ZVFS_MAP_SHARED, fd, 0);
	shared[0] = 1;
	priv[0] = 2;
	args.p1 = shared;
	args.p2 = priv;
	args.p3 = file;

	ret = sys_clone(&args, &child);
	zassert_ok(ret, "sys_clone failed: %d", ret);
	ret = k_waitpid(child, NULL, &wstatus, 0, K_FOREVER);
	zassert_ok(ret, "k_waitpid failed: %d", ret);
	zassert_true(K_WIFEXITED(wstatus), "wait status 0x%x", wstatus);
	zassert_equal(K_WEXITSTATUS(wstatus), 3, "the child saw %d of 3 mappings",
		      K_WEXITSTATUS(wstatus));

	/* shared pages are the same pages in both; the private page was copied */
	zassert_equal(shared[0], 0x11);
	zassert_equal(priv[0], 2);
	zassert_equal(file[0], 0x33);
	zassert_ok(zvfs_msync(file, PAGE, ZVFS_MS_SYNC));
	expect_file_byte(fd, 0, 0x33);

	zassert_ok(zvfs_munmap(shared, PAGE));
	zassert_ok(zvfs_munmap(priv, PAGE));
	zassert_ok(zvfs_munmap(file, PAGE));
	/* the child's copies went with the child */
	expect_pool_free(3);
	zassert_ok(zvfs_close(fd));
}
#endif /* CONFIG_PROCESS_VM */

/* the faulting test cannot unmap: with processes its mapping outlives the aborted thread */
static ZTEST_BMEM void *fault_leftover;

/* a write through a read-only mapping faults */
ZTEST_USER(zvfs_mmap, test_zvfs_mmap_fault)
{
	volatile uint8_t *p;

	if (!IS_ENABLED(CONFIG_MMU) && !IS_ENABLED(CONFIG_USERSPACE)) {
		ztest_test_skip();
	}

	p = map(PAGE, ZVFS_PROT_READ, ZVFS_MAP_PRIVATE | ZVFS_MAP_ANONYMOUS, -1, 0);
	fault_leftover = (void *)p;
	zassert_equal(p[0], 0);

	ztest_set_fault_valid(true);
	p[0] = 1;
	ztest_test_fail();
}

static void file_reset(void)
{
	uint8_t buf[64];
	int fd;

	(void)zvfs_unlink(TFILE);
	fd = zvfs_open(TFILE, ZVFS_O_WRONLY | ZVFS_O_CREAT | ZVFS_O_TRUNC, 0600);
	zassert_true(fd >= 0, "creating %s failed: %d", TFILE, errno);
	for (size_t pos = 0; pos < FILE_SIZE; pos += sizeof(buf)) {
		size_t n = MIN(sizeof(buf), FILE_SIZE - pos);

		for (size_t i = 0; i < n; ++i) {
			buf[i] = file_byte(pos + i);
		}
		zassert_equal(zvfs_write(fd, buf, n), (ssize_t)n);
	}
	zassert_ok(zvfs_close(fd));
}

static void *suite_setup(void)
{
	zassert_ok(fs_mount(&fs_mnt));

	return NULL;
}

/* in the test thread: with processes, only a member of the tests' process can unmap */
static void suite_before(void *arg)
{
	ARG_UNUSED(arg);
	if (fault_leftover != NULL) {
		(void)zvfs_munmap(fault_leftover, PAGE);
		fault_leftover = NULL;
	}
	file_reset();
}

static void suite_teardown(void *arg)
{
	ARG_UNUSED(arg);
	(void)zvfs_unlink(TFILE);
	(void)fs_unmount(&fs_mnt);
}

ZTEST_SUITE(zvfs_mmap, NULL, suite_setup, suite_before, NULL, suite_teardown);
