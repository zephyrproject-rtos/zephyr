/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2024 Schneider Electric
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
 * @file
 * @brief Support for linkable loadable extensions
 *
 * This file describes the APIs for loading and interacting with Linkable
 * Loadable Extensions (LLEXTs) in Zephyr.
 *
 * @defgroup llext_apis Linkable loadable extensions
 * @since 3.5
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/**
 * @brief List of memory regions stored or referenced in the LLEXT subsystem
 *
 * This enum lists the different types of memory regions that are used by the
 * LLEXT subsystem. The names match common ELF file section names; but note
 * that at load time multiple ELF sections with similar flags may be merged
 * together into a single memory region.
 */
enum llext_mem {
	LLEXT_MEM_TEXT,         /**< Executable code */
	LLEXT_MEM_DATA,         /**< Initialized data */
	LLEXT_MEM_RODATA,       /**< Read-only data */
	LLEXT_MEM_BSS,          /**< Uninitialized data */
	LLEXT_MEM_EXPORT,       /**< Exported symbol table */
	LLEXT_MEM_SYMTAB,       /**< Symbol table */
	LLEXT_MEM_STRTAB,       /**< Symbol name strings */
	LLEXT_MEM_SHSTRTAB,     /**< Section name strings */
	LLEXT_MEM_PREINIT,      /**< Array of early setup functions */
	LLEXT_MEM_INIT,         /**< Array of setup functions */
	LLEXT_MEM_FINI,         /**< Array of cleanup functions */

	LLEXT_MEM_COUNT,        /**< Number of regions managed by LLEXT */
};

/** @cond ignore */

/* Number of memory partitions used by LLEXT */
#define LLEXT_MEM_PARTITIONS (LLEXT_MEM_BSS+1)

struct llext_loader;
/** @endcond */

/* Maximim number of dependency LLEXTs */
#define LLEXT_MAX_DEPENDENCIES 8

/**
 * @brief Structure describing a linkable loadable extension
 *
 * This structure holds the data for a loaded extension. It is created by the
 * @ref llext_load function and destroyed by the @ref llext_unload function.
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

	/** Lookup table of memory regions */
	void *mem[LLEXT_MEM_COUNT];

	/** Is the memory for this region allocated on heap? */
	bool mem_on_heap[LLEXT_MEM_COUNT];

	/** Size of each stored region */
	size_t mem_size[LLEXT_MEM_COUNT];

	/** Total llext allocation size */
	size_t alloc_size;

	/**
	 * Table of all global symbols in the extension; used internally as
	 * part of the linking process. E.g. if the extension is built out of
	 * several files, if any symbols are referenced between files, this
	 * table will be used to link them.
	 */
	struct llext_symtable sym_tab;

	/**
	 * Table of symbols exported by the llext via @ref LL_EXTENSION_SYMBOL.
	 * This can be used in the main Zephyr binary to find symbols in the
	 * extension.
	 */
	struct llext_symtable exp_tab;

	/** Extension use counter, prevents unloading while in use */
	unsigned int use_count;

	/** Array of extensions, whose symbols this extension accesses */
	struct llext *dependency[LLEXT_MAX_DEPENDENCIES];
};

/**
 * @brief Advanced llext_load parameters
 *
 * This structure contains advanced parameters for @ref llext_load.
 */
struct llext_load_param {
	/** Perform local relocation */
	bool relocate_local;
	/**
	 * Use the virtual symbol addresses from the ELF, not addresses within
	 * the memory buffer, when calculating relocation targets.
	 */
	bool pre_located;
	/**
	 * Extensions can implement custom ELF sections to be loaded in specific
	 * memory regions, detached from other sections of compatible types.
	 * This optional callback checks whether a section should be detached.
	 */
	bool (*section_detached)(const elf_shdr_t *shdr);
};

/** Default initializer for @ref llext_load_param */
#define LLEXT_LOAD_PARAM_DEFAULT { .relocate_local = true, }

/**
 * @brief Find an llext by name
 *
 * @param[in] name String name of the llext
 * @returns a pointer to the @ref llext, or `NULL` if not found
 */
struct llext *llext_by_name(const char *name);

/**
 * @brief Iterate over all loaded extensions
 *
 * Calls a provided callback function for each registered extension or until the
 * callback function returns a non-0 value.
 *
 * @param[in] fn callback function
 * @param[in] arg a private argument to be provided to the callback function
 * @returns the value returned by the last callback invocation
 * @retval 0 if no extensions are registered
 */
int llext_iterate(int (*fn)(struct llext *ext, void *arg), void *arg);

/**
 * @brief Load and link an extension
 *
 * Loads relevant ELF data into memory and provides a structure to work with it.
 *
 * @param[in] loader An extension loader that provides input data and context
 * @param[in] name A string identifier for the extension
 * @param[out] ext Pointer to the newly allocated @ref llext structure
 * @param[in] ldr_parm Optional advanced load parameters (may be `NULL`)
 *
 * @returns the previous extension use count on success, or a negative error code.
 * @retval -ENOMEM Not enough memory
 * @retval -ENOEXEC Invalid ELF stream
 * @retval -ENOTSUP Unsupported ELF features
 */
int llext_load(struct llext_loader *loader, const char *name, struct llext **ext,
	       const struct llext_load_param *ldr_parm);

/**
 * @brief Unload an extension
 *
 * @param[in] ext Extension to unload
 */
int llext_unload(struct llext **ext);

/** @brief Entry point function signature for an extension. */
typedef void (*llext_entry_fn_t)(void *user_data);

/**
 * @brief Calls bringup functions for an extension.
 *
 * Must be called before accessing any symbol in the extension. Will execute
 * the extension's own setup functions in the caller context.
 * @see llext_bootstrap
 *
 * @param[in] ext Extension to initialize.
 * @returns 0 on success, or a negative error code.
 * @retval -EFAULT A relocation issue was detected
 */
int llext_bringup(struct llext *ext);

/**
 * @brief Calls teardown functions for an extension.
 *
 * Will execute the extension's own cleanup functions in the caller context.
 * After this function completes, the extension is no longer usable and must be
 * fully unloaded with @ref llext_unload.
 * @see llext_bootstrap
 *
 * @param[in] ext Extension to de-initialize.
 * @returns 0 on success, or a negative error code.
 * @retval -EFAULT A relocation issue was detected
 */
int llext_teardown(struct llext *ext);

/**
 * @brief Bring up, execute, and teardown an extension.
 *
 * Calls the extension's own setup functions, an additional entry point and
 * the extension's cleanup functions in the current thread context.
 *
 * This is a convenient wrapper around @ref llext_bringup and @ref
 * llext_teardown that matches the @ref k_thread_entry_t signature, so it can
 * be directly started as a new user or kernel thread via @ref k_thread_create.
 *
 * @param[in] ext Extension to execute. Passed as `p1` in @ref k_thread_create.
 * @param[in] entry_fn Main entry point of the thread after performing
 *                     extension setup. Passed as `p2` in @ref k_thread_create.
 * @param[in] user_data Argument passed to @a entry_fn. Passed as `p3` in
 *                      @ref k_thread_create.
 */
void llext_bootstrap(struct llext *ext, llext_entry_fn_t entry_fn, void *user_data);

/**
 * @brief Get pointers to setup or cleanup functions for an extension.
 *
 * This syscall can be used to get the addresses of all the functions that
 * have to be called for full extension setup or cleanup.
 *
 * @see llext_bootstrap
 *
 * @param[in]    ext Extension to initialize.
 * @param[in]    is_init `true` to get functions to be called at setup time,
 *                       `false` to get the cleanup ones.
 * @param[inout] buf Buffer to store the function pointers in. Can be `NULL`
 *                   to only get the minimum required size.
 * @param[in]    size Allocated size of the buffer in bytes.
 * @returns the size used by the array in bytes, or a negative error code.
 * @retval -EFAULT A relocation issue was detected
 * @retval -ENOMEM Array does not fit in the allocated buffer
 */
__syscall ssize_t llext_get_fn_table(struct llext *ext, bool is_init, void *buf, size_t size);

/**
 * @brief Find the address for an arbitrary symbol.
 *
 * Searches for a symbol address, either in the list of symbols exported by
 * the main Zephyr binary or in an extension's symbol table.
 *
 * @param[in] sym_table Symbol table to lookup symbol in, or `NULL` to search
 *                      in the main Zephyr symbol table
 * @param[in] sym_name Symbol name to find
 *
 * @returns the address of symbol in memory, or `NULL` if not found
 */
const void *llext_find_sym(const struct llext_symtable *sym_table, const char *sym_name);

/**
 * @brief Call a function by name.
 *
 * Expects a symbol representing a `void fn(void)` style function exists
 * and may be called.
 *
 * @param[in] ext Extension to call function in
 * @param[in] sym_name Function name (exported symbol) in the extension
 *
 * @retval 0 Success
 * @retval -ENOENT Symbol name not found
 */
int llext_call_fn(struct llext *ext, const char *sym_name);

/**
 * @brief Add an extension to a memory domain.
 *
 * Allows an extension to be executed in user mode threads when memory
 * protection hardware is enabled by adding memory partitions covering the
 * extension's memory regions to a memory domain.
 *
 * @param[in] ext Extension to add to a domain
 * @param[in] domain Memory domain to add partitions to
 *
 * @returns 0 on success, or a negative error code.
 * @retval -ENOSYS Option @kconfig{CONFIG_USERSPACE} is not enabled or supported
 */
int llext_add_domain(struct llext *ext, struct k_mem_domain *domain);

/**
 * @brief Architecture specific opcode update function
 *
 * ELF files include sections describing a series of _relocations_, which are
 * instructions on how to rewrite opcodes given the actual placement of some
 * symbolic data such as a section, function, or object. These relocations
 * are architecture specific and each architecture supporting LLEXT must
 * implement this.
 *
 * @param[in] rel Relocation data provided by ELF
 * @param[in] loc Address of opcode to rewrite
 * @param[in] sym_base_addr Address of symbol referenced by relocation
 * @param[in] sym_name Name of symbol referenced by relocation
 * @param[in] load_bias `.text` load address
 * @retval 0 Success
 * @retval -ENOTSUP Unsupported relocation
 * @retval -ENOEXEC Invalid relocation
 */
int arch_elf_relocate(elf_rela_t *rel, uintptr_t loc,
			     uintptr_t sym_base_addr, const char *sym_name, uintptr_t load_bias);

/**
 * @brief Locates an ELF section in the file.
 *
 * Searches for a section by name in the ELF file and returns its offset.
 *
 * @param[in] loader Extension loader data and context
 * @param[in] search_name Section name to search for
 * @returns the section offset or a negative error code
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

#include <zephyr/syscalls/llext.h>

#endif /* ZEPHYR_LLEXT_H */
