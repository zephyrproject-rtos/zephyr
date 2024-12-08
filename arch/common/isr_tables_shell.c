/*
 * Copyright (c) 2024 Meta Platforms.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/symtab.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sw_isr_table.h>

static void dump_isr_table_entry(const struct shell *sh, int idx, struct _isr_table_entry *entry)
{

	if ((entry->isr == z_irq_spurious) || (entry->isr == NULL)) {
		return;
	}
#ifdef CONFIG_SYMTAB
	const char *name = symtab_find_symbol_name((uintptr_t)entry->isr, NULL);

	shell_print(sh, "%4d: %s(%p)", idx, name, entry->arg);
#else
	shell_print(sh, "%4d: %p(%p)", idx, entry->isr, entry->arg);
#endif /* CONFIG_SYMTAB */
}

static int cmd_sw_isr_table(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "_sw_isr_table[%d]\n", IRQ_TABLE_SIZE);

	for (int idx = 0; idx < IRQ_TABLE_SIZE; idx++) {
		dump_isr_table_entry(sh, idx, &_sw_isr_table[idx]);
	}

	return 0;
}

#ifdef CONFIG_SHARED_INTERRUPTS
static int cmd_shared_sw_isr_table(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "z_shared_sw_isr_table[%d][%d]\n", IRQ_TABLE_SIZE,
		    CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS);

	for (int idx = 0; idx < IRQ_TABLE_SIZE; idx++) {
		for (int c = 0; c < z_shared_sw_isr_table[idx].client_num; c++) {
			dump_isr_table_entry(sh, idx, &z_shared_sw_isr_table[idx].clients[c]);
		}
	}

	return 0;
}
#endif /* CONFIG_SHARED_INTERRUPTS */

SHELL_STATIC_SUBCMD_SET_CREATE(isr_table_cmds,
			       SHELL_CMD_ARG(sw_isr_table, NULL,
					     "Dump _sw_isr_table.\n"
					     "Usage: isr_table sw_isr_table",
					     cmd_sw_isr_table, 1, 0),
#ifdef CONFIG_SHARED_INTERRUPTS
			       SHELL_CMD_ARG(shared_sw_isr_table, NULL,
					     "Dump z_shared_sw_isr_table.\n"
					     "Usage: isr_table shared_sw_isr_table",
					     cmd_shared_sw_isr_table, 1, 0),
#endif /* CONFIG_SHARED_INTERRUPTS */
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(isr_table, &isr_table_cmds, "ISR tables shell command",
		       NULL, 0, 0);
