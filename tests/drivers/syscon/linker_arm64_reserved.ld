/*
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

MEMORY
{
	LINKER_DT_REGIONS()
}

SECTIONS
{
	LINKER_DT_SECTIONS()
}

#include <zephyr/arch/arm64/scripts/linker.ld>
