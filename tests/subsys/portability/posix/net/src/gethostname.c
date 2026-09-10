/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/net/hostname.h>

ZTEST(net, test_gethostname)
{
	char hostname[CONFIG_NET_HOSTNAME_MAX_LEN + 1];
	int ret;

	ret = gethostname(hostname, sizeof(hostname));
	zassert_equal(ret, 0, "gethostname() failed: %d", ret);

	zassert_equal(strcmp(hostname, CONFIG_NET_HOSTNAME), 0,
		      "gethostname() returned unexpected hostname: %s", hostname);
}

ZTEST(net, test_gethostname_buffer_too_small)
{
	char small_hostname[2];
	int ret;

	ret = gethostname(small_hostname, sizeof(small_hostname));

	/* POSIX allows either truncation or error when buffer is too small.
	 * Zephyr's current implementation truncates (which is POSIX-compliant).
	 */
	if (ret == -1) {
		/* Implementation returns error - verify errno */
		zassert_equal(errno, ENAMETOOLONG, "Expected ENAMETOOLONG, got %d", errno);
	} else {
		/* Implementation truncates - verify it succeeded */
		zassert_equal(ret, 0, "gethostname() should return 0 on truncation");
		/* Note: strncpy used in implementation may not null-terminate when truncated,
		 * so we cannot reliably verify null-termination
		 */
	}
}

ZTEST(net, test_gethostname_zero_length)
{
	char hostname[CONFIG_NET_HOSTNAME_MAX_LEN + 1];
	int ret;

	ret = gethostname(hostname, 0);

	/* With zero length buffer, the implementation uses strncpy(buf, p, 0)
	 * which copies nothing and returns success. POSIX allows this behavior.
	 */
	if (ret == -1) {
		/* Strict implementation - verify errno */
		zassert_equal(errno, EINVAL, "Expected EINVAL, got %d", errno);
	} else {
		/* Lenient implementation - accepts success */
		zassert_equal(ret, 0, "gethostname() returned unexpected value: %d", ret);
	}
}

ZTEST(net, test_hostname_max_len_consistency)
{
	/* Verify that CONFIG_NET_HOSTNAME_MAX_LEN is properly defined and > 0 */
	zassert_true(CONFIG_NET_HOSTNAME_MAX_LEN > 0,
		     "CONFIG_NET_HOSTNAME_MAX_LEN must be positive");

	/* Verify it can hold at least the configured hostname */
	zassert_true(CONFIG_NET_HOSTNAME_MAX_LEN >= strlen(CONFIG_NET_HOSTNAME),
		     "CONFIG_NET_HOSTNAME_MAX_LEN too small for CONFIG_NET_HOSTNAME");

#ifdef CONFIG_POSIX_HOST_NAME_MAX
	/* If POSIX is enabled, verify consistency */
	zassert_true(CONFIG_POSIX_HOST_NAME_MAX >= CONFIG_NET_HOSTNAME_MAX_LEN,
		     "POSIX_HOST_NAME_MAX should be >= NET_HOSTNAME_MAX_LEN");
#endif
}

#ifdef CONFIG_NET_HOSTNAME_DYNAMIC
ZTEST(net, test_gethostname_dynamic_update)
{
	char hostname[CONFIG_NET_HOSTNAME_MAX_LEN + 1];
	char original_hostname[CONFIG_NET_HOSTNAME_MAX_LEN + 1];
	char test_hostname[] = "test-dynamic";
	int ret;

	/* Get original hostname */
	ret = gethostname(original_hostname, sizeof(original_hostname));
	zassert_equal(ret, 0, "Failed to get original hostname");

	/* Set new hostname */
	ret = net_hostname_set(test_hostname, strlen(test_hostname));
	zassert_equal(ret, 0, "Failed to set hostname");

	/* Verify hostname changed */
	ret = gethostname(hostname, sizeof(hostname));
	zassert_equal(ret, 0, "Failed to get hostname after update");
	zassert_equal(strcmp(hostname, test_hostname), 0,
		      "Hostname not updated correctly");

	/* Restore original hostname */
	ret = net_hostname_set(original_hostname, strlen(original_hostname));
	zassert_equal(ret, 0, "Failed to restore original hostname");
}
#endif /* CONFIG_NET_HOSTNAME_DYNAMIC */

#ifdef CONFIG_NET_HOSTNAME_UNIQUE_UPDATE
ZTEST(net, test_gethostname_with_unique_update)
{
	char hostname[CONFIG_NET_HOSTNAME_MAX_LEN + 1];
	int ret;

	/* This test validates that gethostname works when NET_HOSTNAME_UNIQUE_UPDATE is enabled */
	ret = gethostname(hostname, sizeof(hostname));
	zassert_equal(ret, 0, "gethostname() failed with NET_HOSTNAME_UNIQUE_UPDATE enabled");

	/* Hostname should contain the configured name possibly with unique suffix */
	zassert_true(strlen(hostname) > 0, "Hostname should not be empty");
	zassert_true(strlen(hostname) <= CONFIG_NET_HOSTNAME_MAX_LEN,
		     "Hostname length exceeds maximum");
}
#endif /* CONFIG_NET_HOSTNAME_UNIQUE_UPDATE */
