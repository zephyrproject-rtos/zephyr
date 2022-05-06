/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>
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

void test_z_reserve_fd(void)
{
	int fd = z_reserve_fd(); /* function being tested */

	zassert_true(fd >= 0, "fd < 0");

	z_free_fd(fd);
}

void test_z_get_fd_obj_and_vtable(void)
{
	const struct fd_op_vtable *vtable;

	int fd = z_reserve_fd();
	zassert_true(fd >= 0, "fd < 0");

	int *obj;
	obj = z_get_fd_obj_and_vtable(fd, &vtable,
				      NULL); /* function being tested */

	zassert_is_null(obj, "obj is not NULL");

	z_free_fd(fd);
}

void test_z_get_fd_obj(void)
{
	int fd = z_reserve_fd();
	zassert_true(fd >= 0, "fd < 0");

	int err = -1;
	const struct fd_op_vtable *vtable = 0;
	const struct fd_op_vtable *vtable2 = vtable+1;

	int *obj = z_get_fd_obj(fd, vtable, err); /* function being tested */

	/* take branch -- if (_check_fd(fd) < 0) */
	zassert_is_null(obj, "obj not is NULL");

	obj = (void *)1;
	vtable = NULL;

	/* This will set obj and vtable properly */
	z_finalize_fd(fd, obj, vtable);

	obj = z_get_fd_obj(-1, vtable, err); /* function being tested */

	zassert_equal_ptr(obj, NULL, "obj is not NULL when fd < 0");
	zassert_equal(errno, EBADF, "fd: out of bounds error");

	/* take branch -- if (vtable != NULL && fd_entry->vtable != vtable) */
	obj = z_get_fd_obj(fd, vtable2, err); /* function being tested */

	zassert_equal_ptr(obj, NULL, "obj is not NULL - vtable doesn't match");
	zassert_equal(errno, err, "vtable matches");

	z_free_fd(fd);
}

void test_z_finalize_fd(void)
{
	const struct fd_op_vtable *vtable;

	int fd = z_reserve_fd();
	zassert_true(fd >= 0, NULL);

	int *obj = z_get_fd_obj_and_vtable(fd, &vtable, NULL);

	const struct fd_op_vtable *original_vtable = vtable;
	int *original_obj = obj;

	z_finalize_fd(fd, obj, vtable); /* function being tested */

	obj = z_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_equal_ptr(obj, original_obj, "obj is different after finalizing");
	zassert_equal_ptr(vtable, original_vtable, "vtable is different after finalizing");

	z_free_fd(fd);
}

void test_z_alloc_fd(void)
{
	const struct fd_op_vtable *vtable = NULL;
	int *obj = NULL;

	int fd = z_alloc_fd(obj, vtable); /* function being tested */
	zassert_true(fd >= 0, NULL);

	obj = z_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_equal_ptr(obj, NULL, "obj is different after allocating");
	zassert_equal_ptr(vtable, NULL, "vtable is different after allocating");

	z_free_fd(fd);
}

void test_z_free_fd(void)
{
	const struct fd_op_vtable *vtable = NULL;

	int fd = z_reserve_fd();
	zassert_true(fd >= 0, NULL);

	z_free_fd(fd); /* function being tested */

	int *obj = z_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_equal_ptr(obj, NULL, "obj is not NULL after freeing");
}

static void test_cb(void *fd_ptr)
{
	int fd = POINTER_TO_INT(fd_ptr);
	const struct fd_op_vtable *vtable;
	int *obj;

	obj = z_get_fd_obj_and_vtable(fd, &vtable, NULL);

	zassert_not_null(obj, "obj is null");
	zassert_not_null(vtable, "vtable is null");

	z_free_fd(fd);

	obj = z_get_fd_obj_and_vtable(fd, &vtable, NULL);
	zassert_is_null(obj, "obj is still there");
	zassert_equal(errno, EBADF, "fd was found");
}

void test_z_fd_multiple_access(void)
{
	const struct fd_op_vtable *vtable = VTABLE_INIT;
	void *obj = (void *)vtable;

	shared_fd = z_reserve_fd();
	zassert_true(shared_fd >= 0, "fd < 0");

	z_finalize_fd(shared_fd, obj, vtable);

	k_thread_create(&fd_thread, fd_thread_stack,
			K_THREAD_STACK_SIZEOF(fd_thread_stack),
			(k_thread_entry_t)test_cb,
			INT_TO_POINTER(shared_fd), NULL, NULL,
			CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_join(&fd_thread, K_FOREVER);

	/* should be null since freed in the other thread */
	obj = z_get_fd_obj_and_vtable(shared_fd, &vtable, NULL);
	zassert_is_null(obj, "obj is still there");
	zassert_equal(errno, EBADF, "fd was found");
}

void test_main(void)
{
	ztest_test_suite(test_fdtable,
			 ztest_unit_test(test_z_reserve_fd),
			 ztest_unit_test(test_z_get_fd_obj_and_vtable),
			 ztest_unit_test(test_z_get_fd_obj),
			 ztest_unit_test(test_z_finalize_fd),
			 ztest_unit_test(test_z_alloc_fd),
			 ztest_unit_test(test_z_free_fd),
			 ztest_unit_test(test_z_fd_multiple_access)
		);
	ztest_run_test_suite(test_fdtable);
}
