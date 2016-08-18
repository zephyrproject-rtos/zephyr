/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Console handler implementation of shell.h API
 */


#include <zephyr.h>
#include <stdio.h>
#include <string.h>

#include <console/uart_console.h>
#include <misc/printk.h>
#include <misc/util.h>

#include <misc/shell.h>

/* maximum number of command parameters */
#define ARGC_MAX 10

static const struct shell_cmd *commands;

static const char *prompt;

#define STACKSIZE CONFIG_CONSOLE_HANDLER_SHELL_STACKSIZE
static char __stack stack[STACKSIZE];

#define MAX_CMD_QUEUED 3
static struct uart_console_input buf[MAX_CMD_QUEUED];

static struct nano_fifo avail_queue;
static struct nano_fifo cmds_queue;

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

	return prompt;
}

static void line_queue_init(void)
{
	int i;

	for (i = 0; i < MAX_CMD_QUEUED; i++) {
		nano_fifo_put(&avail_queue, &buf[i]);
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

static int show_cmd_help(int argc, char *argv[])
{
	int i;

	if (!argv[0] || argv[0][0] == '\0') {
		goto done;
	}

	for (i = 0; commands[i].cmd_name; i++) {
		if (!strcmp(argv[0], commands[i].cmd_name)) {
			printk("%s %s\n", commands[i].cmd_name,
			       commands[i].help ? commands[i].help : "");
			return 0;
		}
	}

done:
	printk("Unrecognized command: %s\n", argv[0]);
	return 0;
}

static int show_help(int argc, char *argv[])
{
	int i;

	if (argc > 1) {
		return show_cmd_help(--argc, &argv[1]);
	}

	printk("Available commands:\n");
	printk("help\n");

	for (i = 0; commands[i].cmd_name; i++) {
		printk("%s\n", commands[i].cmd_name);
	}

	return 0;
}

static shell_cmd_function_t get_cb(const char *string)
{
	int i;

	if (!string || string[0] == '\0') {
		return NULL;
	}

	if (!strcmp(string, "help")) {
		return show_help;
	}

	for (i = 0; commands[i].cmd_name; i++) {
		if (!strcmp(string, commands[i].cmd_name)) {
			return commands[i].cb;
		}
	}

	return NULL;
}

static void shell(int arg1, int arg2)
{
	char *argv[ARGC_MAX + 1];
	size_t argc;

	while (1) {
		struct uart_console_input *cmd;
		shell_cmd_function_t cb;

		printk("%s", get_prompt());

		cmd = nano_fiber_fifo_get(&cmds_queue, TICKS_UNLIMITED);

		argc = line2argv(cmd->line, argv, ARRAY_SIZE(argv));
		if (!argc) {
			nano_fiber_fifo_put(&avail_queue, cmd);
			continue;
		}

		cb = get_cb(argv[0]);
		if (!cb) {
			if (app_cmd_handler != NULL) {
				cb = app_cmd_handler;
			} else {
				printk("Unrecognized command: %s\n", argv[0]);
				printk("Type 'help' for list of available commands\n");
				nano_fiber_fifo_put(&avail_queue, cmd);
				continue;
			}
		}

		/* Execute callback with arguments */
		if (cb(argc, argv) < 0) {
			show_cmd_help(argc, argv);
		}

		nano_fiber_fifo_put(&avail_queue, cmd);
	}
}

static uint8_t completion(char *line, uint8_t len)
{
	const char *first_match = NULL;
	int common_chars = -1;
	int i;

	for (i = 0; commands[i].cmd_name; i++) {
		int j;

		if (strncmp(line, commands[i].cmd_name, len)) {
			continue;
		}

		if (!first_match) {
			first_match = commands[i].cmd_name;
			continue;
		}

		/* more commands match, print first match */
		if (first_match && (common_chars < 0)) {
			printk("\n%s\n", first_match);
			common_chars = strlen(first_match);
		}

		/* cut common part of matching names */
		for (j = 0; j < common_chars; j++) {
			if (first_match[j] != commands[i].cmd_name[j]) {
				break;
			}
		}

		common_chars = j;

		printk("%s\n", commands[i].cmd_name);
	}

	/* no match, do nothing */
	if (!first_match) {
		return 0;
	}

	if (common_chars >= 0) {
		/* multiple match, restore prompt */
		printk("%s", get_prompt());

		/* add common part after prompt */
		for (i = 0; i < common_chars; i++) {
			printk("%c", first_match[i]);
			line[i] = first_match[i];
		}
	} else {
		common_chars = strlen(first_match);

		/* full command name with trailing spaces, do nothing */
		if (len > common_chars) {
			return 0;
		}

		/* complete matched command name */
		for (i = len; i < common_chars; i++) {
			printk("%c", first_match[i]);
			line[i] = first_match[i];
		}

		/* for convenience add space after command */
		printk(" ");
		line[common_chars++] = ' ';
	}

	return common_chars - len;
}

void shell_init(const char *str, const struct shell_cmd *cmds)
{
	nano_fifo_init(&cmds_queue);
	nano_fifo_init(&avail_queue);

	commands = cmds;

	line_queue_init();

	prompt = str ? str : "";

	task_fiber_start(stack, STACKSIZE, shell, 0, 0, 7, 0);

	/* Register serial console handler */
	uart_register_input(&avail_queue, &cmds_queue, completion);
}

/** @brief Optionally register an app default cmd handler.
 *
 *  @param handler To be called if no cmd found in cmds registered with shell_init.
 */
void shell_register_app_cmd_handler(shell_cmd_function_t handler)
{
	app_cmd_handler = handler;
}

void shell_register_prompt_handler(shell_prompt_function_t handler)
{
	app_prompt_handler = handler;
}
