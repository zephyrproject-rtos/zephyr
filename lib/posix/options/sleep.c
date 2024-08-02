/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>

/**
 * @brief Sleep for a specified number of seconds.
 *
 * See IEEE 1003.1
 */
unsigned sleep(unsigned int seconds)
{
	int rem;

	rem = k_sleep(K_SECONDS(seconds));
	__ASSERT_NO_MSG(rem >= 0);

	return rem / MSEC_PER_SEC;
}
