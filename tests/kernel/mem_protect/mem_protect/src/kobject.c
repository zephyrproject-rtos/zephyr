/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem_protect.h"
#include <zephyr/internal/syscall_handler.h>

/* Kernel objects */

K_THREAD_STACK_DECLARE(child_stack, KOBJECT_STACK_SIZE);
K_THREAD_STACK_DEFINE(extra_stack, KOBJECT_STACK_SIZE);

K_SEM_DEFINE(kobject_sem, SEMAPHORE_INIT_COUNT, SEMAPHORE_MAX_COUNT);
K_SEM_DEFINE(kobject_public_sem, SEMAPHORE_INIT_COUNT, SEMAPHORE_MAX_COUNT);
K_MUTEX_DEFINE(kobject_mutex);

extern struct k_thread child_thread;
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
ZTEST(mem_protect_kobj, test_kobject_access_grant)
{
	set_fault_valid(false);

	k_object_init(random_sem_type);
	k_thread_access_grant(k_current_get(),
			      &kobject_sem,
			      &kobject_mutex,
			      random_sem_type);

	k_thread_user_mode_enter(kobject_access_grant_user_part,
				 NULL, NULL, NULL);

}

/**
 * @brief Test grant access of given NULL kobject
 *
 * @details Call function with a NULL parameter in supervisor mode,
 * nothing happened.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant()
 */
ZTEST(mem_protect_kobj, test_kobject_access_grant_error)
{
	k_object_access_grant(NULL, k_current_get());
}

/**
 * @brief Test grant access of given NULL thread in usermode
 *
 * @details Call function with NULL parameter, an expected fault
 * happened.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant()
 */
ZTEST_USER(mem_protect_kobj, test_kobject_access_grant_error_user)
{
	struct k_queue *q;

	/*
	 * avoid using K_OBJ_PIPE, K_OBJ_MSGQ, or K_OBJ_STACK because the
	 * k_object_alloc() returns an uninitialized kernel object and these
	 * objects are types that can have additional memory allocations that
	 * need to be freed. This becomes a problem on the fault handler clean
	 * up because when it is freeing this uninitialized object the random
	 * data in the object can cause the clean up to try to free random
	 * data resulting in a secondary fault that fails the test.
	 */
	q = k_object_alloc(K_OBJ_QUEUE);
	k_object_access_grant(q, k_current_get());

	set_fault_valid(true);
	/* a K_ERR_KERNEL_OOPS expected */
	k_object_access_grant(q, NULL);
}

/**
 * @brief Test grant access of given NULL kobject in usermode
 *
 * @details Call function with a NULL parameter, an expected fault
 * happened.
 *
 * @see k_thread_access_grant()
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(mem_protect_kobj, test_kobject_access_grant_error_user_null)
{
	set_fault_valid(true);
	/* a K_ERR_KERNEL_OOPS expected */
	k_object_access_grant(NULL, k_current_get());
}

/**
 * @brief Test grant access to all the kobject for thread
 *
 * @details Call function with a NULL parameter, an expected fault
 * happened.
 *
 * @see k_thread_access_all_grant()
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(mem_protect_kobj, test_kobject_access_all_grant_error)
{
	set_fault_valid(true);
	/* a K_ERR_KERNEL_OOPS expected */
	k_object_access_all_grant(NULL);
}

/****************************************************************************/
static void syscall_invalid_kobject_user_part(void *p1, void *p2, void *p3)
{
	k_sem_give(&kobject_sem);

	/* should cause a fault */
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
ZTEST(mem_protect_kobj, test_syscall_invalid_kobject)
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
ZTEST(mem_protect_kobj, test_thread_without_kobject_permission)
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
ZTEST(mem_protect_kobj, test_kobject_revoke_access)
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
 * @brief Test access revoke
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_thread_access_grant(), k_object_access_revoke()
 */
ZTEST(mem_protect_kobj, test_kobject_grant_access_kobj)
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
ZTEST(mem_protect_kobj, test_kobject_grant_access_kobj_invalid)
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
ZTEST(mem_protect_kobj, test_kobject_release_from_user)
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

/**
 * @brief Test release and access grant an invalid kobject
 *
 * @details Validate release and access grant an invalid kernel object.
 *
 * @see k_object_release(), k_object_access_all_grant()
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_kobject_invalid)
{
	int dummy = 0;

	k_object_access_all_grant(&dummy);
	k_object_release(&dummy);
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
ZTEST(mem_protect_kobj, test_kobject_access_all_grant)
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
ZTEST(mem_protect_kobj, test_thread_has_residual_permissions)
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
 * k_object_find()
 */
ZTEST(mem_protect_kobj, test_kobject_access_grant_to_invalid_thread)
{
	static struct k_thread uninit_thread;

	set_fault_valid(false);

	k_object_access_grant(&kobject_sem, &uninit_thread);
	k_object_access_revoke(&kobject_sem, &uninit_thread);

	zassert_not_equal(K_SYSCALL_OBJ(&uninit_thread, K_OBJ_THREAD), 0,
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
ZTEST_USER(mem_protect_kobj, test_kobject_access_invalid_kobject)
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
ZTEST_USER(mem_protect_kobj, test_access_kobject_without_init_access)
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
ZTEST(mem_protect_kobj, test_access_kobject_without_init_with_access)
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
ZTEST(mem_protect_kobj, test_kobject_reinitialize_thread_kobj)
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
 * @details
 * - Test user thread can create new thread.
 * - Verify that given thread and thread stack permissions to the user thread,
 *   allow to create new user thread.
 * - Verify that new created user thread have access to its own thread object
 *   by aborting itself.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_create_new_thread_from_user)
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
 * @details The kernel must prevent new user threads to use initialized (in-use)
 * stack objects. In that case extra_thread is going to be create with in-use
 * stack object child_stack. That will generate error, showing that kernel
 * memory protection is working correctly.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_new_user_thread_with_in_use_stack_obj)
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
ZTEST(mem_protect_kobj, test_create_new_thread_from_user_no_access_stack)
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
ZTEST(mem_protect_kobj, test_create_new_thread_from_user_invalid_stacksize)
{
#ifdef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
	ztest_test_skip();
#endif

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

/****************************************************************************/
/* object validation checks */
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

ZTEST(mem_protect_kobj, test_create_new_thread_from_user_huge_stacksize)
{
#ifdef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
	ztest_test_skip();
#endif

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
ZTEST(mem_protect_kobj, test_create_new_supervisor_thread_from_user)
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
ZTEST(mem_protect_kobj, test_create_new_essential_thread_from_user)
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
 * @brief Thread creation with priority is higher than current thread
 *
 * @details  _handler_k_thread_create validation.
 *
 * @ingroup kernel_memprotect_tests
 */

ZTEST(mem_protect_kobj, test_create_new_higher_prio_thread_from_user)
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
 * @brief Create a new thread whose priority is invalid.
 *
 * @details _handler_k_thread_create validation.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_create_new_invalid_prio_thread_from_user)
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
	/* check that thread is initialized when running */
	zassert_true(k_object_is_valid(&child_thread, K_OBJ_ANY));

	/* check that stack is initialized when running */
	zassert_true(k_object_is_valid(child_stack, K_OBJ_ANY));
}

/**
 * @brief Test when thread exits, kernel marks stack objects uninitialized
 *
 * @details When thread exits, the kernel upon thread exit, should mark
 * the exiting thread and thread stack object as uninitialized
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_mark_thread_exit_uninitialized)
{
	set_fault_valid(false);

	int ret;
	struct k_object *ko;

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
	ko = k_object_find(&child_thread);
	ret = k_object_validate(ko, K_OBJ_ANY, _OBJ_INIT_FALSE);
	zassert_equal(ret, _OBJ_INIT_FALSE);

	/* check stack is uninitialized after thread exit */
	ko = k_object_find(child_stack);
	ret = k_object_validate(ko, K_OBJ_ANY, _OBJ_INIT_FALSE);
	zassert_equal(ret, _OBJ_INIT_FALSE);
}

/****************************************************************************/
/* object validation checks */

static void tThread_object_free_error(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* a K_ERR_CPU_EXCEPTION expected */
	set_fault_valid(true);
	k_object_free(NULL);
}

/**
 * @brief Test free an invalid kernel object
 *
 * @details Spawn a thread free a NULL, an expected fault happened.
 *
 * @see k_object_free()
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_kobject_free_error)
{
	uint32_t perm = K_INHERIT_PERMS;

	if (k_is_user_context()) {
		perm = perm | K_USER;
	}

	k_tid_t tid = k_thread_create(&child_thread, child_stack,
			K_THREAD_STACK_SIZEOF(child_stack),
			tThread_object_free_error,
			(void *)&tid, NULL, NULL,
			K_PRIO_PREEMPT(1), perm, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test alloc an invalid kernel object
 *
 * @details Allocate invalid kernel objects, then no allocation
 * will be returned.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_object_alloc()
 */
ZTEST_USER(mem_protect_kobj, test_kobject_init_error)
{
	/* invalid kernel object allocation */
	zassert_is_null(k_object_alloc(K_OBJ_ANY-1),
			"expected got NULL kobject");
	zassert_is_null(k_object_alloc(K_OBJ_LAST),
			"expected got NULL kobject");

	/* futex not support */
	zassert_is_null(k_object_alloc(K_OBJ_FUTEX),
			"expected got NULL kobject");
}

/**
 * @brief Test kernel object until out of memory
 *
 * @details Create a dynamic kernel object repeatedly until run out
 * of all heap memory, an expected out of memory error generated.
 *
 * @see k_object_alloc()
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_kobj_create_out_of_memory)
{
	int ttype;
	int max_obj = 0;
	void *create_obj[MAX_OBJ] = {0};

	for (ttype = K_OBJ_MEM_SLAB; ttype < K_OBJ_CONDVAR ; ttype++) {

		for (int i = 0; i < MAX_OBJ; i++) {
			create_obj[i] = k_object_alloc(ttype);
			max_obj = i;
			if (create_obj[i] == NULL) {
				break;
			}
		}

		zassert_is_null(create_obj[max_obj],
				"excepted alloc failure");
		printk("==max_obj(%d)\n", max_obj);


		for (int i = 0; i < max_obj; i++) {
			k_object_free((void *)create_obj[i]);
		}

	}
}

#ifdef CONFIG_DYNAMIC_OBJECTS
extern uint8_t _thread_idx_map[CONFIG_MAX_THREAD_BYTES];

#define MAX_THREAD_BITS (CONFIG_MAX_THREAD_BYTES * BITS_PER_BYTE)
#endif

/* @brief Test alloc thread object until out of idex
 *
 * @details Allocate thread object until it out of index, no more
 * thread can be allocated and report an error.
 *
 * @see k_object_alloc()
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_thread_alloc_out_of_idx)
{
#ifdef CONFIG_DYNAMIC_OBJECTS
	struct k_thread *thread[MAX_THREAD_BITS];
	struct k_thread *fail_thread;
	int cur_max = 0;

	for (int i = 0; i < MAX_THREAD_BITS; i++) {

		thread[i] = k_object_alloc(K_OBJ_THREAD);

		if (!thread[i]) {
			cur_max = i;
			break;
		}
	}

	/** TESTPOINT: all the idx bits set to 1 */
	for (int i = 0; i < CONFIG_MAX_THREAD_BYTES; i++) {
		int idx = find_lsb_set(_thread_idx_map[i]);

		zassert_true(idx == 0,
				"idx shall all set to 1 when all used");
	}

	fail_thread = k_object_alloc(K_OBJ_THREAD);
	/** TESTPOINT: thread alloc failed due to out of idx */
	zassert_is_null(fail_thread,
			"mo more kobj[%d](0x%lx) shall be allocated"
			, cur_max, (uintptr_t)thread[cur_max]);


	for (int i = 0; i < cur_max; i++) {
		if (thread[i]) {
			k_object_free(thread[i]);
		}
	}
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Test kernel object allocation
 *
 * @details Allocate all kinds of kernel object and do permission
 * operation functions.
 *
 * @see k_object_alloc()
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_alloc_kobjects)
{
	struct k_thread *t;
	struct k_msgq *m;
	struct k_stack *s;
	struct k_pipe *p;
	struct k_queue *q;
	struct k_mem_slab *mslab;
	struct k_poll_signal *polls;
	struct k_timer *timer;
	struct k_mutex *mutex;
	struct k_condvar *condvar;
	void *ko;

	/* allocate kernel object */

	t = k_object_alloc(K_OBJ_THREAD);
	zassert_not_null(t, "alloc obj (0x%lx)\n", (uintptr_t)t);
	p = k_object_alloc(K_OBJ_PIPE);
	zassert_not_null(p, "alloc obj (0x%lx)\n", (uintptr_t)p);
	k_pipe_init(p, NULL, 0);
	s = k_object_alloc(K_OBJ_STACK);
	zassert_not_null(s, "alloc obj (0x%lx)\n", (uintptr_t)s);
	k_stack_init(s, NULL, 0);
	m = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(m, "alloc obj (0x%lx)\n", (uintptr_t)m);
	k_msgq_init(m, NULL, 0, 0);
	q = k_object_alloc(K_OBJ_QUEUE);
	zassert_not_null(q, "alloc obj (0x%lx)\n", (uintptr_t)q);

	/* release operations */
	k_object_release((void *)t);
	k_object_release((void *)p);
	k_object_release((void *)s);
	k_object_release((void *)m);
	k_object_release((void *)q);

	mslab = k_object_alloc(K_OBJ_MEM_SLAB);
	zassert_not_null(mslab, "alloc obj (0x%lx)\n", (uintptr_t)mslab);
	polls = k_object_alloc(K_OBJ_POLL_SIGNAL);
	zassert_not_null(polls, "alloc obj (0x%lx)\n", (uintptr_t)polls);
	timer = k_object_alloc(K_OBJ_TIMER);
	zassert_not_null(timer, "alloc obj (0x%lx)\n", (uintptr_t)timer);
	mutex = k_object_alloc(K_OBJ_MUTEX);
	zassert_not_null(mutex, "alloc obj (0x%lx)\n", (uintptr_t)mutex);
	condvar = k_object_alloc(K_OBJ_CONDVAR);
	zassert_not_null(condvar, "alloc obj (0x%lx)\n", (uintptr_t)condvar);

	k_object_release((void *)mslab);
	k_object_release((void *)polls);
	k_object_release((void *)timer);
	k_object_release((void *)mutex);

	/* no real object will be allocated */
	ko = k_object_alloc(K_OBJ_ANY);
	zassert_is_null(ko, "alloc obj (0x%lx)\n", (uintptr_t)ko);
	ko = k_object_alloc(K_OBJ_LAST);
	zassert_is_null(ko, "alloc obj (0x%lx)\n", (uintptr_t)ko);

	/* alloc possible device driver */
	ko = k_object_alloc(K_OBJ_LAST-1);
	zassert_not_null(ko, "alloc obj (0x%lx)\n", (uintptr_t)ko);
	k_object_release((void *)ko);
}

/* static kobject for permission testing */
struct k_mem_slab ms;
struct k_msgq mq;
struct k_mutex mutex;
struct k_pipe p;
struct k_queue q;
struct k_poll_signal ps;
struct k_sem sem;
struct k_stack s;
struct k_thread t;
struct k_timer timer;
struct z_thread_stack_element zs;
struct k_futex f;
struct k_condvar condvar;

static void entry_error_perm(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	set_fault_valid(true);
	k_object_access_grant(p1, k_current_get());
}

/**
 * @brief Test grant access failed in user mode
 *
 * @details Before grant access of static kobject to user thread, any
 * grant access to this thread, will trigger an expected thread
 * permission error.
 *
 * @see k_thread_access_grant()
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_kobject_perm_error)
{
#define NUM_KOBJS 13

	void *kobj[NUM_KOBJS];

	kobj[0] = &ms;
	kobj[1] = &mq;
	kobj[2] = &mutex;
	kobj[3] = &p;
	kobj[4] = &q;
	kobj[5] = &ps;
	kobj[6] = &sem;
	kobj[7] = &s;
	kobj[8] = &t;
	kobj[9] = &timer;
	kobj[10] = &zs;
	kobj[11] = &f;
	kobj[12] = &condvar;

	for (int i = 0; i < NUM_KOBJS; i++) {

		k_tid_t tid = k_thread_create(&child_thread, child_stack,
			K_THREAD_STACK_SIZEOF(child_stack),
			entry_error_perm,
			kobj[i], NULL, NULL,
			1, K_USER, K_NO_WAIT);

		k_thread_join(tid, K_FOREVER);
	}

#undef NUM_KOBJS
}

extern const char *otype_to_str(enum k_objects otype);

/**
 * @brief Test get all kernel object list
 *
 * @details Get all of the kernel object in kobject list.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(mem_protect_kobj, test_all_kobjects_str)
{
	enum k_objects otype = K_OBJ_ANY;
	const char *c;
	int  cmp;

	do {
		c = otype_to_str(otype);
		cmp = strcmp(c, "?");
		if (otype != K_OBJ_LAST) {
			zassert_true(cmp != 0,
				"otype %d unexpectedly maps to last entry \"?\"", otype);
		} else {
			zassert_true(cmp == 0,
				"otype %d does not map to last entry \"?\"", otype);
		}
		otype++;
	} while (otype <= K_OBJ_LAST);
}

ZTEST_SUITE(mem_protect_kobj, NULL, NULL, NULL, NULL, NULL);
