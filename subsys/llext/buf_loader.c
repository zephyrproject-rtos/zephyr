/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/llext/buf_loader.h>
#include <zephyr/arch/common/instr_mem.h>
#include <zephyr/sys/util.h>
#include <string.h>

int llext_buf_read(struct llext_loader *l, void *buf, size_t len)
{
	struct llext_buf_loader *buf_l = CONTAINER_OF(l, struct llext_buf_loader, loader);
	size_t end = MIN(buf_l->pos + len, buf_l->len);
	size_t read_len = end - buf_l->pos;
	const uint8_t *src = buf_l->buf + buf_l->pos;

	if (IS_ENABLED(CONFIG_ARCH_HAS_WORD_GRANULAR_ACCESS_INSTR_MEM) &&
	    arch_is_instr_mem(buf, read_len)) {
		arch_memcpy_to_instr(buf, src, read_len);
	} else {
		memcpy(buf, src, read_len);
	}

	buf_l->pos = end;

	return 0;
}

int llext_buf_seek(struct llext_loader *l, size_t pos)
{
	struct llext_buf_loader *buf_l = CONTAINER_OF(l, struct llext_buf_loader, loader);

	buf_l->pos = MIN(pos, buf_l->len);

	return 0;
}

void *llext_buf_peek(struct llext_loader *l, size_t pos)
{
	struct llext_buf_loader *buf_l = CONTAINER_OF(l, struct llext_buf_loader, loader);

	if (pos >= buf_l->len) {
		return NULL;
	}

	return (void *)(buf_l->buf + pos);
}
