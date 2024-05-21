/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/sys/util.h>
#include <zephyr/llext/elf.h>
#include <zephyr/llext/loader.h>
#include <zephyr/llext/llext.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(llext, CONFIG_LLEXT_LOG_LEVEL);

#include <string.h>

#ifdef CONFIG_MMU_PAGE_SIZE
#define LLEXT_PAGE_SIZE CONFIG_MMU_PAGE_SIZE
#else
/* Arm's MPU wants a 32 byte minimum mpu region */
#define LLEXT_PAGE_SIZE 32
#endif

#ifdef CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID
#define SYM_NAME_OR_SLID(name, slid) ((const char *)slid)
#else
#define SYM_NAME_OR_SLID(name, slid) name
#endif

K_HEAP_DEFINE(llext_heap, CONFIG_LLEXT_HEAP_SIZE * 1024);

void *llext_alloc(size_t bytes)
{
	return k_heap_alloc(&llext_heap, bytes, K_NO_WAIT);
}

void *llext_aligned_alloc(size_t align, size_t bytes)
{
	return k_heap_aligned_alloc(&llext_heap, align, bytes, K_NO_WAIT);
}

void llext_free(void *ptr)
{
	k_heap_free(&llext_heap, ptr);
}

void llext_free_sections(struct llext *ext)
{
	for (int i = 0; i < LLEXT_MEM_COUNT; i++) {
		if (ext->mem_on_heap[i]) {
			LOG_DBG("freeing memory region %d", i);
			llext_free(ext->mem[i]);
			ext->mem[i] = NULL;
		}
	}
}

static const char ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};

static sys_slist_t _llext_list = SYS_SLIST_STATIC_INIT(&_llext_list);

static struct k_mutex llext_lock = Z_MUTEX_INITIALIZER(llext_lock);

static elf_shdr_t *llext_section_by_name(struct llext_loader *ldr, const char *search_name)
{
	elf_shdr_t *shdr;
	unsigned int i;
	size_t pos;

	for (i = 0, pos = ldr->hdr.e_shoff;
	     i < ldr->hdr.e_shnum;
	     i++, pos += ldr->hdr.e_shentsize) {
		shdr = llext_peek(ldr, pos);
		if (!shdr) {
			/* The peek() method isn't supported */
			return NULL;
		}

		const char *name = llext_peek(ldr,
					      ldr->sects[LLEXT_MEM_SHSTRTAB].sh_offset +
					      shdr->sh_name);

		if (!strcmp(name, search_name)) {
			return shdr;
		}
	}

	return NULL;
}

ssize_t llext_find_section(struct llext_loader *ldr, const char *search_name)
{
	elf_shdr_t *shdr = llext_section_by_name(ldr, search_name);

	return shdr ? shdr->sh_offset : -ENOENT;
}

/*
 * Note, that while we protect the global llext list while searching, we release
 * the lock before returning the found extension to the caller. Therefore it's
 * a responsibility of the caller to protect against races with a freeing
 * context when calling this function.
 */
struct llext *llext_by_name(const char *name)
{
	k_mutex_lock(&llext_lock, K_FOREVER);

	for (sys_snode_t *node = sys_slist_peek_head(&_llext_list);
	     node != NULL;
	     node = sys_slist_peek_next(node)) {
		struct llext *ext = CONTAINER_OF(node, struct llext, _llext_list);

		if (strncmp(ext->name, name, sizeof(ext->name)) == 0) {
			k_mutex_unlock(&llext_lock);
			return ext;
		}
	}

	k_mutex_unlock(&llext_lock);
	return NULL;
}

int llext_iterate(int (*fn)(struct llext *ext, void *arg), void *arg)
{
	sys_snode_t *node;
	unsigned int i;
	int ret = 0;

	k_mutex_lock(&llext_lock, K_FOREVER);

	for (node = sys_slist_peek_head(&_llext_list), i = 0;
	     node;
	     node = sys_slist_peek_next(node), i++) {
		struct llext *ext = CONTAINER_OF(node, struct llext, _llext_list);

		ret = fn(ext, arg);
		if (ret) {
			break;
		}
	}

	k_mutex_unlock(&llext_lock);
	return ret;
}

const void *llext_find_sym(const struct llext_symtable *sym_table, const char *sym_name)
{
	if (sym_table == NULL) {
		/* Built-in symbol table */
#ifdef CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID
		/* 'sym_name' is actually a SLID to search for */
		uintptr_t slid = (uintptr_t)sym_name;

		/* TODO: perform a binary search instead of linear.
		 * Note that - as of writing - the llext_const_symbol_area
		 * section is sorted in ascending SLID order.
		 * (see scripts/build/llext_prepare_exptab.py)
		 */
		STRUCT_SECTION_FOREACH(llext_const_symbol, sym) {
			if (slid == sym->slid) {
				return sym->addr;
			}
		}
#else
		STRUCT_SECTION_FOREACH(llext_const_symbol, sym) {
			if (strcmp(sym->name, sym_name) == 0) {
				return sym->addr;
			}
		}
#endif
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
	int sect_cnt, i, ret;
	size_t pos;
	elf_shdr_t shdr;

	ldr->sects[LLEXT_MEM_SHSTRTAB] =
		ldr->sects[LLEXT_MEM_STRTAB] =
		ldr->sects[LLEXT_MEM_SYMTAB] = (elf_shdr_t){0};

	/* Find symbol and string tables */
	for (i = 0, sect_cnt = 0, pos = ldr->hdr.e_shoff;
	     i < ldr->hdr.e_shnum && sect_cnt < 3;
	     i++, pos += ldr->hdr.e_shentsize) {
		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			LOG_ERR("failed seeking to position %zu\n", pos);
			return ret;
		}

		ret = llext_read(ldr, &shdr, sizeof(elf_shdr_t));
		if (ret != 0) {
			LOG_ERR("failed reading section header at position %zu\n", pos);
			return ret;
		}

		LOG_DBG("section %d at %zx: name %d, type %d, flags %zx, "
			"ofs %zx, addr %zx, size %zd",
			i, pos,
			shdr.sh_name,
			shdr.sh_type,
			(size_t)shdr.sh_flags,
			(size_t)shdr.sh_offset,
			(size_t)shdr.sh_addr,
			(size_t)shdr.sh_size);

		switch (shdr.sh_type) {
		case SHT_SYMTAB:
		case SHT_DYNSYM:
			LOG_DBG("symtab at %d", i);
			ldr->sects[LLEXT_MEM_SYMTAB] = shdr;
			ldr->sect_map[i] = LLEXT_MEM_SYMTAB;
			sect_cnt++;
			break;
		case SHT_STRTAB:
			if (ldr->hdr.e_shstrndx == i) {
				LOG_DBG("shstrtab at %d", i);
				ldr->sects[LLEXT_MEM_SHSTRTAB] = shdr;
				ldr->sect_map[i] = LLEXT_MEM_SHSTRTAB;
			} else {
				LOG_DBG("strtab at %d", i);
				ldr->sects[LLEXT_MEM_STRTAB] = shdr;
				ldr->sect_map[i] = LLEXT_MEM_STRTAB;
			}
			sect_cnt++;
			break;
		default:
			break;
		}
	}

	if (!ldr->sects[LLEXT_MEM_SHSTRTAB].sh_type ||
	    !ldr->sects[LLEXT_MEM_STRTAB].sh_type ||
	    !ldr->sects[LLEXT_MEM_SYMTAB].sh_type) {
		LOG_ERR("Some sections are missing or present multiple times!");
		return -ENOENT;
	}

	return 0;
}

static const char *llext_string(struct llext_loader *ldr, struct llext *ext,
				enum llext_mem mem_idx, unsigned int idx)
{
	return (char *)ext->mem[mem_idx] + idx;
}

/*
 * Maps the section indexes and copies special section headers for easier use
 */
static int llext_map_sections(struct llext_loader *ldr, struct llext *ext)
{
	int i, j, ret;
	size_t pos;
	elf_shdr_t shdr;
	const char *name;

	for (i = 0, pos = ldr->hdr.e_shoff;
	     i < ldr->hdr.e_shnum;
	     i++, pos += ldr->hdr.e_shentsize) {
		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			return ret;
		}

		ret = llext_read(ldr, &shdr, sizeof(elf_shdr_t));
		if (ret != 0) {
			return ret;
		}

		if ((shdr.sh_type != SHT_PROGBITS && shdr.sh_type != SHT_NOBITS) ||
		    !(shdr.sh_flags & SHF_ALLOC) ||
		    shdr.sh_size == 0) {
			continue;
		}

		name = llext_string(ldr, ext, LLEXT_MEM_SHSTRTAB, shdr.sh_name);

		/* Identify the section type by its flags */
		enum llext_mem mem_idx;

		switch (shdr.sh_type) {
		case SHT_NOBITS:
			mem_idx = LLEXT_MEM_BSS;
			break;
		case SHT_PROGBITS:
			if (shdr.sh_flags & SHF_EXECINSTR) {
				mem_idx = LLEXT_MEM_TEXT;
			} else if (shdr.sh_flags & SHF_WRITE) {
				mem_idx = LLEXT_MEM_DATA;
			} else {
				mem_idx = LLEXT_MEM_RODATA;
			}
			break;
		default:
			LOG_DBG("Not copied section %s", name);
			continue;
		}

		/* Special exception for .exported_sym */
		if (strcmp(name, ".exported_sym") == 0) {
			mem_idx = LLEXT_MEM_EXPORT;
		}

		LOG_DBG("section %d name %s maps to idx %d", i, name, mem_idx);

		ldr->sect_map[i] = mem_idx;
		elf_shdr_t *sect = ldr->sects + mem_idx;

		if (sect->sh_type == SHT_NULL) {
			/* First section of this type, copy all info */
			*sect = shdr;
		} else {
			/* Make sure the sections are compatible before merging */
			if (shdr.sh_flags != sect->sh_flags) {
				LOG_ERR("Unsupported section flags for %s (mem %d)",
					name, mem_idx);
				return -ENOEXEC;
			}

			if (mem_idx == LLEXT_MEM_BSS) {
				/* SHT_NOBITS sections cannot be merged properly:
				 * as they use no space in the file, the logic
				 * below does not work; they must be treated as
				 * independent entities.
				 */
				LOG_ERR("Multiple SHT_NOBITS sections are not supported");
				return -ENOEXEC;
			}

			if (ldr->hdr.e_type == ET_DYN) {
				/* In shared objects, sh_addr is the VMA. Before
				 * merging these sections, make sure the delta
				 * in VMAs matches that of file offsets.
				 */
				if (shdr.sh_addr - sect->sh_addr !=
				    shdr.sh_offset - sect->sh_offset) {
					LOG_ERR("Incompatible section addresses "
						"for %s (mem %d)", name, mem_idx);
					return -ENOEXEC;
				}
			}

			/*
			 * Extend the current section to include the new one
			 * (overlaps are detected later)
			 */
			size_t address = MIN(sect->sh_addr, shdr.sh_addr);
			size_t bot_ofs = MIN(sect->sh_offset, shdr.sh_offset);
			size_t top_ofs = MAX(sect->sh_offset + sect->sh_size,
					     shdr.sh_offset + shdr.sh_size);

			sect->sh_addr = address;
			sect->sh_offset = bot_ofs;
			sect->sh_size = top_ofs - bot_ofs;
		}
	}

	/*
	 * Test that no computed range overlaps. This can happen if sections of
	 * different llext_mem type are interleaved in the ELF file or in VMAs.
	 */
	for (i = 0; i < LLEXT_MEM_COUNT; i++) {
		for (j = i+1; j < LLEXT_MEM_COUNT; j++) {
			elf_shdr_t *x = ldr->sects + i;
			elf_shdr_t *y = ldr->sects + j;

			if (x->sh_type == SHT_NULL || x->sh_size == 0 ||
			    y->sh_type == SHT_NULL || y->sh_size == 0) {
				/* Skip empty sections */
				continue;
			}

			if (ldr->hdr.e_type == ET_DYN) {
				/*
				 * Test all merged VMA ranges for overlaps
				 */
				if ((x->sh_addr <= y->sh_addr &&
				     x->sh_addr + x->sh_size > y->sh_addr) ||
				    (y->sh_addr <= x->sh_addr &&
				     y->sh_addr + y->sh_size > x->sh_addr)) {
					LOG_ERR("VMA range %d (0x%zx +%zd) "
						"overlaps with %d (0x%zx +%zd)",
						i, (size_t)x->sh_addr, (size_t)x->sh_size,
						j, (size_t)y->sh_addr, (size_t)y->sh_size);
					return -ENOEXEC;
				}
			}

			/*
			 * Test file offsets. BSS sections store no
			 * data in the file and must not be included
			 * in checks to avoid false positives.
			 */
			if (i == LLEXT_MEM_BSS || j == LLEXT_MEM_BSS) {
				continue;
			}

			if ((x->sh_offset <= y->sh_offset &&
			     x->sh_offset + x->sh_size > y->sh_offset) ||
			    (y->sh_offset <= x->sh_offset &&
			     y->sh_offset + y->sh_size > x->sh_offset)) {
				LOG_ERR("ELF file range %d (0x%zx +%zd) "
					"overlaps with %d (0x%zx +%zd)",
					i, (size_t)x->sh_offset, (size_t)x->sh_size,
					j, (size_t)y->sh_offset, (size_t)y->sh_size);
				return -ENOEXEC;
			}
		}
	}

	return 0;
}

/*
 * Initialize the memory partition associated with the extension memory
 */
static void llext_init_mem_part(struct llext *ext, enum llext_mem mem_idx,
			uintptr_t start, size_t len)
{
#ifdef CONFIG_USERSPACE
	if (mem_idx < LLEXT_MEM_PARTITIONS) {
		ext->mem_parts[mem_idx].start = start;
		ext->mem_parts[mem_idx].size = len;

		switch (mem_idx) {
		case LLEXT_MEM_TEXT:
			ext->mem_parts[mem_idx].attr = K_MEM_PARTITION_P_RX_U_RX;
			break;
		case LLEXT_MEM_DATA:
		case LLEXT_MEM_BSS:
			ext->mem_parts[mem_idx].attr = K_MEM_PARTITION_P_RW_U_RW;
			break;
		case LLEXT_MEM_RODATA:
			ext->mem_parts[mem_idx].attr = K_MEM_PARTITION_P_RO_U_RO;
			break;
		default:
			break;
		}
		LOG_DBG("mem partition %d start 0x%lx, size %d", mem_idx,
			ext->mem_parts[mem_idx].start,
			ext->mem_parts[mem_idx].size);
	}
#endif

	LOG_DBG("mem idx %d: start 0x%zx, size %zd", mem_idx, (size_t)start, len);
}

static int llext_copy_section(struct llext_loader *ldr, struct llext *ext,
			      enum llext_mem mem_idx)
{
	int ret;

	if (!ldr->sects[mem_idx].sh_size) {
		return 0;
	}
	ext->mem_size[mem_idx] = ldr->sects[mem_idx].sh_size;

	if (ldr->sects[mem_idx].sh_type != SHT_NOBITS &&
	    IS_ENABLED(CONFIG_LLEXT_STORAGE_WRITABLE)) {
		ext->mem[mem_idx] = llext_peek(ldr, ldr->sects[mem_idx].sh_offset);
		if (ext->mem[mem_idx]) {
			llext_init_mem_part(ext, mem_idx, (uintptr_t)ext->mem[mem_idx],
				ldr->sects[mem_idx].sh_size);
			ext->mem_on_heap[mem_idx] = false;
			return 0;
		}
	}

	/* On ARM with an MPU a pow(2, N)*32 sized and aligned region is needed,
	 * otherwise its typically an mmu page (sized and aligned memory region)
	 * we are after that we can assign memory permission bits on.
	 */
#ifndef CONFIG_ARM_MPU
	const uintptr_t sect_alloc = ROUND_UP(ldr->sects[mem_idx].sh_size, LLEXT_PAGE_SIZE);
	const uintptr_t sect_align = LLEXT_PAGE_SIZE;
#else
	uintptr_t sect_alloc = LLEXT_PAGE_SIZE;

	while (sect_alloc < ldr->sects[mem_idx].sh_size) {
		sect_alloc *= 2;
	}
	uintptr_t sect_align = sect_alloc;
#endif

	ext->mem[mem_idx] = llext_aligned_alloc(sect_align, sect_alloc);
	if (!ext->mem[mem_idx]) {
		return -ENOMEM;
	}

	ext->alloc_size += sect_alloc;

	llext_init_mem_part(ext, mem_idx, (uintptr_t)ext->mem[mem_idx],
		sect_alloc);

	if (ldr->sects[mem_idx].sh_type == SHT_NOBITS) {
		memset(ext->mem[mem_idx], 0, ldr->sects[mem_idx].sh_size);
	} else {
		ret = llext_seek(ldr, ldr->sects[mem_idx].sh_offset);
		if (ret != 0) {
			goto err;
		}

		ret = llext_read(ldr, ext->mem[mem_idx], ldr->sects[mem_idx].sh_size);
		if (ret != 0) {
			goto err;
		}
	}

	ext->mem_on_heap[mem_idx] = true;

	return 0;

err:
	llext_free(ext->mem[mem_idx]);
	return ret;
}

static int llext_copy_strings(struct llext_loader *ldr, struct llext *ext)
{
	int ret = llext_copy_section(ldr, ext, LLEXT_MEM_SHSTRTAB);

	if (!ret) {
		ret = llext_copy_section(ldr, ext, LLEXT_MEM_STRTAB);
	}

	return ret;
}

static int llext_copy_sections(struct llext_loader *ldr, struct llext *ext)
{
	for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
		/* strings have already been copied */
		if (ext->mem[mem_idx]) {
			continue;
		}

		int ret = llext_copy_section(ldr, ext, mem_idx);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int llext_count_export_syms(struct llext_loader *ldr, struct llext *ext)
{
	size_t ent_size = ldr->sects[LLEXT_MEM_SYMTAB].sh_entsize;
	size_t syms_size = ldr->sects[LLEXT_MEM_SYMTAB].sh_size;
	int sym_cnt = syms_size / sizeof(elf_sym_t);
	const char *name;
	elf_sym_t sym;
	int i, ret;
	size_t pos;

	LOG_DBG("symbol count %u", sym_cnt);

	for (i = 0, pos = ldr->sects[LLEXT_MEM_SYMTAB].sh_offset;
	     i < sym_cnt;
	     i++, pos += ent_size) {
		if (!i) {
			/* A dummy entry */
			continue;
		}

		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			return ret;
		}

		ret = llext_read(ldr, &sym, ent_size);
		if (ret != 0) {
			return ret;
		}

		uint32_t stt = ELF_ST_TYPE(sym.st_info);
		uint32_t stb = ELF_ST_BIND(sym.st_info);
		uint32_t sect = sym.st_shndx;

		name = llext_string(ldr, ext, LLEXT_MEM_STRTAB, sym.st_name);

		if ((stt == STT_FUNC || stt == STT_OBJECT) && stb == STB_GLOBAL) {
			LOG_DBG("function symbol %d, name %s, type tag %d, bind %d, sect %d",
				i, name, stt, stb, sect);
			ext->sym_tab.sym_cnt++;
		} else {
			LOG_DBG("unhandled symbol %d, name %s, type tag %d, bind %d, sect %d",
				i, name, stt, stb, sect);
		}
	}

	return 0;
}

static int llext_allocate_symtab(struct llext_loader *ldr, struct llext *ext)
{
	struct llext_symtable *sym_tab = &ext->sym_tab;
	size_t syms_size = sym_tab->sym_cnt * sizeof(struct llext_symbol);

	sym_tab->syms = llext_alloc(syms_size);
	if (!sym_tab->syms) {
		return -ENOMEM;
	}
	memset(sym_tab->syms, 0, syms_size);
	ext->alloc_size += syms_size;

	return 0;
}

static int llext_export_symbols(struct llext_loader *ldr, struct llext *ext)
{
	elf_shdr_t *shdr = ldr->sects + LLEXT_MEM_EXPORT;
	struct llext_symbol *sym;
	unsigned int i;

	if (shdr->sh_size < sizeof(struct llext_symbol)) {
		/* Not found, no symbols exported */
		return 0;
	}

	struct llext_symtable *exp_tab = &ext->exp_tab;

	exp_tab->sym_cnt = shdr->sh_size / sizeof(struct llext_symbol);
	exp_tab->syms = llext_alloc(exp_tab->sym_cnt * sizeof(struct llext_symbol));
	if (!exp_tab->syms) {
		return -ENOMEM;
	}

	for (i = 0, sym = ext->mem[LLEXT_MEM_EXPORT];
	     i < exp_tab->sym_cnt;
	     i++, sym++) {
		exp_tab->syms[i].name = sym->name;
		exp_tab->syms[i].addr = sym->addr;
		LOG_DBG("sym %p name %s in %p", sym->addr, sym->name, exp_tab->syms + i);
	}

	return 0;
}

static int llext_copy_symbols(struct llext_loader *ldr, struct llext *ext,
			      bool pre_located)
{
	size_t ent_size = ldr->sects[LLEXT_MEM_SYMTAB].sh_entsize;
	size_t syms_size = ldr->sects[LLEXT_MEM_SYMTAB].sh_size;
	int sym_cnt = syms_size / sizeof(elf_sym_t);
	struct llext_symtable *sym_tab = &ext->sym_tab;
	elf_sym_t sym;
	int i, j, ret;
	size_t pos;

	for (i = 0, pos = ldr->sects[LLEXT_MEM_SYMTAB].sh_offset, j = 0;
	     i < sym_cnt;
	     i++, pos += ent_size) {
		if (!i) {
			/* A dummy entry */
			continue;
		}

		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			return ret;
		}

		ret = llext_read(ldr, &sym, ent_size);
		if (ret != 0) {
			return ret;
		}

		uint32_t stt = ELF_ST_TYPE(sym.st_info);
		uint32_t stb = ELF_ST_BIND(sym.st_info);
		unsigned int sect = sym.st_shndx;

		if ((stt == STT_FUNC || stt == STT_OBJECT) &&
		    stb == STB_GLOBAL && sect != SHN_UNDEF) {
			const char *name = llext_string(ldr, ext, LLEXT_MEM_STRTAB, sym.st_name);

			__ASSERT(j <= sym_tab->sym_cnt, "Miscalculated symbol number %u\n", j);

			sym_tab->syms[j].name = name;

			uintptr_t section_addr;
			void *base;

			if (sect < LLEXT_MEM_BSS) {
				/*
				 * This is just a slight optimisation for cached
				 * sections, we could use the generic path below
				 * for all of them
				 */
				base = ext->mem[ldr->sect_map[sect]];
				section_addr = ldr->sects[ldr->sect_map[sect]].sh_addr;
			} else {
				/* Section header isn't stored, have to read it */
				size_t shdr_pos = ldr->hdr.e_shoff + sect * ldr->hdr.e_shentsize;
				elf_shdr_t shdr;

				ret = llext_seek(ldr, shdr_pos);
				if (ret != 0) {
					LOG_ERR("failed seeking to position %zu\n", shdr_pos);
					return ret;
				}

				ret = llext_read(ldr, &shdr, sizeof(elf_shdr_t));
				if (ret != 0) {
					LOG_ERR("failed reading section header at position %zu\n",
						shdr_pos);
					return ret;
				}

				base = llext_peek(ldr, shdr.sh_offset);
				if (!base) {
					LOG_ERR("cannot handle arbitrary sections without .peek\n");
					return -EOPNOTSUPP;
				}

				section_addr = shdr.sh_addr;
			}

			if (pre_located) {
				sym_tab->syms[j].addr = (uint8_t *)sym.st_value +
					(ldr->hdr.e_type == ET_REL ? section_addr : 0);
			} else {
				sym_tab->syms[j].addr = (uint8_t *)base + sym.st_value -
					(ldr->hdr.e_type == ET_REL ? 0 : section_addr);
			}

			LOG_DBG("function symbol %d name %s addr %p",
				j, name, sym_tab->syms[j].addr);
			j++;
		}
	}

	return 0;
}

/*
 * Find the section, containing the supplied offset and return file offset for
 * that value
 */
static size_t llext_file_offset(struct llext_loader *ldr, size_t offset)
{
	unsigned int i;

	for (i = 0; i < LLEXT_MEM_COUNT; i++)
		if (ldr->sects[i].sh_addr <= offset &&
		    ldr->sects[i].sh_addr + ldr->sects[i].sh_size > offset)
			return offset - ldr->sects[i].sh_addr + ldr->sects[i].sh_offset;

	return offset;
}

__weak void arch_elf_relocate_local(struct llext_loader *ldr, struct llext *ext,
				    const elf_rela_t *rel, const elf_sym_t *sym, size_t got_offset)
{
}

static void llext_link_plt(struct llext_loader *ldr, struct llext *ext,
			   elf_shdr_t *shdr, bool do_local, elf_shdr_t *tgt)
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

		if (ret < 0) {
			LOG_ERR("PLT: failed to read RELA #%u, trying to continue", i);
			continue;
		}

		/* Index in the symbol table */
		unsigned int j = ELF32_R_SYM(rela.r_info);

		if (j >= sym_cnt) {
			LOG_WRN("PLT: idx %u >= %u", j, sym_cnt);
			continue;
		}

		elf_sym_t sym_tbl;

		ret = llext_seek(ldr, sym_shdr->sh_offset + j * sizeof(elf_sym_t));
		if (!ret) {
			ret = llext_read(ldr, &sym_tbl, sizeof(sym_tbl));
		}

		if (ret < 0) {
			LOG_ERR("PLT: failed to read symbol table #%u RELA #%u, trying to continue",
				j, i);
			continue;
		}

		uint32_t stt = ELF_ST_TYPE(sym_tbl.st_info);

		if (stt != STT_FUNC &&
		    stt != STT_SECTION &&
		    stt != STT_OBJECT &&
		    (stt != STT_NOTYPE || sym_tbl.st_shndx != SHN_UNDEF)) {
			continue;
		}

		const char *name = llext_string(ldr, ext, LLEXT_MEM_STRTAB, sym_tbl.st_name);

		/*
		 * Both r_offset and sh_addr are addresses for which the extension
		 * has been built.
		 */
		size_t got_offset;

		if (tgt) {
			got_offset = rela.r_offset + tgt->sh_offset -
				ldr->sects[LLEXT_MEM_TEXT].sh_offset;
		} else {
			got_offset = llext_file_offset(ldr, rela.r_offset) -
				ldr->sects[LLEXT_MEM_TEXT].sh_offset;
		}

		uint32_t stb = ELF_ST_BIND(sym_tbl.st_info);
		const void *link_addr;

		switch (stb) {
		case STB_GLOBAL:
			link_addr = llext_find_sym(NULL,
				SYM_NAME_OR_SLID(name, sym_tbl.st_value));

			if (!link_addr)
				link_addr = llext_find_sym(&ext->sym_tab, name);

			if (!link_addr) {
				LOG_WRN("PLT: cannot find idx %u name %s", j, name);
				continue;
			}

			/* Resolve the symbol */
			*(const void **)(text + got_offset) = link_addr;
			break;
		case STB_LOCAL:
			if (do_local) {
				arch_elf_relocate_local(ldr, ext, &rela, &sym_tbl, got_offset);
			}
		}

		LOG_DBG("symbol %s offset %#zx r-offset %#zx .text offset %#zx stb %u",
			name, got_offset,
			(size_t)rela.r_offset, (size_t)ldr->sects[LLEXT_MEM_TEXT].sh_offset, stb);
	}
}

__weak int arch_elf_relocate(elf_rela_t *rel, uintptr_t loc,
			     uintptr_t sym_base_addr, const char *sym_name, uintptr_t load_bias)
{
	return -EOPNOTSUPP;
}

static int llext_link(struct llext_loader *ldr, struct llext *ext, bool do_local)
{
	uintptr_t loc = 0;
	elf_shdr_t shdr;
	elf_rela_t rel;
	elf_sym_t sym;
	elf_word rel_cnt = 0;
	const char *name;
	int i, ret;
	size_t pos;

	for (i = 0, pos = ldr->hdr.e_shoff;
	     i < ldr->hdr.e_shnum - 1;
	     i++, pos += ldr->hdr.e_shentsize) {
		ret = llext_seek(ldr, pos);
		if (ret != 0) {
			return ret;
		}

		ret = llext_read(ldr, &shdr, sizeof(elf_shdr_t));
		if (ret != 0) {
			return ret;
		}

		/* find relocation sections */
		if (shdr.sh_type != SHT_REL && shdr.sh_type != SHT_RELA) {
			continue;
		}

		rel_cnt = shdr.sh_size / shdr.sh_entsize;

		name = llext_string(ldr, ext, LLEXT_MEM_SHSTRTAB, shdr.sh_name);

		if (strcmp(name, ".rel.text") == 0) {
			loc = (uintptr_t)ext->mem[LLEXT_MEM_TEXT];
		} else if (strcmp(name, ".rel.bss") == 0 ||
			   strcmp(name, ".rela.bss") == 0) {
			loc = (uintptr_t)ext->mem[LLEXT_MEM_BSS];
		} else if (strcmp(name, ".rel.rodata") == 0 ||
			   strcmp(name, ".rela.rodata") == 0) {
			loc = (uintptr_t)ext->mem[LLEXT_MEM_RODATA];
		} else if (strcmp(name, ".rel.data") == 0) {
			loc = (uintptr_t)ext->mem[LLEXT_MEM_DATA];
		} else if (strcmp(name, ".rel.exported_sym") == 0) {
			loc = (uintptr_t)ext->mem[LLEXT_MEM_EXPORT];
		} else if (strcmp(name, ".rela.plt") == 0 ||
			   strcmp(name, ".rela.dyn") == 0) {
			llext_link_plt(ldr, ext, &shdr, do_local, NULL);
			continue;
		} else if (strncmp(name, ".rela", 5) == 0 && strlen(name) > 5) {
			elf_shdr_t *tgt = llext_section_by_name(ldr, name + 5);

			if (tgt)
				llext_link_plt(ldr, ext, &shdr, do_local, tgt);
			continue;
		} else if (strcmp(name, ".rel.dyn") == 0) {
			/* we assume that first load segment starts at MEM_TEXT */
			loc = (uintptr_t)ext->mem[LLEXT_MEM_TEXT];
		}

		LOG_DBG("relocation section %s (%d) linked to section %d has %zd relocations",
			name, i, shdr.sh_link, (size_t)rel_cnt);

		for (int j = 0; j < rel_cnt; j++) {
			/* get each relocation entry */
			ret = llext_seek(ldr, shdr.sh_offset + j * shdr.sh_entsize);
			if (ret != 0) {
				return ret;
			}

			ret = llext_read(ldr, &rel, shdr.sh_entsize);
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

			LOG_DBG("relocation %d:%d info %zx (type %zd, sym %zd) offset %zd sym_name "
				"%s sym_type %d sym_bind %d sym_ndx %d",
				i, j, (size_t)rel.r_info, (size_t)ELF_R_TYPE(rel.r_info),
				(size_t)ELF_R_SYM(rel.r_info),
				(size_t)rel.r_offset, name, ELF_ST_TYPE(sym.st_info),
				ELF_ST_BIND(sym.st_info), sym.st_shndx);

			uintptr_t link_addr, op_loc;

			op_loc = loc + rel.r_offset;

			if (ELF_R_SYM(rel.r_info) == 0) {
				/* no symbol ex: R_ARM_V4BX relocation, R_ARM_RELATIVE  */
				link_addr = 0;
			} else if (sym.st_shndx == SHN_UNDEF) {
				/* If symbol is undefined, then we need to look it up */
				link_addr = (uintptr_t)llext_find_sym(NULL,
					SYM_NAME_OR_SLID(name, sym.st_value));

				if (link_addr == 0) {
					LOG_ERR("Undefined symbol with no entry in "
						"symbol table %s, offset %zd, link section %d",
						name, (size_t)rel.r_offset, shdr.sh_link);
					return -ENODATA;
				} else {
					LOG_INF("found symbol %s at 0x%lx", name, link_addr);
				}
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
				link_addr = (uintptr_t)ext->mem[ldr->sect_map[sym.st_shndx]]
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
	/* Make sure changes to ext sections are flushed to RAM */
	for (i = 0; i < LLEXT_MEM_COUNT; ++i) {
		if (ext->mem[i]) {
			sys_cache_data_flush_range(ext->mem[i], ext->mem_size[i]);
			sys_cache_instr_invd_range(ext->mem[i], ext->mem_size[i]);
		}
	}
#endif

	return 0;
}

/*
 * Load a valid ELF as an extension
 */
static int do_llext_load(struct llext_loader *ldr, struct llext *ext,
			 struct llext_load_param *ldr_parm)
{
	int ret = 0;

	memset(ldr->sects, 0, sizeof(ldr->sects));
	ldr->sect_cnt = 0;
	ext->sym_tab.sym_cnt = 0;

	size_t sect_map_sz = ldr->hdr.e_shnum * sizeof(ldr->sect_map[0]);

	ldr->sect_map = llext_alloc(sect_map_sz);
	if (!ldr->sect_map) {
		LOG_ERR("Failed to allocate memory for section map, size %zu", sect_map_sz);
		ret = -ENOMEM;
		goto out;
	}
	memset(ldr->sect_map, 0, sect_map_sz);

	ldr->sect_cnt = ldr->hdr.e_shnum;
	ext->alloc_size += sect_map_sz;

#ifdef CONFIG_USERSPACE
	ret = k_mem_domain_init(&ext->mem_domain, 0, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to initialize extenion memory domain %d", ret);
		goto out;
	}
#endif

	LOG_DBG("Finding ELF tables...");
	ret = llext_find_tables(ldr);
	if (ret != 0) {
		LOG_ERR("Failed to find important ELF tables, ret %d", ret);
		goto out;
	}

	LOG_DBG("Allocate and copy strings...");
	ret = llext_copy_strings(ldr, ext);
	if (ret != 0) {
		LOG_ERR("Failed to copy ELF string sections, ret %d", ret);
		goto out;
	}

	LOG_DBG("Mapping ELF sections...");
	ret = llext_map_sections(ldr, ext);
	if (ret != 0) {
		LOG_ERR("Failed to map ELF sections, ret %d", ret);
		goto out;
	}

	LOG_DBG("Allocate and copy sections...");
	ret = llext_copy_sections(ldr, ext);
	if (ret != 0) {
		LOG_ERR("Failed to copy ELF sections, ret %d", ret);
		goto out;
	}

	LOG_DBG("Counting exported symbols...");
	ret = llext_count_export_syms(ldr, ext);
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
	ret = llext_copy_symbols(ldr, ext, ldr_parm ? ldr_parm->pre_located : false);
	if (ret != 0) {
		LOG_ERR("Failed to copy symbols, ret %d", ret);
		goto out;
	}

	LOG_DBG("Linking ELF...");
	ret = llext_link(ldr, ext, ldr_parm ? ldr_parm->relocate_local : true);
	if (ret != 0) {
		LOG_ERR("Failed to link, ret %d", ret);
		goto out;
	}

	ret = llext_export_symbols(ldr, ext);
	if (ret != 0) {
		LOG_ERR("Failed to export, ret %d", ret);
		goto out;
	}

out:
	llext_free(ldr->sect_map);

	if (ret != 0) {
		LOG_DBG("Failed to load extension, freeing memory...");
		llext_free_sections(ext);
		llext_free(ext->exp_tab.syms);
	} else {
		LOG_DBG("loaded module, .text at %p, .rodata at %p", ext->mem[LLEXT_MEM_TEXT],
			ext->mem[LLEXT_MEM_RODATA]);
	}

	ext->sym_tab.sym_cnt = 0;
	llext_free(ext->sym_tab.syms);
	ext->sym_tab.syms = NULL;

	return ret;
}

int llext_load(struct llext_loader *ldr, const char *name, struct llext **ext,
	       struct llext_load_param *ldr_parm)
{
	int ret;
	elf_ehdr_t ehdr;

	*ext = llext_by_name(name);

	k_mutex_lock(&llext_lock, K_FOREVER);

	if (*ext) {
		/* The use count is at least 1 */
		ret = (*ext)->use_count++;
		goto out;
	}

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
		*ext = llext_alloc(sizeof(struct llext));
		if (*ext == NULL) {
			LOG_ERR("Not enough memory for extension metadata");
			ret = -ENOMEM;
			goto out;
		}
		memset(*ext, 0, sizeof(struct llext));

		ldr->hdr = ehdr;
		ret = do_llext_load(ldr, *ext, ldr_parm);
		if (ret < 0) {
			llext_free(*ext);
			*ext = NULL;
			goto out;
		}

		strncpy((*ext)->name, name, sizeof((*ext)->name));
		(*ext)->name[sizeof((*ext)->name) - 1] = '\0';
		(*ext)->use_count++;

		sys_slist_append(&_llext_list, &(*ext)->_llext_list);
		LOG_INF("Loaded extension %s", (*ext)->name);

		break;
	default:
		LOG_ERR("Unsupported elf file type %x", ehdr.e_type);
		ret = -EINVAL;
	}

out:
	k_mutex_unlock(&llext_lock);
	return ret;
}

int llext_unload(struct llext **ext)
{
	__ASSERT(*ext, "Expected non-null extension");
	struct llext *tmp = *ext;

	k_mutex_lock(&llext_lock, K_FOREVER);
	__ASSERT(tmp->use_count, "A valid LLEXT cannot have a zero use-count!");

	if (tmp->use_count-- != 1) {
		unsigned int ret = tmp->use_count;

		k_mutex_unlock(&llext_lock);
		return ret;
	}

	/* FIXME: protect the global list */
	sys_slist_find_and_remove(&_llext_list, &tmp->_llext_list);

	*ext = NULL;
	k_mutex_unlock(&llext_lock);

	llext_free_sections(tmp);
	llext_free(tmp->exp_tab.syms);
	llext_free(tmp);

	return 0;
}

int llext_call_fn(struct llext *ext, const char *sym_name)
{
	void (*fn)(void);

	fn = llext_find_sym(&ext->exp_tab, sym_name);
	if (fn == NULL) {
		return -EINVAL;
	}
	fn();

	return 0;
}

int llext_add_domain(struct llext *ext, struct k_mem_domain *domain)
{
#ifdef CONFIG_USERSPACE
	int ret = 0;

	for (int i = 0; i < LLEXT_MEM_PARTITIONS; i++) {
		if (ext->mem_size[i] == 0) {
			continue;
		}
		ret = k_mem_domain_add_partition(domain, &ext->mem_parts[i]);
		if (ret != 0) {
			LOG_ERR("Failed adding memory partition %d to domain %p",
				i, domain);
			return ret;
		}
	}

	return ret;
#else
	return -ENOSYS;
#endif
}
