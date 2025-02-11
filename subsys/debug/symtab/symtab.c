/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdbool.h>

#include <zephyr/debug/symtab.h>

const struct symtab_info *const symtab_get(void)
{
	extern const struct symtab_info z_symtab;

	return &z_symtab;
}

const char *const symtab_find_symbol_name(uintptr_t addr, uint32_t *offset)
{
	const struct symtab_info *const symtab = symtab_get();
	const uint32_t symbol_offset = addr - symtab->first_addr;
	uint32_t left = 0, right = symtab->length;
	uint32_t ret_offset = 0;
	const char *ret_name = "?";

	/* No need to search if the address is out-of-bound */
	if (symbol_offset < symtab->entries[symtab->length].offset) {
		while (left <= right) {
			uint32_t mid = left + (right - left) / 2;

			if ((symbol_offset >= symtab->entries[mid].offset) &&
			(symbol_offset < symtab->entries[mid + 1].offset)) {
				ret_offset = symbol_offset - symtab->entries[mid].offset;
				ret_name = symtab->entries[mid].name;
				break;
			} else if (symbol_offset < symtab->entries[mid].offset) {
				right = mid - 1;
			} else {
				left = mid + 1;
			}
		}
	}

	if (offset != NULL) {
		*offset = ret_offset;
	}

	return ret_name;
}
