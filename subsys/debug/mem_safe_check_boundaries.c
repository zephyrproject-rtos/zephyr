/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file safe memory access routines, software implementation that verifies
 *       accesses are within memory region boundaries.
 *
 * @details See debug/Kconfig and the "Safe memory access" help for details.
 */

#include <kernel.h>
#include <init.h>
#include <errno.h>
#include <toolchain.h>
#include <linker/linker-defs.h>
#include <misc/util.h>
#include <debug/mem_safe.h>
#include <string.h>

#define NUM_REGIONS (CONFIG_MEM_SAFE_NUM_EXTRA_REGIONS + 2)

/*
 * The table of regions has the RO regions at the bottom and the RW regions at
 * the top, and regions are added by moving the ro_end/rw_end pointers towards
 * each other. The table is full when the pointers cross, i.e. when ro_end >
 * rw_end.
 */
struct {
	vaddr_t addr;
	vaddr_t last_byte;
} mem_regions[NUM_REGIONS];

#define ro_base 0
#define rw_base (NUM_REGIONS - 1)

static int ro_end = ro_base;
static int rw_end = rw_base;

#define IMAGE_ROM_START  ((vaddr_t)&_image_rom_start)
#define IMAGE_ROM_END    ((vaddr_t)&_image_rom_end)

#define IMAGE_RAM_START  ((vaddr_t)&_image_ram_start)
#define IMAGE_RAM_END    ((vaddr_t)&_image_ram_end)

#define IMAGE_TEXT_START ((vaddr_t)&_image_text_start)
#define IMAGE_TEXT_END   ((vaddr_t)&_image_text_end)

#define VALID_PERMISSION_MASK  0x00000001   /* permissions use only the lsb */

static inline void write_to_mem(void *dest, void *src, int width)
{
	switch (width) {
	case 4:
		*((vaddr_t *)dest) = *((const vaddr_t *)src);
		break;
	case 2:
		*((u16_t *)dest) = *((const u16_t *)src);
		break;
	case 1:
		*((char *)dest) = *((const char *)src);
		break;
	}
}

static inline int is_in_region(int slot, vaddr_t addr, vaddr_t end_addr)
{
	vaddr_t region_start = mem_regions[slot].addr;
	vaddr_t region_last_byte = mem_regions[slot].last_byte;

	return addr >= region_start && end_addr <= region_last_byte;
}

static inline int is_in_a_ro_region(vaddr_t addr, vaddr_t end_addr)
{
	int slot = ro_base;

	while (slot < ro_end && slot <= rw_end) {
		if (is_in_region(slot, addr, end_addr)) {
			return 1;
		}
		++slot;
	}

	return 0;
}

static inline int is_in_a_rw_region(vaddr_t addr, vaddr_t end_addr)
{
	int slot = rw_base;

	while (slot > rw_end && slot >= ro_end) {
		if (is_in_region(slot, addr, end_addr)) {
			return 1;
		}
		--slot;
	}

	return 0;
}

static inline int mem_probe_no_check(void *p, int perm, size_t num_bytes,
				     void *buf)
{
	vaddr_t addr = (vaddr_t)p;
	vaddr_t end_addr = addr + num_bytes - 1;

	int is_in_rw = is_in_a_rw_region(addr, end_addr);
	int is_in_ro = is_in_a_ro_region(addr, end_addr);

	int valid_mem;
	void *src, *dest;

	if (perm == SYS_MEM_SAFE_READ) {
		dest = buf; src = p;
		valid_mem = is_in_rw || is_in_ro;
	} else {
		dest = p; src = buf;
		valid_mem = is_in_rw;
	}

	if (likely(valid_mem)) {
		write_to_mem(dest, src, num_bytes);
		return 0;
	}
	return -EFAULT;
}

static inline int is_perm_valid(int perm)
{
	return !(perm & ~VALID_PERMISSION_MASK);
}

static inline int is_num_bytes_valid(size_t num_bytes)
{
	return is_power_of_two(num_bytes) && num_bytes <= sizeof(vaddr_t);
}

int _mem_probe(void *p, int perm, size_t num_bytes, void *buf)
{
	if (unlikely(!is_perm_valid(perm))) {
		return -EINVAL;
	}

	if (unlikely(!is_num_bytes_valid(num_bytes))) {
		return -EINVAL;
	}

	return mem_probe_no_check(p, perm, num_bytes, buf);
}

static inline int mem_access(void *p, void *buf, size_t num_bytes,
			     int len, int perm)
{
	char *p_char = p, *buf_char = buf, *p_end = ((char *)p + len);

	while (p_char < p_end) {
		int error = mem_probe_no_check(p_char, perm, num_bytes, buf_char);

		if (unlikely(error < 0)) {
			return error;
		}
		p_char += num_bytes;
		buf_char += num_bytes;
	}

	return 0;
}

static inline int get_align(const u32_t value)
{
	return (value & 1) ? 1 : (value & 2) ? 2 : 4;
}

static inline int get_width(const void *p1, const void *p2,
			    size_t num_bytes, int width)
{
	vaddr_t p1_addr = (vaddr_t)p1, p2_addr = (vaddr_t)p2;

	if (width == 0) {
		u32_t align_check = num_bytes | p1_addr | p2_addr;

		return get_align(align_check);
	}

	if (unlikely(p1_addr & (width - 1) || num_bytes & (width - 1))) {
		return -EINVAL;
	}

	return width;
}

int _mem_safe_read(void *src, char *buf, size_t num_bytes, int width)
{
	width = get_width(src, buf, num_bytes, width);
	return unlikely(width < 0) ? -EINVAL :
	       mem_access(src, buf, width, num_bytes, SYS_MEM_SAFE_READ);
}

int _mem_safe_write(void *dest, char *buf, size_t num_bytes, int width)
{
	width = get_width(dest, buf, num_bytes, width);
	return unlikely(width < 0) ? -EINVAL :
	       mem_access(dest, buf, width, num_bytes, SYS_MEM_SAFE_WRITE);
}

#if defined(CONFIG_XIP)
int _mem_safe_write_to_text_section(void *dest, char *buf, size_t num_bytes)
{
	ARG_UNUSED(dest);
	ARG_UNUSED(buf);
	ARG_UNUSED(num_bytes);

	/* cannot write to text section when it's in ROM */
	return -EFAULT;
}
#else
int _mem_safe_write_to_text_section(void *dest, char *buf, size_t num_bytes)
{
	vaddr_t v = (vaddr_t)dest;
	int is_in_text = ((v >= IMAGE_TEXT_START) &&
			  ((v + num_bytes) <= IMAGE_TEXT_END));

	if (unlikely(!is_in_text)) {
		return -EFAULT;
	}

	memcpy(dest, buf, num_bytes);
	return 0;
}
#endif /* CONFIG_XIP */

int _mem_safe_region_add(void *addr, size_t num_bytes, int perm)
{
	if (unlikely(!is_perm_valid(perm))) {
		return -EINVAL;
	}

	int slot;
	int key = irq_lock();

	if (unlikely(ro_end > rw_end)) {
		irq_unlock(key);
		return -ENOMEM;
	}

	if (perm == SYS_MEM_SAFE_WRITE) {
		slot = rw_end;
		--rw_end;
	} else {
		slot = ro_end;
		++ro_end;
	}

	mem_regions[slot].addr = (vaddr_t)addr;
	mem_regions[slot].last_byte = mem_regions[slot].addr + num_bytes - 1;

	irq_unlock(key);

	return 0;
}

static int init(struct device *unused)
{
	void *addr;
	size_t num_bytes;

	ARG_UNUSED(unused);

	addr = (void *)IMAGE_ROM_START;
	num_bytes = (int)(IMAGE_ROM_END - IMAGE_ROM_START);
	(void)_mem_safe_region_add(addr, num_bytes, SYS_MEM_SAFE_READ);

	addr = (void *)IMAGE_RAM_START;
	num_bytes = (int)(IMAGE_RAM_END - IMAGE_RAM_START);
	(void)_mem_safe_region_add(addr, num_bytes, SYS_MEM_SAFE_WRITE);

	return 0;
}

SYS_INIT(init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
