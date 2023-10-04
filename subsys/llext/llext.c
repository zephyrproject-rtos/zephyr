/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "zephyr/sys/__assert.h"
#include <zephyr/sys/util.h>
#include <zephyr/llext/elf.h>
#include <zephyr/llext/loader.h>
#include <zephyr/llext/llext.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(llext, CONFIG_LLEXT_LOG_LEVEL);

#include <string.h>

K_HEAP_DEFINE(llext_heap, CONFIG_LLEXT_HEAP_SIZE * 1024);

static const char ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};

static inline int llext_read(struct llext_loader *l, void *buf, size_t len)
{
	return l->read(l, buf, len);
}

static inline int llext_seek(struct llext_loader *l, size_t pos)
{
	return l->seek(l, pos);
}

static sys_slist_t _llext_list = SYS_SLIST_STATIC_INIT(&_llext_list);

sys_slist_t *llext_list(void)
{
	return &_llext_list;
}

struct llext *llext_by_name(const char *name)
{
	sys_slist_t *mlist = llext_list();
	sys_snode_t *node = sys_slist_peek_head(mlist);
	struct llext *ext = CONTAINER_OF(node, struct llext, _llext_list);

	while (node != NULL) {
		if (strncmp(ext->name, name, sizeof(ext->name)) == 0) {
			return ext;
		}
		node = sys_slist_peek_next(node);
		ext = CONTAINER_OF(node, struct llext, _llext_list);
	}

	return NULL;
}

const void * const llext_find_sym(const struct llext_symtable *sym_table, const char *sym_name)
{
	if (sym_table == NULL) {
		/* Buildin symbol table */
		STRUCT_SECTION_FOREACH(llext_const_symbol, sym) {
			if (strcmp(sym->name, sym_name) == 0) {
				return sym->addr;
			}
		}
	} else {
		/* find symbols in module */
		for (size_t i = 0; i < sym_table->sym_cnt; i++) {
			if (strcmp(sym_table->syms[i].name, sym_name) == 0) {
				return sym_table->syms[i].addr;
			}
		}
	}

	return NULL;
}

/*
 * Find all relevant string and symbol tables
 */
static int llext_find_tables(struct llext_loader *ldr)
{
	int ret = 0;
	size_t pos = ldr->hdr.e_shoff;
	elf_shdr_t shdr;

	ldr->sects[LLEXT_SECT_SHSTRTAB] =
		ldr->sects[LLEXT_SECT_STRTAB] =
		ldr->sects[LLEXT_SECT_SYMTAB] = (elf_shdr_t){0};

	/* Find symbol and string tables */
	for (int i = 0, str_cnt = 0; i < ldr->hdr.e_shnum && str_cnt < 3; i++) {
		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			LOG_ERR("failed seeking to position %u\n", pos);
			goto out;
		}

		ret = llext_read(ldr, &shdr, sizeof(elf_shdr_t));
		if (ret != 0) {
			LOG_ERR("failed reading section header at position %u\n", pos);
			goto out;
		}

		pos += ldr->hdr.e_shentsize;

		LOG_DBG("section %d at %x: name %d, type %d, flags %x, addr %x, size %d",
			i,
			ldr->hdr.e_shoff + i * ldr->hdr.e_shentsize,
			shdr.sh_name,
			shdr.sh_type,
			shdr.sh_flags,
			shdr.sh_addr,
			shdr.sh_size);

		switch (shdr.sh_type) {
		case SHT_SYMTAB:
		case SHT_DYNSYM:
			LOG_DBG("symtab at %d", i);
			ldr->sects[LLEXT_SECT_SYMTAB] = shdr;
			ldr->sect_map[i] = LLEXT_SECT_SYMTAB;
			str_cnt++;
			break;
		case SHT_STRTAB:
			if (ldr->hdr.e_shstrndx == i) {
				LOG_DBG("shstrtab at %d", i);
				ldr->sects[LLEXT_SECT_SHSTRTAB] = shdr;
				ldr->sect_map[i] = LLEXT_SECT_SHSTRTAB;
			} else {
				LOG_DBG("strtab at %d", i);
				ldr->sects[LLEXT_SECT_STRTAB] = shdr;
				ldr->sect_map[i] = LLEXT_SECT_STRTAB;
			}
			str_cnt++;
			break;
		default:
			break;
		}
	}

	if (!ldr->sects[LLEXT_SECT_SHSTRTAB].sh_type ||
	    !ldr->sects[LLEXT_SECT_STRTAB].sh_type ||
	    !ldr->sects[LLEXT_SECT_SYMTAB].sh_type) {
		LOG_ERR("Some sections are missing or present multiple times!");
		ret = -ENOENT;
	}

out:
	return ret;
}

/*
 * Maps the section indexes and copies special section headers for easier use
 */
static int llext_map_sections(struct llext_loader *ldr)
{
	int ret = 0;
	size_t pos = ldr->hdr.e_shoff;
	elf_shdr_t shdr;
	char name[32];

	for (int i = 0; i < ldr->hdr.e_shnum; i++) {
		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			goto out;
		}

		ret = llext_read(ldr, &shdr, sizeof(elf_shdr_t));
		if (ret != 0) {
			goto out;
		}

		pos += ldr->hdr.e_shentsize;

		elf_word str_idx = shdr.sh_name;

		ret = llext_seek(ldr, ldr->sects[LLEXT_SECT_SHSTRTAB].sh_offset + str_idx);
		if (ret != 0) {
			goto out;
		}

		ret = llext_read(ldr, name, sizeof(name));
		if (ret != 0) {
			goto out;
		}

		name[sizeof(name) - 1] = '\0';

		LOG_DBG("section %d name %s", i, name);

		enum llext_section sect_idx;

		if (strncmp(name, ".text", sizeof(name)) == 0) {
			sect_idx = LLEXT_SECT_TEXT;
		} else if (strncmp(name, ".data", sizeof(name)) == 0) {
			sect_idx = LLEXT_SECT_DATA;
		} else if (strncmp(name, ".rodata", sizeof(name)) == 0) {
			sect_idx = LLEXT_SECT_RODATA;
		} else if (strncmp(name, ".bss", sizeof(name)) == 0) {
			sect_idx = LLEXT_SECT_BSS;
		} else {
			LOG_DBG("Not copied section %s", name);
			continue;
		}

		ldr->sects[sect_idx] = shdr;
		ldr->sect_map[i] = sect_idx;
	}

out:
	return ret;
}

static inline enum llext_section llext_sect_from_mem(enum llext_mem m)
{
	enum llext_section s;

	switch (m) {
	case LLEXT_MEM_BSS:
		s = LLEXT_SECT_BSS;
		break;
	case LLEXT_MEM_DATA:
		s = LLEXT_SECT_DATA;
		break;
	case LLEXT_MEM_RODATA:
		s = LLEXT_SECT_RODATA;
		break;
	case LLEXT_MEM_TEXT:
		s = LLEXT_SECT_TEXT;
		break;
	default:
		CODE_UNREACHABLE;
	}

	return s;
}

static int llext_allocate_mem(struct llext_loader *ldr, struct llext *ext)
{
	int ret = 0;
	enum llext_section sect_idx;

	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		sect_idx = llext_sect_from_mem(mem_idx);

		if (ldr->sects[sect_idx].sh_size > 0) {
			ext->mem[mem_idx] =
				k_heap_aligned_alloc(&llext_heap, sizeof(uintptr_t),
						     ldr->sects[sect_idx].sh_size,
						     K_NO_WAIT);

			if (ext->mem[mem_idx] == NULL) {
				ret = -ENOMEM;
				goto out;
			}
		}
	}

out:
	return ret;
}

static int llext_copy_sections(struct llext_loader *ldr, struct llext *ext)
{
	int ret = 0;
	enum llext_section sect_idx;

	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		sect_idx = llext_sect_from_mem(mem_idx);

		if (ldr->sects[sect_idx].sh_size > 0) {
			ret = llext_seek(ldr, ldr->sects[sect_idx].sh_offset);
			if (ret != 0) {
				goto out;
			}

			ret = llext_read(ldr, ext->mem[mem_idx], ldr->sects[sect_idx].sh_size);
			if (ret != 0) {
				goto out;
			}
		}
	}

out:
	return ret;
}

static int llext_count_export_syms(struct llext_loader *ldr)
{
	int ret = 0;
	elf_sym_t sym;
	size_t ent_size = ldr->sects[LLEXT_SECT_SYMTAB].sh_entsize;
	size_t syms_size = ldr->sects[LLEXT_SECT_SYMTAB].sh_size;
	size_t pos = ldr->sects[LLEXT_SECT_SYMTAB].sh_offset;
	size_t sym_cnt = syms_size / sizeof(elf_sym_t);
	char name[32];

	LOG_DBG("symbol count %u", sym_cnt);

	for (int i = 0; i < sym_cnt; i++) {
		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			goto out;
		}

		ret = llext_read(ldr, &sym, ent_size);
		if (ret != 0) {
			goto out;
		}

		pos += ent_size;

		uint32_t stt = ELF_ST_TYPE(sym.st_info);
		uint32_t stb = ELF_ST_BIND(sym.st_info);
		uint32_t sect = sym.st_shndx;

		ret = llext_seek(ldr, ldr->sects[LLEXT_SECT_STRTAB].sh_offset + sym.st_name);
		if (ret != 0) {
			goto out;
		}

		ret = llext_read(ldr, name, sizeof(name));
		if (ret != 0) {
			goto out;
		}

		name[sizeof(name) - 1] = '\0';

		if (stt == STT_FUNC && stb == STB_GLOBAL) {
			LOG_DBG("function symbol %d, name %s, type tag %d, bind %d, sect %d",
				i, name, stt, stb, sect);
			ldr->sym_cnt++;
		} else {
			LOG_DBG("unhandled symbol %d, name %s, type tag %d, bind %d, sect %d",
				i, name, stt, stb, sect);
		}
	}

out:
	return ret;
}

static inline int llext_allocate_symtab(struct llext_loader *ldr, struct llext *ext)
{
	int ret = 0;

	ext->sym_tab.syms = k_heap_alloc(&llext_heap, ldr->sym_cnt * sizeof(struct llext_symbol),
				       K_NO_WAIT);
	ext->sym_tab.sym_cnt = ldr->sym_cnt;
	memset(ext->sym_tab.syms, 0, ldr->sym_cnt * sizeof(struct llext_symbol));

	return ret;
}

static inline int llext_copy_symbols(struct llext_loader *ldr, struct llext *ext)
{
	int ret = 0;
	elf_sym_t sym;
	size_t ent_size = ldr->sects[LLEXT_SECT_SYMTAB].sh_entsize;
	size_t syms_size = ldr->sects[LLEXT_SECT_SYMTAB].sh_size;
	size_t pos = ldr->sects[LLEXT_SECT_SYMTAB].sh_offset;
	size_t sym_cnt = syms_size / sizeof(elf_sym_t);
	char name[32];
	int i, j = 0;

	for (i = 0; i < sym_cnt; i++) {
		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			goto out;
		}

		ret = llext_read(ldr, &sym, ent_size);
		if (ret != 0) {
			goto out;
		}

		pos += ent_size;

		uint32_t stt = ELF_ST_TYPE(sym.st_info);
		uint32_t stb = ELF_ST_BIND(sym.st_info);
		uint32_t sect = sym.st_shndx;

		ret = llext_seek(ldr, ldr->sects[LLEXT_SECT_STRTAB].sh_offset + sym.st_name);
		if (ret != 0) {
			goto out;
		}

		llext_read(ldr, name, sizeof(name));
		if (ret != 0) {
			goto out;
		}

		if (stt == STT_FUNC && stb == STB_GLOBAL && sect != SHN_UNDEF) {
			ext->sym_tab.syms[j].name = k_heap_alloc(&llext_heap,
							       sizeof(name),
							       K_NO_WAIT);
			strcpy(ext->sym_tab.syms[j].name, name);
			ext->sym_tab.syms[j].addr =
				(void *)((uintptr_t)ext->mem[ldr->sect_map[sym.st_shndx]]
					 + sym.st_value);
			LOG_DBG("function symbol %d name %s addr %p",
				j, name, ext->sym_tab.syms[j].addr);
			j++;
		}
	}

out:
	return ret;
}

static int llext_link(struct llext_loader *ldr, struct llext *ext)
{
	int ret = 0;
	uintptr_t loc = 0;
	elf_shdr_t shdr;
	elf_rel_t rel;
	elf_sym_t sym;
	size_t pos = ldr->hdr.e_shoff;
	elf_word rel_cnt = 0;
	char name[32];

	for (int i = 0; i < ldr->hdr.e_shnum - 1; i++) {
		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			goto out;
		}

		ret = llext_read(ldr, &shdr, sizeof(elf_shdr_t));
		if (ret != 0) {
			goto out;
		}

		pos += ldr->hdr.e_shentsize;

		/* find relocation sections */
		if (shdr.sh_type != SHT_REL && shdr.sh_type != SHT_RELA) {
			continue;
		}

		rel_cnt = shdr.sh_size / sizeof(elf_rel_t);


		ret = llext_seek(ldr, ldr->sects[LLEXT_SECT_SHSTRTAB].sh_offset + shdr.sh_name);
		if (ret != 0) {
			goto out;
		}

		ret = llext_read(ldr, name, sizeof(name));
		if (ret != 0) {
			goto out;
		}

		if (strncmp(name, ".rel.text", sizeof(name)) == 0 ||
		    strncmp(name, ".rela.text", sizeof(name)) == 0) {
			loc = (uintptr_t)ext->mem[LLEXT_MEM_TEXT];
		} else if (strncmp(name, ".rel.bss", sizeof(name)) == 0) {
			loc = (uintptr_t)ext->mem[LLEXT_MEM_BSS];
		} else if (strncmp(name, ".rel.rodata", sizeof(name)) == 0) {
			loc = (uintptr_t)ext->mem[LLEXT_MEM_RODATA];
		} else if (strncmp(name, ".rel.data", sizeof(name)) == 0) {
			loc = (uintptr_t)ext->mem[LLEXT_MEM_DATA];
		}

		LOG_DBG("relocation section %s (%d) linked to section %d has %d relocations",
			name, i, shdr.sh_link, rel_cnt);

		for (int j = 0; j < rel_cnt; j++) {
			/* get each relocation entry */
			ret = llext_seek(ldr, shdr.sh_offset + j * sizeof(elf_rel_t));
			if (ret != 0) {
				goto out;
			}

			ret = llext_read(ldr, &rel, sizeof(elf_rel_t));
			if (ret != 0) {
				goto out;
			}

			/* get corresponding symbol */
			ret = llext_seek(ldr, ldr->sects[LLEXT_SECT_SYMTAB].sh_offset
				    + ELF_R_SYM(rel.r_info) * sizeof(elf_sym_t));
			if (ret != 0) {
				goto out;
			}

			ret = llext_read(ldr, &sym, sizeof(elf_sym_t));
			if (ret != 0) {
				goto out;
			}

			ret = llext_seek(ldr, ldr->sects[LLEXT_SECT_STRTAB].sh_offset +
				    sym.st_name);
			if (ret != 0) {
				goto out;
			}

			ret = llext_read(ldr, name, sizeof(name));
			if (ret != 0) {
				goto out;
			}

			LOG_DBG("relocation %d:%d info %x (type %d, sym %d) offset %d sym_name "
				"%s sym_type %d sym_bind %d sym_ndx %d",
				i, j, rel.r_info, ELF_R_TYPE(rel.r_info), ELF_R_SYM(rel.r_info),
				rel.r_offset, name, ELF_ST_TYPE(sym.st_info),
				ELF_ST_BIND(sym.st_info), sym.st_shndx);

			uintptr_t link_addr, op_loc, op_code;

			op_loc = loc + rel.r_offset;

			/* If symbol is undefined, then we need to look it up */
			if (sym.st_shndx == SHN_UNDEF) {
				link_addr = (uintptr_t)llext_find_sym(NULL, name);

				if (link_addr == 0) {
					LOG_ERR("Undefined symbol with no entry in "
						"symbol table %s, offset %d, link section %d",
						name, rel.r_offset, shdr.sh_link);
					ret = -ENODATA;
					goto out;
				} else {
					op_code = (uintptr_t)(loc + rel.r_offset);

					LOG_INF("found symbol %s at 0x%lx, updating op code 0x%lx",
						name, link_addr, op_code);
				}
			} else if (ELF_ST_TYPE(sym.st_info) == STT_SECTION) {
				/* Current relocation location holds an offset into the section */
				link_addr = (uintptr_t)ext->mem[ldr->sect_map[sym.st_shndx]]
					+ sym.st_value
					+ *((uintptr_t *)op_loc);

				LOG_INF("found section symbol %s addr 0x%lx", name, link_addr);
			} else {
				/* Nothing to relocate here */
				continue;
			}

			LOG_INF("relocating (linking) symbol %s type %d binding %d ndx %d offset "
				"%d link section %d",
				name, ELF_ST_TYPE(sym.st_info), ELF_ST_BIND(sym.st_info),
				sym.st_shndx, rel.r_offset, shdr.sh_link);

			LOG_INF("writing relocation symbol %s type %d sym %d at addr 0x%lx "
				"addr 0x%lx",
				name, ELF_R_TYPE(rel.r_info), ELF_R_SYM(rel.r_info),
				op_loc, link_addr);

			/* relocation */
			arch_elf_relocate(&rel, op_loc, link_addr);
		}
	}

out:
	return ret;
}

/*
 * Load a valid ELF as an extension
 */
static int do_llext_load(struct llext_loader *ldr, struct llext *ext)
{
	int ret = 0;

	memset(ldr->sects, 0, sizeof(ldr->sects));
	ldr->sect_cnt = 0;
	ldr->sym_cnt = 0;

	size_t sect_map_sz = ldr->hdr.e_shnum * sizeof(uint32_t);

	ldr->sect_map = k_heap_alloc(&llext_heap, sect_map_sz, K_NO_WAIT);
	if (!ldr->sect_map) {
		LOG_ERR("Failed to allocate memory for section map, size %u", sect_map_sz);
		ret = -ENOMEM;
		goto out;
	}
	memset(ldr->sect_map, 0, ldr->hdr.e_shnum*sizeof(uint32_t));
	ldr->sect_cnt = ldr->hdr.e_shnum;

	LOG_DBG("Finding ELF tables...");
	ret = llext_find_tables(ldr);
	if (ret != 0) {
		LOG_ERR("Failed to find important ELF tables, ret %d", ret);
		goto out;
	}

	LOG_DBG("Mapping ELF sections...");
	ret = llext_map_sections(ldr);
	if (ret != 0) {
		LOG_ERR("Failed to map ELF sections, ret %d", ret);
		goto out;
	}

	LOG_DBG("Allocation memory for ELF sections...");
	ret = llext_allocate_mem(ldr, ext);
	if (ret != 0) {
		LOG_ERR("Failed to map memory for ELF sections, ret %d", ret);
		goto out;
	}

	LOG_DBG("Copying sections...");
	ret = llext_copy_sections(ldr, ext);
	if (ret != 0) {
		LOG_ERR("Failed to copy ELF sections, ret %d", ret);
		goto out;
	}

	LOG_DBG("Counting exported symbols...");
	ret = llext_count_export_syms(ldr);
	if (ret != 0) {
		LOG_ERR("Failed to count exported ELF symbols, ret %d", ret);
		goto out;
	}

	LOG_DBG("Allocating memory for symbol table...");
	ret = llext_allocate_symtab(ldr, ext);
	if (ret != 0) {
		LOG_ERR("Failed to allocate extension symbol table, ret %d", ret);
		goto out;
	}

	LOG_DBG("Copying symbols...");
	ret = llext_copy_symbols(ldr, ext);
	if (ret != 0) {
		LOG_ERR("Failed to copy symbols, ret %d", ret);
		goto out;
	}

	LOG_DBG("Linking ELF...");
	ret = llext_link(ldr, ext);
	if (ret != 0) {
		LOG_ERR("Failed to link, ret %d", ret);
		goto out;
	}

out:
	if (ldr->sect_map != NULL) {
		k_heap_free(&llext_heap, ldr->sect_map);
	}

	if (ret != 0) {
		LOG_DBG("Failed to load extension, freeing memory...");
		for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
			if (ext->mem[mem_idx] != NULL) {
				k_heap_free(&llext_heap, ext->mem[mem_idx]);
			}
		}
		for (int i = 0; i < ext->sym_tab.sym_cnt; i++) {
			if (ext->sym_tab.syms[i].name != NULL) {
				k_heap_free(&llext_heap, ext->sym_tab.syms[i].name);
			}
		}
		k_heap_free(&llext_heap, ext->sym_tab.syms);
	} else {
		LOG_DBG("loaded module, .text at %p, .rodata at %p", ext->mem[LLEXT_MEM_TEXT],
			ext->mem[LLEXT_MEM_RODATA]);
	}

	return ret;
}

int llext_load(struct llext_loader *ldr, const char *name, struct llext **ext)
{
	int ret = 0;
	elf_ehdr_t ehdr;

	ret = llext_seek(ldr, 0);
	if (ret != 0) {
		LOG_ERR("Failed to seek for ELF header");
		goto out;
	}

	ret = llext_read(ldr, &ehdr, sizeof(ehdr));
	if (ret != 0) {
		LOG_ERR("Failed to read ELF header");
		goto out;
	}

	/* check whether this is an valid elf file */
	if (memcmp(ehdr.e_ident, ELF_MAGIC, sizeof(ELF_MAGIC)) != 0) {
		LOG_HEXDUMP_ERR(ehdr.e_ident, 16, "Invalid ELF, magic does not match");
		ret = -EINVAL;
		goto out;
	}

	switch (ehdr.e_type) {
	case ET_REL:
	case ET_DYN:
		LOG_DBG("Loading relocatable or shared elf");
		*ext = k_heap_alloc(&llext_heap, sizeof(struct llext), K_NO_WAIT);
		if (*ext == NULL) {
			LOG_ERR("Not enough memory for extension metadata");
			ret = -ENOMEM;
			goto out;
		}
		memset(*ext, 0, sizeof(struct llext));

		for (int i = 0; i < LLEXT_MEM_COUNT; i++) {
			(*ext)->mem[i] = NULL;
		}

		ldr->hdr = ehdr;
		ret = do_llext_load(ldr, *ext);
		break;
	default:
		LOG_ERR("Unsupported elf file type %x", ehdr.e_type);
		*ext = NULL;
		ret = -EINVAL;
		goto out;
	}

	if (ret == 0) {
		strncpy((*ext)->name, name, sizeof((*ext)->name));
		(*ext)->name[sizeof((*ext)->name) - 1] = '\0';
		sys_slist_append(&_llext_list, &(*ext)->_llext_list);
		LOG_INF("Loaded extension %s", (*ext)->name);
	}

out:
	return ret;
}

void llext_unload(struct llext *ext)
{
	__ASSERT(ext, "Expected non-null extension");

	sys_slist_find_and_remove(&_llext_list, &ext->_llext_list);

	for (int i = 0; i < LLEXT_MEM_COUNT; i++) {
		if (ext->mem[i] != NULL) {
			LOG_DBG("freeing memory region %d", i);
			k_heap_free(&llext_heap, ext->mem[i]);
			ext->mem[i] = NULL;
		}
	}

	if (ext->sym_tab.syms != NULL) {
		for (int i = 0; i < ext->sym_tab.sym_cnt; i++) {
			k_heap_free(&llext_heap, ext->sym_tab.syms[i].name);
		}
		k_heap_free(&llext_heap, ext->sym_tab.syms);
	}

	k_heap_free(&llext_heap, ext);
}

int llext_call_fn(struct llext *ext, const char *sym_name)
{
	void (*fn)(void);

	fn = llext_find_sym(&ext->sym_tab, sym_name);
	if (fn == NULL) {
		return -EINVAL;
	}
	fn();

	return 0;
}
