/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/llext/elf.h>
#include <zephyr/llext/loader.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/llext_internal.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

#include <string.h>

#include "llext_priv.h"

#ifdef CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID
#define SYM_NAME_OR_SLID(name, slid) ((const char *) slid)
#else
#define SYM_NAME_OR_SLID(name, slid) name
#endif

__weak int arch_elf_relocate(elf_rela_t *rel, uintptr_t loc,
			     uintptr_t sym_base_addr, const char *sym_name, uintptr_t load_bias)
{
	return -ENOTSUP;
}

__weak void arch_elf_relocate_local(struct llext_loader *ldr, struct llext *ext,
				    const elf_rela_t *rel, const elf_sym_t *sym, uint8_t *rel_addr,
				    const struct llext_load_param *ldr_parm)
{
}

__weak void arch_elf_relocate_global(struct llext_loader *ldr, struct llext *ext,
				     const elf_rela_t *rel, const elf_sym_t *sym, uint8_t *rel_addr,
				     const void *link_addr)
{
}

/*
 * Find the memory region containing the supplied offset and return the
 * corresponding file offset
 */
static size_t llext_file_offset(struct llext_loader *ldr, size_t offset)
{
	unsigned int i;

	for (i = 0; i < LLEXT_MEM_COUNT; i++) {
		if (ldr->sects[i].sh_addr <= offset &&
		    ldr->sects[i].sh_addr + ldr->sects[i].sh_size > offset) {
			return offset - ldr->sects[i].sh_addr + ldr->sects[i].sh_offset;
		}
	}

	return offset;
}

/*
 * We increment use-count every time a new dependent is added, and have to
 * decrement it again, when one is removed. Ideally we should be able to add
 * arbitrary numbers of dependencies, but using lists for this doesn't work,
 * because multiple extensions can have common dependencies. Dynamically
 * allocating dependency entries would be too wasteful. In this initial
 * implementation we use an array of dependencies, if at some point we run out
 * of array entries, we'll implement re-allocation.
 * We add dependencies incrementally as we discover them, but we only ever
 * expect them to be removed all at once, when their user is removed. So the
 * dependency array is always "dense" - it cannot have NULL entries between
 * valid ones.
 */
static int llext_dependency_add(struct llext *ext, struct llext *dependency)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ext->dependency); i++) {
		if (ext->dependency[i] == dependency) {
			return 0;
		}

		if (!ext->dependency[i]) {
			ext->dependency[i] = dependency;
			dependency->use_count++;

			return 0;
		}
	}

	return -ENOENT;
}

void llext_dependency_remove_all(struct llext *ext)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ext->dependency) && ext->dependency[i]; i++) {
		/*
		 * The use-count of dependencies is tightly bound to dependent's
		 * life cycle, so it shouldn't underrun.
		 */
		ext->dependency[i]->use_count--;
		__ASSERT(ext->dependency[i]->use_count, "LLEXT dependency use-count underrun!");
		/* No need to NULL-ify the pointer - ext is freed after this */
	}
}

struct llext_extension_sym {
	struct llext *ext;
	const char *sym;
	const void *addr;
};

static int llext_find_extension_sym_iterate(struct llext *ext, void *arg)
{
	struct llext_extension_sym *se = arg;
	const void *addr = llext_find_sym(&ext->exp_tab, se->sym);

	if (addr) {
		se->addr = addr;
		se->ext = ext;
		return 1;
	}

	return 0;
}

static const void *llext_find_extension_sym(const char *sym_name, struct llext **ext)
{
	struct llext_extension_sym se = {.sym = sym_name};

	llext_iterate(llext_find_extension_sym_iterate, &se);
	if (ext) {
		*ext = se.ext;
	}

	return se.addr;
}

static void llext_link_plt(struct llext_loader *ldr, struct llext *ext, elf_shdr_t *shdr,
			   const struct llext_load_param *ldr_parm, elf_shdr_t *tgt)
{
	unsigned int sh_cnt = shdr->sh_size / shdr->sh_entsize;
	/*
	 * CPU address where the .text section is stored, we use .text just as a
	 * reference point
	 */
	uint8_t *text = ext->mem[LLEXT_MEM_TEXT];

	LOG_DBG("Found %p in PLT %u size %zu cnt %u text %p",
		(void *)llext_string(ldr, ext, LLEXT_MEM_SHSTRTAB, shdr->sh_name),
		shdr->sh_type, (size_t)shdr->sh_entsize, sh_cnt, (void *)text);

	const elf_shdr_t *sym_shdr = ldr->sects + LLEXT_MEM_SYMTAB;
	unsigned int sym_cnt = sym_shdr->sh_size / sym_shdr->sh_entsize;

	for (unsigned int i = 0; i < sh_cnt; i++) {
		elf_rela_t rela;

		int ret = llext_seek(ldr, shdr->sh_offset + i * shdr->sh_entsize);

		if (!ret) {
			ret = llext_read(ldr, &rela, sizeof(rela));
		}

		if (ret != 0) {
			LOG_ERR("PLT: failed to read RELA #%u, trying to continue", i);
			continue;
		}

		/* Index in the symbol table */
		unsigned int j = ELF_R_SYM(rela.r_info);

		if (j >= sym_cnt) {
			LOG_WRN("PLT: idx %u >= %u", j, sym_cnt);
			continue;
		}

		elf_sym_t sym;

		ret = llext_seek(ldr, sym_shdr->sh_offset + j * sizeof(elf_sym_t));
		if (!ret) {
			ret = llext_read(ldr, &sym, sizeof(sym));
		}

		if (ret < 0) {
			LOG_ERR("PLT: failed to read symbol table #%u RELA #%u, trying to continue",
				j, i);
			continue;
		}

		uint32_t stt = ELF_ST_TYPE(sym.st_info);

		if (stt != STT_FUNC &&
		    stt != STT_SECTION &&
		    stt != STT_OBJECT &&
		    (stt != STT_NOTYPE || sym.st_shndx != SHN_UNDEF)) {
			continue;
		}

		const char *name = llext_string(ldr, ext, LLEXT_MEM_STRTAB, sym.st_name);

		/*
		 * Both r_offset and sh_addr are addresses for which the extension
		 * has been built.
		 *
		 * NOTE: The calculations below assumes offsets from the
		 * beginning of the .text section in the ELF file can be
		 * applied to the memory location of mem[LLEXT_MEM_TEXT].
		 *
		 * This is valid only when CONFIG_LLEXT_STORAGE_WRITABLE=y
		 * and peek() is usable on the source ELF file.
		 */
		uint8_t *rel_addr = (uint8_t *)ext->mem[LLEXT_MEM_TEXT] -
			ldr->sects[LLEXT_MEM_TEXT].sh_offset;

		if (tgt) {
			/* Relocatable / partially linked ELF. */
			rel_addr += rela.r_offset + tgt->sh_offset;
		} else {
			/* Shared / dynamically linked ELF */
			rel_addr += llext_file_offset(ldr, rela.r_offset);
		}

		uint32_t stb = ELF_ST_BIND(sym.st_info);
		const void *link_addr;

		switch (stb) {
		case STB_GLOBAL:
			/* First try the global symbol table */
			link_addr = llext_find_sym(NULL,
				SYM_NAME_OR_SLID(name, sym.st_value));

			if (!link_addr) {
				/* Next try internal tables */
				link_addr = llext_find_sym(&ext->sym_tab, name);
			}

			if (!link_addr) {
				/* Finally try any loaded tables */
				struct llext *dep;

				link_addr = llext_find_extension_sym(name, &dep);
				if (link_addr) {
					llext_dependency_add(ext, dep);
				}
			}

			if (!link_addr) {
				LOG_WRN("PLT: cannot find idx %u name %s", j, name);
				continue;
			}

			/* Resolve the symbol */
			arch_elf_relocate_global(ldr, ext, &rela, &sym, rel_addr, link_addr);
			break;
		case STB_LOCAL:
			arch_elf_relocate_local(ldr, ext, &rela, &sym, rel_addr, ldr_parm);
		}

		LOG_DBG("symbol %s relocation @%p r-offset %#zx .text offset %#zx stb %u",
			name, (void *)rel_addr,
			(size_t)rela.r_offset, (size_t)ldr->sects[LLEXT_MEM_TEXT].sh_offset, stb);
	}
}

int llext_link(struct llext_loader *ldr, struct llext *ext, const struct llext_load_param *ldr_parm)
{
	uintptr_t sect_base = 0;
	elf_rela_t rel;
	elf_sym_t sym;
	elf_word rel_cnt = 0;
	const char *name;
	int i, ret;

	for (i = 0; i < ext->sect_cnt; ++i) {
		elf_shdr_t *shdr = ext->sect_hdrs + i;

		/* find proper relocation sections */
		switch (shdr->sh_type) {
		case SHT_REL:
			if (shdr->sh_entsize != sizeof(elf_rel_t)) {
				LOG_ERR("Invalid entry size %zd for SHT_REL section %d",
					(size_t)shdr->sh_entsize, i);
				return -ENOEXEC;
			}
			break;
		case SHT_RELA:
			if (IS_ENABLED(CONFIG_ARM)) {
				LOG_ERR("Found unsupported SHT_RELA section %d", i);
				return -ENOTSUP;
			}
			if (shdr->sh_entsize != sizeof(elf_rela_t)) {
				LOG_ERR("Invalid entry size %zd for SHT_RELA section %d",
					(size_t)shdr->sh_entsize, i);
				return -ENOEXEC;
			}
			break;
		default:
			/* ignore this section */
			continue;
		}

		if (shdr->sh_info >= ext->sect_cnt ||
		    shdr->sh_size % shdr->sh_entsize != 0) {
			LOG_ERR("Sanity checks failed for section %d "
				"(info %zd, size %zd, entsize %zd)", i,
				(size_t)shdr->sh_info,
				(size_t)shdr->sh_size,
				(size_t)shdr->sh_entsize);
			return -ENOEXEC;
		}

		rel_cnt = shdr->sh_size / shdr->sh_entsize;

		name = llext_string(ldr, ext, LLEXT_MEM_SHSTRTAB, shdr->sh_name);

		/*
		 * FIXME: The Xtensa port is currently using a different way of
		 * handling relocations that ultimately results in separate
		 * arch-specific code paths. This code should be merged with
		 * the logic below once the differences are resolved.
		 */
		if (IS_ENABLED(CONFIG_XTENSA)) {
			elf_shdr_t *tgt;

			if (strcmp(name, ".rela.plt") == 0 ||
			    strcmp(name, ".rela.dyn") == 0) {
				tgt = NULL;
			} else {
				/*
				 * Entries in .rel.X and .rela.X sections describe references in
				 * section .X to local or global symbols. They point to entries
				 * in the symbol table, describing respective symbols
				 */
				tgt = ext->sect_hdrs + shdr->sh_info;
			}

			llext_link_plt(ldr, ext, shdr, ldr_parm, tgt);
			continue;
		}

		LOG_DBG("relocation section %s (%d) acting on section %d has %zd relocations",
			name, i, shdr->sh_info, (size_t)rel_cnt);

		enum llext_mem mem_idx = ldr->sect_map[shdr->sh_info].mem_idx;

		if (mem_idx == LLEXT_MEM_COUNT) {
			LOG_ERR("Section %d not loaded in any memory region", shdr->sh_info);
			return -ENOEXEC;
		}

		sect_base = (uintptr_t)llext_loaded_sect_ptr(ldr, ext, shdr->sh_info);

		for (int j = 0; j < rel_cnt; j++) {
			/* get each relocation entry */
			ret = llext_seek(ldr, shdr->sh_offset + j * shdr->sh_entsize);
			if (ret != 0) {
				return ret;
			}

			ret = llext_read(ldr, &rel, shdr->sh_entsize);
			if (ret != 0) {
				return ret;
			}

			/* get corresponding symbol */
			ret = llext_seek(ldr, ldr->sects[LLEXT_MEM_SYMTAB].sh_offset
				    + ELF_R_SYM(rel.r_info) * sizeof(elf_sym_t));
			if (ret != 0) {
				return ret;
			}

			ret = llext_read(ldr, &sym, sizeof(elf_sym_t));
			if (ret != 0) {
				return ret;
			}

			name = llext_string(ldr, ext, LLEXT_MEM_STRTAB, sym.st_name);

			LOG_DBG("relocation %d:%d info 0x%zx (type %zd, sym %zd) offset %zd "
				"sym_name %s sym_type %d sym_bind %d sym_ndx %d",
				i, j, (size_t)rel.r_info, (size_t)ELF_R_TYPE(rel.r_info),
				(size_t)ELF_R_SYM(rel.r_info), (size_t)rel.r_offset,
				name, ELF_ST_TYPE(sym.st_info),
				ELF_ST_BIND(sym.st_info), sym.st_shndx);

			uintptr_t link_addr, op_loc;

			op_loc = sect_base + rel.r_offset;

			if (ELF_R_SYM(rel.r_info) == 0) {
				/* no symbol ex: R_ARM_V4BX relocation, R_ARM_RELATIVE  */
				link_addr = 0;
			} else if (sym.st_shndx == SHN_UNDEF) {
				/* If symbol is undefined, then we need to look it up */
				link_addr = (uintptr_t)llext_find_sym(NULL,
					SYM_NAME_OR_SLID(name, sym.st_value));

				if (link_addr == 0) {
					/* Try loaded tables */
					struct llext *dep;

					link_addr = (uintptr_t)llext_find_extension_sym(name, &dep);
					if (link_addr) {
						llext_dependency_add(ext, dep);
					}
				}

				if (link_addr == 0) {
					LOG_ERR("Undefined symbol with no entry in "
						"symbol table %s, offset %zd, link section %d",
						name, (size_t)rel.r_offset, shdr->sh_link);
					return -ENODATA;
				}

				LOG_INF("found symbol %s at 0x%lx", name, link_addr);
			} else if (sym.st_shndx == SHN_ABS) {
				/* Absolute symbol */
				link_addr = sym.st_value;
			} else if ((sym.st_shndx < ldr->hdr.e_shnum) &&
				!IN_RANGE(sym.st_shndx, SHN_LORESERVE, SHN_HIRESERVE)) {
				/* This check rejects all relocations whose target symbol
				 * has a section index higher than the maximum possible
				 * in this ELF file, or belongs in the reserved range:
				 * they will be caught by the `else` below and cause an
				 * error to be returned. This aborts the LLEXT's loading
				 * and prevents execution of improperly relocated code,
				 * which is dangerous.
				 *
				 * Note that the unsupported SHN_COMMON section is rejected
				 * as part of this check. Also note that SHN_ABS would be
				 * rejected as well, but we want to handle it properly:
				 * for this reason, this check must come AFTER handling
				 * the case where the symbol's section index is SHN_ABS!
				 *
				 *
				 * For regular symbols, the link address is obtained by
				 * adding st_value to the start address of the section
				 * in which the target symbol resides.
				 */
				link_addr = (uintptr_t)llext_loaded_sect_ptr(ldr, ext,
									     sym.st_shndx)
					    + sym.st_value;
			} else {
				LOG_ERR("rela section %d, entry %d: cannot apply relocation: "
					"target symbol has unexpected section index %d (0x%X)",
					i, j, sym.st_shndx, sym.st_shndx);
				return -ENOEXEC;
			}

			LOG_INF("writing relocation symbol %s type %zd sym %zd at addr 0x%lx "
				"addr 0x%lx",
				name, (size_t)ELF_R_TYPE(rel.r_info), (size_t)ELF_R_SYM(rel.r_info),
				op_loc, link_addr);

			/* relocation */
			ret = arch_elf_relocate(&rel, op_loc, link_addr, name,
					     (uintptr_t)ext->mem[LLEXT_MEM_TEXT]);
			if (ret != 0) {
				return ret;
			}
		}
	}

#ifdef CONFIG_CACHE_MANAGEMENT
	/* Make sure changes to memory regions are flushed to RAM */
	for (i = 0; i < LLEXT_MEM_COUNT; ++i) {
		if (ext->mem[i]) {
			sys_cache_data_flush_range(ext->mem[i], ext->mem_size[i]);
			sys_cache_instr_invd_range(ext->mem[i], ext->mem_size[i]);
		}
	}

	/* Detached section caches should be synchronized in place */
	if (ldr_parm->section_detached) {
		for (i = 0; i < ext->sect_cnt; ++i) {
			elf_shdr_t *shdr = ext->sect_hdrs + i;

			if (ldr_parm->section_detached(shdr)) {
				void *base = llext_peek(ldr, shdr->sh_offset);

				sys_cache_data_flush_range(base, shdr->sh_size);
				sys_cache_instr_invd_range(base, shdr->sh_size);
			}
		}
	}
#endif

	return 0;
}
