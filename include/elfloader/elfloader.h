/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ELFLOADER_H
#define ELFLOADER_H

#include <elf.h>

/* module exported symbol structure */
typedef struct zmodule_symbol {
	elf_addr addr;
	const char *name;
} zmodule_symbol_t;

/* module structure */
typedef struct zmodule {
	elf_addr virt_start_addr;
	elf_addr load_start_addr;
	elf_word mem_sz;
	zmodule_symbol_t *sym_list;
	elf_word sym_cnt;
} zmodule_t;

/*
 * Load a module into memory dynamically
 */
void *zmodule_load(const char *filename);

/*
 * Find arbitrary symbol's address according to its name.
 * If module pointer is 0, then it will find symbols from zephyr kernel.
 */
void *zmodule_find_sym(zmodule_t *module, const char *sym_name);

#define KERNEL_MODULE ((void *)0x0)

/* relocation for shared object file */
void elfloader_arch_relocate_dyn(zmodule_t *module, elf_rel_t *rel,
		elf_addr sym_addr);

/* relocation for relocatable object file */
void elfloader_arch_relocate_rel(elf_rel_t *rel, elf_shdr_t *apply_sec,
		elf_addr sym_addr);

/*
 * Unload a module
 */
void zmodule_close(zmodule_t *module);

#endif /* ELFLOADER_H */
