/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_H
#define ZEPHYR_LLEXT_H

#include <zephyr/sys/slist.h>
#include <zephyr/llext/elf.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/kernel.h>
#include <sys/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Linkable loadable extensions
 * @defgroup llext Linkable loadable extensions
 * @since 3.5
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/**
 * @brief List of ELF regions that are stored or referenced in the llext
 */
enum llext_mem {
	LLEXT_MEM_TEXT,
	LLEXT_MEM_DATA,
	LLEXT_MEM_RODATA,
	LLEXT_MEM_BSS,
	LLEXT_MEM_EXPORT,
	LLEXT_MEM_SYMTAB,
	LLEXT_MEM_STRTAB,
	LLEXT_MEM_SHSTRTAB,

	LLEXT_MEM_COUNT,
};

#define LLEXT_MEM_PARTITIONS (LLEXT_MEM_BSS+1)

struct llext_loader;

/**
 * @brief Linkable loadable extension
 */
struct llext {
	/** @cond ignore */
	sys_snode_t _llext_list;

#ifdef CONFIG_USERSPACE
	struct k_mem_partition mem_parts[LLEXT_MEM_PARTITIONS];
	struct k_mem_domain mem_domain;
#endif

	/** @endcond */

	/** Name of the llext */
	char name[16];

	/** Lookup table of llext memory regions */
	void *mem[LLEXT_MEM_COUNT];

	/** Is the memory for this section allocated on heap? */
	bool mem_on_heap[LLEXT_MEM_COUNT];

	/** Size of each stored section */
	size_t mem_size[LLEXT_MEM_COUNT];

	/** Total llext allocation size */
	size_t alloc_size;

	/*
	 * These are all global symbols in the extension, all of them don't
	 * have to be exported to other extensions, but this table is needed for
	 * faster internal linking, e.g. if the extension is built out of
	 * several files, if any symbols are referenced between files, this
	 * table will be used to link them.
	 */
	struct llext_symtable sym_tab;

	/** Exported symbols from the llext, may be linked against by other llext */
	struct llext_symtable exp_tab;

	/** Extension use counter, prevents unloading while in use */
	unsigned int use_count;
};

/**
 * @brief Find an llext by name
 *
 * @param[in] name String name of the llext
 * @retval NULL if no llext not found
 * @retval llext if llext found
 */
struct llext *llext_by_name(const char *name);

/**
 * @brief Iterate overall registered llext instances
 *
 * Calls a provided callback function for each registered extension or until the
 * callback function returns a non-0 value.
 *
 * @param[in] fn callback function
 * @param[in] arg a private argument to be provided to the callback function
 * @retval 0 if no extensions are registered
 * @retval value returned by the most recent callback invocation
 */
int llext_iterate(int (*fn)(struct llext *ext, void *arg), void *arg);

/**
 * @brief llext loader parameters
 *
 * These are parameters, not saved in the permanent llext context, needed only
 * for the loader
 */
struct llext_load_param {
	/** Should local relocation be performed */
	bool relocate_local;
};

#define LLEXT_LOAD_PARAM_DEFAULT {.relocate_local = true,}

/**
 * @brief Load and link an extension
 *
 * Loads relevant ELF data into memory and provides a structure to work with it.
 *
 * Only relocatable ELF files are currently supported (partially linked).
 *
 * @param[in] loader An extension loader that provides input data and context
 * @param[in] name A string identifier for the extension
 * @param[out] ext This will hold the pointer to the llext struct
 * @param[in] ldr_parm Loader parameters
 *
 * @retval 0 Success
 * @retval -ENOMEM Not enough memory
 * @retval -EINVAL Invalid ELF stream
 */
int llext_load(struct llext_loader *loader, const char *name, struct llext **ext,
	       struct llext_load_param *ldr_parm);

/**
 * @brief Unload an extension
 *
 * @param[in] ext Extension to unload
 */
int llext_unload(struct llext **ext);

/**
 * @brief Find the address for an arbitrary symbol name.
 *
 * @param[in] sym_table Symbol table to lookup symbol in, if NULL uses base table
 * @param[in] sym_name Symbol name to find
 *
 * @retval NULL if no symbol found
 * @retval addr Address of symbol in memory if found
 */
const void * const llext_find_sym(const struct llext_symtable *sym_table, const char *sym_name);

/**
 * @brief Call a function by name
 *
 * Expects a symbol representing a void fn(void) style function exists
 * and may be called.
 *
 * @param[in] ext Extension to call function in
 * @param[in] sym_name Function name (exported symbol) in the extension
 *
 * @retval 0 success
 * @retval -EINVAL invalid symbol name
 */
int llext_call_fn(struct llext *ext, const char *sym_name);

/**
 * @brief Add the known memory partitions of the extension to a memory domain
 *
 * Allows an extension to be executed in supervisor or user mode threads when
 * memory protection hardware is enabled.
 *
 * @param[in] ext Extension to add to a domain
 * @param[in] domain Memory domain to add partitions to
 *
 * @retval 0 success
 * @retval -errno error
 */
int llext_add_domain(struct llext *ext, struct k_mem_domain *domain);

/**
 * @brief Architecture specific function for updating op codes given a relocation
 *
 * Elf files contain a series of relocations described in a section. These relocation
 * instructions are architecture specific and each architecture supporting extensions
 * must implement this. They are instructions on how to rewrite opcodes given
 * the actual placement of some symbolic data such as a section, function,
 * or object.
 *
 * @param[in] rel Relocation data provided by elf
 * @param[in] opaddr Address of operation to rewrite with relocation
 * @param[in] opval Value of looked up symbol to relocate
 */
void arch_elf_relocate(elf_rela_t *rel, uintptr_t opaddr, uintptr_t opval);

/**
 * @brief Find an ELF section
 *
 * @param loader Extension loader data and context
 * @param search_name Section name to search for
 * @retval Section offset or a negative error code
 */
ssize_t llext_find_section(struct llext_loader *loader, const char *search_name);

/**
 * @brief Architecture specific function for updating addresses via relocation table
 *
 * @param[in] loader Extension loader data and context
 * @param[in] ext Extension to call function in
 * @param[in] rel Relocation data provided by elf
 * @param[in] sym Corresponding symbol table entry
 * @param[in] got_offset Offset within a relocation table
 */
void arch_elf_relocate_local(struct llext_loader *loader, struct llext *ext,
			     const elf_rela_t *rel, const elf_sym_t *sym, size_t got_offset);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_H */
