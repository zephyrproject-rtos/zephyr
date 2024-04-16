/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/sys/slist.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/buf_loader.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(llext_shell, CONFIG_LLEXT_LOG_LEVEL);

#define LLEXT_LIST_HELP "List loaded extensions and their size in memory"

#define LLEXT_LOAD_HEX_HELP								\
	"Load an elf file encoded in hex directly from the shell input. Syntax:\n"	\
	"<ext_name> <ext_hex_string>"

#define LLEXT_UNLOAD_HELP								\
	"Unload an extension by name. Syntax:\n"					\
	"<ext_name>"

#define LLEXT_LIST_SYMBOLS_HELP								\
	"List extension symbols. Syntax:\n"						\
	"<ext_name>"

#define LLEXT_CALL_FN_HELP								\
	"Call extension function with prototype void fn(void). Syntax:\n"		\
	"<ext_name> <function_name>"

static int cmd_llext_list_symbols(const struct shell *sh, size_t argc, char *argv[])
{
	struct llext *m = llext_by_name(argv[1]);

	if (m == NULL) {
		shell_print(sh, "No such llext %s", argv[1]);
		return -EINVAL;
	}

	shell_print(sh, "Extension: %s symbols", m->name);
	shell_print(sh, "| Symbol           | Address    |\n");
	for (elf_word i = 0; i < m->sym_tab.sym_cnt; i++) {
		shell_print(sh, "| %16s | %p |\n", m->sym_tab.syms[i].name,
			    m->sym_tab.syms[i].addr);
	}

	return 0;
}

static void llext_name_get(size_t idx, struct shell_static_entry *entry)
{
	sys_slist_t *ext_list = llext_list();
	sys_snode_t *node = sys_slist_peek_head(ext_list);

	entry->syntax = NULL;

	for (int i = 0; i < idx; i++) {
		node = sys_slist_peek_next(node);

		if (node == NULL) {
			goto out;
		}
	}

	struct llext *ext = CONTAINER_OF(node, struct llext, _llext_list);

	entry->syntax = ext->name;
out:
	entry->syntax = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

}

SHELL_DYNAMIC_CMD_CREATE(msub_llext_name, llext_name_get);

static int cmd_llext_list(const struct shell *sh, size_t argc, char *argv[])
{
	sys_snode_t *node;
	struct llext *ext;

	shell_print(sh, "| Name             | Size        |\n");
	SYS_SLIST_FOR_EACH_NODE(llext_list(), node) {
		ext = CONTAINER_OF(node, struct llext, _llext_list);
		shell_print(sh, "| %16s | %12d |\n", ext->name, ext->mem_size);
	}

	return 0;
}

static uint8_t llext_buf[CONFIG_LLEXT_SHELL_MAX_SIZE];

static int cmd_llext_load_hex(const struct shell *sh, size_t argc, char *argv[])
{
	char name[16];
	size_t hex_len = strnlen(argv[2], CONFIG_LLEXT_SHELL_MAX_SIZE*2+1);
	size_t bin_len = hex_len/2;

	if (bin_len > CONFIG_LLEXT_SHELL_MAX_SIZE) {
		shell_print(sh, "Extension %d bytes too large to load, max %d bytes\n", hex_len/2,
			    CONFIG_LLEXT_SHELL_MAX_SIZE);
		return -ENOMEM;
	}

	strncpy(name, argv[1], sizeof(name));

	size_t llext_buf_len = hex2bin(argv[2], hex_len, llext_buf, CONFIG_LLEXT_SHELL_MAX_SIZE);
	struct llext_buf_loader buf_loader = LLEXT_BUF_LOADER(llext_buf, llext_buf_len);
	struct llext_loader *ldr = &buf_loader.loader;

	LOG_DBG("hex2bin hex len %d, llext buf sz %d, read %d",
		hex_len, CONFIG_LLEXT_SHELL_MAX_SIZE, llext_buf_len);
	LOG_HEXDUMP_DBG(llext_buf, 4, "4 byte MAGIC");

	struct llext *ext;
	int res = llext_load(ldr, name, &ext);

	if (res == 0) {
		shell_print(sh, "Successfully loaded extension %s, addr %p\n", ext->name, ext);
	} else {
		shell_print(sh, "Failed to load extension %s, return code %d\n", name, res);
	}

	return 0;
}

static int cmd_llext_unload(const struct shell *sh, size_t argc, char *argv[])
{
	struct llext *ext = llext_by_name(argv[1]);

	if (ext == NULL) {
		shell_print(sh, "No such extension %s", argv[1]);
		return -EINVAL;
	}

	llext_unload(ext);
	shell_print(sh, "Unloaded extension %s\n", argv[1]);

	return 0;
}

static int cmd_llext_call_fn(const struct shell *sh, size_t argc, char *argv[])
{
	struct llext *ext = llext_by_name(argv[1]);

	if (ext == NULL) {
		shell_print(sh, "No such extension %s", argv[1]);
		return -EINVAL;
	}

	llext_call_fn(ext, argv[2]);

	return 0;
}


/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_llext,
	SHELL_CMD(list, NULL, LLEXT_LIST_HELP, cmd_llext_list),
	SHELL_CMD_ARG(load_hex, NULL, LLEXT_LOAD_HEX_HELP, cmd_llext_load_hex,
		3, 0),
	SHELL_CMD_ARG(unload, &msub_llext_name, LLEXT_UNLOAD_HELP, cmd_llext_unload, 2, 0),
	SHELL_CMD_ARG(list_symbols, &msub_llext_name, LLEXT_LIST_SYMBOLS_HELP,
		      cmd_llext_list_symbols, 2, 0),
	SHELL_CMD_ARG(call_fn, &msub_llext_name, LLEXT_CALL_FN_HELP,
		      cmd_llext_call_fn, 3, 0),

	SHELL_SUBCMD_SET_END
	);
/* clang-format on */

SHELL_CMD_REGISTER(llext, &sub_llext, "Loadable extension commands", NULL);
