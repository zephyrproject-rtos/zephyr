/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/loader.h>
#include <zephyr/logging/log.h>

#include <zephyr/arch/arc/elf.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

/**
 * @brief Architecture specific function for relocating shared elf
 *
 * Elf files contain a series of relocations described in multiple sections.
 * These relocation instructions are architecture specific and each architecture
 * supporting modules must implement this.
 */
int arch_elf_relocate(elf_rela_t *rel, uintptr_t loc, uintptr_t sym_base_addr, const char *sym_name,
		      uintptr_t load_bias)
{
	int ret = 0;

	int reloc_type = ELF32_R_TYPE(rel->r_info);

	switch (reloc_type) {
	case R_ARC_32:
		*(uint32_t *)loc = sym_base_addr;
		break;
	case R_ARC_32_ME:
		uint16_t *upper = (uint16_t *)loc;
		uint16_t *lower = (uint16_t *)upper + 1;
		*upper = (sym_base_addr >> 16);
		*lower = sym_base_addr;
		break;
	default:
		LOG_ERR("unknown relocation: %u\n", reloc_type);
		ret = -ENOEXEC;
		break;
	}

	return ret;
}
