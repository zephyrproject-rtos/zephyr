/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem_protect.h"

#define ERROR_STR \
	("Fault didn't occur when we accessed a unassigned memory domain.")

/****************************************************************************/
K_THREAD_STACK_DEFINE(mem_domain_1_stack, MEM_DOMAIN_STACK_SIZE);
K_THREAD_STACK_DEFINE(mem_domain_2_stack, MEM_DOMAIN_STACK_SIZE);
K_THREAD_STACK_DEFINE(mem_domain_6_stack, MEM_DOMAIN_STACK_SIZE);
struct k_thread mem_domain_1_tid, mem_domain_2_tid, mem_domain_6_tid;

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
