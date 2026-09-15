/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __APPLE__

/*
 * Export iterable-section boundary symbols from Mach-O section pseudo-symbols
 * so the POSIX/native_sim image can keep using the regular Zephyr names.
 */

#define MACHO_ALIAS(sym, target) \
	__asm__(".globl _" #sym "\n_" #sym " = " target)

MACHO_ALIAS(__static_thread_data_list_start, "section$start$__DATA$zthrdat");
MACHO_ALIAS(__static_thread_data_list_end, "section$end$__DATA$zthrdat");
MACHO_ALIAS(_k_kernel_init_pre_entry_list_start, "section$start$__DATA$zkinipre");
MACHO_ALIAS(_k_kernel_init_pre_entry_list_end, "section$end$__DATA$zkinipre");
MACHO_ALIAS(_k_kernel_init_post_entry_list_start, "section$start$__DATA$zkinipst");
MACHO_ALIAS(_k_kernel_init_post_entry_list_end, "section$end$__DATA$zkinipst");
MACHO_ALIAS(_k_mem_slab_list_start, "section$start$__DATA$zkmslab");
MACHO_ALIAS(_k_mem_slab_list_end, "section$end$__DATA$zkmslab");
MACHO_ALIAS(_net_buf_pool_list_start, "section$start$__DATA$znbpool");
MACHO_ALIAS(_net_buf_pool_list_end, "section$end$__DATA$znbpool");
MACHO_ALIAS(_net_if_list_start, "section$start$__DATA$znetif");
MACHO_ALIAS(_net_if_list_end, "section$end$__DATA$znetif");

/*
 * This configuration does not emit static k_heap or net_mgmt event-handler
 * entries. Keep the ranges empty by making start == end.
 */
MACHO_ALIAS(_k_heap_list_start, "section$start$__DATA$znbpool");
MACHO_ALIAS(_k_heap_list_end, "section$start$__DATA$znbpool");
MACHO_ALIAS(_net_mgmt_event_static_handler_list_start, "section$start$__DATA$znetif");
MACHO_ALIAS(_net_mgmt_event_static_handler_list_end, "section$start$__DATA$znetif");

#endif /* __APPLE__ */
