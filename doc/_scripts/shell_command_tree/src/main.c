/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Dump the tree of shell commands compiled into this image as JSON.
 *
 * This is not a sample but a tool used by the documentation build: the
 * ``zephyr:shell-module`` Sphinx directive builds this application with the
 * Kconfig options of the shell module being documented, runs it, and turns the
 * JSON printed between the begin/end markers into a command reference.
 *
 * Only static subcommands are listed. Dynamic subcommand sets are used by shell
 * modules to offer tab completion of arguments (device names, pin numbers,
 * sensor channels, ...), so a command owning one is flagged with
 * ``"dynamic_completion": true`` and, when the completion candidates are device
 * names, with ``"accepts_device_names": true``.
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/printk.h>

#include <string.h>

/* Internal shell helper (see subsys/shell/shell_utils.h) able to walk both
 * static and dynamic subcommands.
 */
const struct shell_static_entry *z_shell_cmd_get(const struct shell_static_entry *parent,
						 size_t idx, struct shell_static_entry *dloc);

TYPE_SECTION_START_EXTERN(union shell_cmd_entry, shell_dynamic_subcmds);
TYPE_SECTION_END_EXTERN(union shell_cmd_entry, shell_dynamic_subcmds);

#define BEGIN_MARKER "--- shell-command-tree-begin ---"
#define END_MARKER   "--- shell-command-tree-end ---"

#define MAX_LEVEL 16

/* Same test as the shell itself uses (is_dynamic_cmd() in shell_utils.c). */
static bool is_dynamic_set(const union shell_cmd_entry *set)
{
	return (set >= TYPE_SECTION_START(shell_dynamic_subcmds)) &&
	       (set < TYPE_SECTION_END(shell_dynamic_subcmds));
}

static bool is_device_name(const char *name)
{
	if (device_get_binding(name) != NULL) {
		return true;
	}

	return IS_ENABLED(CONFIG_DEVICE_DT_METADATA) && device_get_by_dt_nodelabel(name) != NULL;
}

static void print_indent(int level)
{
	for (int i = 0; i < level; i++) {
		printk("  ");
	}
}

/* Print @p str as a JSON string literal, quotes included. */
static void print_json_string(const char *str)
{
	printk("\"");

	for (const unsigned char *p = (const unsigned char *)str; *p != '\0'; p++) {
		switch (*p) {
		case '"':
			printk("\\\"");
			break;
		case '\\':
			printk("\\\\");
			break;
		case '\n':
			printk("\\n");
			break;
		case '\r':
			printk("\\r");
			break;
		case '\t':
			printk("\\t");
			break;
		default:
			if (*p < 0x20) {
				printk("\\u%04x", *p);
			} else {
				printk("%c", *p);
			}
			break;
		}
	}

	printk("\"");
}

static void print_json_member(int level, const char *key, const char *value)
{
	print_indent(level);
	print_json_string(key);
	printk(": ");
	print_json_string(value);
}

static void print_json_flag(int level, const char *key)
{
	printk(",\n");
	print_indent(level);
	printk("\"%s\": true", key);
}

static void print_command(const struct shell_static_entry *entry, int level)
{
	struct shell_static_entry dloc;
	const struct shell_static_entry *sub;
	bool has_subcommands = false;
	size_t idx = 0;

	print_indent(level);
	printk("{\n");
	print_json_member(level + 1, "name", entry->syntax);

	if (entry->help != NULL) {
		if (shell_help_is_structured(entry->help)) {
			const struct shell_cmd_help *help =
				(const struct shell_cmd_help *)entry->help;

			if (help->description != NULL && help->description[0] != '\0') {
				printk(",\n");
				print_json_member(level + 1, "description", help->description);
			}
			if (help->usage != NULL && help->usage[0] != '\0') {
				printk(",\n");
				print_json_member(level + 1, "usage", help->usage);
			}
		} else if (entry->help[0] != '\0') {
			printk(",\n");
			print_json_member(level + 1, "help", entry->help);
		}
	}

	printk(",\n");
	print_indent(level + 1);
	printk("\"args\": {\"mandatory\": %u, \"optional\": %u}", entry->args.mandatory,
	       entry->args.optional);

	if (entry->subcmd == NULL) {
		goto done;
	}

	if (is_dynamic_set(entry->subcmd)) {
		print_json_flag(level + 1, "dynamic_completion");

		sub = z_shell_cmd_get(entry, 0, &dloc);
		if (sub != NULL && sub->syntax != NULL && is_device_name(sub->syntax)) {
			print_json_flag(level + 1, "accepts_device_names");
		}

		goto done;
	}

	if (level >= MAX_LEVEL) {
		printk(",\n");
		print_indent(level + 1);
		printk("\"error\": \"command tree too deep, increase MAX_LEVEL\"");
		goto done;
	}

	while ((sub = z_shell_cmd_get(entry, idx++, &dloc)) != NULL) {
		/* Disabled SHELL_COND_CMD() and friends leave entries with an empty syntax */
		if (sub->syntax[0] == '\0') {
			continue;
		}

		if (!has_subcommands) {
			has_subcommands = true;
			printk(",\n");
			print_indent(level + 1);
			printk("\"subcommands\": [\n");
		} else {
			printk(",\n");
		}

		print_command(sub, level + 2);
	}

	if (has_subcommands) {
		printk("\n");
		print_indent(level + 1);
		printk("]");
	}

done:
	printk("\n");
	print_indent(level);
	printk("}");
}

int main(void)
{
	struct shell_static_entry dloc;
	const struct shell_static_entry *entry;
	size_t idx = 0;

	printk("%s\n", BEGIN_MARKER);
	printk("{\n  \"commands\": [\n");

	while ((entry = z_shell_cmd_get(NULL, idx, &dloc)) != NULL) {
		if (idx > 0) {
			printk(",\n");
		}
		print_command(entry, 2);
		idx++;
	}

	printk("\n  ]\n}\n");
	printk("%s\n", END_MARKER);

	return 0;
}
