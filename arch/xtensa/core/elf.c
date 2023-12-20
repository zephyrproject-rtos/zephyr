/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/loader.h>
#include <zephyr/llext/debug_str.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

#define R_XTENSA_NONE           0
#define R_XTENSA_32             1
#define R_XTENSA_RTLD           2
#define R_XTENSA_GLOB_DAT       3
#define R_XTENSA_JMP_SLOT       4
#define R_XTENSA_RELATIVE       5
#define R_XTENSA_PLT            6

/**
 * @brief Architecture specific function for relocating shared elf
 *
 * Elf files contain a series of relocations described in multiple sections.
 * These relocation instructions are architecture specific and each architecture
 * supporting modules must implement this.
 */
void arch_elf_relocate_local(struct llext_loader *ldr, struct llext *ext,
			     elf_rela_t *rel, size_t got_offset)
{
	uint8_t *text = ext->mem[LLEXT_MEM_TEXT];
	int type = ELF32_R_TYPE(rel->r_info);

	if (type == R_XTENSA_RELATIVE) {
		elf_word ptr_offset = *(elf_word *)(text + got_offset);

		LOG_DBG("relocation type %u offset %#x value %#x",
			type, got_offset, ptr_offset);

#ifdef CONFIG_LLEXT_DEBUG_STRINGS
		LOG_DBG("writing at %s a relocation to addr %s",
			llext_addr_str(ldr, ext, (uintptr_t) (text + got_offset)),
			llext_addr_str(ldr, ext, (uintptr_t) (text + ptr_offset -
							      ldr->sects[LLEXT_MEM_TEXT].sh_addr)));
#else
		LOG_DBG("writing at %p a relocation to addr %p",
			text + got_offset,
			text + ptr_offset - ldr->sects[LLEXT_MEM_TEXT].sh_addr);
#endif

		/* Relocate a local symbol: Xtensa specific */
		*(elf_word *)(text + got_offset) = (elf_word)(text + ptr_offset -
							      ldr->sects[LLEXT_MEM_TEXT].sh_addr);
	} else {
		LOG_DBG("relocation type %u not implemented", type);
	}
}

#ifdef CONFIG_LLEXT_DEBUG_STRINGS

/**
 * @brief Architecture specific function for printing relocation type
 */

const char *arch_r_type_str(unsigned int r_type)
{
	static const char * const R_TYPE_DESC[] = {
		[R_XTENSA_NONE]     = "R_XTENSA_NONE",
		[R_XTENSA_32]       = "R_XTENSA_32",
		[R_XTENSA_RTLD]     = "R_XTENSA_RTLD",
		[R_XTENSA_GLOB_DAT] = "R_XTENSA_GLOB_DAT",
		[R_XTENSA_JMP_SLOT] = "R_XTENSA_JMP_SLOT",
		[R_XTENSA_RELATIVE] = "R_XTENSA_RELATIVE",
		[R_XTENSA_PLT]      = "R_XTENSA_PLT",
	};
	static char num_buf[12];

	if ((r_type < ARRAY_SIZE(R_TYPE_DESC)) && (R_TYPE_DESC[r_type])) {
		return R_TYPE_DESC[r_type];
	}

	/* not found, return the number */
	sprintf(num_buf, "%d", r_type);
	return num_buf;
}

#endif
