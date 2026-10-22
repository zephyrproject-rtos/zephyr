/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/ref.h>
#include <zephyr/sys/util.h>

struct test_obj {
	int data;
	struct sys_ref ref;
};

static struct test_obj *released_obj;
static int release_count;

static void test_obj_release(struct sys_ref *const ref)
{
	released_obj = CONTAINER_OF(ref, struct test_obj, ref);
	release_count++;
}

/* sys_refcount_init() establishes a single reference. */
ZTEST(sys_ref, test_refcount_init)
{
	struct sys_refcount rc;

	sys_refcount_init(&rc);

	zassert_true(sys_refcount_dec(&rc), "A single decrement should reach zero");
}

/* sys_refcount_dec() returns true only once the last reference is dropped. */
ZTEST(sys_ref, test_refcount_inc_dec)
{
	struct sys_refcount rc;

	sys_refcount_init(&rc);
	sys_refcount_inc(&rc);
	sys_refcount_inc(&rc);

	zassert_false(sys_refcount_dec(&rc), "Two references still held");
	zassert_false(sys_refcount_dec(&rc), "One reference still held");
	zassert_true(sys_refcount_dec(&rc), "Last reference dropped");
}

/* SYS_REFCOUNT_INITIALIZER is equivalent to sys_refcount_init(). */
ZTEST(sys_ref, test_refcount_static_init)
{
	struct sys_refcount rc = SYS_REFCOUNT_INITIALIZER;

	zassert_true(sys_refcount_dec(&rc), "A single decrement should reach zero");
}

/* A saturated counter stays pinned and never reaches zero. */
ZTEST(sys_ref, test_refcount_saturation)
{
	struct sys_refcount rc;

	atomic_set(&rc.value, SYS_REFCOUNT_SATURATED);

	sys_refcount_inc(&rc);
	zassert_equal(atomic_get(&rc.value), SYS_REFCOUNT_SATURATED,
		      "Increment must not move a saturated counter");

	zassert_false(sys_refcount_dec(&rc), "Saturated counter never reaches zero");
	zassert_equal(atomic_get(&rc.value), SYS_REFCOUNT_SATURATED,
		      "Decrement must not move a saturated counter");
}

/* The release handler runs exactly once, on the last put, with the object. */
ZTEST(sys_ref, test_ref_get_put)
{
	struct test_obj obj = { .data = 42 };

	released_obj = NULL;
	release_count = 0;

	sys_ref_init(&obj.ref);
	sys_ref_get(&obj.ref);

	zassert_false(sys_ref_put(&obj.ref, test_obj_release), "Object still referenced");
	zassert_equal(release_count, 0, "Release must not run while referenced");

	zassert_true(sys_ref_put(&obj.ref, test_obj_release), "Last reference dropped");
	zassert_equal(release_count, 1, "Release must run exactly once");
	zassert_equal_ptr(released_obj, &obj, "Release must receive the enclosing object");
}

/* SYS_REF_INITIALIZER is equivalent to sys_ref_init(). */
ZTEST(sys_ref, test_ref_static_init)
{
	struct test_obj obj = { .data = 13, .ref = SYS_REF_INITIALIZER };

	released_obj = NULL;
	release_count = 0;

	zassert_true(sys_ref_put(&obj.ref, test_obj_release), "Single reference dropped");
	zassert_equal(release_count, 1, "Release must run exactly once");
	zassert_equal_ptr(released_obj, &obj, "Release must receive the enclosing object");
}

/* Balanced get/put keeps the object alive until the final put. */
ZTEST(sys_ref, test_ref_balanced)
{
	struct test_obj obj = { 0 };

	released_obj = NULL;
	release_count = 0;

	sys_ref_init(&obj.ref);

	for (int i = 0; i < 17; i++) {
		sys_ref_get(&obj.ref);
	}

	for (int i = 0; i < 17; i++) {
		zassert_false(sys_ref_put(&obj.ref, test_obj_release), "Still referenced");
	}

	zassert_equal(release_count, 0, "Release must not run while referenced");

	zassert_true(sys_ref_put(&obj.ref, test_obj_release), "Last reference dropped");
	zassert_equal(release_count, 1, "Release must run exactly once");
}

ZTEST_SUITE(sys_ref, NULL, NULL, NULL, NULL, NULL);
