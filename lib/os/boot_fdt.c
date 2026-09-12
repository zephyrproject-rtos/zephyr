/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/sys/boot_fdt.h>
#include <zephyr/sys/byteorder.h>

#define FDT_MAGIC 0xd00dfeed
#define FDT_HEADER_MIN_SIZE 8U

int sys_boot_fdt_copy(uint8_t *dst, size_t dst_size, const uint8_t *src,
		      uint32_t *fdt_size)
{
	uint32_t magic;
	uint32_t size;

	if ((dst == NULL) || (src == NULL) || (fdt_size == NULL)) {
		return -EINVAL;
	}

	magic = sys_get_be32(src);
	size = sys_get_be32(src + sizeof(magic));

	if ((magic != FDT_MAGIC) || (size < FDT_HEADER_MIN_SIZE)) {
		return -EINVAL;
	}

	*fdt_size = size;

	if (size > dst_size) {
		return -ENOSPC;
	}

	memcpy(dst, src, size);

	return 0;
}
