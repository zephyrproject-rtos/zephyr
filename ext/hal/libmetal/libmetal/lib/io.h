/*
 * Copyright (c) 2015 - 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	io.h
 * @brief	I/O access primitives for libmetal.
 */

#ifndef __METAL_IO__H__
#define __METAL_IO__H__

#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <metal/assert.h>
#include <metal/compiler.h>
#include <metal/atomic.h>
#include <metal/sys.h>
#include <metal/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup io IO Interfaces
 *  @{ */

#ifdef __MICROBLAZE__
#define NO_ATOMIC_64_SUPPORT
#endif

struct metal_io_region;

/** Generic I/O operations. */
struct metal_io_ops {
	uint64_t	(*read)(struct metal_io_region *io,
				unsigned long offset,
				memory_order order,
				int width);
	void		(*write)(struct metal_io_region *io,
				 unsigned long offset,
				 uint64_t value,
				 memory_order order,
				 int width);
	int		(*block_read)(struct metal_io_region *io,
				unsigned long offset,
				void *restrict dst,
				memory_order order,
				int len);
	int		(*block_write)(struct metal_io_region *io,
				 unsigned long offset,
				 const void *restrict src,
				 memory_order order,
				 int len);
	void		(*block_set)(struct metal_io_region *io,
				 unsigned long offset,
				 unsigned char value,
				 memory_order order,
				 int len);
	void		(*close)(struct metal_io_region *io);
};

/** Libmetal I/O region structure. */
struct metal_io_region {
	void			*virt;      /**< base virtual address */
	const metal_phys_addr_t	*physmap;   /**< table of base physical address
                                                 of each of the pages in the I/O
                                                 region */
	size_t			size;       /**< size of the I/O region */
	unsigned long		page_shift; /**< page shift of I/O region */
	metal_phys_addr_t	page_mask;  /**< page mask of I/O region */
	unsigned int		mem_flags;  /**< memory attribute of the
						 I/O region */
	struct metal_io_ops	ops;        /**< I/O region operations */
};

/**
 * @brief	Open a libmetal I/O region.
 *
 * @param[in, out]	io		I/O region handle.
 * @param[in]		virt		Virtual address of region.
 * @param[in]		physmap		Array of physical addresses per page.
 * @param[in]		size		Size of region.
 * @param[in]		page_shift	Log2 of page size (-1 for single page).
 * @param[in]		mem_flags	Memory flags
 * @param[in]		ops			ops
 */
void
metal_io_init(struct metal_io_region *io, void *virt,
	      const metal_phys_addr_t *physmap, size_t size,
	      unsigned page_shift, unsigned int mem_flags,
	      const struct metal_io_ops *ops);

/**
 * @brief	Close a libmetal shared memory segment.
 * @param[in]	io	I/O region handle.
 */
static inline void metal_io_finish(struct metal_io_region *io)
{
	if (io->ops.close)
		(*io->ops.close)(io);
	memset(io, 0, sizeof(*io));
}

/**
 * @brief	Get size of I/O region.
 *
 * @param[in]	io	I/O region handle.
 * @return	Size of I/O region.
 */
static inline size_t metal_io_region_size(struct metal_io_region *io)
{
	return io->size;
}

/**
 * @brief	Get virtual address for a given offset into the I/O region.
 * @param[in]	io	I/O region handle.
 * @param[in]	offset	Offset into shared memory segment.
 * @return	NULL if offset is out of range, or pointer to offset.
 */
static inline void *
metal_io_virt(struct metal_io_region *io, unsigned long offset)
{
	return (io->virt != METAL_BAD_VA && offset <= io->size
		? (uint8_t *)io->virt + offset
		: NULL);
}

/**
 * @brief	Convert a virtual address to offset within I/O region.
 * @param[in]	io	I/O region handle.
 * @param[in]	virt	Virtual address within segment.
 * @return	METAL_BAD_OFFSET if out of range, or offset.
 */
static inline unsigned long
metal_io_virt_to_offset(struct metal_io_region *io, void *virt)
{
	size_t offset = (uint8_t *)virt - (uint8_t *)io->virt;
	return (offset < io->size ? offset : METAL_BAD_OFFSET);
}

/**
 * @brief	Get physical address for a given offset into the I/O region.
 * @param[in]	io	I/O region handle.
 * @param[in]	offset	Offset into shared memory segment.
 * @return	METAL_BAD_PHYS if offset is out of range, or physical address
 *		of offset.
 */
static inline metal_phys_addr_t
metal_io_phys(struct metal_io_region *io, unsigned long offset)
{
	unsigned long page = (io->page_shift >=
			     sizeof(offset) * CHAR_BIT ?
			     0 : offset >> io->page_shift);
	return (io->physmap != NULL && offset <= io->size
		? io->physmap[page] + (offset & io->page_mask)
		: METAL_BAD_PHYS);
}

/**
 * @brief	Convert a physical address to offset within I/O region.
 * @param[in]	io	I/O region handle.
 * @param[in]	phys	Physical address within segment.
 * @return	METAL_BAD_OFFSET if out of range, or offset.
 */
static inline unsigned long
metal_io_phys_to_offset(struct metal_io_region *io, metal_phys_addr_t phys)
{
	unsigned long offset =
		(io->page_mask == (metal_phys_addr_t)(-1) ?
		phys - io->physmap[0] :  phys & io->page_mask);
	do {
		if (metal_io_phys(io, offset) == phys)
			return offset;
		offset += io->page_mask + 1;
	} while (offset < io->size);
	return METAL_BAD_OFFSET;
}

/**
 * @brief	Convert a physical address to virtual address.
 * @param[in]	io	Shared memory segment handle.
 * @param[in]	phys	Physical address within segment.
 * @return	NULL if out of range, or corresponding virtual address.
 */
static inline void *
metal_io_phys_to_virt(struct metal_io_region *io, metal_phys_addr_t phys)
{
	return metal_io_virt(io, metal_io_phys_to_offset(io, phys));
}

/**
 * @brief	Convert a virtual address to physical address.
 * @param[in]	io	Shared memory segment handle.
 * @param[in]	virt	Virtual address within segment.
 * @return	METAL_BAD_PHYS if out of range, or corresponding
 *		physical address.
 */
static inline metal_phys_addr_t
metal_io_virt_to_phys(struct metal_io_region *io, void *virt)
{
	return metal_io_phys(io, metal_io_virt_to_offset(io, virt));
}

/**
 * @brief	Read a value from an I/O region.
 * @param[in]	io	I/O region handle.
 * @param[in]	offset	Offset into I/O region.
 * @param[in]	order	Memory ordering.
 * @param[in]	width	Width in bytes of datatype to read.  This must be 1, 2,
 *			4, or 8, and a compile time constant for this function
 *			to inline cleanly.
 * @return	Value.
 */
static inline uint64_t
metal_io_read(struct metal_io_region *io, unsigned long offset,
	      memory_order order, int width)
{
	void *ptr = metal_io_virt(io, offset);

	if (io->ops.read)
		return (*io->ops.read)(io, offset, order, width);
	else if (ptr && sizeof(atomic_uchar) == width)
		return atomic_load_explicit((atomic_uchar *)ptr, order);
	else if (ptr && sizeof(atomic_ushort) == width)
		return atomic_load_explicit((atomic_ushort *)ptr, order);
	else if (ptr && sizeof(atomic_uint) == width)
		return atomic_load_explicit((atomic_uint *)ptr, order);
	else if (ptr && sizeof(atomic_ulong) == width)
		return atomic_load_explicit((atomic_ulong *)ptr, order);
#ifndef NO_ATOMIC_64_SUPPORT
	else if (ptr && sizeof(atomic_ullong) == width)
		return atomic_load_explicit((atomic_ullong *)ptr, order);
#endif
	metal_assert(0);
	return 0; /* quiet compiler */
}

/**
 * @brief	Write a value into an I/O region.
 * @param[in]	io	I/O region handle.
 * @param[in]	offset	Offset into I/O region.
 * @param[in]	value	Value to write.
 * @param[in]	order	Memory ordering.
 * @param[in]	width	Width in bytes of datatype to read.  This must be 1, 2,
 *			4, or 8, and a compile time constant for this function
 *			to inline cleanly.
 */
static inline void
metal_io_write(struct metal_io_region *io, unsigned long offset,
	       uint64_t value, memory_order order, int width)
{
	void *ptr = metal_io_virt(io, offset);
	if (io->ops.write)
		(*io->ops.write)(io, offset, value, order, width);
	else if (ptr && sizeof(atomic_uchar) == width)
		atomic_store_explicit((atomic_uchar *)ptr, value, order);
	else if (ptr && sizeof(atomic_ushort) == width)
		atomic_store_explicit((atomic_ushort *)ptr, value, order);
	else if (ptr && sizeof(atomic_uint) == width)
		atomic_store_explicit((atomic_uint *)ptr, value, order);
	else if (ptr && sizeof(atomic_ulong) == width)
		atomic_store_explicit((atomic_ulong *)ptr, value, order);
#ifndef NO_ATOMIC_64_SUPPORT
	else if (ptr && sizeof(atomic_ullong) == width)
		atomic_store_explicit((atomic_ullong *)ptr, value, order);
#endif
	else
		metal_assert (0);
}

#define metal_io_read8_explicit(_io, _ofs, _order)			\
	metal_io_read((_io), (_ofs), (_order), 1)
#define metal_io_read8(_io, _ofs)					\
	metal_io_read((_io), (_ofs), memory_order_seq_cst, 1)
#define metal_io_write8_explicit(_io, _ofs, _val, _order)		\
	metal_io_write((_io), (_ofs), (_val), (_order), 1)
#define metal_io_write8(_io, _ofs, _val)				\
	metal_io_write((_io), (_ofs), (_val), memory_order_seq_cst, 1)

#define metal_io_read16_explicit(_io, _ofs, _order)			\
	metal_io_read((_io), (_ofs), (_order), 2)
#define metal_io_read16(_io, _ofs)					\
	metal_io_read((_io), (_ofs), memory_order_seq_cst, 2)
#define metal_io_write16_explicit(_io, _ofs, _val, _order)		\
	metal_io_write((_io), (_ofs), (_val), (_order), 2)
#define metal_io_write16(_io, _ofs, _val)				\
	metal_io_write((_io), (_ofs), (_val), memory_order_seq_cst, 2)

#define metal_io_read32_explicit(_io, _ofs, _order)			\
	metal_io_read((_io), (_ofs), (_order), 4)
#define metal_io_read32(_io, _ofs)					\
	metal_io_read((_io), (_ofs), memory_order_seq_cst, 4)
#define metal_io_write32_explicit(_io, _ofs, _val, _order)		\
	metal_io_write((_io), (_ofs), (_val), (_order), 4)
#define metal_io_write32(_io, _ofs, _val)				\
	metal_io_write((_io), (_ofs), (_val), memory_order_seq_cst, 4)

#define metal_io_read64_explicit(_io, _ofs, _order)			\
	metal_io_read((_io), (_ofs), (_order), 8)
#define metal_io_read64(_io, _ofs)					\
	metal_io_read((_io), (_ofs), memory_order_seq_cst, 8)
#define metal_io_write64_explicit(_io, _ofs, _val, _order)		\
	metal_io_write((_io), (_ofs), (_val), (_order), 8)
#define metal_io_write64(_io, _ofs, _val)				\
	metal_io_write((_io), (_ofs), (_val), memory_order_seq_cst, 8)

/**
 * @brief	Read a block from an I/O region.
 * @param[in]	io	I/O region handle.
 * @param[in]	offset	Offset into I/O region.
 * @param[in]	dst	destination to store the read data.
 * @param[in]	len	length in bytes to read.
 * @return      On success, number of bytes read. On failure, negative value
 */
int metal_io_block_read(struct metal_io_region *io, unsigned long offset,
	       void *restrict dst, int len);

/**
 * @brief	Write a block into an I/O region.
 * @param[in]	io	I/O region handle.
 * @param[in]	offset	Offset into I/O region.
 * @param[in]	src	source to write.
 * @param[in]	len	length in bytes to write.
 * @return      On success, number of bytes written. On failure, negative value
 */
int metal_io_block_write(struct metal_io_region *io, unsigned long offset,
	       const void *restrict src, int len);

/**
 * @brief	fill a block of an I/O region.
 * @param[in]	io	I/O region handle.
 * @param[in]	offset	Offset into I/O region.
 * @param[in]	value	value to fill into the block
 * @param[in]	len	length in bytes to fill.
 * @return      On success, number of bytes filled. On failure, negative value
 */
int metal_io_block_set(struct metal_io_region *io, unsigned long offset,
	       unsigned char value, int len);

#include <metal/system/@PROJECT_SYSTEM@/io.h>

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_IO__H__ */
