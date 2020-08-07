/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEBUG_COREDUMP_INTERNAL_H_
#define DEBUG_COREDUMP_INTERNAL_H_

#include <toolchain.h>

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

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
 * @brief Signal to coredump subsys there is an error.
 */
void z_coredump_error(void);

typedef void (*z_coredump_backend_start_t)(void);
typedef void (*z_coredump_backend_end_t)(void);
typedef void (*z_coredump_backend_error_t)(void);
typedef int (*z_coredump_backend_buffer_output_t)(uint8_t *buf, size_t buflen);

struct z_coredump_backend_api {
	/* Signal to backend of the start of coredump. */
	z_coredump_backend_start_t		start;

	/* Signal to backend of the end of coredump. */
	z_coredump_backend_end_t		end;

	/* Signal to backend an error has been encountered. */
	z_coredump_backend_error_t		error;

	/* Raw buffer output */
	z_coredump_backend_buffer_output_t	buffer_output;
};

/**
 * @endcond
 */

#endif /* DEBUG_COREDUMP_INTERNAL_H_ */
