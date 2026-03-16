/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "llext_kheap.h"

#ifdef CONFIG_LLEXT_HEAP_DYNAMIC
#ifdef CONFIG_HARVARD
struct k_heap llext_instr_heap;
struct k_heap llext_data_heap;
#else
struct k_heap llext_heap;
#endif
#else /* !CONFIG_LLEXT_HEAP_DYNAMIC */
#ifdef CONFIG_HARVARD
Z_HEAP_DEFINE_IN_SECT(llext_instr_heap, (CONFIG_LLEXT_INSTR_HEAP_SIZE * KB(1)),
		      __attribute__((section(".rodata.llext_instr_heap"))));
Z_HEAP_DEFINE_IN_SECT(llext_data_heap, (CONFIG_LLEXT_DATA_HEAP_SIZE * KB(1)),
		      __attribute__((section(".data.llext_data_heap"))));
#else /* !CONFIG_HARVARD */
K_HEAP_DEFINE(llext_heap, CONFIG_LLEXT_HEAP_SIZE * KB(1));
#endif
#endif /* CONFIG_LLEXT_HEAP_DYNAMIC */
