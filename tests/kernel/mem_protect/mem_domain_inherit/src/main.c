/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/libc-hooks.h> /* for z_libc_partition */
#include <zephyr/ztest.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define MAGIC_A 0xa5U
#define MAGIC_B 0x5aU

/* Two partitions with one byte of interest each */
K_APPMEM_PARTITION_DEFINE(part_a);
K_APPMEM_PARTITION_DEFINE(part_b);
K_APP_BMEM(part_a) static volatile uint8_t byte_a;
K_APP_BMEM(part_b) static volatile uint8_t byte_b;

/* The domain whose partitions are inherited, and the thread that is in it */
static struct k_mem_domain src_domain;
static struct k_thread member_thread;
static K_THREAD_STACK_DEFINE(member_stack, STACK_SIZE);

/* The domain that inherits, and the user thread that runs in it */
static struct k_mem_domain dst_domain;
static struct k_thread writer_thread;
static K_THREAD_STACK_DEFINE(writer_stack, STACK_SIZE);

static void never_run_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
}

/* Runs in user mode: both writes fault unless the partitions were inherited */
static void writer_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	byte_a = MAGIC_A;
	byte_b = MAGIC_B;
}

static void *setup(void)
{
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&part_a, &part_b
	};

	zassert_ok(k_mem_domain_init(&src_domain, ARRAY_SIZE(parts), parts),
		   "failed to initialize the source memory domain");
	zassert_ok(k_mem_domain_init(&dst_domain, 0, NULL),
		   "failed to initialize the destination memory domain");

	/* The thread only lends its domain to the calls under test */
	k_thread_create(&member_thread, member_stack, K_THREAD_STACK_SIZEOF(member_stack),
			never_run_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(1), K_USER, K_FOREVER);
	zassert_ok(k_mem_domain_add_thread(&src_domain, &member_thread),
		   "failed to add thread to the source memory domain");

	return NULL;
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	k_thread_abort(&member_thread);
}

/**
 * @brief Verify that a domain can inherit the partitions of a thread's domain.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * k_mem_domain_inherit_thread_partitions() must give the destination domain
 * every partition of the domain the thread is a member of, so that a user
 * thread placed in the destination can reach the same memory. A second call
 * finds the same partitions already there, or no room for them on targets
 * with few partition slots: it must fail without changing the destination.
 *
 * Test steps:
 * - Inherit into an empty domain from a thread that is a member of another
 *   domain, and compare the partition counts.
 * - Run a user thread in the inheriting domain that writes to data in two of
 *   the inherited partitions.
 * - Inherit again and expect -ENOSPC if the partitions no longer fit, -EINVAL
 *   otherwise, with the partition count unchanged.
 *
 * Expected result:
 * - The partitions are inherited, grant access, and are not added twice.
 *
 * @see k_mem_domain_inherit_thread_partitions()
 */
ZTEST(mem_domain_inherit, test_inherit_thread_partitions)
{
	uint8_t inherited;
	int free_slots;
	int expected;

	zassert_equal(dst_domain.num_partitions, 0, "destination domain is not empty");

	zassert_ok(k_mem_domain_inherit_thread_partitions(&dst_domain, &member_thread),
		   "failed to inherit the partitions");
	inherited = dst_domain.num_partitions;
	zassert_equal(inherited, src_domain.num_partitions, "inherited %u partitions, expected %u",
		      inherited, src_domain.num_partitions);

	byte_a = 0U;
	byte_b = 0U;

	k_thread_create(&writer_thread, writer_stack, K_THREAD_STACK_SIZEOF(writer_stack),
			writer_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(1), K_USER, K_FOREVER);
	zassert_ok(k_mem_domain_add_thread(&dst_domain, &writer_thread),
		   "failed to add thread to the destination memory domain");
	k_thread_start(&writer_thread);
	zassert_ok(k_thread_join(&writer_thread, K_FOREVER));

	zassert_equal(byte_a, MAGIC_A, "user thread could not write to the first partition");
	zassert_equal(byte_b, MAGIC_B, "user thread could not write to the second partition");

	/* Every partition now overlaps one the destination already has, which
	 * is only looked at if there would be room for them.
	 */
	free_slots = arch_mem_domain_max_partitions_get() - inherited;
	expected = (inherited > free_slots) ? -ENOSPC : -EINVAL;
	zassert_equal(k_mem_domain_inherit_thread_partitions(&dst_domain, &member_thread), expected,
		      "should fail to inherit the same partitions again");
	zassert_equal(dst_domain.num_partitions, inherited,
		      "failed call changed the destination memory domain");
}

ZTEST_SUITE(mem_domain_inherit, NULL, setup, NULL, NULL, teardown);
