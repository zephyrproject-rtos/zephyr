/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/debug_str.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

/**
 * @brief Architecture specific function for relocating partially linked (static) elf
 *
 * Elf files contain a series of relocations described in a section. These relocation
 * instructions are architecture specific and each architecture supporting extensions
 * must implement this.
 *
 * The relocation codes for arm are well documented
 * https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst#relocation
 */
void arch_elf_relocate(elf_rela_t *rel, uintptr_t opaddr, uintptr_t opval)
{
	elf_word reloc_type = ELF32_R_TYPE(rel->r_info);

	switch (reloc_type) {
	case R_ARM_ABS32:
		/* Update the absolute address of a load/store instruction */
		*((uint32_t *)opaddr) = (uint32_t)opval;
		break;
	default:
		LOG_DBG("Unsupported ARM elf relocation type %d at address %lx",
			reloc_type, opaddr);
		break;
	}
}

#ifdef CONFIG_LLEXT_DEBUG_STRINGS

/**
 * @brief Architecture specific function for printing relocation type
 */

const char *arch_r_type_str(unsigned int r_type)
{
	static char num_buf[12];

	switch (r_type) {
	case R_ARM_ABS32:
		return "R_ARM_ABS32";
	default:
		/* not found, return the number */
		sprintf(num_buf, "%d", r_type);
		return num_buf;
	}
}

#endif
