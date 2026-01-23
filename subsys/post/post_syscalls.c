/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POST subsystem syscall implementations
 */

#include <zephyr/kernel.h>
#include <zephyr/post/post.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(post, CONFIG_POST_LOG_LEVEL);

/*
 * Syscall implementations
 */

int z_impl_post_run_test_user(uint32_t test_id)
{
	const struct post_test *test = post_get_test(test_id);

	if (test == NULL) {
		return -ENOENT;
	}

	/* Check if test is allowed from userspace */
	if (!(test->flags & POST_FLAG_USERSPACE_OK)) {
		LOG_WRN("Test %u not allowed from userspace", test_id);
		return -EACCES;
	}

	/* Check if test is safe for runtime */
	if (!(test->flags & POST_FLAG_RUNTIME_OK)) {
		LOG_WRN("Test %u not safe for runtime execution", test_id);
		return -EPERM;
	}

	return (int)post_run_test(test_id);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_post_run_test_user(uint32_t test_id)
{
	return z_impl_post_run_test_user(test_id);
}
#include <zephyr/syscalls/post_run_test_user_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_post_get_result_user(uint32_t test_id, enum post_result *result)
{
	struct post_result_record record;
	int ret;

	if (result == NULL) {
		return -EINVAL;
	}

	ret = post_get_result(test_id, &record);
	if (ret != 0) {
		return ret;
	}

	*result = record.result;
	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_post_get_result_user(uint32_t test_id,
					      enum post_result *result)
{
	int ret;
	enum post_result kern_result;

	/* Validate user pointer */
	K_OOPS(K_SYSCALL_MEMORY_WRITE(result, sizeof(*result)));

	ret = z_impl_post_get_result_user(test_id, &kern_result);
	if (ret == 0) {
		K_OOPS(k_usermode_to_copy(result, &kern_result,
					  sizeof(kern_result)));
	}

	return ret;
}
#include <zephyr/syscalls/post_get_result_user_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_post_get_summary_user(uint32_t *passed, uint32_t *failed,
				 uint32_t *skipped)
{
	return post_get_summary(passed, failed, skipped);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_post_get_summary_user(uint32_t *passed,
					       uint32_t *failed,
					       uint32_t *skipped)
{
	int ret;
	uint32_t kern_passed = 0, kern_failed = 0, kern_skipped = 0;

	/* Validate user pointers (any can be NULL) */
	if (passed != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(passed, sizeof(*passed)));
	}
	if (failed != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(failed, sizeof(*failed)));
	}
	if (skipped != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(skipped, sizeof(*skipped)));
	}

	ret = z_impl_post_get_summary_user(&kern_passed, &kern_failed,
					   &kern_skipped);

	if (ret >= 0) {
		if (passed != NULL) {
			K_OOPS(k_usermode_to_copy(passed, &kern_passed,
						  sizeof(kern_passed)));
		}
		if (failed != NULL) {
			K_OOPS(k_usermode_to_copy(failed, &kern_failed,
						  sizeof(kern_failed)));
		}
		if (skipped != NULL) {
			K_OOPS(k_usermode_to_copy(skipped, &kern_skipped,
						  sizeof(kern_skipped)));
		}
	}

	return ret;
}
#include <zephyr/syscalls/post_get_summary_user_mrsh.c>
#endif /* CONFIG_USERSPACE */
