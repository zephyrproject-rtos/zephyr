#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/device.h>
#include <string.h>

const struct shell_static_entry *z_shell_cmd_get(const struct shell_static_entry *parent,
						 size_t idx, struct shell_static_entry *dloc);

TYPE_SECTION_START_EXTERN(union shell_cmd_entry, shell_root_cmds);
TYPE_SECTION_END_EXTERN(union shell_cmd_entry, shell_root_cmds);

#define MAX_CMD_DEPTH 16

static bool is_device_name_entry(const struct shell_static_entry *entry)
{
	const struct device *dev_list;
	size_t dev_count = z_device_get_all_static(&dev_list);
	for (size_t j = 0; j < dev_count; j++) {
		const struct device *dev = &dev_list[j];
		if (dev->name && strcmp(dev->name, entry->syntax) == 0) {
			return true;
		}
	}

	return false;
}

static void build_cmd_path(char *buf, size_t buflen, const char *path[], int level)
{
	buf[0] = '\0';
	for (int i = 0; i < level; ++i) {
		strncat(buf, path[i], buflen - strlen(buf) - 1);
		if (i < level - 1) {
			strncat(buf, " ", buflen - strlen(buf) - 1);
		}
	}
}

static void print_yaml_string(const char *str, int indent)
{
	if (!str || *str == '\0') {
		return;
	}

	printk("|\n");
	char temp_str[1024];
	strncpy(temp_str, str, sizeof(temp_str) - 1);
	temp_str[sizeof(temp_str) - 1] = '\0';

	char *line = strtok(temp_str, "\n");
	while (line != NULL) {
		for (int i = 0; i < indent + 2; i++) {
			printk(" ");
		}
		printk("%s\n", line);
		line = strtok(NULL, "\n");
	}
}

static void print_yaml_indent(int level)
{
	for (int i = 0; i < level; i++) {
		printk("  ");
	}
}

static void print_yaml_scalar(int level, const char *key, const char *value)
{
	print_yaml_indent(level);
	printk("  %s: %s\n", key, value);
}

static void print_yaml_block(int level, const char *key, const char *value)
{
	print_yaml_indent(level);
	printk("  %s: ", key);
	print_yaml_string(value, level * 2 + 2);
}

static void print_cmd_tree_yaml(const struct shell_static_entry *cmd, int level, const char *path[],
				int path_len)
{
	if (!cmd || !cmd->syntax || cmd->syntax[0] == '\0') {
		return;
	}
	if (path_len >= MAX_CMD_DEPTH) {
		printk("# ERROR: Command tree too deep at '%s', increase MAX_CMD_DEPTH!\n",
		       cmd->syntax);
		return;
	}

	char full_cmd[256];
	path[path_len] = cmd->syntax;
	build_cmd_path(full_cmd, sizeof(full_cmd), path, path_len + 1);

	print_yaml_indent(level);
	printk("- name: %s\n", cmd->syntax);

	print_yaml_scalar(level, "command", full_cmd);

	if (cmd->help) {
		const char *help = cmd->help;

		if (shell_help_is_structured(help)) {
			const struct shell_cmd_help *structured =
				(const struct shell_cmd_help *)help;

			if (structured->description) {
				print_yaml_block(level, "description",
						 structured->description);
			}

			if (structured->usage) {
				print_yaml_block(level, "usage",
						 structured->usage);
			}
		} else {
			print_yaml_block(level, "help", help);
		}
	}

	struct shell_static_entry dloc;
	size_t idx = 0;
	bool found_device_entries = false;
	bool has_subcommands = false;

	while (1) {
		const struct shell_static_entry *sub = z_shell_cmd_get(cmd, idx++, &dloc);
		if (!sub) {
			break;
		}

		if (is_device_name_entry(sub)) {
			found_device_entries = true;
			continue;
		}
		has_subcommands = true;
		break;
	}

	if (has_subcommands) {
		print_yaml_indent(level);
		printk("  subcommands:\n");

		idx = 0;
		while (1) {
			const struct shell_static_entry *sub = z_shell_cmd_get(cmd, idx++, &dloc);
			if (!sub) {
				break;
			}

			if (is_device_name_entry(sub)) {
				continue;
			}

			print_cmd_tree_yaml(sub, level + 2, path, path_len + 1);
		}
	}

	if (found_device_entries) {
		print_yaml_indent(level);
		printk("  accepts_device_names: true\n");
	}
}

static int shell_doc_generator_init(void)
{
	printk("# Zephyr Shell Command Reference\n");
	printk("# This file documents all available shell commands and subcommands in YAML "
	       "format\n");
	printk("# Device-specific arguments have been filtered out for clarity\n\n");
	printk("commands:\n");

	const char *path[MAX_CMD_DEPTH] = {0};
	for (const union shell_cmd_entry *cmd = TYPE_SECTION_START(shell_root_cmds);
	     cmd < TYPE_SECTION_END(shell_root_cmds); ++cmd) {
		print_cmd_tree_yaml(cmd->entry, 1, path, 0);
	}
	printk("-- EOF --\n");
	return 0;
}

SYS_INIT(shell_doc_generator_init, APPLICATION, 99);
