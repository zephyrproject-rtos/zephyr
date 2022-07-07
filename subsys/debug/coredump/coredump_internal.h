/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEBUG_COREDUMP_INTERNAL_H_
#define DEBUG_COREDUMP_INTERNAL_H_

#include <zephyr/toolchain.h>

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

#define COREDUMP_BEGIN_STR	"BEGIN#"
#define COREDUMP_END_STR	"END#"
#define COREDUMP_ERROR_STR	"ERROR CANNOT DUMP#"

/*
 * Need to prefix coredump strings to make it easier to parse
 * as log module adds its own prefixes.
 */
#define COREDUMP_PREFIX_STR	"#CD:"

struct z_coredump_memory_region_t {
	uintptr_t	start;
	uintptr_t	end;
};

extern struct z_coredump_memory_region_t z_coredump_memory_regions[];

/**
 * @brief Mark the start of coredump
 *
 * This sets up coredump subsys so coredump can be commenced.
 *
 * For example, backend needs to be initialized before any
 * output can be stored.
 */
void z_coredump_start(void);

/**
 * @brief Mark the end of coredump
 *
 * This tells the coredump subsys to finalize the coredump
 * session.
 *
 * For example, backend may need to flush the output.
 */
void z_coredump_end(void);

/**
 * @endcond
 */

#endif /* DEBUG_COREDUMP_INTERNAL_H_ */
