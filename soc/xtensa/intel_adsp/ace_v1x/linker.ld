/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/devicetree_regions.h>
#include <ace-link.ld>

MEMORY
{
    LINKER_DT_REGIONS()
}

SECTIONS
{
    LINKER_DT_SECTIONS()
}
