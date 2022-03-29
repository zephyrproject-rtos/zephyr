/*
 * Copyright (c) Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <linker/sections.h>
#include <devicetree.h>
#include <linker/devicetree_regions.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

MEMORY
{
	LINKER_DT_REGIONS()
}

SECTIONS
{
	LINKER_DT_SECTIONS()
}

#include <arch/arm64/scripts/linker.ld>
