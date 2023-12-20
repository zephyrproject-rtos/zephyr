/*
 * Copyright (c) 2023 Arduino srl
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdio.h>

#include <zephyr/llext/loader.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/debug_str.h>
#include <zephyr/llext/elf.h>

static const char * const STT_DESC[] = {
	[STT_NOTYPE]  = "NOTYPE",
	[STT_OBJECT]  = "OBJECT",
	[STT_FUNC]    = "FUNC",
	[STT_SECTION] = "SECTION",
	[STT_FILE]    = "FILE",
	[STT_COMMON]  = "COMMON",
};

static const char * const STB_DESC[] = {
	[STB_LOCAL]  = "LOCAL",
	[STB_GLOBAL] = "GLOBAL",
	[STB_WEAK]   = "WEAK",
};

static const char * const MEM_DESC[] = {
	[LLEXT_MEM_TEXT]     = "TEXT",
	[LLEXT_MEM_DATA]     = "DATA",
	[LLEXT_MEM_RODATA]   = "RODATA",
	[LLEXT_MEM_BSS]      = "BSS",
	[LLEXT_MEM_EXPORT]   = "EXPORT",
	[LLEXT_MEM_SYMTAB]   = "SYMTAB",
	[LLEXT_MEM_STRTAB]   = "STRTAB",
	[LLEXT_MEM_SHSTRTAB] = "SHSTRTAB",
};

const char *elf_st_bind_str(unsigned int stb)
{
	static char num_buf[12];

	if ((stb < ARRAY_SIZE(STB_DESC)) && STB_DESC[stb]) {
		return STB_DESC[stb];
	}

	/* not found, return number */
	snprintf(num_buf, sizeof(num_buf), "(%u)", stb);
	return num_buf;
}

const char *elf_st_type_str(unsigned int stt)
{
	static char num_buf[12];

	if ((stt < ARRAY_SIZE(STT_DESC)) && STT_DESC[stt]) {
		return STT_DESC[stt];
	}

	/* not found, return number */
	snprintf(num_buf, sizeof(num_buf), "(%u)", stt);
	return num_buf;
}

const char *llext_mem_str(enum llext_mem mem)
{
	static char num_buf[12];

	if ((mem < ARRAY_SIZE(MEM_DESC)) && MEM_DESC[mem]) {
		return MEM_DESC[mem];
	}

	/* not found, return number */
	snprintf(num_buf, sizeof(num_buf), "(%d)", mem);
	return num_buf;
}

const char *elf_sect_str(struct llext_loader *ldr, unsigned int shndx)
{
	switch (shndx) {
	case SHN_UNDEF:
		return "UNDEF";
	case SHN_ABS:
		return "ABS";
	case SHN_COMMON:
		return "COMMON";
	default:
		return llext_mem_str(ldr->sect_map[shndx]);
	}
}

static const char *llext_find_sym_by_addr(const struct llext_symtable *sym_table, uintptr_t addr)
{
	if (sym_table == NULL) {
		/* Built-in symbol table */
		STRUCT_SECTION_FOREACH(llext_const_symbol, sym) {
			if (addr == (uintptr_t) sym->addr) {
				return sym->name;
			}
		}
	} else {
		/* find symbols in module */
		for (size_t i = 0; i < sym_table->sym_cnt; i++) {
			if (addr == (uintptr_t) sym_table->syms[i].addr) {
				return sym_table->syms[i].name;
			}
		}
	}

	return NULL;
}

#define BUF_LEN 32
#define NUM_BUFS 4
const char *llext_addr_str(struct llext_loader *ldr, struct llext *ext, uintptr_t addr)
{
	static char str_bufs[NUM_BUFS][BUF_LEN];
	static int bufidx;
	const char *name;
	bool found = false;

	/* rotate buffers */
	bufidx = (bufidx + 1) % NUM_BUFS;

	/* search for the address in the built-in symbol table */
	name = llext_find_sym_by_addr(NULL, addr);
	if (name) {
		snprintf(str_bufs[bufidx], BUF_LEN, "builtin %s", name);
		found = true;
	}

	if (!found) {
		/* search for the address in the extension symbol table */
		name = llext_find_sym_by_addr(&ext->sym_tab, addr);
		if (name) {
			snprintf(str_bufs[bufidx], BUF_LEN, "global %s", name);
			found = true;
		}
	}

	if (!found) {
		/* search for the address in the module memory sections */
		for (enum llext_mem m = 0; m < LLEXT_MEM_COUNT; ++m) {
			if (ext->mem[m] && (addr >= (uintptr_t)ext->mem[m]) &&
			    (addr < (uintptr_t)ext->mem[m] + ext->mem_size[m])) {
				snprintf(str_bufs[bufidx], BUF_LEN, "mem %s+0x%lx",
					 llext_mem_str(m), addr - (uintptr_t)ext->mem[m]);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		/* unknown address, print as number */
		snprintf(str_bufs[bufidx], BUF_LEN, "addr 0x%lx (?)", addr);
	}

	str_bufs[bufidx][BUF_LEN-1] = '\0';
	return str_bufs[bufidx];
}
