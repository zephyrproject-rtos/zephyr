/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/sys/slist.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/modules/elf.h>
#include <zephyr/modules/module.h>
#include <zephyr/modules/buf_stream.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modules_shell, CONFIG_MODULES_LOG_LEVEL);

#define MODULE_LIST_HELP "List loaded modules and their size in memory"

#define MODULE_LOAD_HEX_HELP									\
	"Load an elf file encoded in hex directly from the shell input. Syntax:\n"		\
	"<module_name> <module_hex_string>"

#define MODULE_UNLOAD_HELP									\
	"Unload a module by name. Syntax:\n"							\
	"<module_name>"

#define MODULE_LIST_SYMBOLS_HELP								\
	"List module symbols. Syntax:\n"							\
	"<module_name>"

#define MODULE_CALL_FN_HELP									\
	"Call module function with prototype void fn(void). Syntax:\n"				\
	"<module_name> <function_name>"

static int cmd_module_list_symbols(const struct shell *sh, size_t argc, char *argv[])
{
	struct module *m = module_from_name(argv[1]);

	if (m == NULL) {
		shell_print(sh, "No such module %s", argv[1]);
		return -EINVAL;
	}

	shell_print(sh, "Module: %s symbols", m->name);
	shell_print(sh, "| Symbol           | Address    |\n");
	for (elf_word i = 0; i < m->sym_tab.sym_cnt; i++) {
		shell_print(sh, "| %16s | %p |\n", m->sym_tab.syms[i].name,
			    m->sym_tab.syms[i].addr);
	}

	return 0;
}

static void module_name_get(size_t idx, struct shell_static_entry *entry)
{
	sys_slist_t *mlist = module_list();
	sys_snode_t *node = sys_slist_peek_head(mlist);

	entry->syntax = NULL;

	for (int i = 0; i < idx; i++) {
		node = sys_slist_peek_next(node);

		if (node == NULL) {
			goto out;

			}
	}

	struct module *m = CONTAINER_OF(node, struct module, _mod_list);

	entry->syntax = m->name;
out:
	entry->syntax = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

}

SHELL_DYNAMIC_CMD_CREATE(msub_module_name, module_name_get);

static int cmd_module_list(const struct shell *sh, size_t argc, char *argv[])
{
	sys_snode_t *node;
	struct module *m;

	shell_print(sh, "| Name             | Size        |\n");
	SYS_SLIST_FOR_EACH_NODE(module_list(), node) {
		m = CONTAINER_OF(node, struct module, _mod_list);
		shell_print(sh, "| %16s | %12d |\n", m->name, m->mem_size);
	}

	return 0;
}

#define MODULE_MAX_SIZE 8192
static uint8_t module_buf[MODULE_MAX_SIZE];

static int cmd_module_load_hex(const struct shell *sh, size_t argc, char *argv[])
{
	char name[16];
	size_t hex_len = strnlen(argv[2], MODULE_MAX_SIZE*2+1);
	size_t bin_len = hex_len/2;

	if (bin_len > MODULE_MAX_SIZE) {
		shell_print(sh, "Module %d bytes too large to load, max %d bytes\n", hex_len/2,
			    MODULE_MAX_SIZE);
		return -ENOMEM;
	}

	strncpy(name, argv[1], sizeof(name));

	size_t module_buf_len = hex2bin(argv[2], hex_len, module_buf, MODULE_MAX_SIZE);
	struct module_buf_stream buf_stream = MODULE_BUF_STREAM(module_buf, module_buf_len);
	struct module_stream *stream = &buf_stream.stream;

	LOG_DBG("hex2bin hex len %d, module buf sz %d, read %d",
		hex_len, MODULE_MAX_SIZE, module_buf_len);
	LOG_HEXDUMP_DBG(module_buf, 4, "4 byte MAGIC");

	struct module *m;
	int res = module_load(stream, name, &m);

	if (res == 0) {
		shell_print(sh, "Successfully loaded module %s, addr %p\n", m->name, m);
	} else {
		shell_print(sh, "Failed to load module %s, return code %d\n", name, res);
	}

	return 0;
}

static int cmd_module_unload(const struct shell *sh, size_t argc, char *argv[])
{
	struct module *m = module_from_name(argv[1]);

	if (m == NULL) {
		shell_print(sh, "No such module %s", argv[1]);
		return -EINVAL;
	}

	module_unload(m);
	shell_print(sh, "Unloaded module %s\n", argv[1]);

	return 0;
}

static int cmd_module_call_fn(const struct shell *sh, size_t argc, char *argv[])
{
	struct module *m = module_from_name(argv[1]);

	if (m == NULL) {
		shell_print(sh, "No such module %s", argv[1]);
		return -EINVAL;
	}

	module_call_fn(m, argv[2]);

	return 0;
}


/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_module,
	SHELL_CMD(list, NULL, MODULE_LIST_HELP, cmd_module_list),
	SHELL_CMD_ARG(load_hex, NULL, MODULE_LOAD_HEX_HELP, cmd_module_load_hex,
		3, 0),
	SHELL_CMD_ARG(unload, &msub_module_name, MODULE_UNLOAD_HELP, cmd_module_unload, 2, 0),
	SHELL_CMD_ARG(list_symbols, &msub_module_name, MODULE_LIST_SYMBOLS_HELP,
		      cmd_module_list_symbols, 2, 0),
	SHELL_CMD_ARG(call_fn, &msub_module_name, MODULE_CALL_FN_HELP,
		      cmd_module_call_fn, 3, 0),

	SHELL_SUBCMD_SET_END
	);
/* clang-format on */

SHELL_CMD_REGISTER(module, &sub_module, "Loadable module commands", NULL);
