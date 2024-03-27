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

K_HEAP_DEFINE(llext_heap, CONFIG_LLEXT_HEAP_SIZE * 1024);

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

const void * const llext_find_sym(const struct llext_symtable *sym_table, const char *sym_name)
{
	if (sym_table == NULL) {
		/* Built-in symbol table */
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

		LOG_DBG("section %d at %zx: name %d, type %d, flags %zx, addr %zx, size %zd",
			i,
			(size_t)ldr->hdr.e_shoff + i * ldr->hdr.e_shentsize,
			shdr.sh_name,
			shdr.sh_type,
			(size_t)shdr.sh_flags,
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
	int i, ret;
	size_t pos;
	elf_shdr_t shdr, rodata = {.sh_addr = ~0},
		high_shdr = {.sh_offset = 0}, low_shdr = {.sh_offset = ~0};
	const char *name;

	ldr->sects[LLEXT_MEM_RODATA].sh_size = 0;

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

		/* Identify the lowest and the highest data sections */
		if (!(shdr.sh_flags & SHF_EXECINSTR) &&
		    shdr.sh_type == SHT_PROGBITS) {
			if (shdr.sh_offset > high_shdr.sh_offset) {
				high_shdr = shdr;
			}
			if (shdr.sh_offset < low_shdr.sh_offset) {
				low_shdr = shdr;
			}
		}

		name = llext_string(ldr, ext, LLEXT_MEM_SHSTRTAB, shdr.sh_name);

		LOG_DBG("section %d name %s", i, name);

		enum llext_mem mem_idx;

		/*
		 * .rodata section is optional. If there isn't one, use the
		 * first read-only data section
		 */
		if (shdr.sh_addr && !(shdr.sh_flags & (SHF_WRITE | SHF_EXECINSTR)) &&
		    shdr.sh_addr < rodata.sh_addr) {
			rodata = shdr;
			LOG_DBG("rodata: select %#zx name %s", (size_t)shdr.sh_addr, name);
		}

		/*
		 * Keep in mind, that when using relocatable (partially linked)
		 * objects, ELF segments aren't created, so ldr->sect_map[] and
		 * ldr->sects[] don't contain all the sections
		 */
		if (strcmp(name, ".text") == 0) {
			mem_idx = LLEXT_MEM_TEXT;
		} else if (strcmp(name, ".data") == 0) {
			mem_idx = LLEXT_MEM_DATA;
		} else if (strcmp(name, ".rodata") == 0) {
			mem_idx = LLEXT_MEM_RODATA;
		} else if (strcmp(name, ".bss") == 0) {
			mem_idx = LLEXT_MEM_BSS;
		} else if (strcmp(name, ".exported_sym") == 0) {
			mem_idx = LLEXT_MEM_EXPORT;
		} else {
			LOG_DBG("Not copied section %s", name);
			continue;
		}

		ldr->sects[mem_idx] = shdr;
		ldr->sect_map[i] = mem_idx;
	}

	ldr->prog_data_size = high_shdr.sh_size + high_shdr.sh_offset - low_shdr.sh_offset;

	/* No verbatim .rodata, use an automatically selected one */
	if (!ldr->sects[LLEXT_MEM_RODATA].sh_size) {
		ldr->sects[LLEXT_MEM_RODATA] = rodata;
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

	ext->mem[mem_idx] = k_heap_aligned_alloc(&llext_heap, sect_align,
						 sect_alloc,
						 K_NO_WAIT);

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
	k_heap_free(&llext_heap, ext->mem[mem_idx]);
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

		if (stt == STT_FUNC && stb == STB_GLOBAL) {
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

	sym_tab->syms = k_heap_alloc(&llext_heap, syms_size, K_NO_WAIT);
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
	exp_tab->syms = k_heap_alloc(&llext_heap, exp_tab->sym_cnt * sizeof(struct llext_symbol),
				     K_NO_WAIT);
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

static int llext_copy_symbols(struct llext_loader *ldr, struct llext *ext)
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

		if (stt == STT_FUNC && stb == STB_GLOBAL && sect != SHN_UNDEF) {
			enum llext_mem mem_idx = ldr->sect_map[sect];
			const char *name = llext_string(ldr, ext, LLEXT_MEM_STRTAB, sym.st_name);

			__ASSERT(j <= sym_tab->sym_cnt, "Miscalculated symbol number %u\n", j);

			sym_tab->syms[j].name = name;
			sym_tab->syms[j].addr = (void *)((uintptr_t)ext->mem[mem_idx] +
							 sym.st_value -
							 (ldr->hdr.e_type == ET_REL ? 0 :
							  ldr->sects[mem_idx].sh_addr));
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
			link_addr = llext_find_sym(NULL, name);

			if (!link_addr)
				link_addr = llext_find_sym(&ext->sym_tab, name);

			if (!link_addr) {
				LOG_WRN("PLT: cannot find idx %u name %s", j, name);
				continue;
			}

			if (!rela.r_offset) {
				LOG_WRN("PLT: zero offset idx %u name %s", j, name);
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

__weak void arch_elf_relocate(elf_rela_t *rel, uintptr_t opaddr, uintptr_t opval)
{
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

			/* If symbol is undefined, then we need to look it up */
			if (sym.st_shndx == SHN_UNDEF) {
				link_addr = (uintptr_t)llext_find_sym(NULL, name);

				if (link_addr == 0) {
					LOG_ERR("Undefined symbol with no entry in "
						"symbol table %s, offset %zd, link section %d",
						name, (size_t)rel.r_offset, shdr.sh_link);
					return -ENODATA;
				}
			} else if (ELF_ST_TYPE(sym.st_info) == STT_SECTION ||
				   ELF_ST_TYPE(sym.st_info) == STT_FUNC ||
				   ELF_ST_TYPE(sym.st_info) == STT_OBJECT) {
				/* Link address is relative to the start of the section */
				link_addr = (uintptr_t)ext->mem[ldr->sect_map[sym.st_shndx]]
					+ sym.st_value;

				LOG_INF("found section symbol %s addr 0x%lx", name, link_addr);
			} else {
				/* Nothing to relocate here */
				LOG_DBG("not relocated");
				continue;
			}

			LOG_INF("relocating (linking) symbol %s type %d binding %d ndx %d offset "
				"%zd link section %d",
				name, ELF_ST_TYPE(sym.st_info), ELF_ST_BIND(sym.st_info),
				sym.st_shndx, (size_t)rel.r_offset, shdr.sh_link);

			LOG_INF("writing relocation symbol %s type %zd sym %zd at addr 0x%lx "
				"addr 0x%lx",
				name, (size_t)ELF_R_TYPE(rel.r_info), (size_t)ELF_R_SYM(rel.r_info),
				op_loc, link_addr);

			/* relocation */
			arch_elf_relocate(&rel, op_loc, link_addr);
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

	ldr->sect_map = k_heap_alloc(&llext_heap, sect_map_sz, K_NO_WAIT);
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
	ret = llext_copy_symbols(ldr, ext);
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
	k_heap_free(&llext_heap, ldr->sect_map);

	if (ret != 0) {
		LOG_DBG("Failed to load extension, freeing memory...");
		for (enum llext_mem mem_idx = 0; mem_idx < LLEXT_MEM_COUNT; mem_idx++) {
			if (ext->mem_on_heap[mem_idx]) {
				k_heap_free(&llext_heap, ext->mem[mem_idx]);
			}
		}
		k_heap_free(&llext_heap, ext->exp_tab.syms);
	} else {
		LOG_DBG("loaded module, .text at %p, .rodata at %p", ext->mem[LLEXT_MEM_TEXT],
			ext->mem[LLEXT_MEM_RODATA]);
	}

	ext->sym_tab.sym_cnt = 0;
	k_heap_free(&llext_heap, ext->sym_tab.syms);
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
		*ext = k_heap_alloc(&llext_heap, sizeof(struct llext), K_NO_WAIT);
		if (*ext == NULL) {
			LOG_ERR("Not enough memory for extension metadata");
			ret = -ENOMEM;
			goto out;
		}
		memset(*ext, 0, sizeof(struct llext));

		ldr->hdr = ehdr;
		ret = do_llext_load(ldr, *ext, ldr_parm);
		if (ret < 0) {
			k_heap_free(&llext_heap, *ext);
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

	for (int i = 0; i < LLEXT_MEM_COUNT; i++) {
		if (tmp->mem_on_heap[i]) {
			LOG_DBG("freeing memory region %d", i);
			k_heap_free(&llext_heap, tmp->mem[i]);
			tmp->mem[i] = NULL;
		}
	}

	k_heap_free(&llext_heap, tmp->exp_tab.syms);
	k_heap_free(&llext_heap, tmp);

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
