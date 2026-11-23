/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Heaps and network buffers first used from POST_KERNEL, in a region usable from there */
#if DT_HAS_CHOSEN(bflb_heap_ram_late) && defined(CONFIG_MEMC)
SECTION_PROLOGUE(_bflb_heap_ram_late, (NOLOAD),)
{
	*(".noinit.*subsys/net/ip/net_pkt.c.[0-9]*")
	*(".noinit.*lib/libc/common/source/stdlib/malloc.c*")
	*(.mbedtls_heap)
} GROUP_LINK_IN(LINKER_DT_NODE_REGION_NAME_TOKEN(DT_CHOSEN(bflb_heap_ram_late)))
#endif

/* Heaps and network packet pools, in a region usable from early boot */
#if DT_HAS_CHOSEN(bflb_heap_ram_early)
SECTION_PROLOGUE(_bflb_heap_ram_early, (NOLOAD),)
{
	*(.noinit.*kheap_buf__system_heap*)
	*(".noinit.*lib/libc/common/source/stdlib/malloc.c*")
	*(.mbedtls_heap)
	*(".noinit.*subsys/net/ip/net_pkt.c*")
} GROUP_LINK_IN(LINKER_DT_NODE_REGION_NAME_TOKEN(DT_CHOSEN(bflb_heap_ram_early)))
#endif
