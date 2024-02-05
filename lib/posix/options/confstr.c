/*
 * Copyright (c) 2024 Perrot Gaetan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/unistd.h>
#include <errno.h>

size_t confstr(int name, char *buf, size_t len)
{
	switch (name) {
	case _CS_PATH:
		/* Implementation for _CS_PATH */
		break;
	case _CS_POSIX_V7_ILP32_OFF32_CFLAGS:
		/* Implementation for _CS_POSIX_V7_ILP32_OFF32_CFLAGS */
		break;
	case _CS_POSIX_V7_ILP32_OFF32_LDFLAGS:
		/* Implementation for _CS_POSIX_V7_ILP32_OFF32_LDFLAGS */
		break;
	case _CS_POSIX_V7_ILP32_OFF32_LIBS:
		/* Implementation for _CS_POSIX_V7_ILP32_OFF32_LIBS */
		break;
	case _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS:
		/* Implementation for _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS */
		break;
	case _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS:
		/* Implementation for _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS */
		break;
	case _CS_POSIX_V7_ILP32_OFFBIG_LIBS:
		/* Implementation for _CS_POSIX_V7_ILP32_OFFBIG_LIBS */
		break;
	case _CS_POSIX_V7_LP64_OFF64_CFLAGS:
		/* Implementation for _CS_POSIX_V7_LP64_OFF64_CFLAGS */
		break;
	case _CS_POSIX_V7_LP64_OFF64_LDFLAGS:
		/* Implementation for _CS_POSIX_V7_LP64_OFF64_LDFLAGS */
		break;
	case _CS_POSIX_V7_LP64_OFF64_LIBS:
		/* Implementation for _CS_POSIX_V7_LP64_OFF64_LIBS */
		break;
	case _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS:
		/* Implementation for _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS */
		break;
	case _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS:
		/* Implementation for _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS */
		break;
	case _CS_POSIX_V7_LPBIG_OFFBIG_LIBS:
		/* Implementation for _CS_POSIX_V7_LPBIG_OFFBIG_LIBS */
		break;
	case _CS_POSIX_V7_THREADS_CFLAGS:
		/* Implementation for _CS_POSIX_V7_THREADS_CFLAGS */
		break;
	case _CS_POSIX_V7_THREADS_LDFLAGS:
		/* Implementation for _CS_POSIX_V7_THREADS_LDFLAGS */
		break;
	case _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS:
		/* Implementation for _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS */
		break;
	case _CS_V7_ENV:
		/* Implementation for _CS_V7_ENV */
		break;
	default:
		errno = EINVAL;
		break;
	}
	return 0;
}
