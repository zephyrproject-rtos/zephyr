/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <kernel_internal.h>
#include <zephyr/toolchain.h>
#include <zephyr/debug/coredump.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "coredump_internal.h"

#ifdef CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_LINKER_RAM
#ifdef CONFIG_DEBUG_COREDUMP_CRC
/*
 * When the CRC trailer is enabled, coredump_buffer_output() snapshots each
 * emitted chunk through z_coredump_crc_scratch. That buffer is written while
 * the dump is running, so it must not itself be part of the dumped image: in a
 * whole-image RAM dump it would be captured mid-mutation and its source would
 * overlap the scratch. Split the RAM range so the scratch buffer is carved out
 * of the dump entirely. Empty sub-ranges (scratch at either edge of RAM) are
 * skipped by coredump_memory_dump().
 */
struct z_coredump_memory_region_t __weak z_coredump_memory_regions[] = {
	{(uintptr_t)&_image_ram_start, (uintptr_t)&z_coredump_crc_scratch[0]},
	{(uintptr_t)&z_coredump_crc_scratch[COREDUMP_CRC_SCRATCH_SIZE],
	 (uintptr_t)&_image_ram_end},
	{0, 0} /* End of list */
};
#else
struct z_coredump_memory_region_t __weak z_coredump_memory_regions[] = {
	{(uintptr_t)&_image_ram_start, (uintptr_t)&_image_ram_end},
	{0, 0} /* End of list */
};
#endif /* CONFIG_DEBUG_COREDUMP_CRC */
#endif
