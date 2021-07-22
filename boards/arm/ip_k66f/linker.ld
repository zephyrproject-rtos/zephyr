/*
 * Copyright (c) 2021 DENX Software Engineeering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for ip_k66f board to allow ethernet
 * interfaces placement.
 */

/*
 * Explicit placement of the ethernet interfaces
 *
 * eth0 is the "master" port for the DSA switch and hence shall be
 * placed on the index (position) 1 (as net_if_get_by_index() uses
 * internally '_net_if_list_start[index - 1]' linker generated symbol).
 *
 * lan{123} shall be placed afterwards (sorted) so the order will be
 * correctly preserved.
 *
 * If ip_k66f have had eth1 interface - it shall be placed afterwards
 *
 * The last entry with `._net_if.static.*` is to accommodate ethernet
 * interfaces created in tests/samples - like e.g. _net_if.static.eth_test)
 * in tests/net/vlan/src/main.c
 */

#define NETWORK_RAM_SECTIONS                                    \
SECTION_DATA_PROLOGUE(net_if_area,,SUBALIGN(4))                 \
{                                                               \
	_net_if_list_start = .;                                 \
	KEEP(*(._net_if.static.dts*));                          \
	KEEP(*(SORT_BY_NAME(._net_if.static.dsa_slave*)));      \
	KEEP(*(SORT_BY_NAME(._net_if.static.*)));               \
	_net_if_list_end = .;                                   \
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)            \
Z_ITERABLE_SECTION_RAM(net_if_dev, 4)                           \
Z_ITERABLE_SECTION_RAM(net_l2, 4)				\
Z_ITERABLE_SECTION_RAM(eth_bridge, 4)

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
