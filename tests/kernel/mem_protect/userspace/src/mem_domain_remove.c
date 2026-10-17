/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/app_memory/app_memdomain.h>
/* for z_libc_partition */
#include <zephyr/sys/libc-hooks.h>

extern void set_fault(unsigned int reason);
extern void clear_fault(void);

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
#define MAGIC1 0xA1
#endif
#define MAGIC2 0xB2
#define MAGIC3 0xC3

/* Test buffers live in linker-managed application memory partitions, so
 * their size and alignment satisfy each arch's MPU/MMU requirements
 * automatically.
 */
#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
K_APPMEM_PARTITION_DEFINE(remove_part);
K_APP_BMEM(remove_part) volatile uint8_t remove_byte;
#endif
K_APPMEM_PARTITION_DEFINE(hole_part_a);
K_APPMEM_PARTITION_DEFINE(hole_part_b);
K_APPMEM_PARTITION_DEFINE(hole_part_c);
K_APP_BMEM(hole_part_a) volatile uint8_t hole_byte_a;
K_APP_BMEM(hole_part_b) volatile uint8_t hole_byte_b;
K_APP_BMEM(hole_part_c) volatile uint8_t hole_byte_c;

static struct k_thread part_thread;
static K_THREAD_STACK_DEFINE(part_stack, 2048);

/* File-scope domains: the ztest thread reuses the same stack slot across
 * tests, so two stack-local domains would alias one address, which the
 * x86 arch code rejects as a double arch_mem_domain_init().
 */
#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
static struct k_mem_domain remove_dom;
#endif
static struct k_mem_domain hole_dom;

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
static K_SEM_DEFINE(part_started, 0, 1);
static K_SEM_DEFINE(part_go, 0, 1);

static void removed_part_write(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(&part_started);
	k_sem_take(&part_go, K_FOREVER);
	remove_byte = MAGIC1;
}

/**
 * @brief Removed partition must not stay accessible to a started thread
 *
 * @details A user thread that is already started when
 * k_mem_domain_remove_partition() runs must lose access to the removed
 * partition. On Armv8-R AArch64 the arch remove hook rescanned the
 * domain before the kernel invalidated the slot, programming the
 * removed partition right back (issue #115177, defect 1).
 *
 * Only architectures with a synchronous memory domain API update the
 * memory configuration of already started threads on partition removal,
 * so this test is limited to those.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace, test_domain_remove_partition_started_thread)
{
	/* z_libc_partition carries the arch TLS pointer that the user
	 * thread entry code reads on some architectures (e.g. Arm).
	 */
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&remove_part,
	};
	k_tid_t tid;

	clear_fault();
	remove_byte = 0;

	zassert_equal(k_mem_domain_init(&remove_dom, ARRAY_SIZE(parts), parts), 0);

	tid = k_thread_create(&part_thread, part_stack, K_THREAD_STACK_SIZEOF(part_stack),
			      removed_part_write, NULL, NULL, NULL, 1, K_USER, K_FOREVER);
	k_thread_access_grant(tid, &part_started, &part_go);
	zassert_equal(k_mem_domain_add_thread(&remove_dom, tid), 0);

	/* The thread must be running before the removal happens. */
	k_thread_start(tid);
	zassert_equal(k_sem_take(&part_started, K_FOREVER), 0);

	zassert_equal(k_mem_domain_remove_partition(&remove_dom, &remove_part), 0);

	/* The correct outcome is a fault on the write; tolerate it so the
	 * assertion below can distinguish it from a silent write.
	 */
	set_fault(K_ERR_CPU_EXCEPTION);
	k_sem_give(&part_go);
	zassert_equal(k_thread_join(tid, K_FOREVER), 0);
	clear_fault();

	zassert_not_equal(remove_byte, MAGIC1,
			  "removed partition still accessible to started thread");
}
#endif /* CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API */

static void hole_part_write(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	hole_byte_b = MAGIC2; /* control: must always succeed */
	hole_byte_c = MAGIC3; /* valid partition behind the hole */
}

/**
 * @brief A hole in partitions[] must not skip valid partitions
 *
 * @details Add P1,P2,P3 to a domain, remove P1 so the partition array
 * becomes [hole, P2, P3], then run a user thread accessing P3's memory.
 * The access must succeed. On Armv8-R AArch64 and ARC the region
 * programming loop decremented its remaining-partition counter for
 * holes too, so P3 was never programmed into the MPU (issue #115177,
 * defect 2).
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace, test_domain_remove_partition_hole_skip)
{
	/* z_libc_partition carries the arch TLS pointer that the user
	 * thread entry code reads on some architectures (e.g. Arm).
	 */
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&hole_part_a,
		&hole_part_b,
		&hole_part_c,
	};
	k_tid_t tid;

	clear_fault();
	hole_byte_a = 0;
	hole_byte_b = 0;
	hole_byte_c = 0;

	zassert_equal(k_mem_domain_init(&hole_dom, ARRAY_SIZE(parts), parts), 0);

	tid = k_thread_create(&part_thread, part_stack, K_THREAD_STACK_SIZEOF(part_stack),
			      hole_part_write, NULL, NULL, NULL, 1, K_USER, K_FOREVER);
	zassert_equal(k_mem_domain_add_thread(&hole_dom, tid), 0);

	/* leaves partitions[] = [z_libc, hole, P2, P3], num_partitions = 3
	 * when z_libc_partition exists, [hole, P2, P3] otherwise.
	 */
	zassert_equal(k_mem_domain_remove_partition(&hole_dom, &hole_part_a), 0);

	/* On the buggy kernel the write to P3 faults; tolerate it so the
	 * assertion below can report the failure gracefully.
	 */
	set_fault(K_ERR_CPU_EXCEPTION);
	k_thread_start(tid);
	zassert_equal(k_thread_join(tid, K_FOREVER), 0);
	clear_fault();

	zassert_equal(hole_byte_b, MAGIC2, "control partition P2 not accessible");
	zassert_equal(hole_byte_c, MAGIC3, "valid partition P3 skipped by hole in partitions[]");
}
