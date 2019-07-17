/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>
#include <sys/fdtable.h>
#include <errno.h>

void test_z_reserve_fd(void)
{
	int fd = z_reserve_fd(); /* function being tested */

	zassert_equal(fd >= 0, true, "fd < 0");

	z_free_fd(fd);
}

void test_z_get_fd_obj_and_vtable(void)
{
	const struct fd_op_vtable *vtable;

	int fd = z_reserve_fd();

	int *obj;
	obj = z_get_fd_obj_and_vtable(fd, &vtable); /* function being tested */

	zassert_equal(obj != NULL, true, "obj is NULL");

	z_free_fd(fd);
}

void test_z_get_fd_obj(void)
{
	int fd = z_reserve_fd();
	int err = -1;
	const struct fd_op_vtable *vtable = 0;
	const struct fd_op_vtable *vtable2 = vtable+1;

	int *obj = z_get_fd_obj(fd, vtable, err); /* function being tested */

	zassert_equal(obj != NULL, true, "obj is NULL");
	zassert_equal(errno != EBADF && errno != ENFILE, true, "errno not set");

	/* take branch -- if (_check_fd(fd) < 0) */
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
	int *obj = z_get_fd_obj_and_vtable(fd, &vtable);

	const struct fd_op_vtable *original_vtable = vtable;
	int *original_obj = obj;

	z_finalize_fd(fd, obj, vtable); /* function being tested */

	obj = z_get_fd_obj_and_vtable(fd, &vtable);

	zassert_equal_ptr(obj, original_obj, "obj is different after finalizing");
	zassert_equal_ptr(vtable, original_vtable, "vtable is different after finalizing");

	z_free_fd(fd);
}

void test_z_alloc_fd(void)
{
	const struct fd_op_vtable *vtable = NULL;
	int *obj = NULL;

	int fd = z_alloc_fd(obj, vtable); /* function being tested */

	obj = z_get_fd_obj_and_vtable(fd, &vtable);

	zassert_equal_ptr(obj, NULL, "obj is different after allocating");
	zassert_equal_ptr(vtable, NULL, "vtable is different after allocating");

	z_free_fd(fd);
}

void test_z_free_fd(void)
{
	const struct fd_op_vtable *vtable = NULL;

	int fd = z_reserve_fd();

	z_free_fd(fd); /* function being tested */

	int *obj = z_get_fd_obj_and_vtable(fd, &vtable);

	zassert_equal_ptr(obj, NULL, "obj is not NULL after freeing");
}

void test_main(void)
{
	ztest_test_suite(test_fdtable,
				ztest_unit_test(test_z_reserve_fd),
				ztest_unit_test(test_z_get_fd_obj_and_vtable),
				ztest_unit_test(test_z_get_fd_obj),
				ztest_unit_test(test_z_finalize_fd),
				ztest_unit_test(test_z_alloc_fd),
				ztest_unit_test(test_z_free_fd)
				);
	ztest_run_test_suite(test_fdtable);
}
