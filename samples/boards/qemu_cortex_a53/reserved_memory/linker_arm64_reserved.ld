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

MEMORY
{
    DT_RESERVED_MEM_REGIONS()
}

SECTIONS
{
    DT_RESERVED_MEM_SECTIONS()
}

#include <arch/arm64/scripts/linker.ld>
