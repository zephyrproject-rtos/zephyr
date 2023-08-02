/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_COMMON_INCLUDE_MALLOC_H_
#define ZEPHYR_LIB_LIBC_COMMON_INCLUDE_MALLOC_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mallinfo {
	size_t arena;    /* total space allocated from system */
	size_t ordblks;  /* number of non-inuse chunks */
	size_t smblks;   /* unused -- always zero */
	size_t hblks;    /* number of mmapped regions */
	size_t hblkhd;   /* total space in mmapped regions */
	size_t usmblks;  /* unused -- always zero */
	size_t fsmblks;  /* unused -- always zero */
	size_t uordblks; /* total allocated space */
	size_t fordblks; /* total non-inuse space */
	size_t keepcost; /* top-most, releasable (via malloc_trim) space */
};

struct mallinfo mallinfo(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_LIBC_COMMON_INCLUDE_MALLOC_H_ */
