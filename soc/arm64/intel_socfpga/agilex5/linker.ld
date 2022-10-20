/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/devicetree.h>

#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* Added below memory and section macros to parse
* the zephyr,memory-region compatible nodes
*/
MEMORY
{
	LINKER_DT_REGIONS()
}

#include <zephyr/arch/arm64/scripts/linker.ld>
