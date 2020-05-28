/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem_protect.h"
#include <syscall_handler.h>

/* Kernel objects */

K_THREAD_STACK_DEFINE(child_stack, KOBJECT_STACK_SIZE);
K_THREAD_STACK_DEFINE(extra_stack, KOBJECT_STACK_SIZE);

K_SEM_DEFINE(kobject_sem, SEMAPHORE_INIT_COUNT, SEMAPHORE_MAX_COUNT);
K_SEM_DEFINE(kobject_public_sem, SEMAPHORE_INIT_COUNT, SEMAPHORE_MAX_COUNT);
K_MUTEX_DEFINE(kobject_mutex);

struct k_thread child_thread;
struct k_thread extra_thread;

struct k_sem *random_sem_type;
struct k_sem kobject_sem_not_hash_table;
struct k_sem kobject_sem_no_init_no_access;
struct k_sem kobject_sem_no_init_access;

/****************************************************************************/
static void kobject_access_grant_user_part(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);
	k_sem_take(random_sem_type, K_FOREVER);
}

/**
 * @brief Test access to a invalid semaphore who's address is NULL
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant(), k_thread_user_mode_enter()
 */
void test_kobject_access_grant(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	z_object_init(random_sem_type);
	k_thread_access_grant(k_current_get(),
			      &kobject_sem,
			      &kobject_mutex,
			      random_sem_type);

	k_thread_user_mode_enter(kobject_access_grant_user_part,
				 NULL, NULL, NULL);

}
/****************************************************************************/
static void syscall_invalid_kobject_user_part(void *p1, void *p2, void *p3)
{
	k_sem_give(&kobject_sem);

	/* should causdde a fault */
	set_fault_valid(true);

	/* should cause fault. typecasting to override compiler warning */
	k_sem_take((struct k_sem *)&kobject_mutex, K_FOREVER);
}

/**
 * @brief Test syscall can take a different type of kobject
 *
 * @details Test syscall can take a different type of kobject and syscall will
 * generate fatal error if check fails.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant()
 */
void test_syscall_invalid_kobject(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(k_current_get(),
			      &kobject_sem,
			      &kobject_mutex);

	k_thread_user_mode_enter(syscall_invalid_kobject_user_part,
				 NULL, NULL, NULL);
}

/****************************************************************************/
static void thread_without_kobject_permission_user_part(void *p1, void *p2,
							void *p3)
{
	/* should cause a fault */
	set_fault_valid(true);
	k_sem_give(&kobject_sem);
}

/**
 * @brief Test user thread can access a k_object without grant
 *
 * @details The kernel will fail system call on kernel object that tracks thread
 * permissions, on thread that don't have permission granted on the object.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant(), k_thread_user_mode_enter()
 */
void test_thread_without_kobject_permission(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(k_current_get(),
			      &kobject_mutex);

	k_thread_user_mode_enter(thread_without_kobject_permission_user_part,
				 NULL, NULL, NULL);

}

/****************************************************************************/
static void kobject_revoke_access_user_part(void *p1, void *p2, void *p3)
{
	/* should cause a fault */
	if ((uintptr_t)p1 == 1U) {
		set_fault_valid(false);
	} else {
		set_fault_valid(true);
	}
	k_sem_give(&kobject_sem);
}

/**
 * @brief Test access revoke
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant(), k_object_access_revoke()
 */
void test_kobject_revoke_access(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(k_current_get(),
			      &kobject_sem);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			kobject_revoke_access_user_part,
			(void *)1, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);


	k_thread_join(&child_thread, K_FOREVER);
	k_object_access_revoke(&kobject_sem, k_current_get());

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			kobject_revoke_access_user_part,
			(void *)2, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);
	k_thread_join(&child_thread, K_FOREVER);
}

/****************************************************************************/
/* grant access to all user threads that follow */
static void kobject_grant_access_child_entry(void *p1, void *p2, void *p3)
{
	k_sem_give(&kobject_sem);
	k_object_access_grant(&kobject_sem, &extra_thread);
}

static void kobject_grant_access_extra_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&kobject_sem, K_FOREVER);
}

/**
 * @brief Test grant access
 *
 * @details Will grant access to another thread for the
 * semaphore it holds.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant()
 */
void test_kobject_grant_access_kobj(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(k_current_get(),
			      &kobject_sem, &extra_thread);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			kobject_grant_access_child_entry,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);

	k_thread_create(&extra_thread,
			extra_stack,
			KOBJECT_STACK_SIZE,
			kobject_grant_access_extra_entry,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);
	k_thread_join(&extra_thread, K_FOREVER);
}

/****************************************************************************/
static void grant_access_kobj_invalid_child(void *p1, void *p2, void *p3)
{
	k_sem_give(&kobject_sem);

	set_fault_valid(true);

	k_object_access_grant(&kobject_sem, &extra_thread);
	zassert_unreachable("k_object validation  failure");
}

/**
 * @brief Test access grant between threads
 *
 * @details Test access grant to thread B from thread A which doesn't have
 * required permissions.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant()
 */
void test_kobject_grant_access_kobj_invalid(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread, &kobject_sem);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			grant_access_kobj_invalid_child,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/****************************************************************************/
static void release_from_user_child(void *p1, void *p2, void *p3)
{
	k_sem_give(&kobject_sem);
	k_object_release(&kobject_sem);

	set_fault_valid(true);

	k_sem_give(&kobject_sem);
}

/**
 * @brief Test revoke permission of a k_object from userspace
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant(), k_object_release()
 */
void test_kobject_release_from_user(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(k_current_get(),
			      &kobject_sem);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			release_from_user_child,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}
/****************************************************************************/
static void access_all_grant_child_give(void *p1, void *p2, void *p3)
{
	k_sem_give(&kobject_public_sem);
}

static void access_all_grant_child_take(void *p1, void *p2, void *p3)
{
	k_sem_take(&kobject_public_sem, K_FOREVER);
}

/**
 * @brief Test supervisor thread grants kernel objects all access public status
 *
 * @details System makes kernel object kobject_public_sem public to all threads
 * Test the access to that kernel object by creating two new user threads.
 *
 * @see k_object_access_all_grant()
 *
 * @ingroup kernel_memprotect_tests
 */
void test_kobject_access_all_grant(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_object_access_all_grant(&kobject_public_sem);
	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			access_all_grant_child_give,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);
	k_thread_join(&child_thread, K_FOREVER);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			access_all_grant_child_take,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}
/****************************************************************************/

static void residual_permissions_child_success(void *p1, void *p2, void *p3)
{
	k_sem_give(&kobject_sem);
}

static void residual_permissions_child_fail(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_sem_take(&kobject_sem, K_FOREVER);
	zassert_unreachable("Failed to clear permission on a deleted thread");
}

/**
 * @brief Test access permission of a terminated thread
 *
 * @details If a deleted thread with some permissions is
 * recreated with the same tid, check if it still has the
 * permissions.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant()
 */
void test_thread_has_residual_permissions(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(k_current_get(),
			      &kobject_sem);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			residual_permissions_child_success,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			residual_permissions_child_fail,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/****************************************************************************/
/**
 * @brief Test grant access to a valid kobject but invalid thread id
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_object_access_grant(), k_object_access_revoke(),
 * z_object_find()
 */
void test_kobject_access_grant_to_invalid_thread(void *p1, void *p2, void *p3)
{
	static struct k_thread uninit_thread;

	set_fault_valid(false);

	k_object_access_grant(&kobject_sem, &uninit_thread);
	k_object_access_revoke(&kobject_sem, &uninit_thread);

	zassert_not_equal(Z_SYSCALL_OBJ(&uninit_thread, K_OBJ_THREAD), 0,
			  "Access granted/revoked to invalid thread k_object");
}

/****************************************************************************/
/**
 * @brief Object validation checks
 *
 * @details Test syscall on a kobject which is not present in the hash table.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_kobject_access_invalid_kobject(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_sem_take(&kobject_sem_not_hash_table, K_SECONDS(1));
	zassert_unreachable("k_object validation  failure.");

}

/****************************************************************************/
/**
 * @brief Object validation checks without init access
 *
 * @details Test syscall on a kobject which is not initialized
 * and has no access
 *
 * @ingroup kernel_memprotect_tests
 */
void test_access_kobject_without_init_access(void *p1,
					     void *p2, void *p3)
{
	set_fault_valid(true);

	k_sem_take(&kobject_sem_no_init_no_access, K_SECONDS(1));
	zassert_unreachable("k_object validation  failure");

}
/****************************************************************************/
/* object validation checks */
static void without_init_with_access_child(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_sem_take(&kobject_sem_no_init_access, K_SECONDS(1));
	zassert_unreachable("_SYSCALL_OBJ implementation failure.");
}

/**
 * @brief Test syscall on a kobject which is not initialized and has access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant()
 */
void test_access_kobject_without_init_with_access(void *p1,
						  void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(k_current_get(),
			      &kobject_sem_no_init_access);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			without_init_with_access_child,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/****************************************************************************/
/* object validation checks */
static void reinitialize_thread_kobj_extra(void *p1, void *p2, void *p3)
{
	zassert_unreachable("_SYSCALL_OBJ implementation failure.");
}

static void reinitialize_thread_kobj_child(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_thread_create(&extra_thread,
			extra_stack,
			KOBJECT_STACK_SIZE,
			reinitialize_thread_kobj_extra,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	zassert_unreachable("_SYSCALL_OBJ implementation failure.");

}
/**
 * @brief Test to reinitialize the k_thread object
 *
 * @ingroup kernel_memprotect_tests
 */
void test_kobject_reinitialize_thread_kobj(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			reinitialize_thread_kobj_child,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/****************************************************************************/
/* object validation checks */
static void new_thread_from_user_extra(void *p1, void *p2, void *p3)
{
	k_thread_abort(&extra_thread);
}

static void new_thread_from_user_child(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);
	k_thread_create(&extra_thread,
			extra_stack,
			KOBJECT_STACK_SIZE,
			new_thread_from_user_extra,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/**
 * @brief Test thread create from a user thread and check permissions
 *
 * @details - Test user thread can create new thread.
 * - Verify that given thread and thread stack permissions to the user thread,
 *   allow to create new user thread.
 * - Veify that new created user thread have access to its own thread object
 *   by aborting itself.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_create_new_thread_from_user(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread,
			      &extra_thread,
			      &extra_stack);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			new_thread_from_user_child,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/* Additional functions for test below
 * User thread create with in-use stack objects
 */
static void new_thrd_from_user_with_in_use_stack(void *p1, void *p2, void *p3)
{
	zassert_unreachable("New user thread init with in-use stack obj");
}

static void new_user_thrd_child_with_in_use_stack(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_thread_create(&extra_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			new_thrd_from_user_with_in_use_stack,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/**
 * @brief Test create new user thread from a user thread with in-use stack obj
 *
 * @details The kernel must prevent new user threads to use initiliazed (in-use)
 * stack objects. In that case extra_thread is going to be create with in-use
 * stack object child_stack. That will generate error, showing that kernel
 * memory protection is working correctly.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_new_user_thread_with_in_use_stack_obj(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread,
			      &extra_thread,
			      &extra_stack,
			      &child_stack);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			new_user_thrd_child_with_in_use_stack,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

static void from_user_no_access_stack_extra_entry(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

static void from_user_no_access_stack_child_entry(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_thread_create(&extra_thread,
			extra_stack,
			KOBJECT_STACK_SIZE,
			from_user_no_access_stack_extra_entry,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);
}
/**
 * @brief Test creates new thread from usermode without stack access
 *
 * @details Create a new thread from user and the user doesn't have access
 * to the stack region of new thread.
 * _handler_k_thread_create validation.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_create_new_thread_from_user_no_access_stack(void *p1,
						      void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread,
			      &extra_thread);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			from_user_no_access_stack_child_entry,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);
	k_thread_join(&child_thread, K_FOREVER);
}

/****************************************************************************/
/* object validation checks */
#ifndef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
static void from_user_invalid_stacksize_extra(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

static void from_user_invalid_stacksize_child(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_thread_create(&extra_thread,
			extra_stack,
			-1,
			from_user_invalid_stacksize_extra,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);
	zassert_unreachable("k_object validation failure in k thread create");
}
/**
 * @brief Test to validate user thread spawning with stack overflow
 *
 * @details Create a new thread from user and use a huge stack
 * size which overflows. This is _handler_k_thread_create validation.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_create_new_thread_from_user_invalid_stacksize(void *p1,
							void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread,
			      &extra_thread,
			      &child_stack);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			from_user_invalid_stacksize_child,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);
	k_thread_join(&child_thread, K_FOREVER);
}
#else
void test_create_new_thread_from_user_invalid_stacksize(void *p1,
							void *p2, void *p3)
{
	ztest_test_skip();
}
#endif

/****************************************************************************/
/* object validation checks */
#ifndef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
static void user_huge_stacksize_extra(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

static void user_huge_stacksize_child(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_thread_create(&extra_thread,
			extra_stack,
			K_THREAD_STACK_SIZEOF(extra_stack) + 1,
			user_huge_stacksize_extra,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/**
 * @brief Test to check stack overflow from user thread
 *
 * @details Create a new thread from user and use a stack
 * bigger than allowed size. This is_handler_k_thread_create
 * validation.
 *
 * @ingroup kernel_memprotect_tests
 */

void test_create_new_thread_from_user_huge_stacksize(void *p1,
						     void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread,
			      &extra_thread,
			      &extra_stack);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			user_huge_stacksize_child,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}
#else
void test_create_new_thread_from_user_huge_stacksize(void *p1,
						     void *p2, void *p3)
{
	ztest_test_skip();
}
#endif

/****************************************************************************/
/* object validation checks */

static void supervisor_from_user_extra(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

static void supervisor_from_user_child(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_thread_create(&extra_thread,
			extra_stack,
			KOBJECT_STACK_SIZE,
			supervisor_from_user_extra,
			NULL, NULL, NULL,
			0, 0, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}

/**
 * @brief Test to create a new supervisor thread from user
 *
 * @details The system kernel must prevent user threads from creating supervisor
 * threads.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_create_new_supervisor_thread_from_user(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread,
			      &extra_thread,
			      &extra_stack);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			supervisor_from_user_child,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/****************************************************************************/
/* object validation checks */

static void essential_thread_from_user_extra(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

static void essential_thread_from_user_child(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_thread_create(&extra_thread,
			extra_stack,
			KOBJECT_STACK_SIZE,
			essential_thread_from_user_extra,
			NULL, NULL, NULL,
			0, K_USER | K_ESSENTIAL, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/**
 * @brief Create a new essential thread from user.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_create_new_essential_thread_from_user(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread,
			      &extra_thread,
			      &extra_stack);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			essential_thread_from_user_child,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);
	k_thread_join(&child_thread, K_FOREVER);
}

/****************************************************************************/
/* object validation checks */

static void higher_prio_from_user_extra(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

static void higher_prio_from_user_child(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_thread_create(&extra_thread,
			extra_stack,
			KOBJECT_STACK_SIZE,
			higher_prio_from_user_extra,
			NULL, NULL, NULL,
			-1, K_USER, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/**
 * @brief Thread creation with prority is higher than current thread
 *
 * @details  _handler_k_thread_create validation.
 *
 * @ingroup kernel_memprotect_tests
 */

void test_create_new_higher_prio_thread_from_user(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread,
			      &extra_thread,
			      &extra_stack);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			higher_prio_from_user_child,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/****************************************************************************/
/* object validation checks */

static void invalid_prio_from_user_extra(void *p1, void *p2, void *p3)
{
	zassert_unreachable("k_object validation failure in k thread create");
}

static void invalid_prio_from_user_child(void *p1, void *p2, void *p3)
{
	set_fault_valid(true);

	k_thread_create(&extra_thread,
			extra_stack,
			KOBJECT_STACK_SIZE,
			invalid_prio_from_user_extra,
			NULL, NULL, NULL,
			6000, K_USER, K_NO_WAIT);

	zassert_unreachable("k_object validation failure in k thread create");
}
/**
 * @brief Create a new thread whose prority is invalid.
 *
 * @details _handler_k_thread_create validation.
 *
 * @ingroup kernel_memprotect_tests
 */

void test_create_new_invalid_prio_thread_from_user(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	k_thread_access_grant(&child_thread,
			      &extra_thread,
			      &extra_stack);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			invalid_prio_from_user_child,
			NULL, NULL, NULL,
			0, K_USER, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);
}

/* Function to init thread's stack objects */
static void thread_stack_init_objects(void *p1, void *p2, void *p3)
{
	int ret;
	struct z_object *ko;

	/* check that thread is initialized when running */
	ko = z_object_find(&child_thread);
	ret = z_object_validate(ko, K_OBJ_ANY, _OBJ_INIT_TRUE);
	zassert_equal(ret, _OBJ_INIT_TRUE, NULL);

	/* check that stack is initialized when running */
	ko = z_object_find(child_stack);
	ret = z_object_validate(ko, K_OBJ_ANY, _OBJ_INIT_TRUE);
	zassert_equal(ret, _OBJ_INIT_TRUE, NULL);
}

/**
 * @brief Test when thread exits, kernel marks stack objects uninitialized
 *
 * @details When thread exits, the kernel upon thread exit, should mark
 * the exiting thread and thread stack object as uninitialized
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mark_thread_exit_uninitialized(void)
{
	set_fault_valid(false);

	int ret;
	struct z_object *ko;

	k_thread_access_grant(&child_thread,
			      &child_stack);

	k_thread_create(&child_thread,
			child_stack,
			KOBJECT_STACK_SIZE,
			thread_stack_init_objects,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(&child_thread, K_FOREVER);

	/* check thread is uninitialized after its exit */
	ko = z_object_find(&child_thread);
	ret = z_object_validate(ko, K_OBJ_ANY, _OBJ_INIT_FALSE);
	zassert_equal(ret, _OBJ_INIT_FALSE, NULL);

	/* check stack is uninitialized after thread exit */
	ko = z_object_find(child_stack);
	ret = z_object_validate(ko, K_OBJ_ANY, _OBJ_INIT_FALSE);
	zassert_equal(ret, _OBJ_INIT_FALSE, NULL);
}
