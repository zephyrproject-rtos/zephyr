/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modules/elf.h>
#include <zephyr/modules/module.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(modules);

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
void arch_elf_relocate_local(struct module_stream *ms, elf_rela_t *rel, size_t got_offset)
{
	uint8_t *text = module_peek(ms, ms->sects[MOD_SECT_TEXT].sh_offset);
	int type = ELF32_R_TYPE(rel->r_info);

	if (type == R_XTENSA_RELATIVE) {
		size_t ptr_offset = *(size_t *)(text + got_offset);

		LOG_DBG("relocation type %u offset %#x value %#x",
			type, got_offset, ptr_offset);

		/* Relocate a local symbol: Xtensa specific */
		*(uintptr_t *)(text + got_offset) = (uintptr_t)(text + ptr_offset -
								ms->sects[MOD_SECT_TEXT].sh_addr);
	}
}
