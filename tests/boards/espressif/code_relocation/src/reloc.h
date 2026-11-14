/* SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RELOC_H_
#define RELOC_H_

#include <stdint.h>
#include <zephyr/kernel.h>

#define RELOC_DATA_INIT     0x1234U
#define EXT_BUF_WORDS       1024U
#define RELOC_EXT_BSS_SIZE  16U
#define EXT_HEAP_SIZE       4096U
#define EXT_SLAB_BLOCK_SIZE 64U
#define EXT_SLAB_BLOCKS     8U

extern const uint32_t reloc_rodata[4];
extern uint32_t reloc_data;
extern uint32_t reloc_bss;
extern uint32_t reloc_noinit;
extern uint32_t ext_buf[EXT_BUF_WORDS];
extern uint8_t reloc_ext_bss[RELOC_EXT_BSS_SIZE];
extern struct k_heap ext_heap;
extern struct k_mem_slab ext_slab;

uint32_t reloc_func(uint32_t x);
uint32_t reloc_func_no_cache(uint32_t x);

#endif /* RELOC_H_ */
