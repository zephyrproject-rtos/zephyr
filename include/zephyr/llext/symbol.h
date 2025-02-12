/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_SYMBOL_H
#define ZEPHYR_LLEXT_SYMBOL_H

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/toolchain.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Linkable loadable extension symbol definitions
 *
 * This file provides a set of macros and structures for defining and exporting
 * symbols from the base image to extensions and vice versa, so that proper
 * linking can be done between the two entities.
 *
 * @defgroup llext_symbols Exported symbol definitions
 * @ingroup llext_apis
 * @{
 */

/**
 * @brief Constant symbols are unchangeable named memory addresses
 *
 * Symbols may be named function or global objects that have been exported
 * for linking. These constant symbols are useful in the base image
 * as they may be placed in ROM.
 *
 * @note When updating this structure, make sure to also update the
 * 'scripts/build/llext_prepare_exptab.py' build script.
 */
struct llext_const_symbol {
	/** At build time, we always write to 'name'.
	 *  At runtime, which field is used depends
	 *  on CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID.
	 */
	union {
		/** Name of symbol */
		const char *const name;

		/** Symbol Link Identifier */
		const uintptr_t slid;
	};

	/** Address of symbol */
	const void *const addr;
};
BUILD_ASSERT(sizeof(struct llext_const_symbol) == 2 * sizeof(uintptr_t));

/**
 * @brief Symbols are named memory addresses
 *
 * Symbols may be named function or global objects that have been exported
 * for linking. These are mutable and should come from extensions where
 * the location may need updating depending on where memory is placed.
 */
struct llext_symbol {
	/** Name of symbol */
	const char *name;

	/** Address of symbol */
	void *addr;
};


/**
 * @brief A symbol table
 *
 * An array of symbols
 */
struct llext_symtable {
	/** Number of symbols in the table */
	size_t sym_cnt;

	/** Array of symbols */
	struct llext_symbol *syms;
};


/**
 * @brief Export a constant symbol to extensions
 *
 * Takes a symbol (function or object) by symbolic name and adds the name
 * and address of the symbol to a table of symbols that may be referenced
 * by extensions.
 *
 * @param x Symbol to export to extensions
 */
#if defined(CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID)
#define EXPORT_SYMBOL(x)							\
	static const char Z_GENERIC_SECTION("llext_exports_strtab") __used	\
		x ## _sym_name[] = STRINGIFY(x);				\
	static const STRUCT_SECTION_ITERABLE(llext_const_symbol, x ## _sym) = {	\
		.name = x ## _sym_name, .addr = (const void *)&x,		\
	}
#elif defined(CONFIG_LLEXT)
#define EXPORT_SYMBOL(x)							\
	static const STRUCT_SECTION_ITERABLE(llext_const_symbol, x ## _sym) = {	\
		.name = STRINGIFY(x), .addr = (const void *)&x,			\
	}
#else
#define EXPORT_SYMBOL(x)
#endif

/**
 * @brief Exports a symbol from an extension to the base image
 *
 * This macro can be used in extensions to add a symbol (function or object)
 * to the extension's exported symbol table, so that it may be referenced by
 * the base image.
 *
 * @param x Extension symbol to export to the base image
 */
#if defined(CONFIG_LLEXT) && defined(LL_EXTENSION_BUILD)
#define LL_EXTENSION_SYMBOL(x)							\
	static const struct llext_const_symbol					\
			Z_GENERIC_SECTION(".exported_sym") __used		\
			x ## _sym = {						\
		.name = STRINGIFY(x), .addr = (const void *)&x,			\
	}
#else
#define LL_EXTENSION_SYMBOL(x)
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_LLEXT_SYMBOL_H */
