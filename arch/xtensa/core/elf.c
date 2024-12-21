/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/llext_internal.h>
#include <zephyr/llext/loader.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

/*
 * ELF relocation tables on Xtensa contain relocations of different types. They
 * specify how the relocation should be performed. Which relocations are used
 * depends on the type of the ELF object (e.g. shared or partially linked
 * object), structure of the object (single or multiple source files), compiler
 * flags used (e.g. -fPIC), etc. Also not all relocation table entries should be
 * acted upon. Some of them describe relocations that have already been
 * resolved by the linker. We have to distinguish them from actionable
 * relocations and only need to handle the latter ones.
 */
#define R_XTENSA_NONE           0
#define R_XTENSA_32             1
#define R_XTENSA_RTLD           2
#define R_XTENSA_GLOB_DAT       3
#define R_XTENSA_JMP_SLOT       4
#define R_XTENSA_RELATIVE       5
#define R_XTENSA_PLT            6
#define R_XTENSA_ASM_EXPAND	11
#define R_XTENSA_SLOT0_OP	20

static void xtensa_elf_relocate(struct llext_loader *ldr, struct llext *ext,
				const elf_rela_t *rel, uintptr_t addr,
				uint8_t *loc, int type, uint32_t stb)
{
	elf_word *got_entry = (elf_word *)loc;

	switch (type) {
	case R_XTENSA_RELATIVE:
		;
		/* Relocate a local symbol: Xtensa specific. Seems to only be used with PIC */
		unsigned int sh_ndx;

		for (sh_ndx = 0; sh_ndx < ext->sect_cnt; sh_ndx++) {
			if (ext->sect_hdrs[sh_ndx].sh_addr <= *got_entry &&
			    *got_entry <
			    ext->sect_hdrs[sh_ndx].sh_addr + ext->sect_hdrs[sh_ndx].sh_size)
				break;
		}

		if (sh_ndx == ext->sect_cnt) {
			LOG_ERR("%#x not found in any of the sections", *got_entry);
			return;
		}

		*got_entry += (uintptr_t)llext_loaded_sect_ptr(ldr, ext, sh_ndx) -
			ext->sect_hdrs[sh_ndx].sh_addr;
		break;
	case R_XTENSA_GLOB_DAT:
	case R_XTENSA_JMP_SLOT:
		if (stb == STB_GLOBAL) {
			*got_entry = addr;
		}
		break;
	case R_XTENSA_32:
		/* Used for both LOCAL and GLOBAL bindings */
		*got_entry += addr;
		break;
	case R_XTENSA_SLOT0_OP:
		/* Apparently only actionable with LOCAL bindings */
		;
		elf_sym_t rsym;
		int ret = llext_seek(ldr, ldr->sects[LLEXT_MEM_SYMTAB].sh_offset +
				     ELF_R_SYM(rel->r_info) * sizeof(elf_sym_t));

		if (!ret) {
			ret = llext_read(ldr, &rsym, sizeof(elf_sym_t));
		}
		if (ret) {
			LOG_ERR("Failed to read a symbol table entry, LLEXT linking might fail.");
			return;
		}

		/*
		 * So far in all observed use-cases
		 * llext_loaded_sect_ptr(ldr, ext, rsym.st_shndx) was already
		 * available as the "addr" argument of this function, supplied
		 * by arch_elf_relocate_local() from its non-STT_SECTION branch.
		 */
		uintptr_t link_addr = (uintptr_t)llext_loaded_sect_ptr(ldr, ext, rsym.st_shndx) +
			rsym.st_value + rel->r_addend;
		ssize_t value = (link_addr - (((uintptr_t)got_entry + 3) & ~3)) >> 2;

		/* Check the opcode */
		if ((loc[0] & 0xf) == 1 && !loc[1] && !loc[2]) {
			/* L32R: low nibble is 1 */
			loc[1] = value & 0xff;
			loc[2] = (value >> 8) & 0xff;
		} else if ((loc[0] & 0xf) == 5 && !(loc[0] & 0xc0) && !loc[1] && !loc[2]) {
			/* CALLn: low nibble is 5 */
			loc[0] = (loc[0] & 0x3f) | ((value << 6) & 0xc0);
			loc[1] = (value >> 2) & 0xff;
			loc[2] = (value >> 10) & 0xff;
		} else {
			LOG_DBG("%p: unhandled OPC or no relocation %02x%02x%02x inf %#x offs %#x",
				(void *)loc, loc[2], loc[1], loc[0],
				rel->r_info, rel->r_offset);
			break;
		}

		break;
	case R_XTENSA_ASM_EXPAND:
		/* Nothing to do */
		break;
	default:
		LOG_DBG("Unsupported relocation type %u", type);

		return;
	}

	LOG_DBG("Applied relocation to %#x type %u at %p",
		*(uint32_t *)((uintptr_t)got_entry & ~3), type, (void *)got_entry);
}

/**
 * @brief Architecture specific function for STB_LOCAL ELF relocations
 */
void arch_elf_relocate_local(struct llext_loader *ldr, struct llext *ext, const elf_rela_t *rel,
			     const elf_sym_t *sym, uint8_t *rel_addr,
			     const struct llext_load_param *ldr_parm)
{
	int type = ELF32_R_TYPE(rel->r_info);
	uintptr_t sh_addr;

	if (ELF_ST_TYPE(sym->st_info) == STT_SECTION) {
		elf_shdr_t *shdr = ext->sect_hdrs + sym->st_shndx;

		/* shdr->sh_addr is NULL when not built for a specific address */
		sh_addr = shdr->sh_addr &&
			(!ldr_parm->section_detached || !ldr_parm->section_detached(shdr)) ?
			shdr->sh_addr : (uintptr_t)llext_loaded_sect_ptr(ldr, ext, sym->st_shndx);
	} else {
		sh_addr = ldr->sects[LLEXT_MEM_TEXT].sh_addr;
	}

	xtensa_elf_relocate(ldr, ext, rel, sh_addr, rel_addr, type, ELF_ST_BIND(sym->st_info));
}

/**
 * @brief Architecture specific function for STB_GLOBAL ELF relocations
 */
void arch_elf_relocate_global(struct llext_loader *ldr, struct llext *ext, const elf_rela_t *rel,
			      const elf_sym_t *sym, uint8_t *rel_addr, const void *link_addr)
{
	int type = ELF32_R_TYPE(rel->r_info);

	/* For global relocations we expect the initial value for R_XTENSA_RELATIVE to be zero */
	if (type == R_XTENSA_RELATIVE && *(elf_word *)rel_addr) {
		LOG_WRN("global: non-zero relative value %#x", *(elf_word *)rel_addr);
	}

	xtensa_elf_relocate(ldr, ext, rel, (uintptr_t)link_addr, rel_addr, type,
			    ELF_ST_BIND(sym->st_info));
}
