/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <elfloader.h>

#ifdef CONFIG_64BIT

/* x86 64-bit relocation for DYN file */
void elfloader_arch_relocate_dyn(zmodule_t *module, elf_rel_t *rel,
		elf_addr sym_addr)
{
}

/* x86 64-bit relocation for REL file */
void elfloader_arch_relocate_rel(elf_rel_t *rel)
{
}

#else

/* x86 32-bit relocation for DYN file */
void elfloader_arch_relocate_dyn(zmodule_t *module, elf_rel_t *rel,
		elf_addr sym_addr)
{
	elf_addr *where;

	where = (elf_addr *)(module->load_start_addr
			+ rel->r_offset - module->virt_start_addr);
	switch (ELF32_R_TYPE(rel->r_info)) {
	case R_386_GLOB_DAT:
	case R_386_JUMP_SLOT:
		*where = sym_addr;
		break;
	case R_386_RELATIVE:
		*where = sym_addr + *where;
		break;
	default:
		/* unsupported relocation type */
		break;
	}
}

/* x86 32-bit relocation for REL file */
void elfloader_arch_relocate_rel(elf_rel_t *rel, elf_shdr_t *apply_sec,
		elf_addr sym_addr)
{
	elf_addr *where;

	where = (elf_addr *)(apply_sec->sh_addr + rel->r_offset);
	switch (ELF32_R_TYPE(rel->r_info)) {
	case R_386_32:
		*where = sym_addr;
		break;
	case R_386_PC32:
		*where = sym_addr - (elf_addr)where;
		break;
	default:
		/* unsupported relocation type */
		break;
	}
}

#endif
