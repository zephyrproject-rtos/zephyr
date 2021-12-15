/*
 * Copyright (c) Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <linker/sections.h>
#include <devicetree.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>
#include <linker/devicetree_regions.h>

MEMORY
{
	LINKER_DT_REGION_FROM_NODE(DT_NODELABEL(sram_cache), rw)
	LINKER_DT_REGION_FROM_NODE(DT_NODELABEL(sram_no_cache), rw)
	LINKER_DT_REGION_FROM_NODE(DT_NODELABEL(sram_dtcm_fake), rw)
}

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
