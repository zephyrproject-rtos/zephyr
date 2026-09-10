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
struct z_coredump_memory_region_t __weak z_coredump_memory_regions[] = {
	{(uintptr_t)&_image_ram_start, (uintptr_t)&_image_ram_end},
	{0, 0} /* End of list */
};
#endif
