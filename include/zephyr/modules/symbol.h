/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULE_SYMBOL_H
#define ZEPHYR_MODULE_SYMBOL_H

#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Loadable module symbol type and export macro
 * @ingroup modules Modules
 * @{
 */

/**
 * @brief A symbol (named memory address)
 */
struct module_symbol {
	/* string name of the symbol */
	char *name;

	/* If an object is exported, it might be writable, so no "const" */
	void *addr;
};

/**
 * @brief Export a symbol to a table of symbols
 *
 * Takes a symbol (function or object) by symbolic name and adds the name
 * and address of the symbol to a table of symbols that may be used for linking.
 *
 * If a module exports its functions, they will have to be re-linked (relocated),
 * so these structures cannot be constant as the address of the symbols may
 * will need to be updated.
 */
#define EXPORT_SYMBOL(x)					 \
	STRUCT_SECTION_ITERABLE(module_symbol, x ## _sym) = { \
		.name = STRINGIFY(x), .addr = &x,		 \
	}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_MODULE_SYMBOL_H */
