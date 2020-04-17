/*
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem_protect.h"
#include <sys/mempool.h>

#define ERROR_STR \
	("Fault didn't occur when we accessed a unassigned memory domain.")

/****************************************************************************/
K_THREAD_STACK_DEFINE(mem_domain_1_stack, MEM_DOMAIN_STACK_SIZE);
K_THREAD_STACK_DEFINE(mem_domain_2_stack, MEM_DOMAIN_STACK_SIZE);
K_THREAD_STACK_DEFINE(mem_domain_6_stack, MEM_DOMAIN_STACK_SIZE);
struct k_thread mem_domain_1_tid, mem_domain_2_tid, mem_domain_6_tid;
struct k_mem_domain domain0, domain1, domain4, name_domain, overlap_domain;

struct k_thread user_thread0, parent_thr_md, child_thr_md;
k_tid_t usr_tid0, usr_tid1, child_tid;
K_THREAD_STACK_DEFINE(user_thread0_stack, STACK_SIZE_MD);
K_THREAD_STACK_DEFINE(child_thr_stack_md, STACK_SIZE_MD);
K_THREAD_STACK_DEFINE(parent_thr_stack_md, STACK_SIZE_MD);

struct k_sem sync_sem_md;

uint8_t __aligned(MEM_REGION_ALLOC) buf0[MEM_REGION_ALLOC];
K_MEM_PARTITION_DEFINE(part0, buf0, sizeof(buf0),
		K_MEM_PARTITION_P_RW_U_RW);

K_APPMEM_PARTITION_DEFINE(part1);
K_APP_BMEM(part1) uint8_t __aligned(MEM_REGION_ALLOC) buf1[MEM_REGION_ALLOC];
struct k_mem_partition *app1_parts[] = {
	&part1
};
K_APPMEM_PARTITION_DEFINE(part_arch);
K_APP_BMEM(part_arch) uint8_t __aligned(MEM_REGION_ALLOC) \
buf_arc[MEM_REGION_ALLOC];

K_APPMEM_PARTITION_DEFINE(part2);
K_APP_DMEM(part2) int part2_var = 1356;
K_APP_BMEM(part2) int part2_zeroed_var = 20420;
K_APP_BMEM(part2) int part2_bss_var;

SYS_MEM_POOL_DEFINE(data_pool, NULL, BLK_SIZE_MIN_MD, BLK_SIZE_MAX_MD,
		BLK_NUM_MAX_MD, BLK_ALIGN_MD, K_APP_DMEM_SECTION(part2));

/****************************************************************************/
/* The mem domains needed.*/
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_buf[MEM_REGION_ALLOC];
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_buf1[MEM_REGION_ALLOC];

/* partitions added later in the test cases.*/
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part1[MEM_REGION_ALLOC];
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part2[MEM_REGION_ALLOC];
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part3[MEM_REGION_ALLOC];
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part4[MEM_REGION_ALLOC];
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part5[MEM_REGION_ALLOC];
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part6[MEM_REGION_ALLOC];
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part7[MEM_REGION_ALLOC];
uint8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part8[MEM_REGION_ALLOC];

K_MEM_PARTITION_DEFINE(mem_domain_memory_partition,
		       mem_domain_buf,
		       sizeof(mem_domain_buf),
		       K_MEM_PARTITION_P_RW_U_RW);

#if defined(CONFIG_X86) || \
	((defined(CONFIG_ARMV8_M_BASELINE) || \
		defined(CONFIG_ARMV8_M_MAINLINE)) \
		&& defined(CONFIG_CPU_HAS_ARM_MPU))
K_MEM_PARTITION_DEFINE(mem_domain_memory_partition1,
		       mem_domain_buf1,
		       sizeof(mem_domain_buf1),
		       K_MEM_PARTITION_P_RO_U_RO);
#else
K_MEM_PARTITION_DEFINE(mem_domain_memory_partition1,
		       mem_domain_buf1,
		       sizeof(mem_domain_buf1),
		       K_MEM_PARTITION_P_RW_U_RO);
#endif

struct k_mem_partition *mem_domain_memory_partition_array[] = {
	&mem_domain_memory_partition,
	&ztest_mem_partition
};

struct k_mem_partition *mem_domain_memory_partition_array1[] = {
	&mem_domain_memory_partition1,
	&ztest_mem_partition
};
struct k_mem_domain mem_domain_mem_domain;
struct k_mem_domain mem_domain1;

/****************************************************************************/
/* Common init functions */
static inline void mem_domain_init(void)
{
	k_mem_domain_init(&mem_domain_mem_domain,
			  ARRAY_SIZE(mem_domain_memory_partition_array),
			  mem_domain_memory_partition_array);
}

static inline void set_valid_fault_value(int test_case_number)
{
	switch (test_case_number) {
	case 1:
		set_fault_valid(false);
		break;
	case 2:
		set_fault_valid(true);
		break;
	default:
		set_fault_valid(false);
		break;
	}
}

/****************************************************************************/
/* Userspace function */
static void mem_domain_for_user(void *tc_number, void *p2, void *p3)
{
	set_valid_fault_value((int)(uintptr_t)tc_number);

	mem_domain_buf[0] = 10U;
	if (valid_fault == false) {
		ztest_test_pass();
	} else {
		ztest_test_fail();
	}
}

static void mem_domain_test_1(void *tc_number, void *p2, void *p3)
{
	if ((uintptr_t)tc_number == 1U) {
		mem_domain_buf[0] = 10U;
		k_mem_domain_add_thread(&mem_domain_mem_domain,
					k_current_get());
	}

	k_thread_user_mode_enter(mem_domain_for_user, tc_number, NULL, NULL);

}

/****************************************************************************/
/**
 * @brief Check if the mem_domain is configured and accessible for userspace
 *
 * @ingroup kernel_memgroup_tests
 *
 * @see k_mem_domain_init()
 */
void test_mem_domain_valid_access(void)
{
	uintptr_t tc_number = 1U;

	mem_domain_init();

	k_thread_create(&mem_domain_1_tid,
			mem_domain_1_stack,
			MEM_DOMAIN_STACK_SIZE,
			mem_domain_test_1,
			(void *)tc_number, NULL, NULL,
			10, 0, K_NO_WAIT);

	k_thread_join(&mem_domain_1_tid, K_FOREVER);
}

/****************************************************************************/
/**
 * @brief Test to check memory domain invalid access
 *
 * @details If mem domain was not added to the thread and a access to it should
 * cause a fault.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_invalid_access(void)
{
	uintptr_t tc_number = 2U;

	k_thread_create(&mem_domain_2_tid,
			mem_domain_2_stack,
			MEM_DOMAIN_STACK_SIZE,
			mem_domain_test_1,
			(void *)tc_number, NULL, NULL,
			10, 0, K_NO_WAIT);

	k_thread_join(&mem_domain_2_tid, K_FOREVER);
}
/***************************************************************************/
static void thread_entry_rw(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	uint8_t read_data = mem_domain_buf[0];

	/* Just to avoid compiler warnings */
	(void) read_data;

	/* Write to the partition */
	mem_domain_buf[0] = 99U;

	ztest_test_pass();
}
/**
 * @brief Test memory domain read/write access
 *
 * @details Provide read/write access to a partition
 * and verify access from a user thread added to it
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_partitions_user_rw(void)
{
	/* Initialize the memory domain */
	k_mem_domain_init(&mem_domain_mem_domain,
			  ARRAY_SIZE(mem_domain_memory_partition_array),
			  mem_domain_memory_partition_array);

	k_mem_domain_add_thread(&mem_domain_mem_domain,
				k_current_get());

	k_thread_user_mode_enter(thread_entry_rw, NULL, NULL, NULL);
}
/****************************************************************************/
static void user_thread_entry_ro(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	/* Read the partition */
	uint8_t read_data = mem_domain_buf1[0];

	/* Just to avoid compiler warning */
	(void) read_data;

	/* Writing to the partition, this should generate fault
	 * as the partition has read only permission for
	 * user threads
	 */
	mem_domain_buf1[0] = 10U;

	zassert_unreachable("The user thread is allowed to access a read only"
			    " partition of a memory domain");
}
/**
 * @brief Test memory domain read/write access for user thread
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_add_thread()
 */
void test_mem_domain_partitions_user_ro(void)
{
	/* Initialize the memory domain containing the partition
	 * with read only access privilege
	 */
	k_mem_domain_init(&mem_domain1,
			  ARRAY_SIZE(mem_domain_memory_partition_array1),
			  mem_domain_memory_partition_array1);

	k_mem_domain_add_thread(&mem_domain1, k_current_get());

	k_thread_user_mode_enter(user_thread_entry_ro, NULL, NULL, NULL);
}

/****************************************************************************/
/**
 * @brief Test memory domain read/write access for kernel thread
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_partitions_supervisor_rw(void)
{
	k_mem_domain_init(&mem_domain_mem_domain,
			  ARRAY_SIZE(mem_domain_memory_partition_array1),
			  mem_domain_memory_partition_array1);

	k_mem_domain_add_thread(&mem_domain_mem_domain, k_current_get());

	/* Create a supervisor thread */
	k_thread_create(&mem_domain_1_tid, mem_domain_1_stack,
			MEM_DOMAIN_STACK_SIZE, (k_thread_entry_t)thread_entry_rw,
			NULL, NULL, NULL, 10, K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(&mem_domain_1_tid, K_FOREVER);
}

/****************************************************************************/

/* add partitions */
K_MEM_PARTITION_DEFINE(mem_domain_tc3_part1_struct,
		       mem_domain_tc3_part1,
		       sizeof(mem_domain_tc3_part1),
		       K_MEM_PARTITION_P_RW_U_RW);

K_MEM_PARTITION_DEFINE(mem_domain_tc3_part2_struct,
		       mem_domain_tc3_part2,
		       sizeof(mem_domain_tc3_part2),
		       K_MEM_PARTITION_P_RW_U_RW);

K_MEM_PARTITION_DEFINE(mem_domain_tc3_part3_struct,
		       mem_domain_tc3_part3,
		       sizeof(mem_domain_tc3_part3),
		       K_MEM_PARTITION_P_RW_U_RW);

K_MEM_PARTITION_DEFINE(mem_domain_tc3_part4_struct,
		       mem_domain_tc3_part4,
		       sizeof(mem_domain_tc3_part4),
		       K_MEM_PARTITION_P_RW_U_RW);

K_MEM_PARTITION_DEFINE(mem_domain_tc3_part5_struct,
		       mem_domain_tc3_part5,
		       sizeof(mem_domain_tc3_part5),
		       K_MEM_PARTITION_P_RW_U_RW);

K_MEM_PARTITION_DEFINE(mem_domain_tc3_part6_struct,
		       mem_domain_tc3_part6,
		       sizeof(mem_domain_tc3_part6),
		       K_MEM_PARTITION_P_RW_U_RW);

K_MEM_PARTITION_DEFINE(mem_domain_tc3_part7_struct,
		       mem_domain_tc3_part7,
		       sizeof(mem_domain_tc3_part7),
		       K_MEM_PARTITION_P_RW_U_RW);

K_MEM_PARTITION_DEFINE(mem_domain_tc3_part8_struct,
		       mem_domain_tc3_part8,
		       sizeof(mem_domain_tc3_part8),
		       K_MEM_PARTITION_P_RW_U_RW);


static struct k_mem_partition *mem_domain_tc3_partition_array[] = {
	&ztest_mem_partition,
	&mem_domain_tc3_part1_struct,
	&mem_domain_tc3_part2_struct,
	&mem_domain_tc3_part3_struct,
	&mem_domain_tc3_part4_struct,
	&mem_domain_tc3_part5_struct,
	&mem_domain_tc3_part6_struct,
	&mem_domain_tc3_part7_struct,
	&mem_domain_tc3_part8_struct
};

static struct k_mem_domain mem_domain_tc3_mem_domain;

static void mem_domain_for_user_tc3(void *max_partitions, void *p2, void *p3)
{
	uintptr_t index;

	set_fault_valid(true);

	/* fault should be hit on the first index itself. */
	for (index = 0U;
	     (index < (uintptr_t)max_partitions) && (index < 8);
	     index++) {
		*(uintptr_t *)mem_domain_tc3_partition_array[index]->start =
			10U;
	}

	zassert_unreachable(ERROR_STR);
	ztest_test_fail();
}

/**
 * @brief Test to check addition of partitions into a mem domain.
 *
 * @details If the access to any of the partitions are denied
 * it will cause failure. The memory domain is not added to
 * the thread and fault occurs when the user tries to access
 * that region.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_domain_add_partitions_invalid(void)
{
	/* Subtract one since the domain is initialized with one partition
	 * already present.
	 */
	uint8_t max_partitions = (uint8_t)arch_mem_domain_max_partitions_get() - 1;
	uint8_t index;

	mem_domain_init();
	k_mem_domain_init(&mem_domain_tc3_mem_domain,
			  1,
			  mem_domain_memory_partition_array);

	for (index = 0U; (index < max_partitions) && (index < 8); index++) {
		k_mem_domain_add_partition(&mem_domain_tc3_mem_domain,
					   mem_domain_tc3_partition_array \
					   [index]);

	}
	/* The next add_thread is done so that the
	 * memory domain for mem_domain_tc3_mem_domain partitions are
	 * initialized. Because the pages/regions will not be configuired for
	 * the partitions in mem_domain_tc3_mem_domain when do a add_partition.
	 */
	k_mem_domain_add_thread(&mem_domain_tc3_mem_domain,
				k_current_get());

	/* configure a different memory domain for the current thread. */
	k_mem_domain_add_thread(&mem_domain_mem_domain,
				k_current_get());

	k_thread_user_mode_enter(mem_domain_for_user_tc3,
				 (void *)(uintptr_t)max_partitions,
				 NULL,
				 NULL);

}

/****************************************************************************/
static void mem_domain_for_user_tc4(void *max_partitions, void *p2, void *p3)
{
	uintptr_t index;

	set_fault_valid(false);

	for (index = 0U; (index < (uintptr_t)p2) && (index < 8); index++) {
		*(uintptr_t *)mem_domain_tc3_partition_array[index]->start =
			10U;
	}

	ztest_test_pass();
}

/**
 * @brief Test case to check addition of parititions into a mem domain.
 *
 * @details If the access to any of the partitions are denied
 * it will cause failure.
 *
 * @see k_mem_domain_init(), k_mem_domain_add_partition(),
 * k_mem_domain_add_thread(), k_thread_user_mode_enter()
 */
void test_mem_domain_add_partitions_simple(void)
{

	uint8_t max_partitions = (uint8_t)arch_mem_domain_max_partitions_get();
	uint8_t index;

	k_mem_domain_init(&mem_domain_tc3_mem_domain,
			  1,
			  mem_domain_tc3_partition_array);

	/* Add additional partitions up to the max permitted number. */
	for (index = 1U; (index < max_partitions) && (index < 8); index++) {
		k_mem_domain_add_partition(&mem_domain_tc3_mem_domain,
					   mem_domain_tc3_partition_array \
					   [index]);

	}

	k_mem_domain_add_thread(&mem_domain_tc3_mem_domain,
				k_current_get());

	k_thread_user_mode_enter(mem_domain_for_user_tc4,
				 (void *)(uintptr_t)max_partitions,
				 NULL,
				 NULL);

}

/****************************************************************************/
/* Test removal of a partition. */
static void mem_domain_for_user_tc5(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	/* will generate a fault */
	mem_domain_tc3_part1[0] = 10U;
	zassert_unreachable(ERROR_STR);
}
/**
 * @brief Test the removal of the partition
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_remove_partition(),
 * k_mem_domain_add_thread(), k_thread_user_mode_enter()
 */
void test_mem_domain_remove_partitions_simple(void)
{
	k_mem_domain_add_thread(&mem_domain_tc3_mem_domain,
				k_current_get());

	k_mem_domain_remove_partition(&mem_domain_tc3_mem_domain,
				      &mem_domain_tc3_part1_struct);


	k_thread_user_mode_enter(mem_domain_for_user_tc5,
				 NULL, NULL, NULL);

}

/****************************************************************************/
static void mem_domain_test_6_1(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	mem_domain_tc3_part1[0] = 10U;
	k_thread_abort(k_current_get());
}

static void mem_domain_test_6_2(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	mem_domain_tc3_part1[0] = 10U;
	zassert_unreachable(ERROR_STR);
}

/**
 * @brief Test the removal of partitions with inheritance check
 *
 * @details First check if the memory domain is inherited.
 * After that remove a partition then again check access to it.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_remove_partition()
 */
void test_mem_domain_remove_partitions(void)
{
	uint8_t max_partitions = (uint8_t)arch_mem_domain_max_partitions_get();

	/* Re-initialize domain with the max permitted number of partitions */
	k_mem_domain_init(&mem_domain_tc3_mem_domain,
			  MIN(max_partitions,
			    ARRAY_SIZE(mem_domain_tc3_partition_array)),
			  mem_domain_tc3_partition_array);

	k_mem_domain_add_thread(&mem_domain_tc3_mem_domain,
				k_current_get());

	mem_domain_tc3_part1[0] = 10U;
	mem_domain_tc3_part2[0] = 10U;

	k_thread_create(&mem_domain_6_tid,
			mem_domain_6_stack,
			MEM_DOMAIN_STACK_SIZE,
			mem_domain_test_6_1,
			NULL, NULL, NULL,
			-1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(&mem_domain_6_tid, K_FOREVER);

	k_mem_domain_remove_partition(&mem_domain_tc3_mem_domain,
				      &mem_domain_tc3_part1_struct);

	k_thread_create(&mem_domain_6_tid,
			mem_domain_6_stack,
			MEM_DOMAIN_STACK_SIZE,
			mem_domain_test_6_2,
			NULL, NULL, NULL,
			-1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(&mem_domain_6_tid, K_FOREVER);
}

/**
 * @brief Test access memory domain APIs allowed to supervisor threads only
 *
 * @details
 * - Create a test case to use a memory domain APIs k_mem_domain_init()
 *   and k_mem_domain_add_partition()
 * - Run that test in kernel mode -no errors should happen.
 * - Run that test in user mode -Zephyr fatal error will happen (reason 0),
 *   because user threads can't use memory domain APIs.
 * - At the same time test that system support the definition of memory domains.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_init(), k_mem_domain_add_partition()
 */
void test_mem_domain_api_kernel_thread_only(void)
{
	set_fault_valid(true);

	k_mem_domain_init(&domain0, 0, NULL);
	k_mem_domain_add_partition(&domain0, &part0);
}

void user_handler_func(void *p1, void *p2, void *p3)
{
	/* Read the partition */
	uint8_t read_data = buf1[0];

	/* Just to avoid compiler warning */
	(void) read_data;

	/* Writing to the partition, this should generate fault
	 * as the partition has read only permission for
	 * user threads
	 */
	buf1[0] = 10U;
}

/**
 * @brief Test system auto determine memory partition base addresses and sizes
 *
 * @details
 * - Ensure that system automatically determine memory partition
 *   base address and size according to its content correctly by taking
 *   memory partition's size and compare it with size of content.
 * - If size of memory partition is equal to the size of content -test passes.
 * - If possible to take base address of the memory partition -test passes.
 * - Test that system supports the definition of memory partitions and it has
 *   a starting address and a size.
 * - Test that OS supports removing thread from its memory domain assignment.
 * - Test that user thread can't write into memory partition. Try to write data,
 *   that will lead to the fatal error.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_part_auto_determ_size(void)
{
	set_fault_valid(true);

	k_sem_init(&sync_sem_md, SYNC_SEM_INIT_COUNT, SYNC_SEM_MAX_COUNT);
	k_thread_access_grant(&user_thread0, &sync_sem_md);

	/* check that size of memory partition is correct */
	zassert_true(part1.size == MEM_REGION_ALLOC, "Size of memory partition"
			" determined not correct according to its content");
	/* check base address of memory partition determined at build time */
	zassert_true(part1.start, "Base address of memory partition"
			" not determined at build time");

	k_mem_domain_init(&domain1, ARRAY_SIZE(app1_parts), app1_parts);

	/* That line below is a replacament for the obsolete API call
	 * k_mem_domain_remove_thread(k_current_get())
	 * Test possibility to remove thread from its domain memory assignment,
	 * so the current thread assigned to the domain1 will be removed
	 */
	k_mem_domain_add_thread(&domain0, k_current_get());

	usr_tid0 = k_thread_create(&user_thread0, user_thread0_stack,
					K_THREAD_STACK_SIZEOF(
						user_thread0_stack),
					user_handler_func,
					NULL, NULL, NULL,
					PRIORITY_MD,
					K_USER, K_NO_WAIT);

	/* Add user thread to the memory domain, and try to write to its
	 * partition. That will generate fatal error, because user thread not
	 * allowed to write*/
	k_mem_domain_add_thread(&domain1, usr_tid0);
}

/**
 * @brief Test partitions sized per the constraints of the MPU hardware
 *
 * @details
 * - Test that system automatically determine memory partition size according
 *   to the constraints of the platform's MPU hardware.
 * - Different platforms like x86, ARM and ARC have different MPU hardware for
 *   memory management.
 * - That test checks that MPU hardware works as expected and gives for the
 *   memory partition the most suitable and possible size.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_part_auto_determ_size_per_mpu(void)
{
	zassert_true(part_arch.size == MEM_REGION_ALLOC, NULL);
}

void child_thr_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
}

void parent_thr_handler(void *p1, void *p2, void *p3)
{
	k_tid_t child_tid = k_thread_create(&child_thr_md, child_thr_stack_md,
					K_THREAD_STACK_SIZEOF(child_thr_stack_md),
					child_thr_handler,
					NULL, NULL, NULL,
					PRIORITY_MD, 0, K_NO_WAIT);
	struct k_thread *thread = child_tid;

	zassert_true(thread->mem_domain_info.mem_domain == &domain4, NULL);
	k_sem_give(&sync_sem_md);
}

/**
 * @brief Test memory partition is inherited by the child thread
 *
 * @details
 * - Define memory partition and add it to the parent thread.
 * - Parent thread creates a child thread. That child thread should be already
 *   added to the parent's memory domain too.
 * - It will be checked by using thread->mem_domain_info.mem_domain
 * - If that child thread structure is equal to the current
 *   memory domain address, then pass the test. It means child thread inherited
 *   the memory domain assignment of their parent.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_add_thread(), k_mem_remove_thread()
 */
void test_mem_part_inherit_by_child_thr(void)
{
	k_sem_reset(&sync_sem_md);
	k_mem_domain_init(&domain4, 0, NULL);
	k_mem_domain_add_partition(&domain4, &part0);
	/* that line below is a replacament for
	 * k_mem_domain_remove_thread(k_current_get())
	 */
	k_mem_domain_add_thread(&domain0, k_current_get());

	k_tid_t parent_tid = k_thread_create(&parent_thr_md, parent_thr_stack_md,
					K_THREAD_STACK_SIZEOF(parent_thr_stack_md),
					parent_thr_handler,
					NULL, NULL, NULL,
					PRIORITY_MD, 0, K_NO_WAIT);
	k_mem_domain_add_thread(&domain4, parent_tid);
	k_sem_take(&sync_sem_md, K_FOREVER);
}

/**
 * @brief Test system provide means to obtain names of the data and BSS sections
 *
 * @details
 * - Define memory partition and define system memory pool using macros
 *   SYS_MEM_POOL_DEFINE
 * - Section name of the destination binary section for pool data will be
 *   obtained at build time by macros K_APP_DMEM_SECTION() which obtaines
 *   a section name.
 * - Then to check that system memory pool initialized correctly by allocating
 *   a block from it and check that it is not NULL.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see K_APP_DMEM_SECTION()
 */
void test_macros_obtain_names_data_bss(void)
{
	sys_mem_pool_init(&data_pool);
	void *block;

	block = sys_mem_pool_alloc(&data_pool, BLK_SIZE_MAX_MD - DESC_SIZE);
	zassert_not_null(block, NULL);
}

/**
 * @brief Test assigning global data and BSS variables to memory partitions
 *
 * @details Test that system supports application assigning global data and BSS
 * variables using macros K_APP_BMEM() and K_APP_DMEM
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_part_assign_bss_vars_zero(void)
{
	uint32_t read_data;
	/* The global variable part2_var will be inside the bounds of part2
	 * and be initialized with 1356 at boot.
	 */
	read_data = part2_var;
	zassert_true(read_data == 1356, NULL);

	/* The global variable part2_zeroed_var will be inside the bounds of
	 * part2 and must be zeroed at boot size K_APP_BMEM() was used,
	 * indicating a BSS variable.
	 */
	read_data = part2_zeroed_var;
	zassert_true(read_data == 0, NULL);

	/* The global variable part2_var will be inside the bounds of
	 * part2 and must be zeroed at boot size K_APP_BMEM() was used,
	 * indicating a BSS variable.
	 */
	read_data = part2_bss_var;
	zassert_true(read_data == 0, NULL);
}
