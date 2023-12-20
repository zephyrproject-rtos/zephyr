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
void arch_elf_relocate(elf_rela_t *rel, uintptr_t opaddr, uintptr_t opval)
{
	int type = ELF32_R_TYPE(rel->r_info);

	switch(type) {
		case R_XTENSA_GLOB_DAT:
		case R_XTENSA_JMP_SLOT:
		case R_XTENSA_RELATIVE: {
			/* Update the absolute address */
			*((elf_word *)opaddr) = opval;
		}
		break;
	default:
		LOG_DBG("relocation type %u not implemented", type);
		break;
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
