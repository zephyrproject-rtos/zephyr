/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <limits.h>
#include <metal/io.h>
#include <metal/sys.h>

void metal_io_init(struct metal_io_region *io, void *virt,
	      const metal_phys_addr_t *physmap, size_t size,
	      unsigned page_shift, unsigned int mem_flags,
	      const struct metal_io_ops *ops)
{
	const struct metal_io_ops nops = {NULL, NULL, NULL, NULL, NULL, NULL};

	io->virt = virt;
	io->physmap = physmap;
	io->size = size;
	io->page_shift = page_shift;
	if (page_shift >= sizeof(io->page_mask) * CHAR_BIT)
		/* avoid overflow */
		io->page_mask = -1UL;
	else
		io->page_mask = (1UL << page_shift) - 1UL;
	io->mem_flags = mem_flags;
	io->ops = ops ? *ops : nops;
	metal_sys_io_mem_map(io);
}

int metal_io_block_read(struct metal_io_region *io, unsigned long offset,
	       void *restrict dst, int len)
{
	void *ptr = metal_io_virt(io, offset);
	int retlen;

	if (offset > io->size)
		return -ERANGE;
	if ((offset + len) > io->size)
		len = io->size - offset;
	retlen = len;
	if (io->ops.block_read) {
		retlen = (*io->ops.block_read)(
			io, offset, dst, memory_order_seq_cst, len);
	} else {
		atomic_thread_fence(memory_order_seq_cst);
		while ( len && (
			((uintptr_t)dst % sizeof(int)) ||
			((uintptr_t)ptr % sizeof(int)))) {
			*(unsigned char *)dst =
				*(const unsigned char *)ptr;
			dst++;
			ptr++;
			len--;
		}
		for (; len >= (int)sizeof(int); dst += sizeof(int),
					ptr += sizeof(int),
					len -= sizeof(int))
			*(unsigned int *)dst = *(const unsigned int *)ptr;
		for (; len != 0; dst++, ptr++, len--)
			*(unsigned char *)dst =
				*(const unsigned char *)ptr;
	}
	return retlen;
}

int metal_io_block_write(struct metal_io_region *io, unsigned long offset,
	       const void *restrict src, int len)
{
	void *ptr = metal_io_virt(io, offset);
	int retlen;

	if (offset > io->size)
		return -ERANGE;
	if ((offset + len) > io->size)
		len = io->size - offset;
	retlen = len;
	if (io->ops.block_write) {
		retlen = (*io->ops.block_write)(
			io, offset, src, memory_order_seq_cst, len);
	} else {
		while ( len && (
			((uintptr_t)ptr % sizeof(int)) ||
			((uintptr_t)src % sizeof(int)))) {
			*(unsigned char *)ptr =
				*(const unsigned char *)src;
			ptr++;
			src++;
			len--;
		}
		for (; len >= (int)sizeof(int); ptr += sizeof(int),
					src += sizeof(int),
					len -= sizeof(int))
			*(unsigned int *)ptr = *(const unsigned int *)src;
		for (; len != 0; ptr++, src++, len--)
			*(unsigned char *)ptr =
				*(const unsigned char *)src;
		atomic_thread_fence(memory_order_seq_cst);
	}
	return retlen;
}

int metal_io_block_set(struct metal_io_region *io, unsigned long offset,
	       unsigned char value, int len)
{
	void *ptr = metal_io_virt(io, offset);
	int retlen = len;

	if (offset > io->size)
		return -ERANGE;
	if ((offset + len) > io->size)
		len = io->size - offset;
	retlen = len;
	if (io->ops.block_set) {
		(*io->ops.block_set)(
			io, offset, value, memory_order_seq_cst, len);
	} else {
		unsigned int cint = value;
		unsigned int i;

		for (i = 1; i < sizeof(int); i++)
			cint |= ((unsigned int)value << (8 * i));

		for (; len && ((uintptr_t)ptr % sizeof(int)); ptr++, len--)
			*(unsigned char *)ptr = (unsigned char) value;
		for (; len >= (int)sizeof(int); ptr += sizeof(int),
						len -= sizeof(int))
			*(unsigned int *)ptr = cint;
		for (; len != 0; ptr++, len--)
			*(unsigned char *)ptr = (unsigned char) value;
		atomic_thread_fence(memory_order_seq_cst);
	}
	return retlen;
}

