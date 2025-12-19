/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_MEMREF_MEMREF_PRIV_H_
#define ZEPHYR_LIB_MEMREF_MEMREF_PRIV_H_

#include <zephyr/sys/memref.h>
#include <zephyr/toolchain.h>

#define MEMREF_CTRL_MAGIC 0x4d454d52U

static ALWAYS_INLINE void *memref_mem_from_ctrl(struct memref_ctrl_block *ctrl)
{
	return (void *)((uint8_t *)ctrl + MEMREF_CTRL_BLOCK_SIZE);
}

static inline struct memref_ctrl_block *memref_ctrl_from_mem(void *mem)
{
	return (struct memref_ctrl_block *)((uint8_t *)mem - MEMREF_CTRL_BLOCK_SIZE);
}

static inline bool memref_ctrl_valid(const struct memref_ctrl_block *ctrl)
{
#ifdef CONFIG_ASSERT
	return ctrl->magic == MEMREF_CTRL_MAGIC && ctrl->backend != NULL;
#else
	ARG_UNUSED(ctrl);

	return true;
#endif
}

void *z_memref_ctrl_init(struct memref_ctrl_block *ctrl, const struct memref_backend *be);

#endif /* ZEPHYR_LIB_MEMREF_MEMREF_PRIV_H_ */
