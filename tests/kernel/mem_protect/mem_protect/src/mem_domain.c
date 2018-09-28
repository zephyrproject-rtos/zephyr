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
__kernel struct k_thread mem_domain_1_tid, mem_domain_2_tid, mem_domain_6_tid;

/****************************************************************************/
/* The mem domains needed.*/
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_buf[MEM_REGION_ALLOC];
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_buf1[MEM_REGION_ALLOC];

/* partitions added later in the test cases.*/
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part1[MEM_REGION_ALLOC];
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part2[MEM_REGION_ALLOC];
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part3[MEM_REGION_ALLOC];
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part4[MEM_REGION_ALLOC];
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part5[MEM_REGION_ALLOC];
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part6[MEM_REGION_ALLOC];
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part7[MEM_REGION_ALLOC];
__kernel u8_t MEM_DOMAIN_ALIGNMENT mem_domain_tc3_part8[MEM_REGION_ALLOC];

K_MEM_PARTITION_DEFINE(mem_domain_memory_partition,
		       mem_domain_buf,
		       sizeof(mem_domain_buf),
		       K_MEM_PARTITION_P_RW_U_RW);

#ifdef CONFIG_X86
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
	&mem_domain_memory_partition
};

struct k_mem_partition *mem_domain_memory_partition_array1[] = {
	&mem_domain_memory_partition1
};
__kernel struct k_mem_domain mem_domain_mem_domain;
__kernel struct k_mem_domain mem_domain1;

/****************************************************************************/
/* Common init functions */
static inline void mem_domain_init(void)
{
	k_mem_domain_init(&mem_domain_mem_domain,
			  MEM_PARTITION_INIT_NUM,
			  mem_domain_memory_partition_array);
}

static inline void set_valid_fault_value(int test_case_number)
{
	switch (test_case_number) {
	case 1:
		valid_fault = false;
		break;
	case 2:
		valid_fault = true;
		break;
	default:
		valid_fault = false;
		break;
	}
	USERSPACE_BARRIER;
}

/****************************************************************************/
/* Userspace function */
void mem_domain_for_user(void *tc_number, void *p2, void *p3)
{
	set_valid_fault_value((u32_t)tc_number);

	mem_domain_buf[0] = 10;
	if (valid_fault == false) {
		ztest_test_pass();
	} else {
		ztest_test_fail();
	}
}


void mem_domain_test_1(void *tc_number, void *p2, void *p3)
{
	if ((u32_t)tc_number == 1) {
		mem_domain_buf[0] = 10;
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
void test_mem_domain_valid_access(void *p1, void *p2, void *p3)
{
	u32_t tc_number = 1;

	mem_domain_init();

	k_thread_create(&mem_domain_1_tid,
			mem_domain_1_stack,
			MEM_DOMAIN_STACK_SIZE,
			mem_domain_test_1,
			(void *)tc_number, NULL, NULL,
			10, 0, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

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
void test_mem_domain_invalid_access(void *p1, void *p2, void *p3)
{
	u32_t tc_number = 2;

	k_thread_create(&mem_domain_2_tid,
			mem_domain_2_stack,
			MEM_DOMAIN_STACK_SIZE,
			mem_domain_test_1,
			(void *)tc_number, NULL, NULL,
			10, 0, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);
}
/***************************************************************************/
static void thread_entry_rw(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	u8_t read_data = mem_domain_buf[0];

	/* Just to avoid compiler warnings */
	(void) read_data;

	/* Write to the partition */
	mem_domain_buf[0] = 99;

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
			  MEM_PARTITION_INIT_NUM,
			  mem_domain_memory_partition_array);

	k_mem_domain_add_thread(&mem_domain_mem_domain,
				k_current_get());

	k_thread_user_mode_enter(thread_entry_rw, NULL, NULL, NULL);
}
/****************************************************************************/
static void user_thread_entry_ro(void *p1, void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	/* Read the partition */
	u8_t read_data = mem_domain_buf1[0];

	/* Just to avoid compiler warning */
	(void) read_data;

	/* Writing to the partition, this should generate fault
	 * as the partition has read only permission for
	 * user threads
	 */
	mem_domain_buf1[0] = 10;

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
			  MEM_PARTITION_INIT_NUM,
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
			  MEM_PARTITION_INIT_NUM,
			  mem_domain_memory_partition_array1);

	k_mem_domain_add_thread(&mem_domain_mem_domain, k_current_get());

	/* Create a supervisor thread */
	k_thread_create(&mem_domain_1_tid, mem_domain_1_stack,
			MEM_DOMAIN_STACK_SIZE, (k_thread_entry_t)thread_entry_rw,
			NULL, NULL, NULL, 10, K_INHERIT_PERMS, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);
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


struct k_mem_partition *mem_domain_tc3_partition_array[] = {
	&mem_domain_tc3_part1_struct,
	&mem_domain_tc3_part2_struct,
	&mem_domain_tc3_part3_struct,
	&mem_domain_tc3_part4_struct,
	&mem_domain_tc3_part5_struct,
	&mem_domain_tc3_part6_struct,
	&mem_domain_tc3_part7_struct,
	&mem_domain_tc3_part8_struct
};

__kernel struct k_mem_domain mem_domain_tc3_mem_domain;

void mem_domain_for_user_tc3(void *max_partitions, void *p2, void *p3)
{
	u32_t index;

	valid_fault = true;
	USERSPACE_BARRIER;

	/* fault should be hit on the first index itself. */
	for (index = 0;
	     (index < (u32_t)max_partitions) && (index < 8);
	     index++) {
		*(u32_t *)mem_domain_tc3_partition_array[index]->start = 10;
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
void test_mem_domain_add_partitions_invalid(void *p1, void *p2, void *p3)
{
	/* Subtract one since the domain is initialized with one partition
	 * already present.
	 */
	u8_t max_partitions = (u8_t)_arch_mem_domain_max_partitions_get() - 1;
	u8_t index;

	mem_domain_init();
	k_mem_domain_init(&mem_domain_tc3_mem_domain,
			  1,
			  mem_domain_memory_partition_array);

	for (index = 0; (index < max_partitions) && (index < 8); index++) {
		k_mem_domain_add_partition(&mem_domain_tc3_mem_domain,
					   mem_domain_tc3_partition_array \
					   [index]);

	}
	/* The next add_thread and remove_thread is done so that the
	 * memory domain for mem_domain_tc3_mem_domain partitions are
	 * initialized. Because the pages/regions will not be configuired for
	 * the partitions in mem_domain_tc3_mem_domain when do a add_partition.
	 */
	k_mem_domain_add_thread(&mem_domain_tc3_mem_domain,
				k_current_get());

	k_mem_domain_remove_thread(k_current_get());

	/* configure a different memory domain for the current thread. */
	k_mem_domain_add_thread(&mem_domain_mem_domain,
				k_current_get());

	k_thread_user_mode_enter(mem_domain_for_user_tc3,
				 (void *)(u32_t)max_partitions,
				 NULL,
				 NULL);

}

/****************************************************************************/
void mem_domain_for_user_tc4(void *max_partitions, void *p2, void *p3)
{
	u32_t index;

	valid_fault = false;
	USERSPACE_BARRIER;

	for (index = 0; (index < (u32_t)p2) && (index < 8); index++) {
		*(u32_t *)mem_domain_tc3_partition_array[index]->start = 10;
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
void test_mem_domain_add_partitions_simple(void *p1, void *p2, void *p3)
{

	u8_t max_partitions = (u8_t)_arch_mem_domain_max_partitions_get();
	u8_t index;

	k_mem_domain_init(&mem_domain_tc3_mem_domain,
			  1,
			  mem_domain_tc3_partition_array);

	for (index = 1; (index < max_partitions) && (index < 8); index++) {
		k_mem_domain_add_partition(&mem_domain_tc3_mem_domain,
					   mem_domain_tc3_partition_array \
					   [index]);

	}

	k_mem_domain_add_thread(&mem_domain_tc3_mem_domain,
				k_current_get());

	k_thread_user_mode_enter(mem_domain_for_user_tc4,
				 (void *)(u32_t)max_partitions,
				 NULL,
				 NULL);

}

/****************************************************************************/
/* Test removal of a partition. */
void mem_domain_for_user_tc5(void *p1, void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	/* will generate a fault */
	mem_domain_tc3_part1[0] = 10;
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
void test_mem_domain_remove_partitions_simple(void *p1, void *p2, void *p3)
{
	k_mem_domain_add_thread(&mem_domain_tc3_mem_domain,
				k_current_get());

	k_mem_domain_remove_partition(&mem_domain_tc3_mem_domain,
				      &mem_domain_tc3_part1_struct);


	k_thread_user_mode_enter(mem_domain_for_user_tc5,
				 NULL, NULL, NULL);

}

/****************************************************************************/
void mem_domain_test_6_1(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	mem_domain_tc3_part2[0] = 10;
	k_thread_abort(k_current_get());
}

void mem_domain_test_6_2(void *p1, void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	mem_domain_tc3_part2[0] = 10;
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
void test_mem_domain_remove_partitions(void *p1, void *p2, void *p3)
{

	k_mem_domain_add_thread(&mem_domain_tc3_mem_domain,
				k_current_get());

	mem_domain_tc3_part2[0] = 10;

	k_thread_create(&mem_domain_6_tid,
			mem_domain_6_stack,
			MEM_DOMAIN_STACK_SIZE,
			mem_domain_test_6_1,
			NULL, NULL, NULL,
			10, K_USER | K_INHERIT_PERMS, K_NO_WAIT);


	k_sem_take(&sync_sem, MSEC(100));

	k_mem_domain_remove_partition(&mem_domain_tc3_mem_domain,
				      &mem_domain_tc3_part2_struct);

	k_thread_create(&mem_domain_6_tid,
			mem_domain_6_stack,
			MEM_DOMAIN_STACK_SIZE,
			mem_domain_test_6_2,
			NULL, NULL, NULL,
			10, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);

}
/****************************************************************************/

void mem_domain_for_user_tc7(void *p1, void *p2, void *p3)
{
	valid_fault = true;
	USERSPACE_BARRIER;

	/* will generate a fault */
	mem_domain_tc3_part4[0] = 10;
	zassert_unreachable(ERROR_STR);
}

/**
 * @brief Test removal of a thread from the memory domain.
 *
 * @details Till now all the test suite would have tested add thread.
 * this ensures that remove is working correctly.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_remove_thread()
 */
void test_mem_domain_remove_thread(void *p1, void *p2, void *p3)
{

	k_mem_domain_add_thread(&mem_domain_tc3_mem_domain,
				k_current_get());


	k_mem_domain_remove_thread(k_current_get());


	k_thread_user_mode_enter(mem_domain_for_user_tc7,
				 NULL, NULL, NULL);

}
/****************************************************************************/
/**
 * @brief Test k_mem_domain_destroy API
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_add_thread(), k_mem_domain_destroy()
 * */
void test_mem_domain_destroy(void)
{
	k_mem_domain_init(&mem_domain1,
			  MEM_PARTITION_INIT_NUM,
			  mem_domain_memory_partition_array1);

	k_mem_domain_add_thread(&mem_domain1, k_current_get());

	k_tid_t tid = k_current_get();

	if (tid->mem_domain_info.mem_domain == &mem_domain1) {
		k_mem_domain_destroy(&mem_domain1);

		zassert_true(tid->mem_domain_info.mem_domain !=
			     &mem_domain1, "The thread has reference to"
			     " memory domain which is already destroyed");
	} else {
		zassert_unreachable("k_mem_domain_add_thread() failed");
	}
}
