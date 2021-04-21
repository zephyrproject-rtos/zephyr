/*
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem_protect.h"
#include <kernel_internal.h> /* For z_main_thread */

static struct k_thread child_thread;
static K_THREAD_STACK_DEFINE(child_stack, 512 + CONFIG_TEST_EXTRA_STACKSIZE);

/* Special memory domain for test case purposes */
static struct k_mem_domain test_domain;

#define PARTS_USED	2
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

static K_THREAD_DEFINE(zzz_thread, 256 + CONFIG_TEST_EXTRA_STACKSIZE,
		       zzz_entry, NULL, NULL, NULL, 0, 0, 0);

void test_mem_domain_setup(void)
{
	int max_parts = arch_mem_domain_max_partitions_get();
	struct k_mem_partition *parts[] = { &ro_part, &ztest_mem_partition };

	num_rw_parts = max_parts - PARTS_USED;
	zassert_true(NUM_RW_PARTS >= num_rw_parts,
		     "CONFIG_MAX_DOMAIN_PARTITIONS incorrectly tuned, %d should be at least %d",
		     CONFIG_MAX_DOMAIN_PARTITIONS, max_parts);
	zassert_true(num_rw_parts > 0, "no free memory partitions");

	k_mem_domain_init(&test_domain, ARRAY_SIZE(parts), parts);

	for (unsigned int i = 0; i < num_rw_parts; i++) {
		rw_parts[i].start = (uintptr_t)&rw_bufs[i];
		rw_parts[i].size = MEM_REGION_ALLOC;
		rw_parts[i].attr = K_MEM_PARTITION_P_RW_U_RW;

		for (unsigned int j = 0; j < MEM_REGION_ALLOC; j++) {
			rw_bufs[i][j] = (j % 256U);
		}

		k_mem_domain_add_partition(&test_domain, &rw_parts[i]);
	}

	for (unsigned int j = 0; j < MEM_REGION_ALLOC; j++) {
		ro_buf[j] = (j % 256U);
	}
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
 * @brief Check if the mem_domain is configured and accessible for userspace
 *
 * Join a memory domain with a read-write memory partition and a read-only
 * partition within it, and show that the data in the partition is accessible
 * as expected by the permissions provided.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_valid_access(void)
{
	spawn_child_thread(rw_part_access, &test_domain, false);
	spawn_child_thread(ro_part_access, &test_domain, false);
}

/**
 * @brief Show that a user thread can't touch partitions not in its domain
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_invalid_access(void)
{
	/* child not added to test_domain, will fault for both */
	spawn_child_thread(rw_part_access, NULL, true);
	spawn_child_thread(ro_part_access, NULL, true);
}

/**
 * @brief Show that a read-only partition can't be written to
 *
 * @ingroup kernel_memgroup_tests
 */
void test_mem_domain_no_writes_to_ro(void)
{
	/* Show that trying to write to a read-only partition causes a fault */
	spawn_child_thread(ro_write_entry, &test_domain, true);
}

/**
 * @brief Show that adding/removing partitions works
 *
 * Show that removing a partition doesn't affect access to other partitions.
 * Show that removing a partition generates a fault if its data is accessed.
 * Show that adding a partition back restores access from a user thread.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_remove_add_partition(void)
{
	k_mem_domain_remove_partition(&test_domain, &rw_parts[0]);

	/* Should still work, we didn't remove ro_part */
	spawn_child_thread(ro_part_access, &test_domain, false);

	/* This will fault, we removed one of the rw_part from the domain */
	spawn_child_thread(rw_part_access, &test_domain, true);

	/* Restore test_domain contents so we don't mess up other tests */
	k_mem_domain_add_partition(&test_domain, &rw_parts[0]);

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
	k_mem_domain_init(&no_access_domain, 0, NULL);
}

static void mem_domain_add_partition_entry(void *p1, void *p2, void *p3)
{
	k_mem_domain_add_partition(&test_domain, &no_access_part);
}

static void mem_domain_remove_partition_entry(void *p1, void *p2, void *p3)
{
	k_mem_domain_remove_partition(&test_domain, &ro_part);
}

static void mem_domain_add_thread_entry(void *p1, void *p2, void *p3)
{
	k_mem_domain_add_thread(&test_domain, zzz_thread);
}

/**
 * @brief Test access memory domain APIs allowed to supervisor threads only
 *
 * Show that invoking any of the memory domain APIs from user mode leads to
 * a fault.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_init(), k_mem_domain_add_partition(),
 *	k_mem_domain_remove_partition(), k_mem_domain_add_thread()
 */
void test_mem_domain_api_supervisor_only(void)
{
	/* All of these should fault when invoked from a user thread */
	spawn_child_thread(mem_domain_init_entry, NULL, true);
	spawn_child_thread(mem_domain_add_partition_entry, NULL, true);
	spawn_child_thread(mem_domain_remove_partition_entry, NULL, true);
	spawn_child_thread(mem_domain_add_thread_entry, NULL, true);
}

/**
 * @brief Show that boot threads belong to the default memory domain
 *
 * Static threads and the main thread are supposed to start as members of
 * the default memory domain. Prove this is the case by examining the
 * memory domain membership of z_main_thread and a static thread.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_boot_threads(void)
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
 * @brief Show that moving a thread from one domain to another works
 *
 * Start a thread and have it spin. Then while it is spinning, show that
 * adding it to another memory domain doesn't cause any faults.
 *
 * This test is of particular importance on SMP systems where the child
 * thread is spinning on a different CPU concurrently with the migration
 * operation.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_add_thread()
 */

#if CONFIG_MP_NUM_CPUS > 1
#define PRIO	K_PRIO_COOP(0)
#else
#define PRIO	K_PRIO_PREEMPT(1)
#endif

void test_mem_domain_migration(void)
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
void test_mem_part_overlap(void)
{
	set_fault_valid(true);

	k_mem_domain_add_partition(&test_domain, &overlap_part);
}


extern struct k_spinlock z_mem_domain_lock;

static ZTEST_BMEM bool need_recover_spinlock;

static struct k_mem_domain test_domain_fail;

void post_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	if (need_recover_spinlock) {
		k_spin_release(&z_mem_domain_lock);

		need_recover_spinlock = false;
	}
}

#if defined(CONFIG_ASSERT)
static volatile uint8_t __aligned(MEM_REGION_ALLOC) misc_buf[MEM_REGION_ALLOC];
K_MEM_PARTITION_DEFINE(find_no_part, misc_buf, sizeof(misc_buf),
		       K_MEM_PARTITION_P_RO_U_RO);

/**
 * @brief Test error case of removing memory partition fail
 *
 * @details Try to remove a partition not in the domain will cause
 * assertion, then triggher an expected fatal error.
 * And while the fatal error happened, the memory domain spinlock
 * is held, we need to release them to make other follow test case.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_remove_part_fail(void)
{
	struct k_mem_partition *no_parts = &find_no_part;

	need_recover_spinlock = true;
	set_fault_valid(true);
	k_mem_domain_remove_partition(&test_domain, no_parts);
}
#else
void test_mem_domain_remove_part_fail(void)
{
	ztest_test_skip();
}
#endif

/**
 * @brief Test error case of initializing memory domain fail
 *
 * @details Try to initialize a domain with invalid partition, then see
 * if an expected fatal error happens.
 * And while the fatal error happened, the memory domain spinlock
 * is held, we need to release them to make other follow test case.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_init_fail(void)
{
	struct k_mem_partition *no_parts[] = {&ro_part, 0};

	/* init another domain fail, expected fault happened */
	need_recover_spinlock = true;
	set_fault_valid(true);
	k_mem_domain_init(&test_domain_fail, ARRAY_SIZE(no_parts),
			no_parts);

	/* For acrh which not CONFIG_ARCH_MEM_DOMAIN_DATA, if assert is off,
	 * it will reach here.
	 */
#if !defined(CONFIG_ASSERT) && defined(CONFIG_ARCH_MEM_DOMAIN_DATA)
	zassert_unreachable("should not be here");
#endif
}
