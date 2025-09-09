/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/mman.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/sys/fdtable.h>

#include <zephyr/ztest.h>

#define _page_size COND_CODE_1(CONFIG_MMU, (CONFIG_MMU_PAGE_SIZE), (CONFIG_POSIX_PAGE_SIZE))

#define SHM_SIZE 8

#define VALID_SHM_PATH     "/foo"
#define INVALID_SHM_PATH   "foo"
#define EMPTY_SHM_PATH     ""
#define TOO_SHORT_SHM_PATH "/"

#define INVALID_MODE 0
#define VALID_MODE   0666

#define INVALID_FLAGS 0
#define VALID_FLAGS   (O_RDWR | O_CREAT)
#define CREATE_FLAGS  VALID_FLAGS
#define OPEN_FLAGS    (VALID_FLAGS & ~O_CREAT)

/* account for stdin, stdout, stderr */
#define N (CONFIG_ZVFS_OPEN_MAX - 3)

/* we need to have at least 2 shared memory objects */
BUILD_ASSERT(N >= 2, "CONFIG_ZVFS_OPEN_MAX must be > 4");

#define S_TYPEISSHM(st) (((st)->st_mode & ZVFS_MODE_IFMT) == ZVFS_MODE_IFSHM)

ZTEST(xsi_realtime, test_shm_open)
{
	int ret;
	int fd[N];
	struct stat st;

	{
		/* degenerate error cases */
		zassert_not_ok(shm_open(NULL, INVALID_FLAGS, INVALID_MODE));
		zassert_not_ok(shm_open(NULL, INVALID_FLAGS, VALID_MODE));
		zassert_not_ok(shm_open(NULL, VALID_FLAGS, INVALID_MODE));
		zassert_not_ok(shm_open(NULL, VALID_FLAGS, VALID_MODE));
		zassert_not_ok(shm_open(INVALID_SHM_PATH, VALID_FLAGS, VALID_MODE));
		zassert_not_ok(shm_open(EMPTY_SHM_PATH, VALID_FLAGS, VALID_MODE));
		zassert_not_ok(shm_open(TOO_SHORT_SHM_PATH, VALID_FLAGS, VALID_MODE));
		zassert_not_ok(shm_open(VALID_SHM_PATH, INVALID_FLAGS, INVALID_MODE));
		zassert_not_ok(shm_open(VALID_SHM_PATH, INVALID_FLAGS, VALID_MODE));
		zassert_not_ok(shm_open(VALID_SHM_PATH, VALID_FLAGS, INVALID_MODE));
	}

	/* open / close 1 file descriptor referring to VALID_SHM_PATH */
	fd[0] = shm_open(VALID_SHM_PATH, VALID_FLAGS, VALID_MODE);
	zassert_true(fd[0] >= 0, "shm_open(%s, %x, %04o) failed: %d", VALID_SHM_PATH, VALID_FLAGS,
		     VALID_MODE, errno);

	/* should have size 0 and be a shared memory object */
	zassert_ok(fstat(fd[0], &st));
	zassert_equal(st.st_size, 0);
	zassert_true(S_TYPEISSHM(&st));

	/* technically, the order of close / shm_unlink can be reversed too */
	zassert_ok(close(fd[0]));
	ret = shm_unlink(VALID_SHM_PATH);
	zassert_true(ret == 0 || (ret == -1 && errno == ENOENT),
		     "unexpected return / errno from shm_unlink: %d / %d", ret, errno);

	/* open / close N file descriptors referring to VALID_SHM_PATH */
	for (size_t i = 0; i < N; ++i) {
		fd[i] = shm_open(VALID_SHM_PATH, i == 0 ? CREATE_FLAGS : OPEN_FLAGS, VALID_MODE);
		zassert_true(fd[i] >= 0, "shm_open(%s, %x, %04o) failed: %d", VALID_SHM_PATH,
			     VALID_FLAGS, VALID_MODE, errno);
	}
	zassert_ok(shm_unlink(VALID_SHM_PATH));
	for (size_t i = N; i > 0; --i) {
		zassert_ok(close(fd[i - 1]));
	}
}

ZTEST(xsi_realtime, test_shm_unlink)
{
	int fd;

	{
		/* degenerate error cases */
		zassert_not_ok(shm_unlink(NULL));
		zassert_not_ok(shm_unlink(INVALID_SHM_PATH));
		zassert_not_ok(shm_unlink(EMPTY_SHM_PATH));
		zassert_not_ok(shm_unlink(TOO_SHORT_SHM_PATH));
	}

	/* open / close 1 file descriptor referring to VALID_SHM_PATH */
	fd = shm_open(VALID_SHM_PATH, VALID_FLAGS, VALID_MODE);
	zassert_true(fd >= 0, "shm_open(%s, %x, %04o) failed: %d", VALID_SHM_PATH, VALID_FLAGS,
		     VALID_MODE, errno);
	/* technically, the order of close / shm_unlink can be reversed too */
	zassert_ok(close(fd));
	zassert_ok(shm_unlink(VALID_SHM_PATH));
	/* should not be able to re-open the same path without O_CREAT */
	zassert_not_ok(shm_open(VALID_SHM_PATH, OPEN_FLAGS, VALID_MODE));
}

ZTEST(xsi_realtime, test_shm_read_write)
{
	int fd[N];

	for (size_t i = 0; i < N; ++i) {
		char cbuf = 0xff;

		fd[i] = shm_open(VALID_SHM_PATH, i == 0 ? CREATE_FLAGS : OPEN_FLAGS, VALID_MODE);
		zassert_true(fd[i] >= 0, "shm_open(%s, %x, %04o) failed: %d", VALID_SHM_PATH,
			     VALID_FLAGS, VALID_MODE, errno);
		if (i == 0) {
			/* size 0 on create / zero characters written */
			zassert_equal(write(fd[0], "", 1), 0,
				      "write() should fail on newly create shm fd with size 0");
			/* size 0 on create / zero characters read */
			zassert_equal(read(fd[0], &cbuf, 1), 0,
				      "read() should fail on newly create shm fd with size 0");

			BUILD_ASSERT(SHM_SIZE >= 1);
			zassert_ok(ftruncate(fd[0], SHM_SIZE));

			zassert_equal(write(fd[0], "\x42", 1), 1, "write() failed on fd %d: %d\n",
				      fd[0], errno);

			continue;
		}

		zassert_equal(read(fd[i], &cbuf, 1), 1, "read() failed on fd %d: %d\n", fd[i],
			      errno);
		zassert_equal(cbuf, 0x42,
			      "Failed to read byte over fd %d: expected: 0x%02x actual: 0x%02x",
			      fd[i], 0x42, cbuf);
	}

	for (size_t i = N; i > 0; --i) {
		zassert_ok(close(fd[i - 1]));
	}

	zassert_ok(shm_unlink(VALID_SHM_PATH));
}

ZTEST(xsi_realtime, test_shm_mmap)
{
	int fd[N];
	void *addr[N];

	if (!IS_ENABLED(CONFIG_MMU)) {
		ztest_test_skip();
	}

	for (size_t i = 0; i < N; ++i) {
		fd[i] = shm_open(VALID_SHM_PATH, i == 0 ? CREATE_FLAGS : OPEN_FLAGS, VALID_MODE);
		zassert_true(fd[i] >= 0, "shm_open(%s, %x, %04o) failed : %d", VALID_SHM_PATH,
			     VALID_FLAGS, VALID_MODE, errno);

		if (i == 0) {
			/* cannot map shm of size zero */
			zassert_not_ok(mmap(NULL, _page_size, PROT_READ | PROT_WRITE, MAP_SHARED,
					    fd[0], 0));

			zassert_ok(ftruncate(fd[0], _page_size));
		}

		addr[i] = mmap(NULL, _page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd[i], 0);
		zassert_not_equal(MAP_FAILED, addr[i], "mmap() failed: %d", errno);

		if ((i & 1) == 0) {
			memset(addr[0], i & 0xff, _page_size);
		} else {
			zassert_mem_equal(addr[i], addr[i - 1], _page_size);
		}
	}

	for (size_t i = N; i > 0; --i) {
		zassert_ok(close(fd[i - 1]));
	}

	for (size_t i = N; i > 0; --i) {
		zassert_ok(munmap(addr[i - 1], _page_size));
		/*
		 * Note: for some reason, in Zephyr, unmapping a physical page once, removes all
		 * virtual mappings. When that behaviour changes, remove the break below and adjust
		 * shm.c accordingly.
		 */
		break;
	}

	zassert_ok(shm_unlink(VALID_SHM_PATH));
}
