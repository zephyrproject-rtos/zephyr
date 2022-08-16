/*
 * Copyright (c) 2021 intel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest_error_hook.h>

static struct k_poll_signal signal_err;
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#ifdef CONFIG_USERSPACE
/**
 * @brief Test API k_poll with error number events in user mode
 *
 * @details Using API k_poll with error number
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_k_poll_user_num_err)
{
	struct k_poll_event events;

	ztest_set_fault_valid(true);
	k_poll(&events, -1, K_NO_WAIT);
}

/**
 * @brief Test API k_poll with error member of events in user mode
 *
 * @details Using API k_poll with error member
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_k_poll_user_mem_err)
{
	ztest_set_fault_valid(true);
	k_poll(NULL, 3, K_NO_WAIT);
}

/**
 * @brief Test API k_poll with NULL sem event in user mode
 *
 * @details Define a poll event, and using API k_poll with NULL sem
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_k_poll_user_type_sem_err)
{
	struct k_poll_event event[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 NULL),
	};

	ztest_set_fault_valid(true);
	k_poll(event, 1, K_NO_WAIT);
}

/**
 * @brief Test API k_poll with NULL signal event in user mode
 *
 * @details Define a poll, and using API k_poll with NULL signal
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_k_poll_user_type_signal_err)
{
	struct k_poll_event event[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 NULL),
	};

	ztest_set_fault_valid(true);
	k_poll(event, 1, K_NO_WAIT);
}

/**
 * @brief Test API k_poll with NULL fifo event in user mode
 *
 * @details Define a poll, and using API k_poll with NULL fifo
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_k_poll_user_type_fifo_err)
{
	struct k_poll_event event[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 NULL),
	};

	ztest_set_fault_valid(true);
	k_poll(event, 1, K_NO_WAIT);
}

/**
 * @brief Test API k_poll with NULL message queue event in user mode
 *
 * @details Define a poll, and using API k_poll with NULL message queue
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_k_poll_user_type_msgq_err)
{
	struct k_poll_event event[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 NULL),
	};

	ztest_set_fault_valid(true);
	k_poll(event, 1, K_NO_WAIT);
}


/**
 * @brief Test API k_poll_signal_init with NULL in user mode
 *
 * @details Using API k_poll_signal_init with NULL as
 * parameter to check if a error will be met.
 *
 * @see k_poll_signal_init()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_poll_signal_init_null)
{
	ztest_set_fault_valid(true);
	k_poll_signal_init(NULL);
}

/**
 * @brief Test API k_poll_signal_check with NULL object in user mode
 *
 * @details Using API k_poll with NULL object
 * as parameter to check if a error will be met.
 *
 * @see k_poll_signal_check()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_poll_signal_check_obj)
{
	unsigned int signaled;
	int result;

	ztest_set_fault_valid(true);
	k_poll_signal_check(NULL, &signaled, &result);
}

/**
 * @brief Test API k_poll_signal_check with unread address
 * in user mode
 *
 * @details Using k_poll_signal_check with
 * unread results as parameter to check if a error
 * will be met.
 *
 * @see k_poll_signal_check()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_poll_signal_check_signal)
{
	unsigned int result;

	k_poll_signal_init(&signal_err);

	ztest_set_fault_valid(true);
	k_poll_signal_check(&signal_err, NULL, &result);
}



/**
 * @brief Test API k_poll_signal_check with unread address
 * in user mode
 *
 * @details Using k_poll_signal_check with
 * unread signaled as parameter to check if a error
 * will be met.
 *
 * @see k_poll_signal_check()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_poll_signal_check_result)
{
	int signaled;

	k_poll_signal_init(&signal_err);

	ztest_set_fault_valid(true);
	k_poll_signal_check(&signal_err, &signaled, NULL);
}

/**
 * @brief Test API k_poll_signal_raise with unread address
 * in user mode
 *
 * @details Using k_poll_signal_raise with
 * NULL as parameter to check if a error
 * will be met.
 *
 * @see k_poll_signal_raise()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_poll_signal_raise_null)
{
	int result = 0;

	ztest_set_fault_valid(true);
	k_poll_signal_raise(NULL, result);
}

/**
 * @brief Test API k_poll_signal_reset with unread address
 * in user mode
 *
 * @details Using k_poll_signal_reset with
 * NULL as parameter to check if a error
 * will be met.
 *
 * @see k_poll_signal_reset()
 *
 * @ingroup kernel_poll_tests
 */
ZTEST_USER(poll_api, test_poll_signal_reset_null)
{
	ztest_set_fault_valid(true);
	k_poll_signal_reset(NULL);
}
#endif /* CONFIG_USERSPACE */

void poll_fail_grant_access(void)
{
	k_thread_access_grant(k_current_get(), &signal_err);
}
