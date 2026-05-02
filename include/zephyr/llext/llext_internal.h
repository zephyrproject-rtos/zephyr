/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_INTERNAL_H
#define ZEPHYR_LLEXT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/llext/llext.h>

/**
 * @file
 * @brief Private header for linkable loadable extensions
 */

/** @cond ignore */


struct llext_elf_sect_map {
	enum llext_mem mem_idx;
	size_t offset;
};

const void *llext_loaded_sect_ptr(struct llext_loader *ldr, struct llext *ext, unsigned int sh_ndx);


static inline const char *llext_string(const struct llext_loader *ldr, const struct llext *ext,
	enum llext_mem mem_idx, unsigned int idx)
{
	return (const char *)ext->mem[mem_idx] + idx;
}

static inline uintptr_t llext_get_reloc_instruction_location(struct llext_loader *ldr,
							     struct llext *ext,
							     int shndx,
							     const elf_rela_t *rela)
{
	return (uintptr_t) llext_loaded_sect_ptr(ldr, ext, shndx) + rela->r_offset;
}

static inline const char *llext_section_name(const struct llext_loader *ldr,
					     const struct llext *ext,
					     const elf_shdr_t *shdr)
{
	return llext_string(ldr, ext, LLEXT_MEM_SHSTRTAB, shdr->sh_name);
}

static inline const char *llext_symbol_name(const struct llext_loader *ldr,
					    const struct llext *ext,
					    const elf_sym_t *sym)
{
	if (ELF_ST_TYPE(sym->st_info) == STT_SECTION) {
		return llext_section_name(ldr, ext, ext->sect_hdrs + sym->st_shndx);
	} else {
		return llext_string(ldr, ext, LLEXT_MEM_STRTAB, sym->st_name);
	}
}

/*
 * Determine address of a symbol.
 */
int llext_lookup_symbol(struct llext_loader *ldr, struct llext *ext, uintptr_t *link_addr,
			const elf_rela_t *rel, const elf_sym_t *sym, const char *name,
			const elf_shdr_t *shdr);

/*
 * Read the symbol entry corresponding to a relocation from the binary.
 */
int llext_read_symbol(struct llext_loader *ldr, struct llext *ext, const elf_rela_t *rel,
		      elf_sym_t *sym);

/** @endcond */

/**
 * @brief Architecture specific function for local binding relocations
 *
 * @param[in] loader Extension loader data and context
 * @param[in] ext Extension to call function in
 * @param[in] rel Relocation data provided by elf
 * @param[in] sym Corresponding symbol table entry
 * @param[in] rel_addr Address where relocation should be performed
 * @param[in] ldr_parm Loader parameters
 * @returns 0 on success or a negative error code
 */
int arch_elf_relocate_local(struct llext_loader *loader, struct llext *ext, const elf_rela_t *rel,
			    const elf_sym_t *sym, uint8_t *rel_addr,
			    const struct llext_load_param *ldr_parm);

/**
 * @brief Architecture specific function for global binding relocations
 *
 * @param[in] loader Extension loader data and context
 * @param[in] ext Extension to call function in
 * @param[in] rel Relocation data provided by elf
 * @param[in] sym Corresponding symbol table entry
 * @param[in] rel_addr Address where relocation should be performed
 * @param[in] link_addr target address for table-based relocations
 * @returns 0 on success or a negative error code
 */
int arch_elf_relocate_global(struct llext_loader *loader, struct llext *ext, const elf_rela_t *rel,
			     const elf_sym_t *sym, uint8_t *rel_addr, const void *link_addr);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_INTERNAL_H */
