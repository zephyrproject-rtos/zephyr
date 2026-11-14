/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/memref.h>
#include <zephyr/sys/zassert.h>

#include "memref_priv.h"

ZASSERT_MODULE(DEFAULT);

static void memref_release(struct sys_ref *ref)
{
	struct memref_ctrl_block *ctrl = CONTAINER_OF(ref, struct memref_ctrl_block, ref);
	const struct memref_backend *be;

	ZASSERT(memref_ctrl_valid(ctrl), "Not a memref allocation");
	be = ctrl->backend;

#ifdef CONFIG_ASSERT
	ctrl->magic = 0U;
#endif
#ifdef CONFIG_MEMREF_DESTROY_CB
	if (ctrl->destroy != NULL) {
		ctrl->destroy(memref_mem_from_ctrl(ctrl));
	}
#endif
	be->free(be->ctx, ctrl);
}

void *z_memref_ctrl_init(struct memref_ctrl_block *ctrl, const struct memref_backend *be)
{
	sys_ref_init(&ctrl->ref);
	ctrl->backend = be;

#ifdef CONFIG_MEMREF_DESTROY_CB
	ctrl->destroy = NULL;
#endif
#ifdef CONFIG_ASSERT
	ctrl->magic = MEMREF_CTRL_MAGIC;
#endif
	return memref_mem_from_ctrl(ctrl);
}

void memref_unref(void *mem)
{
	if (mem == NULL) {
		return;
	}

	ZASSERT(memref_ctrl_valid(memref_ctrl_from_mem(mem)), "Not a memref allocation");
	sys_ref_put(&memref_ctrl_from_mem(mem)->ref, memref_release);
}

void memref_ref(void *mem)
{
	ZASSERT(mem != NULL);
	ZASSERT(memref_ctrl_valid(memref_ctrl_from_mem(mem)), "Not a memref allocation");
	sys_ref_get(&memref_ctrl_from_mem(mem)->ref);
}
