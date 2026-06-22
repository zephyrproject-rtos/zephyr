/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdbool.h>

#include <zephyr/debug/symtab.h>
#include <zephyr/shell/shell.h>

const struct symtab_info *symtab_get(void)
{
	extern const struct symtab_info z_symtab;

	return &z_symtab;
}

const char *symtab_find_symbol_name(uintptr_t addr, uint32_t *offset)
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

#ifdef CONFIG_SYMTAB_SHELL
static int cmd_symtab_list(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct symtab_info *const symtab = symtab_get();

	for (uint32_t i = 0; i < symtab->length; i++) {
		const struct z_symtab_entry *const entry = &symtab->entries[i];

		shell_print(sh, "%d\t%p  %s", i + 1, (void *)(entry->offset + symtab->first_addr),
			    entry->name);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(symtab_cmds,
	SHELL_CMD(list, NULL, "Show symbol list.", cmd_symtab_list),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(symtab, &symtab_cmds, "Symbol table shell commands", NULL);
#endif /* CONFIG_SYMTAB_SHELL */
