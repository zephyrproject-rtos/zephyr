/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <metal/io.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT zephyr_addr_translation

#define LOG_MODULE_NAME addr_translation
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* No more than 1 instance should be used */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error "Only one node with zephyr_addr_translation compatible should be used"
#endif

#define TRANSLATION_REG_ADDRESS(idx, inst) DT_INST_REG_ADDR_BY_IDX(inst, idx)
/* Array of drv physical address of each of the pages in the I/O region */
static const metal_phys_addr_t physmap[] = {
	LISTIFY(DT_NUM_REGS(DT_DRV_INST(0)), TRANSLATION_REG_ADDRESS, (,), 0)
};

static metal_phys_addr_t translate_offset_to_phys(struct metal_io_region *io,
						  unsigned long offset)
{
	unsigned long page = (io->page_shift >= sizeof(offset) * CHAR_BIT ?
			     0 : offset >> io->page_shift);
	return (offset < io->size
		? physmap[page] + (offset & io->page_mask)
		: METAL_BAD_PHYS);
}

static unsigned long translate_phys_to_offset(struct metal_io_region *io,
					      metal_phys_addr_t phys)
{
	unsigned long offset =
		(io->page_mask == (metal_phys_addr_t)(-1) ?
		phys - physmap[0] :  phys & io->page_mask);
	do {
		if (translate_offset_to_phys(io, offset) == phys) {
			return offset;
		}
		offset += io->page_mask + 1;
	} while (offset < io->size);

	return METAL_BAD_OFFSET;
}

static const struct metal_io_ops openamp_addr_translation_ops = {
	.phys_to_offset = translate_phys_to_offset,
	.offset_to_phys = translate_offset_to_phys,
};

#define OPENAMP_ADDR_TRANSLATION_INIT(idx)                                    \
	DEVICE_DT_INST_DEFINE(idx, NULL,                                      \
			      NULL, NULL, NULL,                               \
			      POST_KERNEL,                                    \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,            \
			      &openamp_addr_translation_ops,                  \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(OPENAMP_ADDR_TRANSLATION_INIT)
