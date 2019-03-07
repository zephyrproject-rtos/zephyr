/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/cache.h>
#include <metal/io.h>
#include <nuttx/arch.h>

static uint64_t metal_io_read_(struct metal_io_region *io,
			       unsigned long offset,
			       memory_order order,
			       int width)
{
	uint64_t value = 0;

	metal_io_block_read(io, offset, &value, width);
	return value;
}

static void metal_io_write_(struct metal_io_region *io,
			    unsigned long offset,
			    uint64_t value,
			    memory_order order,
			    int width)
{
	metal_io_block_write(io, offset, &value, width);
}

static int metal_io_block_read_(struct metal_io_region *io,
				unsigned long offset,
				void *restrict dst,
				memory_order order,
				int len)
{
	void *va = metal_io_virt(io, offset);

	metal_cache_invalidate(va, len);
	memcpy(dst, va, len);

	return len;
}

static int metal_io_block_write_(struct metal_io_region *io,
				 unsigned long offset,
				 const void *restrict src,
				 memory_order order,
				 int len)
{
	void *va = metal_io_virt(io, offset);

	memcpy(va, src, len);
	metal_cache_flush(va, len);

	return len;
}

static void metal_io_block_set_(struct metal_io_region *io,
				unsigned long offset,
				unsigned char value,
				memory_order order,
				int len)
{
	void *va = metal_io_virt(io, offset);

	memset(va, value, len);
	metal_cache_flush(va, len);
}

static void metal_io_close_(struct metal_io_region *io)
{
}

static metal_phys_addr_t metal_io_offset_to_phys_(struct metal_io_region *io,
						  unsigned long offset)
{
	return up_addrenv_va_to_pa((char *)io->virt + offset);
}

static unsigned long metal_io_phys_to_offset_(struct metal_io_region *io,
					      metal_phys_addr_t phys)
{
	return (char *)up_addrenv_pa_to_va(phys) - (char *)io->virt;
}

static metal_phys_addr_t metal_io_phys_start_ = 0;

static struct metal_io_region metal_io_region_ = {
	.virt = NULL,
	.physmap = &metal_io_phys_start_,
	.size = (size_t)-1,
	.page_shift = sizeof(metal_phys_addr_t) * CHAR_BIT,
	.page_mask = (metal_phys_addr_t)-1,
	.mem_flags = 0,
	.ops = {
		.read = metal_io_read_,
		.write = metal_io_write_,
		.block_read = metal_io_block_read_,
		.block_write = metal_io_block_write_,
		.block_set = metal_io_block_set_,
		.close = metal_io_close_,
		.offset_to_phys = metal_io_offset_to_phys_,
		.phys_to_offset = metal_io_phys_to_offset_,
	},
};

struct metal_io_ops *metal_io_get_ops(void)
{
	return &metal_io_region_.ops;
}

struct metal_io_region *metal_io_get_region(void)
{
	return &metal_io_region_;
}
