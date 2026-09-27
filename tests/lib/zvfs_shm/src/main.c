/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/zvfs.h>
#include <zephyr/zvfs/shm.h>
#include <zephyr/ztest.h>

#define NAME  "/zvfs_shm"
#define OTHER "/zvfs_other"
#define PAGE  CONFIG_ZVFS_MMAP_PAGE_SIZE
#define RDWR  (ZVFS_O_RDWR | ZVFS_O_CREAT)

BUILD_ASSERT(CONFIG_ZVFS_SHM_MAX == 2, "the tests fill the pool with two objects");

static int shm_open_ok(const char *name, int flags)
{
	int fd = zvfs_shm_open(name, flags, 0600);

	zassert_true(fd >= 0, "zvfs_shm_open(%s, %#x) failed: %d", name, flags, errno);

	return fd;
}

static void expect_open_error(const char *name, int flags, int err)
{
	errno = 0;
	zassert_equal(zvfs_shm_open(name, flags, 0600), -1);
	zassert_equal(errno, err, "errno %d, expected %d", errno, err);
}

ZTEST_USER(zvfs_shm, test_zvfs_shm_open)
{
	int fd;
	int fd2;
	struct zvfs_stat st;
	char c;

	/* arguments */
	expect_open_error(NAME, ZVFS_O_WRONLY | ZVFS_O_CREAT, EINVAL);
	expect_open_error(NAME, ZVFS_O_RDONLY | ZVFS_O_TRUNC, EINVAL);
	expect_open_error(NAME, RDWR | 0x100000, EINVAL);
	expect_open_error("", RDWR, EINVAL);
	expect_open_error("/a-name-that-is-longer-than-sixteen", RDWR, ENAMETOOLONG);

	/* no object: creation is explicit */
	expect_open_error(NAME, ZVFS_O_RDWR, ENOENT);
	fd = shm_open_ok(NAME, RDWR);
	expect_open_error(NAME, RDWR | ZVFS_O_EXCL, EEXIST);

	/* a new object is empty until sized, and sized once */
	zassert_ok(zvfs_fstat(fd, &st));
	zassert_equal(st.size, 0);
	zassert_equal(st.mode & ZVFS_MODE_IFMT, ZVFS_MODE_IFSHM);
	zassert_equal(zvfs_read(fd, &c, 1), 0);
	zassert_ok(zvfs_ftruncate(fd, PAGE));
	zassert_ok(zvfs_ftruncate(fd, PAGE));
	errno = 0;
	zassert_equal(zvfs_ftruncate(fd, 2 * PAGE), -1);
	zassert_equal(errno, EBUSY);
	errno = 0;
	zassert_equal(zvfs_ftruncate(fd, -1), -1);
	zassert_equal(errno, EINVAL);
	zassert_ok(zvfs_fstat(fd, &st));
	zassert_equal(st.size, PAGE);

	/* the second descriptor sees the same object */
	fd2 = shm_open_ok(NAME, ZVFS_O_RDONLY);
	zassert_equal(zvfs_write(fd, "\x42", 1), 1);
	zassert_equal(zvfs_read(fd2, &c, 1), 1);
	zassert_equal(c, 0x42);
	expect_open_error(NAME, RDWR | ZVFS_O_TRUNC, EBUSY);

	/* the pool */
	zassert_ok(zvfs_close(fd2));
	fd2 = shm_open_ok(OTHER, RDWR);
	expect_open_error("/third", RDWR, ENOSPC);
	zassert_ok(zvfs_close(fd2));
	zassert_ok(zvfs_shm_unlink(OTHER));

	zassert_ok(zvfs_close(fd));
	zassert_ok(zvfs_shm_unlink(NAME));
}

ZTEST_USER(zvfs_shm, test_zvfs_shm_unlink)
{
	int fd;
	int fd2;
	char c;

	errno = 0;
	zassert_equal(zvfs_shm_unlink(""), -1);
	zassert_equal(errno, EINVAL);
	errno = 0;
	zassert_equal(zvfs_shm_unlink("/a-name-that-is-longer-than-sixteen"), -1);
	zassert_equal(errno, ENAMETOOLONG);
	errno = 0;
	zassert_equal(zvfs_shm_unlink(NAME), -1);
	zassert_equal(errno, ENOENT);

	/* the name goes at once; the object stays with its descriptors */
	fd = shm_open_ok(NAME, RDWR);
	zassert_ok(zvfs_ftruncate(fd, PAGE));
	zassert_equal(zvfs_write(fd, "\x17", 1), 1);
	zassert_ok(zvfs_shm_unlink(NAME));
	expect_open_error(NAME, ZVFS_O_RDWR, ENOENT);
	errno = 0;
	zassert_equal(zvfs_shm_unlink(NAME), -1);
	zassert_equal(errno, ENOENT);
	zassert_equal(zvfs_read_offset(fd, &c, 1, &(size_t){0}), 1);
	zassert_equal(c, 0x17);

	/* the name can be created again while the old object lives on */
	fd2 = shm_open_ok(NAME, RDWR);
	zassert_equal(zvfs_read(fd2, &c, 1), 0, "the new object is not empty");
	zassert_ok(zvfs_close(fd));
	zassert_ok(zvfs_close(fd2));
	zassert_ok(zvfs_shm_unlink(NAME));

	/* an unlinked, closed object frees its pool slot */
	fd = shm_open_ok(NAME, RDWR);
	fd2 = shm_open_ok(OTHER, RDWR);
	zassert_ok(zvfs_close(fd));
	zassert_ok(zvfs_shm_unlink(NAME));
	zassert_ok(zvfs_close(fd2));
	zassert_ok(zvfs_shm_unlink(OTHER));
	fd = shm_open_ok(NAME, RDWR);
	fd2 = shm_open_ok(OTHER, RDWR);
	zassert_ok(zvfs_close(fd));
	zassert_ok(zvfs_close(fd2));
	zassert_ok(zvfs_shm_unlink(NAME));
	zassert_ok(zvfs_shm_unlink(OTHER));
}

/* the object's pages back every mapping, and keep the object alive */
ZTEST_USER(zvfs_shm, test_zvfs_shm_mmap)
{
	int fd;
	int fd2;
	uint8_t *p = NULL;
	uint8_t *q = NULL;
	char c;

	fd = shm_open_ok(NAME, RDWR);
	errno = 0;
	zassert_equal(zvfs_mmap(PAGE, ZVFS_PROT_READ | ZVFS_PROT_WRITE, ZVFS_MAP_SHARED, fd, 0, (void **)&p), -1);
	zassert_equal(errno, ENXIO, "an unsized object cannot be mapped");
	zassert_ok(zvfs_ftruncate(fd, PAGE));
	errno = 0;
	zassert_equal(zvfs_mmap(PAGE, ZVFS_PROT_READ | ZVFS_PROT_WRITE, ZVFS_MAP_SHARED, fd, PAGE, (void **)&p), -1);
	zassert_equal(errno, ENXIO, "a range past the object cannot be mapped");

	fd2 = shm_open_ok(NAME, ZVFS_O_RDWR);
	zassert_ok(zvfs_mmap(PAGE, ZVFS_PROT_READ | ZVFS_PROT_WRITE, ZVFS_MAP_SHARED, fd, 0, (void **)&p));
	zassert_ok(zvfs_mmap(PAGE, ZVFS_PROT_READ, ZVFS_MAP_SHARED, fd2, 0, (void **)&q));
	zassert_equal(p, q, "mappings of one object do not alias");

	/* the mapping, the other mapping and the descriptors see one memory */
	p[3] = 0x5a;
	zassert_equal(q[3], 0x5a);
	zassert_equal(zvfs_read_offset(fd2, &c, 1, &(size_t){3}), 1);
	zassert_equal(c, 0x5a);
	zassert_equal(zvfs_write_offset(fd, "\x66", 1, &(size_t){7}), 1);
	zassert_equal(p[7], 0x66);

	/* unlinked and closed, the object lives on for its mappings */
	zassert_ok(zvfs_shm_unlink(NAME));
	zassert_ok(zvfs_close(fd));
	zassert_ok(zvfs_close(fd2));
	zassert_equal(p[3], 0x5a);
	p[3] = 0x33;
	zassert_equal(q[3], 0x33);

	/* the mappings share the address: unmapping it releases both and frees the object */
	zassert_ok(zvfs_munmap(p, PAGE));
	fd = shm_open_ok(NAME, RDWR);
	fd2 = shm_open_ok(OTHER, RDWR);
	zassert_ok(zvfs_close(fd));
	zassert_ok(zvfs_close(fd2));
	zassert_ok(zvfs_shm_unlink(NAME));
	zassert_ok(zvfs_shm_unlink(OTHER));
}

ZTEST_SUITE(zvfs_shm, NULL, NULL, NULL, NULL, NULL);
