/* SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>

#include "reloc.h"

__noinit uint32_t ext_buf[EXT_BUF_WORDS];

K_HEAP_DEFINE(ext_heap, EXT_HEAP_SIZE);
K_MEM_SLAB_DEFINE(ext_slab, EXT_SLAB_BLOCK_SIZE, EXT_SLAB_BLOCKS, 4);
