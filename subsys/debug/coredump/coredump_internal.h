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

struct z_coredump_memory_region_t {
	uintptr_t	start;
	uintptr_t	end;
};

extern struct z_coredump_memory_region_t z_coredump_memory_regions[];

#ifdef CONFIG_DEBUG_COREDUMP_CRC
/**
 * @brief Size of the CRC scratch buffer (bytes).
 *
 * coredump_buffer_output() copies each chunk through this scratch buffer so the
 * running CRC and the emitted bytes are computed from the exact same snapshot.
 */
#define COREDUMP_CRC_SCRATCH_SIZE 32U

/**
 * @brief Scratch buffer used to snapshot each output chunk for the CRC trailer.
 *
 * This buffer is mutated while the dump runs. It is given a dedicated symbol so
 * the linker-RAM memory-region list can carve it out of the dumped image (see
 * coredump_memory_regions.c); otherwise it would be captured mid-mutation and,
 * in a whole-image dump, overlap its own source.
 */
extern uint8_t z_coredump_crc_scratch[COREDUMP_CRC_SCRATCH_SIZE];
#endif /* CONFIG_DEBUG_COREDUMP_CRC */

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
