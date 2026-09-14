/*
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem_protect.h"
#include <kernel_internal.h> /* For z_main_thread */
#include <zephyr/sys/libc-hooks.h> /* for z_libc_partition */

static struct k_thread child_thread;
K_THREAD_STACK_DEFINE(child_stack, KOBJECT_STACK_SIZE);

/* Special memory domain for test case purposes */
static struct k_mem_domain test_domain;

#if Z_LIBC_PARTITION_EXISTS
#define PARTS_USED	3
#else
#define PARTS_USED	2
#endif
/* Maximum number of allowable memory partitions defined by the build */
#define NUM_RW_PARTS	(CONFIG_MAX_DOMAIN_PARTITIONS - PARTS_USED)

/* Max number of allowable partitions, derived at runtime. Might be less. */
ZTEST_BMEM int num_rw_parts;

/* Set of read-write buffers each in their own partition */
static volatile uint8_t __aligned(MEM_REGION_ALLOC)
	rw_bufs[NUM_RW_PARTS][MEM_REGION_ALLOC];
static struct k_mem_partition rw_parts[NUM_RW_PARTS];

/* A single read-only partition */
static volatile uint8_t __aligned(MEM_REGION_ALLOC) ro_buf[MEM_REGION_ALLOC];
K_MEM_PARTITION_DEFINE(ro_part, ro_buf, sizeof(ro_buf),
		       K_MEM_PARTITION_P_RO_U_RO);
/* A partition to test overlap that has same ro_buf as a partition ro_part */
K_MEM_PARTITION_DEFINE(overlap_part, ro_buf, sizeof(ro_buf),
		       K_MEM_PARTITION_P_RW_U_RW);

/* Static thread, used by a couple tests */
static void zzz_entry(void *p1, void *p2, void *p3)
{
	k_sleep(K_FOREVER);
}

static K_THREAD_DEFINE(zzz_thread, 512 + CONFIG_TEST_EXTRA_STACK_SIZE,
		       zzz_entry, NULL, NULL, NULL, 0, 0, 0);

void *test_mem_domain_setup(void)
{
	int max_parts = arch_mem_domain_max_partitions_get();
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&ro_part, &ztest_mem_partition
	};

	num_rw_parts = max_parts - PARTS_USED;
	zassert_true(num_rw_parts <= NUM_RW_PARTS,
			"CONFIG_MAX_DOMAIN_PARTITIONS incorrectly tuned, %d should be at least %d",
			CONFIG_MAX_DOMAIN_PARTITIONS, max_parts);
	zassert_true(num_rw_parts > 0, "no free memory partitions");

	zassert_equal(
		k_mem_domain_init(&test_domain, ARRAY_SIZE(parts), parts),
		0, "failed to initialize memory domain");

	for (unsigned int i = 0; i < num_rw_parts; i++) {
		rw_parts[i].start = (uintptr_t)&rw_bufs[i];
		rw_parts[i].size = MEM_REGION_ALLOC;
		rw_parts[i].attr = K_MEM_PARTITION_P_RW_U_RW;

		for (unsigned int j = 0; j < MEM_REGION_ALLOC; j++) {
			rw_bufs[i][j] = (j % 256U);
		}

		zassert_equal(
			k_mem_domain_add_partition(&test_domain, &rw_parts[i]),
			0, "cannot add memory partition");
	}

	for (unsigned int j = 0; j < MEM_REGION_ALLOC; j++) {
		ro_buf[j] = (j % 256U);
	}

	return NULL;
}

void test_mem_domain_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

#if defined(CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT)
	zassert_equal(k_mem_domain_deinit(&test_domain), 0,
		      "failed to de-initialize memory domain");
#endif /* CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT */
}

/* Helper function; run a function under a child user thread.
 * If domain is not NULL, add the child thread to that domain, instead of
 * whatever it would inherit.
 */
static void spawn_child_thread(k_thread_entry_t entry,
			       struct k_mem_domain *domain, bool should_fault)
{
	set_fault_valid(should_fault);

	k_thread_create(&child_thread, child_stack,
			K_THREAD_STACK_SIZEOF(child_stack), entry,
			NULL, NULL, NULL, 0, K_USER, K_FOREVER);
	k_thread_name_set(&child_thread, "child_thread");
	if (domain != NULL) {
		k_mem_domain_add_thread(domain, &child_thread);
	}
	k_thread_start(&child_thread);
	k_thread_join(&child_thread, K_FOREVER);

	if (should_fault && valid_fault) {
		/* valid_fault gets cleared if an expected exception
		 * took place
		 */
		printk("test function %p was supposed to fault but didn't\n",
		       entry);
		ztest_test_fail();
	}
}

/* read and write to all the rw_parts */
static void rw_part_access(void *p1, void *p2, void *p3)
{
	for (unsigned int i = 0; i < num_rw_parts; i++) {
		for (unsigned int j = 0; j < MEM_REGION_ALLOC; j++) {
			/* Test read */
			zassert_equal(rw_bufs[i][j], j % 256U,
				      "bad data in rw_buf[%d][%d]", i, j);
			/* Test writes */
			rw_bufs[i][j]++;
			rw_bufs[i][j]--;
		}
	}
}

/* read the ro_part */
static void ro_part_access(void *p1, void *p2, void *p3)
{
	for (unsigned int i = 0; i < MEM_REGION_ALLOC; i++) {
		zassert_equal(ro_buf[i], i % 256U,
			      "bad data in ro_buf[%d]", i);
	}
}

/* attempt to write to ro_part */
static void ro_write_entry(void *p1, void *p2, void *p3)
{
	/* Should fault here */
	ro_buf[0] = 200;
}

/**
 * @brief Verify that partitions in a thread's domain are accessible as
 *        declared.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * A user thread that is a member of a domain must reach the data in that
 * domain's partitions with exactly the declared permissions: read-write data
 * is writable, read-only data is readable.
 *
 * Test steps:
 * - Spawn a user thread in the test domain that reads and writes the
 *   read-write partition.
 * - Spawn another that reads the read-only partition.
 *
 * Expected result:
 * - Both accesses succeed with no fault.
 *
 * @see k_mem_domain_add_thread()
 */
ZTEST(mem_protect_domain, test_mem_domain_valid_access)
{
	spawn_child_thread(rw_part_access, &test_domain, false);
	spawn_child_thread(ro_part_access, &test_domain, false);
}

/**
 * @brief Verify that partitions outside a thread's domain are unreachable.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The same accesses as the valid case, made by a thread that was never added
 * to the test domain, must fault: domain membership is what grants access,
 * not the existence of the partition. The suite's fault hook marks the
 * faults as expected.
 *
 * Test steps:
 * - Spawn a user thread outside the test domain that touches the read-write
 *   partition.
 * - Spawn another that touches the read-only partition.
 *
 * Expected result:
 * - Both accesses fault.
 */
ZTEST(mem_protect_domain, test_mem_domain_invalid_access)
{
	/* child not added to test_domain, will fault for both */
	spawn_child_thread(rw_part_access, NULL, true);
	spawn_child_thread(ro_part_access, NULL, true);
}

/**
 * @brief Verify that a read-only partition rejects writes from its own
 *        domain.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Being inside the domain grants the declared permission, not more: a member
 * thread writing data in the read-only partition must fault even though it
 * can read it freely.
 *
 * Test steps:
 * - Spawn a user thread in the test domain that writes a variable in the
 *   read-only partition.
 *
 * Expected result:
 * - The write faults.
 */
ZTEST(mem_protect_domain, test_mem_domain_no_writes_to_ro)
{
	/* Show that trying to write to a read-only partition causes a fault */
	spawn_child_thread(ro_write_entry, &test_domain, true);
}

/**
 * @brief Verify that partition removal and re-addition track access rights.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Removing a partition from a domain must revoke exactly that partition:
 * other partitions stay reachable, the removed one faults, and adding it
 * back restores access. All three transitions are observed from member user
 * threads.
 *
 * Test steps:
 * - Remove one read-write partition and show another is still accessible.
 * - Access the removed partition and expect a fault.
 * - Add the partition back and access it again.
 *
 * Expected result:
 * - Access follows the domain's current partition set at each step.
 *
 * @see k_mem_domain_remove_partition()
 * @see k_mem_domain_add_partition()
 */
ZTEST(mem_protect_domain, test_mem_domain_remove_add_partition)
{
	zassert_equal(
		k_mem_domain_remove_partition(&test_domain, &rw_parts[0]),
		0, "failed to remove memory partition");

	/* Should still work, we didn't remove ro_part */
	spawn_child_thread(ro_part_access, &test_domain, false);

	/* This will fault, we removed one of the rw_part from the domain */
	spawn_child_thread(rw_part_access, &test_domain, true);

	/* Restore test_domain contents so we don't mess up other tests */
	zassert_equal(
		k_mem_domain_add_partition(&test_domain, &rw_parts[0]),
		0, "failed to add memory partition");

	/* Should work again */
	spawn_child_thread(rw_part_access, &test_domain, false);
}

/* user mode will attempt to initialize this and fail */
static struct k_mem_domain no_access_domain;

/* Extra partition that a user thread can't add to a domain */
static volatile uint8_t __aligned(MEM_REGION_ALLOC)
	no_access_buf[MEM_REGION_ALLOC];
K_MEM_PARTITION_DEFINE(no_access_part, no_access_buf, sizeof(no_access_buf),
		       K_MEM_PARTITION_P_RW_U_RW);

static void mem_domain_init_entry(void *p1, void *p2, void *p3)
{
	zassert_equal(
		k_mem_domain_init(&no_access_domain, 0, NULL),
		0, "failed to initialize memory domain");

#if defined(CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT)
	zassert_equal(k_mem_domain_deinit(&no_access_domain), 0,
		      "failed to de-initialize memory domain");
#endif /* CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT */
}

static void mem_domain_add_partition_entry(void *p1, void *p2, void *p3)
{
	zassert_equal(
		k_mem_domain_add_partition(&test_domain, &no_access_part),
		0, "failed to add memory partition");
}

static void mem_domain_remove_partition_entry(void *p1, void *p2, void *p3)
{
	zassert_equal(
		k_mem_domain_remove_partition(&test_domain, &ro_part),
		0, "failed to remove memory partition");
}

static void mem_domain_add_thread_entry(void *p1, void *p2, void *p3)
{
	k_mem_domain_add_thread(&test_domain, zzz_thread);
}

/**
 * @brief Verify that the memory domain APIs are supervisor-only.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The domain configuration calls have no syscall wrappers: they reshape the
 * protection landscape and must be unreachable from user mode. A user thread
 * invoking any of them has to fault on the attempt.
 *
 * Test steps:
 * - From a user thread, call k_mem_domain_init(), add_partition,
 *   remove_partition and add_thread in turn.
 *
 * Expected result:
 * - Each call faults before doing anything.
 *
 * @see k_mem_domain_init()
 * @see k_mem_domain_add_partition()
 * @see k_mem_domain_remove_partition()
 * @see k_mem_domain_add_thread()
 */
ZTEST(mem_protect_domain, test_mem_domain_api_supervisor_only)
{
	/* All of these should fault when invoked from a user thread */
	spawn_child_thread(mem_domain_init_entry, NULL, true);
	spawn_child_thread(mem_domain_add_partition_entry, NULL, true);
	spawn_child_thread(mem_domain_remove_partition_entry, NULL, true);
	spawn_child_thread(mem_domain_add_thread_entry, NULL, true);
}

/**
 * @brief Verify that boot-time threads start in the default memory domain.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Threads that exist before the application configures anything -- the main
 * thread and statically defined threads -- must be members of the default
 * domain, or they would run with no defined memory view at all. Membership
 * is read directly from the thread structures.
 *
 * Test steps:
 * - Check z_main_thread's domain membership.
 * - Start a static thread and check its membership.
 *
 * Expected result:
 * - Both belong to k_mem_domain_default.
 */
ZTEST(mem_protect_domain, test_mem_domain_boot_threads)
{
	/* Check that a static thread got put in the default memory domain */
	zassert_true(zzz_thread->mem_domain_info.mem_domain ==
		     &k_mem_domain_default, "unexpected mem domain %p",
		     zzz_thread->mem_domain_info.mem_domain);

	/* Check that the main thread is also a member of the default domain */
	zassert_true(z_main_thread.mem_domain_info.mem_domain ==
		     &k_mem_domain_default, "unexpected mem domain %p",
		     z_main_thread.mem_domain_info.mem_domain);

	k_thread_abort(zzz_thread);
}

static ZTEST_BMEM volatile bool spin_done;
static K_SEM_DEFINE(spin_sem, 0, 1);

static void spin_entry(void *p1, void *p2, void *p3)
{
	printk("spin thread entry\n");
	k_sem_give(&spin_sem);

	while (!spin_done) {
		k_busy_wait(1);
	}
	printk("spin thread completed\n");
}

/**
 * @brief Verify that a running thread can be migrated between domains.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * k_mem_domain_add_thread() may be applied to a thread that is actively
 * running, and the migration must not fault it -- the destination domain
 * also carries the partitions the thread is using. On SMP the thread spins
 * on another CPU while the migration happens, which is the racy path this
 * case exists for. Re-adding the thread to the domain it is already in must
 * be a no-op.
 *
 * Test steps:
 * - Start a user thread that spins on shared state.
 * - While it runs, add it to the test domain, then add it again.
 * - Release the spin and join the thread.
 *
 * Expected result:
 * - The thread keeps running through the migration and the repeated add
 *   changes nothing.
 *
 * @see k_mem_domain_add_thread()
 */

#if CONFIG_MP_MAX_NUM_CPUS > 1
#define PRIO	K_PRIO_COOP(0)
#else
#define PRIO	K_PRIO_PREEMPT(1)
#endif

ZTEST(mem_protect_domain, test_mem_domain_migration)
{
	int ret;

	set_fault_valid(false);

	k_thread_create(&child_thread, child_stack,
			K_THREAD_STACK_SIZEOF(child_stack), spin_entry,
			NULL, NULL, NULL,
			PRIO, K_USER | K_INHERIT_PERMS, K_FOREVER);
	k_thread_name_set(&child_thread, "child_thread");
	k_object_access_grant(&spin_sem, &child_thread);
	k_thread_start(&child_thread);

	/* Ensure that the child thread has started */
	ret = k_sem_take(&spin_sem, K_FOREVER);
	zassert_equal(ret, 0, "k_sem_take failed");

	/* Now move it to test_domain. This domain also has the ztest partition,
	 * so the child thread should keep running and not explode
	 */
	printk("migrate to new domain\n");
	k_mem_domain_add_thread(&test_domain, &child_thread);

	/**TESTPOINT: add to existing domain will do nothing */
	k_mem_domain_add_thread(&test_domain, &child_thread);

	/* set spin_done so the child thread completes */
	printk("set test completion\n");
	spin_done = true;

	k_thread_join(&child_thread, K_FOREVER);
}

/**
 * @brief Test system assert when new partition overlaps the existing partition
 *
 * @details
 * Test Objective:
 * - Test assertion if the new partition overlaps existing partition in domain
 *
 * Testing techniques:
 * - System testing
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define testing memory partition overlap_part with the same start ro_buf
 *  as has the existing memory partition ro_part
 * -# Try to add overlap_part to the memory domain. When adding the new
 *  partition to the memory domain the system will assert that new partition
 *  overlaps with the existing partition ro_part .
 *
 * Expected Test Result:
 * - Must happen an assertion error indicating that the new partition overlaps
 *   the existing one.
 *
 * Pass/Fail Criteria:
 * - Success if the overlap assertion will happen.
 * - Failure if the overlap assertion will not happen.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_add_partition()
 */
ZTEST(mem_protect_domain, test_mem_part_overlap)
{
	set_fault_valid(false);

	zassert_not_equal(
		k_mem_domain_add_partition(&test_domain, &overlap_part),
		0, "should fail to add memory partition");
}

extern struct k_spinlock z_mem_domain_lock;

static struct k_mem_domain test_domain_fail;

static volatile uint8_t __aligned(MEM_REGION_ALLOC)
	exceed_buf[MEM_REGION_ALLOC];

K_MEM_PARTITION_DEFINE(exceed_part, exceed_buf, sizeof(exceed_buf),
		      K_MEM_PARTITION_P_RW_U_RW);

/**
 * @brief Test system assert when adding memory partitions more than possible
 *
 * @details
 * - Add memory partitions one by one and more than architecture allows to add.
 * - When partitions added more than it is allowed by architecture, test that
 *   k_mem_domain_add_partition() returns non-zero.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_domain, test_mem_part_assert_add_overmax)
{
	int max_parts = num_rw_parts + PARTS_USED;

	/* Make sure the partitions of the domain is full, used in
	 * previous test cases.
	 */
	zassert_equal(max_parts, arch_mem_domain_max_partitions_get(),
			"domain still have room of partitions(%d).",
			max_parts);

	set_fault_valid(false);

	/* Add one more partition will fail due to exceeding */
	zassert_not_equal(
		k_mem_domain_add_partition(&test_domain, &exceed_part),
		0, "should fail to add memory partition");
}


#if defined(CONFIG_ASSERT)
static volatile uint8_t __aligned(MEM_REGION_ALLOC) misc_buf[MEM_REGION_ALLOC];
K_MEM_PARTITION_DEFINE(find_no_part, misc_buf, sizeof(misc_buf),
		       K_MEM_PARTITION_P_RO_U_RO);

/**
 * @brief Test error case of removing memory partition fail
 *
 * @details Try to remove a partition not in the domain.
 * k_mem_domain_remove_partition() should return non-zero.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_domain, test_mem_domain_remove_part_fail)
{
	struct k_mem_partition *no_parts = &find_no_part;

	set_fault_valid(false);

	zassert_not_equal(
		k_mem_domain_remove_partition(&test_domain, no_parts),
		0, "should fail to remove memory partition");
}
#else
ZTEST(mem_protect_domain, test_mem_domain_remove_part_fail)
{
	ztest_test_skip();
}
#endif

/**
 * @brief Test error case of initializing memory domain fail
 *
 * @details Try to initialize a domain with invalid partition.
 * k_mem_domain_init() should return non-zero.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_domain, test_mem_domain_init_fail)
{
	struct k_mem_partition *no_parts[] = {&ro_part, 0};

	/* init another domain fail */
	set_fault_valid(false);

	zassert_not_equal(
		k_mem_domain_init(&test_domain_fail, ARRAY_SIZE(no_parts),
				  no_parts),
		0, "should fail to initialize memory domain");

#if defined(CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT)
	zassert_equal(k_mem_domain_deinit(&test_domain_fail), 0,
		      "cannot de-initialize memory domain");
#endif /* CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT */
}

/**
 * @brief Test error case of de-initializing memory domain fail
 *
 * @details Try to de-initialize a domain with various invalid
 * conditions.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_domain, test_mem_domain_deinit_fail)
{
#if defined(CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT)
	set_fault_valid(false);

	/* Should not be able to de-init the default domain. */
	zassert_equal(k_mem_domain_deinit(&k_mem_domain_default), -EINVAL,
		      "should fail de-initializing default domain");

	/* Create a thread and attach it to the test_domain.
	 * We should not be able to de-initialize the domain while
	 * there is a thread attached to it.
	 */
	k_thread_create(&child_thread, child_stack, K_THREAD_STACK_SIZEOF(child_stack),
			rw_part_access,	NULL, NULL, NULL, 0, K_USER, K_FOREVER);
	k_thread_name_set(&child_thread, "child_thread");
	k_mem_domain_add_thread(&test_domain, &child_thread);

	zassert_equal(k_mem_domain_deinit(&test_domain), -EBUSY,
		      "should fail de-initializing test domain with threads attached");

	/* Let the thread run to the end so any memory domain related
	 * cleanup will be done.
	 */
	k_thread_start(&child_thread);
	k_thread_join(&child_thread, K_FOREVER);

	/* Note that we cannot test the proper de-initialization of test_domain
	 * here (... where this should succeed). It is because the test_domain
	 * is still being used for other tests in this test suite.
	 * Instead, it will be tested in test_mem_domain_teardown() when all
	 * tests have run.
	 */

#else  /* CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT */
	ztest_test_skip();
#endif /* CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT */
}

/**
 * @brief Test error case of adding null memory partition fail
 *
 * @details Try to add a null partition to memory domain.
 * k_mem_domain_add_partition() should return error.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_domain, test_mem_part_add_error_null)
{
	/* add partition fail */
	set_fault_valid(false);

	zassert_not_equal(
		k_mem_domain_add_partition(&test_domain_fail, NULL),
		0, "should fail to add memory partition");
}

static volatile uint8_t __aligned(MEM_REGION_ALLOC) nosize_buf[MEM_REGION_ALLOC];
K_MEM_PARTITION_DEFINE(nonsize_part, nosize_buf, sizeof(nosize_buf),
			K_MEM_PARTITION_P_RO_U_RO);

/**
 * @brief Test error case of adding zero sized memory partition fail
 *
 * @details Try to add a zero sized partition to memory domain.
 * k_mem_domain_add_partition() should return error.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_domain, test_mem_part_add_error_zerosize)
{
	struct k_mem_partition *nosize_part = &nonsize_part;

	nosize_part->size = 0U;

	/* add partition fail */
	set_fault_valid(false);

	zassert_not_equal(
		k_mem_domain_add_partition(&test_domain_fail, nosize_part),
		0, "should fail to add memory partition");
}

/**
 * @brief Test error case of memory partition address wraparound
 *
 * @details Try to add a partition whose address is wraparound.
 * k_mem_domain_add_partition() should return error.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_domain, test_mem_part_error_wraparound)
{
#ifdef CONFIG_64BIT
	K_MEM_PARTITION_DEFINE(wraparound_part, 0xfffffffffffff800, 2048,
		       K_MEM_PARTITION_P_RO_U_RO);
#else
	K_MEM_PARTITION_DEFINE(wraparound_part, 0xfffff800, 2048,
		       K_MEM_PARTITION_P_RO_U_RO);
#endif

	/* add partition fail */
	set_fault_valid(false);

	zassert_not_equal(
		k_mem_domain_add_partition(&test_domain_fail, &wraparound_part),
		0, "should fail to add memory partition");
}

/**
 * @brief Test error case of removing memory partition fail
 *
 * @details Try to remove a partition size mismatched will result
 * in k_mem_domain_remove_partition() returning error.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_domain, test_mem_part_remove_error_zerosize)
{
	struct k_mem_partition *no_parts = &find_no_part;

	zassert_equal(
		k_mem_domain_remove_partition(&test_domain, &rw_parts[0]),
		0, "failed to remove memory partition");

	zassert_equal(
		k_mem_domain_add_partition(&test_domain, no_parts),
		0, "failed to add memory partition");

	no_parts->size = 0U;

	/* remove partition fail */
	set_fault_valid(false);

	zassert_not_equal(
		k_mem_domain_remove_partition(&test_domain, no_parts),
		0, "should fail to remove memory partition");
}

ZTEST_SUITE(mem_protect_domain, NULL, test_mem_domain_setup, NULL,
		NULL, test_mem_domain_teardown);
