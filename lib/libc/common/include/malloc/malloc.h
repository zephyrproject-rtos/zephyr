/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_COMMON_INCLUDE_MALLOC_H_
#define ZEPHYR_LIB_LIBC_COMMON_INCLUDE_MALLOC_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory allocation statistics. This structure only defines the fields
 * supported by the Zephyr implementation to save space.
 */
struct mallinfo {
	size_t arena;    /* total space allocated from system */
	size_t uordblks; /* total allocated space */
	size_t fordblks; /* total non-inuse space */
};

struct mallinfo mallinfo(void);
void malloc_stats(void);

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_COMMON_INCLUDE_MALLOC_H_ */
