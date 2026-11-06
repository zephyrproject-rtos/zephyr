/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/memref.h>
#include <zephyr/sys/zassert.h>
#include <zephyr/sys/math_extras.h>

ZASSERT_MODULE(DEFAULT);

static ALWAYS_INLINE struct memref_ctrl_block *ctrl_from_mem(void *mem)
{
	return (struct memref_ctrl_block *)((uint8_t *)mem - MEMREF_CTRL_BLOCK_SIZE);
}

static ALWAYS_INLINE void *mem_from_ctrl(struct memref_ctrl_block *ctrl)
{
	return (void *)((uint8_t *)ctrl + MEMREF_CTRL_BLOCK_SIZE);
}

static void memref_release(struct sys_ref *ref)
{
	struct memref_ctrl_block *ctrl = CONTAINER_OF(ref, struct memref_ctrl_block, ref);
	const struct memref_backend *be = ctrl->backend;

#ifdef CONFIG_MEMREF_DESTROY_CB
	if (ctrl->destroy != NULL) {
		ctrl->destroy(mem_from_ctrl(ctrl));
	}
#endif
	be->free(be->ctx, ctrl);
}

void *memref_alloc(const struct memref_backend *be, size_t size)
{
	size_t total;
	struct memref_ctrl_block *ctrl;

	ZASSERT(be != NULL && be->alloc != NULL && be->free != NULL);

	if (unlikely(size_add_overflow(size, MEMREF_CTRL_BLOCK_SIZE, &total))) {
		return NULL;
	}

	ctrl = be->alloc(be->ctx, total);
	if (ctrl == NULL) {
		return NULL;
	}

	sys_ref_init(&ctrl->ref);
#ifdef CONFIG_MEMREF_DESTROY_CB
	ctrl->destroy = NULL;
#endif
	ctrl->backend = be;

	return mem_from_ctrl(ctrl);
}

void *memref_calloc(const struct memref_backend *be, size_t nmemb, size_t size)
{
	size_t payload;
	void *mem;

	if (unlikely(size_mul_overflow(nmemb, size, &payload))) {
		return NULL;
	}

	mem = memref_alloc(be, payload);
	if (mem == NULL) {
		return NULL;
	}

	return memset(mem, 0, payload);
}

#ifdef CONFIG_MEMREF_DESTROY_CB
void *memref_alloc_cb(const struct memref_backend *be, size_t size, memref_destroy_cb_t destroy)
{
	void *mem = memref_alloc(be, size);

	if (mem != NULL) {
		ctrl_from_mem(mem)->destroy = destroy;
	}

	return mem;
}

void *memref_calloc_cb(const struct memref_backend *be, size_t nmemb, size_t size,
		       memref_destroy_cb_t destroy)
{
	void *mem = memref_calloc(be, nmemb, size);

	if (mem != NULL) {
		ctrl_from_mem(mem)->destroy = destroy;
	}

	return mem;
}
#endif /* CONFIG_MEMREF_DESTROY_CB */

void memref_unref(void *mem)
{
	if (mem == NULL) {
		return;
	}

	sys_ref_put(&ctrl_from_mem(mem)->ref, memref_release);
}

void memref_ref(void *mem)
{
	struct memref_ctrl_block *ctrl;

	ZASSERT(mem != NULL);

	ctrl = ctrl_from_mem(mem);
	ZASSERT(ctrl->backend != NULL);
	sys_ref_get(&ctrl->ref);
}
