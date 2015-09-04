/* btshell.c - Bluetooth shell */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>

#include <nanokernel.h>
#include <arch/cpu.h>

#include <console/uart_console.h>
#include <misc/printk.h>
#include <misc/util.h>

#include "btshell.h"

/* maximum number of command parameters */
#define ARGC_MAX 10

static struct shell_cmd *commands;

static const char *prompt;

#define STACKSIZE 2000
static char __stack stack[STACKSIZE];

#define MAX_CMD_QUEUED 3
static struct uart_console_input buf[MAX_CMD_QUEUED];

static struct nano_fifo avail_queue;
static struct nano_fifo cmds_queue;

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
			printk("Too many parameters (max %u)\n", size - 1);
			return 0;
		}
	}

	/* keep it POSIX style where argv[argc] is required to be NULL */
	argv[argc] = NULL;

	return argc;
}

static void show_help(int argc, char *argv[])
{
	int i;

	printk("Available commands:\n");
	printk("help\n");

	for (i = 0; commands[i].cmd_name; i++) {
		printk("%s\n", commands[i].cmd_name);
	}
}

static cmd_function_t get_cb(const char *string)
{
	size_t len;
	int i;

	len = strlen(string);
	if (!len) {
		return NULL;
	}

	if (!strncmp(string, "help", len)) {
		return show_help;
	}

	for (i = 0; commands[i].cmd_name; i++) {
		if (!strncmp(string, commands[i].cmd_name, len)) {
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
		cmd_function_t cb;

		printk(prompt);

		cmd = nano_fiber_fifo_get_wait(&cmds_queue);

		argc = line2argv(cmd->line, argv, ARRAY_SIZE(argv));
		if (!argc) {
			nano_fiber_fifo_put(&avail_queue, cmd);
			continue;
		}

		cb = get_cb(argv[0]);
		if (!cb) {
			printk("Unrecognized command: %s\n", argv[0]);
			printk("Type 'help' for list of available commands\n");
			nano_fiber_fifo_put(&avail_queue, cmd);
			continue;
		}

		/* Execute callback with arguments */
		cb(argc, argv);

		nano_fiber_fifo_put(&avail_queue, cmd);
	}
}

void shell_init(const char *str, struct shell_cmd *cmds)
{
	nano_fifo_init(&cmds_queue);
	nano_fifo_init(&avail_queue);

	commands = cmds;

	line_queue_init();

	prompt = str ? str : "";

	task_fiber_start(stack, STACKSIZE, shell, 0, 0, 7, 0);

	/* Register serial console handler */
	uart_register_input(&avail_queue, &cmds_queue);
}
