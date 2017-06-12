/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Console handler implementation of shell.h API
 */


#include <zephyr.h>
#include <stdio.h>
#include <string.h>

#include <console/console.h>
#include <misc/printk.h>
#include <misc/util.h>

#ifdef CONFIG_UART_CONSOLE
#include <console/uart_console.h>
#endif
#ifdef CONFIG_TELNET_CONSOLE
#include <console/telnet_console.h>
#endif

#include <shell/shell.h>

#define ARGC_MAX 10
#define COMMAND_MAX_LEN 50
#define MODULE_NAME_MAX_LEN 20
/* additional chars are " >" (include '\0' )*/
#define PROMPT_SUFFIX 3
#define PROMPT_MAX_LEN (MODULE_NAME_MAX_LEN + PROMPT_SUFFIX)

/* command table is located in the dedicated memory segment (.shell_) */
extern struct shell_module __shell_cmd_start[];
extern struct shell_module __shell_cmd_end[];
#define NUM_OF_SHELL_ENTITIES (__shell_cmd_end - __shell_cmd_start)

static const char *prompt;
static char default_module_prompt[PROMPT_MAX_LEN];
static int default_module = -1;

#define STACKSIZE CONFIG_CONSOLE_SHELL_STACKSIZE
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);
static struct k_thread shell_thread;

#define MAX_CMD_QUEUED CONFIG_CONSOLE_SHELL_MAX_CMD_QUEUED
static struct console_input buf[MAX_CMD_QUEUED];

static struct k_fifo avail_queue;
static struct k_fifo cmds_queue;

static shell_cmd_function_t app_cmd_handler;
static shell_prompt_function_t app_prompt_handler;

static const char *get_prompt(void)
{
	if (app_prompt_handler) {
		const char *str;

		str = app_prompt_handler();
		if (str) {
			return str;
		}
	}

	if (default_module != -1) {
		if (__shell_cmd_start[default_module].prompt) {
			const char *ret;

			ret = __shell_cmd_start[default_module].prompt();
			if (ret) {
				return ret;
			}
		}

		return default_module_prompt;
	}

	return prompt;
}

static void line_queue_init(void)
{
	int i;

	for (i = 0; i < MAX_CMD_QUEUED; i++) {
		k_fifo_put(&avail_queue, &buf[i]);
	}
}

static size_t line2argv(char *str, char *argv[], size_t size)
{
	size_t argc = 0;

	if (!strlen(str)) {
		return 0;
	}

	while (*str && *str == ' ') {
		str++;
	}

	if (!*str) {
		return 0;
	}

	argv[argc++] = str;

	while ((str = strchr(str, ' '))) {
		*str++ = '\0';

		while (*str && *str == ' ') {
			str++;
		}

		if (!*str) {
			break;
		}

		argv[argc++] = str;

		if (argc == size) {
			printk("Too many parameters (max %zu)\n", size - 1);
			return 0;
		}
	}

	/* keep it POSIX style where argv[argc] is required to be NULL */
	argv[argc] = NULL;

	return argc;
}

static int get_destination_module(const char *module_str)
{
	int i;

	for (i = 0; i < NUM_OF_SHELL_ENTITIES; i++) {
		if (!strncmp(module_str,
			     __shell_cmd_start[i].module_name,
			     MODULE_NAME_MAX_LEN)) {
			return i;
		}
	}

	return -1;
}

/* For a specific command: argv[0] = module name, argv[1] = command name
 * If a default module was selected: argv[0] = command name
 */
static const char *get_command_and_module(char *argv[], int *module)
{
	*module = -1;

	if (!argv[0]) {
		printk("Unrecognized command\n");
		return NULL;
	}

	if (default_module == -1) {
		if (!argv[1] || argv[1][0] == '\0') {
			printk("Unrecognized command: %s\n", argv[0]);
			return NULL;
		}

		*module = get_destination_module(argv[0]);
		if (*module == -1) {
			printk("Illegal module %s\n", argv[0]);
			return NULL;
		}

		return argv[1];
	}

	*module = default_module;
	return argv[0];
}

static int show_cmd_help(char *argv[])
{
	const char *command = NULL;
	int module = -1;
	const struct shell_module *shell_module;
	int i;

	command = get_command_and_module(argv, &module);
	if ((module == -1) || (command == NULL)) {
		return 0;
	}

	shell_module = &__shell_cmd_start[module];
	for (i = 0; shell_module->commands[i].cmd_name; i++) {
		if (!strcmp(command, shell_module->commands[i].cmd_name)) {
			printk("%s %s\n",
			       shell_module->commands[i].cmd_name,
			       shell_module->commands[i].help ?
			       shell_module->commands[i].help : "");
			return 0;
		}
	}

	printk("Unrecognized command: %s\n", argv[0]);
	return -EINVAL;
}

static void print_module_commands(const int module)
{
	const struct shell_module *shell_module = &__shell_cmd_start[module];
	int i;

	printk("help\n");

	for (i = 0; shell_module->commands[i].cmd_name; i++) {
		printk("%s\n", shell_module->commands[i].cmd_name);
	}
}

static int show_help(int argc, char *argv[])
{
	int module;

	/* help per command */
	if ((argc > 2) || ((default_module != -1) && (argc == 2))) {
		return show_cmd_help(&argv[1]);
	}

	/* help per module */
	if ((argc == 2) || ((default_module != -1) && (argc == 1))) {
		if (default_module == -1) {
			module = get_destination_module(argv[1]);
			if (module == -1) {
				printk("Illegal module %s\n", argv[1]);
				return -EINVAL;
			}
		} else {
			module = default_module;
		}

		print_module_commands(module);
		printk("\nEnter 'exit' to leave current module.\n");
	} else { /* help for all entities */
		printk("Available modules:\n");
		for (module = 0; module < NUM_OF_SHELL_ENTITIES; module++) {
			printk("%s\n", __shell_cmd_start[module].module_name);
		}
		printk("\nTo select a module, enter 'select <module name>'.\n");
	}

	return 0;
}

static int set_default_module(const char *name)
{
	int module;

	if (strlen(name) > MODULE_NAME_MAX_LEN) {
		printk("Module name %s is too long, default is not changed\n",
			name);
		return -EINVAL;
	}

	module = get_destination_module(name);

	if (module == -1) {
		printk("Illegal module %s, default is not changed\n", name);
		return -EINVAL;
	}

	default_module = module;

	strncpy(default_module_prompt, name, MODULE_NAME_MAX_LEN);
	strcat(default_module_prompt, "> ");

	return 0;
}

static int select_module(int argc, char *argv[])
{
	if (argc == 1) {
		default_module = -1;
		return 0;
	}

	return set_default_module(argv[1]);
}

static int exit_module(int argc, char *argv[])
{
	if (argc == 1) {
		default_module = -1;
	}

	return 0;
}

static shell_cmd_function_t get_cb(int *argc, char *argv[], int *module)
{
	const char *first_string = argv[0];
	const struct shell_module *shell_module;
	const char *command;
	int i;

	if (!first_string || first_string[0] == '\0') {
		printk("Illegal parameter\n");
		return NULL;
	}

	if (!strcmp(first_string, "help")) {
		return show_help;
	}

	if (!strcmp(first_string, "select")) {
		return select_module;
	}

	if (!strcmp(first_string, "exit")) {
		return exit_module;
	}

	if ((*argc == 1) && (default_module == -1)) {
		printk("Missing parameter\n");
		return NULL;
	}

	command = get_command_and_module(argv, module);
	if ((*module == -1) || (command == NULL)) {
		return NULL;
	}

	shell_module = &__shell_cmd_start[*module];
	for (i = 0; shell_module->commands[i].cmd_name; i++) {
		if (!strcmp(command, shell_module->commands[i].cmd_name)) {
			return shell_module->commands[i].cb;
		}
	}

	return NULL;
}

static inline void print_cmd_unknown(char *argv)
{
	printk("Unrecognized command: %s\n", argv);
	printk("Type 'help' for list of available commands\n");
}

int shell_exec(char *line)
{
	char *argv[ARGC_MAX + 1];
	int argc;
	int module = default_module;
	int err;
	shell_cmd_function_t cb;

	argc = line2argv(line, argv, ARRAY_SIZE(argv));
	if (!argc) {
		return -EINVAL;
	}

	err = argc;

	cb = get_cb(&argc, argv, &module);
	if (!cb) {
		if (app_cmd_handler != NULL) {
			cb = app_cmd_handler;
		} else {
			print_cmd_unknown(argv[0]);
			return -EINVAL;
		}
	}

	/* Execute callback with arguments */
	if (module != -1 && module != default_module) {
		/* Ajust parameters to point to the actual command */
		err = cb(argc - 1, &argv[1]);
	} else {
		err = cb(argc, argv);
	}

	if (err < 0) {
		show_cmd_help(argv);
	}

	return err;
}

static void shell(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		struct console_input *cmd;

		printk("%s", get_prompt());

		cmd = k_fifo_get(&cmds_queue, K_FOREVER);

		shell_exec(cmd->line);

		k_fifo_put(&avail_queue, cmd);
	}
}

static int get_command_to_complete(char *str, char **command_prefix)
{
	char dest_str[MODULE_NAME_MAX_LEN];
	int dest = -1;
	char *start;

	/* remove ' ' at the beginning of the line */
	while (*str && *str == ' ') {
		str++;
	}

	if (!*str) {
		return -1;
	}

	start = str;

	if (default_module != -1) {
		dest = default_module;
		/* caller function already checks str len and put '\0' */
		*command_prefix = str;
	}

	/*
	 * In case of a default module: only one parameter is possible.
	 * Otherwise, only two parameters are possibles.
	 */
	str = strchr(str, ' ');
	if (default_module != -1) {
		return (str == NULL) ? dest : -1;
	}

	if (str == NULL) {
		return -1;
	}

	if ((str - start + 1) >= MODULE_NAME_MAX_LEN) {
		return -1;
	}

	strncpy(dest_str, start, (str - start + 1));
	dest_str[str - start] = '\0';
	dest = get_destination_module(dest_str);
	if (dest == -1) {
		return -1;
	}

	str++;

	/* caller func has already checked str len and put '\0' at the end */
	*command_prefix = str;
	str = strchr(str, ' ');

	/* only two parameters are possibles in case of no default module */
	return (str == NULL) ? dest : -1;
}

static u8_t completion(char *line, u8_t len)
{
	const char *first_match = NULL;
	int common_chars = -1, space = 0;
	int i, dest, command_len;
	const struct shell_module *module;
	char *command_prefix;

	if (len >= (MODULE_NAME_MAX_LEN + COMMAND_MAX_LEN - 1)) {
		return 0;
	}

	/*
	 * line to completion is not ended by '\0' as the line that gets from
	 * k_fifo_get function
	 */
	line[len] = '\0';
	dest = get_command_to_complete(line, &command_prefix);
	if (dest == -1) {
		return 0;
	}

	command_len = strlen(command_prefix);

	module = &__shell_cmd_start[dest];

	for (i = 0; module->commands[i].cmd_name; i++) {
		int j;

		if (strncmp(command_prefix,
			module->commands[i].cmd_name, command_len)) {
			continue;
		}

		if (!first_match) {
			first_match = module->commands[i].cmd_name;
			continue;
		}

		/* more commands match, print first match */
		if (first_match && (common_chars < 0)) {
			printk("\n%s\n", first_match);
			common_chars = strlen(first_match);
		}

		/* cut common part of matching names */
		for (j = 0; j < common_chars; j++) {
			if (first_match[j] != module->commands[i].cmd_name[j]) {
				break;
			}
		}

		common_chars = j;

		printk("%s\n", module->commands[i].cmd_name);
	}

	/* no match, do nothing */
	if (!first_match) {
		return 0;
	}

	if (common_chars >= 0) {
		/* multiple match, restore prompt */
		printk("%s", get_prompt());
		printk("%s", line);
	} else {
		common_chars = strlen(first_match);
		space = 1;
	}

	/* complete common part */
	for (i = command_len; i < common_chars; i++) {
		printk("%c", first_match[i]);
		line[len++] = first_match[i];
	}

	/* for convenience add space after command */
	if (space) {
		printk(" ");
		line[len] = ' ';
	}

	return common_chars - command_len + space;
}


void shell_init(const char *str)
{
	k_fifo_init(&cmds_queue);
	k_fifo_init(&avail_queue);

	line_queue_init();

	prompt = str ? str : "";

	k_thread_create(&shell_thread, stack, STACKSIZE, shell, NULL, NULL,
			NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* Register serial console handler */
#ifdef CONFIG_UART_CONSOLE
	uart_register_input(&avail_queue, &cmds_queue, completion);
#endif
#ifdef CONFIG_TELNET_CONSOLE
	telnet_register_input(&avail_queue, &cmds_queue, completion);
#endif
}

/** @brief Optionally register an app default cmd handler.
 *
 *  @param handler To be called if no cmd found in cmds registered with
 *  shell_init.
 */
void shell_register_app_cmd_handler(shell_cmd_function_t handler)
{
	app_cmd_handler = handler;
}

void shell_register_prompt_handler(shell_prompt_function_t handler)
{
	app_prompt_handler = handler;
}

void shell_register_default_module(const char *name)
{
	int result = set_default_module(name);

	if (result != -1) {
		printk("\n%s", default_module_prompt);
	}
}
