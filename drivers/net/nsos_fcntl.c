/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * fcntl.h related code common to Zephyr (top: nsos_sockets.c) and Linux
 * (bottom: nsos_adapt.c).
 *
 * It is needed by both sides to share the same macro definitions/values
 * (prefixed with NSOS_MID_), which is not possible to achieve with two separate
 * standard libc libraries, since they use different values for the same
 * symbols.
 */

/*
 * When building for Zephyr, use Zephyr specific fcntl definitions.
 */
#ifdef __ZEPHYR__
#include <zephyr/posix/fcntl.h>
#else
#include <fcntl.h>
#endif

#include "nsi_errno.h"
#include "nsos_fcntl.h"

#include <stdbool.h>

static int fl_to_nsos_mid_(int flags, bool strict)
{
	int flags_mid = 0;

#define TO_NSOS_MID(_flag)				\
	if (flags & (_flag)) {				\
		flags &= ~(_flag);			\
		flags_mid |= NSOS_MID_ ## _flag;	\
	}

	TO_NSOS_MID(O_RDONLY);
	TO_NSOS_MID(O_WRONLY);
	TO_NSOS_MID(O_RDWR);

	TO_NSOS_MID(O_APPEND);
	TO_NSOS_MID(O_EXCL);
	TO_NSOS_MID(O_NONBLOCK);

#undef TO_NSOS_MID

	if (strict && flags != 0) {
		return -NSI_ERRNO_MID_EINVAL;
	}

	return flags_mid;
}

int fl_to_nsos_mid(int flags)
{
	return fl_to_nsos_mid_(flags, false);
}

int fl_to_nsos_mid_strict(int flags)
{
	return fl_to_nsos_mid_(flags, true);
}

int fl_from_nsos_mid(int flags_mid)
{
	int flags = 0;

#define FROM_NSOS_MID(_flag)				\
	if (flags_mid & NSOS_MID_ ## _flag) {		\
		flags_mid &= ~NSOS_MID_ ## _flag;	\
		flags |= _flag;				\
	}

	FROM_NSOS_MID(O_RDONLY);
	FROM_NSOS_MID(O_WRONLY);
	FROM_NSOS_MID(O_RDWR);

	FROM_NSOS_MID(O_APPEND);
	FROM_NSOS_MID(O_EXCL);
	FROM_NSOS_MID(O_NONBLOCK);

#undef FROM_NSOS_MID

	return flags;
}
