/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * This module can be used to covert fcntl.h constants between the embedded and
 * host C libraries.
 *
 * It is needed by both sides to share the same macro definitions/values
 * (prefixed with NSOS_MID_), which is not possible to achieve with two separate
 * standard libc libraries, since they use different values for the same
 * symbols.
 */

#include <stdbool.h>
#include <fcntl.h>

#include "nsi_errno.h"
#include "nsi_fcntl.h"

static int nsi_fcntl_to_mid_(int flags, bool strict)
{
	int flags_mid = 0;

#define TO_MID(_flag)				\
	if (flags & (_flag)) {				\
		flags &= ~(_flag);			\
		flags_mid |= NSI_FCNTL_MID_ ## _flag;	\
	}

	TO_MID(O_RDONLY);
	TO_MID(O_WRONLY);
	TO_MID(O_RDWR);

	TO_MID(O_CREAT);
	TO_MID(O_TRUNC);
	TO_MID(O_APPEND);
	TO_MID(O_EXCL);
	TO_MID(O_NONBLOCK);

#undef TO_MID

	if (strict && flags != 0) {
		return -NSI_ERRNO_MID_EINVAL;
	}

	return flags_mid;
}

int nsi_fcntl_to_mid(int flags)
{
	return nsi_fcntl_to_mid_(flags, false);
}

int nsi_fcntl_to_mid_strict(int flags)
{
	return nsi_fcntl_to_mid_(flags, true);
}

int nsi_fcntl_from_mid(int flags_mid)
{
	int flags = 0;

#define FROM_MID(_flag)				\
	if (flags_mid & NSI_FCNTL_MID_ ## _flag) {		\
		flags_mid &= ~NSI_FCNTL_MID_ ## _flag;	\
		flags |= _flag;				\
	}

	FROM_MID(O_RDONLY);
	FROM_MID(O_WRONLY);
	FROM_MID(O_RDWR);

	FROM_MID(O_CREAT);
	FROM_MID(O_TRUNC);
	FROM_MID(O_APPEND);
	FROM_MID(O_EXCL);
	FROM_MID(O_NONBLOCK);

#undef FROM_MID

	return flags;
}
