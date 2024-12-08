/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_SYMTAB_H_
#define ZEPHYR_INCLUDE_DEBUG_SYMTAB_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup symtab_apis Symbol Table API
 * @ingroup os_services
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 */

struct z_symtab_entry {
	const uint32_t offset;
	const char *const name;
};

/**
 * INTERNAL_HIDDEN @endcond
 */

struct symtab_info {
	/* Absolute address of the first symbol */
	const uintptr_t first_addr;
	/* Number of symbol entries */
	const uint32_t length;
	/* Symbol entries */
	const struct z_symtab_entry *const entries;
};

/**
 * @brief Get the pointer to the symbol table.
 *
 * @return Pointer to the symbol table.
 */
const struct symtab_info *symtab_get(void);

/**
 * @brief Find the symbol name with a binary search
 *
 * @param[in] addr Address of the symbol to find
 * @param[out] offset Offset of the symbol from the nearest symbol. If the symbol can't be found,
 * this will be 0.
 *
 * @return Name of the nearest symbol if found, otherwise "?" is returned.
 */
const char *symtab_find_symbol_name(uintptr_t addr, uint32_t *offset);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEBUG_SYMTAB_H_ */
