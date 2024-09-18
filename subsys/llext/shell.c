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
#include <zephyr/llext/fs_loader.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(llext_shell, CONFIG_LLEXT_LOG_LEVEL);

#define LLEXT_LIST_HELP "List loaded extensions and their size in memory"

#define LLEXT_LOAD_HEX_HELP                                                                        \
	"Load an elf file encoded in hex directly from the shell input. Syntax:\n"                 \
	"<ext_name> <ext_hex_string>"

#define LLEXT_UNLOAD_HELP                                                                          \
	"Unload an extension by name. Syntax:\n"                                                   \
	"<ext_name>"

#define LLEXT_LIST_SYMBOLS_HELP                                                                    \
	"List extension symbols. Syntax:\n"                                                        \
	"<ext_name>"

#define LLEXT_CALL_FN_HELP                                                                         \
	"Call extension function with prototype void fn(void). Syntax:\n"                          \
	"<ext_name> <function_name>"

#ifdef CONFIG_FILE_SYSTEM
#define LLEXT_LOAD_FS_HELP                                                                         \
	"Load an elf file directly from filesystem. Syntax:\n"                                     \
	"<ext_name> <ext_llext_file_name>"

#endif /* CONFIG_FILE_SYSTEM */

static int cmd_llext_list_symbols(const struct shell *sh, size_t argc, char *argv[])
{
	struct llext *m = llext_by_name(argv[1]);

	if (m == NULL) {
		shell_print(sh, "No such llext %s", argv[1]);
		return -ENOENT;
	}

	shell_print(sh, "Extension: %s symbols", m->name);
	shell_print(sh, "| Symbol           | Address    |");
	for (elf_word i = 0; i < m->sym_tab.sym_cnt; i++) {
		shell_print(sh, "| %16s | %p |", m->sym_tab.syms[i].name,
			    m->sym_tab.syms[i].addr);
	}

	return 0;
}

struct llext_shell_cmd {
	unsigned int tgt;
	unsigned int idx;
	struct llext *ext;
};

static int llext_shell_name_cb(struct llext *ext, void *arg)
{
	struct llext_shell_cmd *cmd = arg;

	if (cmd->tgt == cmd->idx) {
		cmd->ext = ext;
		return 1;
	}

	cmd->idx++;

	return 0;
}

static void llext_name_get(size_t idx, struct shell_static_entry *entry)
{
	struct llext_shell_cmd cmd = {.tgt = idx};

	llext_iterate(llext_shell_name_cb, &cmd);

	entry->syntax = cmd.ext ? cmd.ext->name : NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
	entry->handler = NULL;
	entry->args.mandatory = 0;
	entry->args.optional = 0;
}
SHELL_DYNAMIC_CMD_CREATE(msub_llext_name, llext_name_get);

static void llext_name_arg_get(size_t idx, struct shell_static_entry *entry)
{
	llext_name_get(idx, entry);
	if (entry->syntax) {
		entry->args.mandatory = 1;
	}
}
SHELL_DYNAMIC_CMD_CREATE(msub_llext_name_arg, llext_name_arg_get);

struct llext_shell_list {
	const struct shell *sh;
};

static int llext_shell_list_cb(struct llext *ext, void *arg)
{
	struct llext_shell_list *sl = arg;

	shell_print(sl->sh, "| %16s | %12d |", ext->name, ext->alloc_size);
	return 0;
}

static int cmd_llext_list(const struct shell *sh, size_t argc, char *argv[])
{
	struct llext_shell_list sl = {.sh = sh};

	shell_print(sh, "| Name             | Size         |");
	return llext_iterate(llext_shell_list_cb, &sl);
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

	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext;
	int res = llext_load(ldr, name, &ext, &ldr_parm);

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
		return -ENOENT;
	}

	llext_unload(&ext);
	shell_print(sh, "Unloaded extension %s\n", argv[1]);

	return 0;
}

static int cmd_llext_call_fn(const struct shell *sh, size_t argc, char *argv[])
{
	struct llext *ext = llext_by_name(argv[1]);

	if (ext == NULL) {
		shell_print(sh, "No such extension %s", argv[1]);
		return -ENOENT;
	}

	llext_call_fn(ext, argv[2]);

	return 0;
}

#ifdef CONFIG_FILE_SYSTEM
static int cmd_llext_load_fs(const struct shell *sh, size_t argc, char *argv[])
{
	int res;
	struct fs_dirent dirent;

	res = fs_stat(argv[2], &dirent);
	if (res) {
		shell_error(sh, "Failed to obtain file %s, return code %d\n", argv[2], res);
		return res;
	}

	if (dirent.type != FS_DIR_ENTRY_FILE) {
		shell_error(sh, "Not a file %s", argv[2]);
		return -ENOEXEC;
	}

	struct llext_fs_loader fs_loader = LLEXT_FS_LOADER(argv[2]);
	struct llext_loader *ldr = &fs_loader.loader;
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext;

	res = llext_load(ldr, argv[1], &ext, &ldr_parm);
	if (res < 0) {
		shell_print(sh, "Failed to load extension %s, return code %d\n", ext->name, res);
		return -ENOEXEC;
	}
	shell_print(sh, "Successfully loaded extension %s, addr %p\n", ext->name, ext);
	return 0;
}
#endif

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_llext,
	SHELL_CMD(list, NULL, LLEXT_LIST_HELP, cmd_llext_list),
#ifdef CONFIG_FILE_SYSTEM
	SHELL_CMD_ARG(load_llext, NULL, LLEXT_LOAD_FS_HELP, cmd_llext_load_fs, 3, 0),
#endif
	SHELL_CMD_ARG(load_hex, NULL, LLEXT_LOAD_HEX_HELP, cmd_llext_load_hex, 3, 0),
	SHELL_CMD_ARG(unload, &msub_llext_name, LLEXT_UNLOAD_HELP, cmd_llext_unload, 2, 0),
	SHELL_CMD_ARG(list_symbols, &msub_llext_name, LLEXT_LIST_SYMBOLS_HELP,
		      cmd_llext_list_symbols, 2, 0),
	SHELL_CMD_ARG(call_fn, &msub_llext_name_arg, LLEXT_CALL_FN_HELP,
		      cmd_llext_call_fn, 3, 0),

	SHELL_SUBCMD_SET_END
	);
/* clang-format on */

SHELL_CMD_REGISTER(llext, &sub_llext, "Loadable extension commands", NULL);
