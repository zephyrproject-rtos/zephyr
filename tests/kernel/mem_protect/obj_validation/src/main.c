/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <syscall_handler.h>
#include <ztest.h>

#define SEM_ARRAY_SIZE	16

static __kernel struct k_sem semarray[SEM_ARRAY_SIZE];
K_SEM_DEFINE(sem1, 0, 1);
static __kernel struct k_sem sem2;
static __kernel char bad_sem[sizeof(struct k_sem)];
static struct k_sem sem3;

static int test_object(struct k_sem *sem, int retval)
{
	int ret;

	if (retval) {
		/* Expected to fail; bypass _obj_validation_check() so we don't
		 * fill the logs with spam
		 */
		ret = _k_object_validate(_k_object_find(sem), K_OBJ_SEM, 0);
	} else {
		ret = _obj_validation_check(_k_object_find(sem), sem,
					    K_OBJ_SEM, 0);
	}

	if (ret != retval) {
		TC_PRINT("FAIL check of %p is not %d\n", sem, retval);
		return 1;
	}
	return 0;
}

void object_permission_checks(struct k_sem *sem, bool skip_init)
{
	/* Should fail because we don't have perms on this object */
	zassert_false(test_object(sem, -EPERM),
		      "object should not have had permission granted");

	k_object_access_grant(sem, k_current_get());

	if (!skip_init) {
		/* Should fail, not initialized and we have no permissions */
		zassert_false(test_object(sem, -EINVAL),
			      "object should not have been initialized");
		k_sem_init(sem, 0, 1);
	}

	/* This should succeed now */
	zassert_false(test_object(sem, 0),
		      "object should have had sufficient permissions");
}

void test_generic_object(void)
{
	struct k_sem stack_sem;

	/* None of these should be even in the table */
	zassert_false(test_object(&stack_sem, -EBADF), NULL);
	zassert_false(test_object((struct k_sem *)&bad_sem, -EBADF), NULL);
	zassert_false(test_object((struct k_sem *)0xFFFFFFFF, -EBADF), NULL);
#ifdef CONFIG_APPLICATION_MEMORY
	zassert_false(test_object(&sem3, -EBADF), NULL);
#else
	object_permission_checks(&sem3, false);
#endif
	object_permission_checks(&sem1, true);
	object_permission_checks(&sem2, false);

	for (int i = 0; i < SEM_ARRAY_SIZE; i++) {
		object_permission_checks(&semarray[i], false);
	}
}

void test_main(void)
{
	ztest_test_suite(object_validation,
			 ztest_unit_test(test_generic_object));
	ztest_run_test_suite(object_validation);
}
