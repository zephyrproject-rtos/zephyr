/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EXT2_MEM_H__
#define __EXT2_MEM_H__

/* Memory allocation. */
void *ext2_heap_alloc(size_t size);
void  ext2_heap_free(void *ptr);

#endif /* __EXT2_MEM_H__ */
