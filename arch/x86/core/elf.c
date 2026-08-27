/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/llext_internal.h>
#include <zephyr/llext/loader.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

#ifdef CONFIG_64BIT
#define R_X86_64_64         1
#define R_X86_64_PC32       2
#define R_X86_64_PLT32      4
#define R_X86_64_32        10
#define R_X86_64_32S       11

/**
 * @brief Architecture specific function for relocating shared elf
 *
 * Elf files contain a series of relocations described in multiple sections.
 * These relocation instructions are architecture specific and each architecture
 * supporting modules must implement this.
 *
 * The relocation codes are well documented:
 *
 * https://refspecs.linuxfoundation.org/elf/x86_64-abi-0.95.pdf (intel64)
 */
int arch_elf_relocate(struct llext_loader *ldr, struct llext *ext, elf_rela_t *rel,
	const elf_shdr_t *shdr)
{
	int ret = 0;
	const uintptr_t loc = llext_get_reloc_instruction_location(ldr, ext, shdr->sh_info, rel);
	elf_sym_t sym;
	uintptr_t sym_base_addr;
	const char *sym_name;

	ret = llext_read_symbol(ldr, ext, rel, &sym);

	if (ret != 0) {
		LOG_ERR("Could not read symbol from binary!");
		return ret;
	}

	sym_name = llext_symbol_name(ldr, ext, &sym);

	ret = llext_lookup_symbol(ldr, ext, &sym_base_addr, rel, &sym, sym_name, shdr);

	if (ret != 0) {
		LOG_ERR("Could not find symbol %s!", sym_name);
		return ret;
	}

	sym_base_addr += rel->r_addend;

	int reloc_type = ELF32_R_TYPE(rel->r_info);

	switch (reloc_type) {
	case R_X86_64_PC32:
	case R_X86_64_PLT32:
		*(uint32_t *)loc = sym_base_addr - loc;
		break;
	case R_X86_64_64:
	case R_X86_64_32:
	case R_X86_64_32S:
		*(uint32_t *)loc = sym_base_addr;
		break;
	default:
		LOG_ERR("unknown relocation: %u\n", reloc_type);
		ret = -ENOEXEC;
		break;
	}

	return ret;
}
#else
#define R_386_32           1
#define R_286_PC32         2

/**
 * @brief Architecture specific function for relocating shared elf
 *
 * Elf files contain a series of relocations described in multiple sections.
 * These relocation instructions are architecture specific and each architecture
 * supporting modules must implement this.
 *
 * The relocation codes are well documented:
 *
 * https://docs.oracle.com/cd/E19683-01/817-3677/chapter6-26/index.html (ia32)
 */
int arch_elf_relocate(struct llext_loader *ldr, struct llext *ext, elf_rela_t *rel,
	const elf_shdr_t *shdr)
{
	int ret = 0;
	const uintptr_t loc = llext_get_reloc_instruction_location(ldr, ext, shdr->sh_info, rel);
	elf_sym_t sym;
	uintptr_t sym_base_addr;
	const char *sym_name;

	/* x86 uses elf_rel_t records with no addends */
	uintptr_t addend = *(uintptr_t *)loc;

	ret = llext_read_symbol(ldr, ext, rel, &sym);

	if (ret != 0) {
		LOG_ERR("Could not read symbol from binary!");
		return ret;
	}

	sym_name = llext_symbol_name(ldr, ext, &sym);

	ret = llext_lookup_symbol(ldr, ext, &sym_base_addr, rel, &sym, sym_name, shdr);

	if (ret != 0) {
		LOG_ERR("Could not find symbol %s!", sym_name);
		return ret;
	}

	sym_base_addr += addend;

	int reloc_type = ELF32_R_TYPE(rel->r_info);

	switch (reloc_type) {
	case R_386_32:
		*(uint32_t *)loc = sym_base_addr;
		break;
	case R_286_PC32:
		*(uint32_t *)loc = sym_base_addr - loc;
		break;
	default:
		LOG_ERR("unknown relocation: %u\n", reloc_type);
		ret = -ENOEXEC;
		break;
	}

	return ret;
}
#endif
