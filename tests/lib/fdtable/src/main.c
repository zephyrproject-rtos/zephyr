/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <errno.h>

/* The thread will test that the refcounting of fd object will
 * work as expected.
 */
static struct k_thread fd_thread;
static int shared_fd;

static struct fd_op_vtable fd_vtable = { 0 };

#define VTABLE_INIT (&fd_vtable)

K_THREAD_STACK_DEFINE(fd_thread_stack, CONFIG_ZTEST_STACK_SIZE +
		      CONFIG_TEST_EXTRA_STACK_SIZE);

ZTEST(fdtable, test_zvfs_reserve_fd)
{
	int fd = zvfs_reserve_fd(); /* function being tested */

	zassert_true(fd >= 0, "fd < 0");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_get_fd_obj_and_vtable)
{
	const struct fd_op_vtable *vtable;

	int fd = zvfs_reserve_fd();
	zassert_true(fd >= 0, "fd < 0");

	int *obj;
	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable,
				      NULL); /* function being tested */

	zassert_is_null(obj, "obj is not NULL");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_get_fd_obj)
{
	int fd = zvfs_reserve_fd();
	zassert_true(fd >= 0, "fd < 0");

	int err = -1;
	const struct fd_op_vtable *vtable = 0;
	const struct fd_op_vtable *vtable2 = vtable+1;

	int *obj = zvfs_get_fd_obj(fd, vtable, err); /* function being tested */

	/* take branch -- if (_check_fd(fd) < 0) */
	zassert_is_null(obj, "obj not is NULL");

	obj = (void *)1;
	vtable = NULL;

	/* This will set obj and vtable properly */
	zvfs_finalize_fd(fd, obj, vtable);

	obj = zvfs_get_fd_obj(-1, vtable, err); /* function being tested */

	zassert_equal_ptr(obj, NULL, "obj is not NULL when fd < 0");
	zassert_equal(errno, EBADF, "fd: out of bounds error");

	/* take branch -- if (vtable != NULL && fd_entry->vtable != vtable) */
	obj = zvfs_get_fd_obj(fd, vtable2, err); /* function being tested */

	zassert_equal_ptr(obj, NULL, "obj is not NULL - vtable doesn't match");
	zassert_equal(errno, err, "vtable matches");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_finalize_fd)
{
	const struct fd_op_vtable *vtable;

	int fd = zvfs_reserve_fd();
	zassert_true(fd >= 0);

	int *obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	const struct fd_op_vtable *original_vtable = vtable;
	int *original_obj = obj;

	zvfs_finalize_fd(fd, obj, vtable); /* function being tested */

	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_equal_ptr(obj, original_obj, "obj is different after finalizing");
	zassert_equal_ptr(vtable, original_vtable, "vtable is different after finalizing");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_alloc_fd)
{
	const struct fd_op_vtable *vtable = NULL;
	int *obj = NULL;

	int fd = zvfs_alloc_fd(obj, vtable); /* function being tested */
	zassert_true(fd >= 0);

	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_equal_ptr(obj, NULL, "obj is different after allocating");
	zassert_equal_ptr(vtable, NULL, "vtable is different after allocating");

	zvfs_free_fd(fd);
}

ZTEST(fdtable, test_zvfs_free_fd)
{
	const struct fd_op_vtable *vtable = NULL;

	int fd = zvfs_reserve_fd();
	zassert_true(fd >= 0);

	zvfs_free_fd(fd); /* function being tested */

	int *obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_equal_ptr(obj, NULL, "obj is not NULL after freeing");
}

static void test_cb(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int fd = POINTER_TO_INT(p1);
	const struct fd_op_vtable *vtable;
	int *obj;

	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_not_null(obj, "obj is null");
	zassert_not_null(vtable, "vtable is null");

	zvfs_free_fd(fd);

	obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);
	zassert_is_null(obj, "obj is still there");
	zassert_equal(errno, EBADF, "fd was found");
}

ZTEST(fdtable, test_z_fd_multiple_access)
{
	const struct fd_op_vtable *vtable = VTABLE_INIT;
	void *obj = (void *)vtable;

	shared_fd = zvfs_reserve_fd();
	zassert_true(shared_fd >= 0, "fd < 0");

	zvfs_finalize_fd(shared_fd, obj, vtable);

	k_thread_create(&fd_thread, fd_thread_stack,
			K_THREAD_STACK_SIZEOF(fd_thread_stack),
			test_cb,
			INT_TO_POINTER(shared_fd), NULL, NULL,
			CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_join(&fd_thread, K_FOREVER);

	/* should be null since freed in the other thread */
	obj = zvfs_get_fd_obj_and_vtable(shared_fd, &vtable, NULL);
	zassert_is_null(obj, "obj is still there");
	zassert_equal(errno, EBADF, "fd was found");
}

ZTEST_SUITE(fdtable, NULL, NULL, NULL, NULL, NULL);
