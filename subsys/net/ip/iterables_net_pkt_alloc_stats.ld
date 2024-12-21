/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_RAM(net_pkt_alloc_stats_slab, Z_LINK_ITERABLE_SUBALIGN)
