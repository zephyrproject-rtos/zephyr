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

#include "btshell.h"

/* Static command array, commands identified by string cmd_name,
 * which shall be allocated in the caller
 * TODO: Check possibility of using statically allocated array
 */
#define MAX_CMD_NUM	20
/* After last command there is zero entry to make it possible to
 * use statically initialized arrays with last zero entry.
 */
static struct {
	const char *cmd_name;
	cmd_function_t cb;
} commands[MAX_CMD_NUM + 1];

static uint8_t last_cmd_index = 0;

static const char *prompt;

#define STACKSIZE 512
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

void shell_cmd_register(const char *string, cmd_function_t cb)
{
	if (last_cmd_index > MAX_CMD_NUM - 1) {
		return;
	}

	commands[last_cmd_index].cmd_name = string;
	commands[last_cmd_index].cb = cb;

	last_cmd_index++;
}

static size_t parse_string(char *p)
{
	size_t argc = 0;

	while ((p = strchr(p, ' '))) {
		*p++ = '\0';
		argc++;
	}

	return argc;
}

static cmd_function_t get_cb(const char *string)
{
	int i;

	for (i = 0; commands[i].cmd_name; i++) {
		if (!strncmp(string, commands[i].cmd_name, strlen(string))) {
			return commands[i].cb;
		}
	}

	return NULL;
}

static int show_commands(int argc, const char *argv)
{
	int i;

	printk("Available commands:\n");

	for (i = 0; commands[i].cmd_name; i++) {
		printk("%s\n", commands[i].cmd_name);
	}

	return 0;
}

static void shell(int arg1, int arg2)
{
	while (1) {
		struct uart_console_input *cmd;
		cmd_function_t cb;
		size_t argc;

		printk(prompt);

		cmd = nano_fiber_fifo_get_wait(&cmds_queue);

		argc = parse_string(cmd->line);

		/* NOTE: example of using command line and arguments
		 *
		 * for (int i = 0; i <= argc; i++) {
		 *	printk("%s: arg %d: %s\n", __func__, i, p);
		 *	p += strlen(p) + 1;
		 * }
		 */

		cb = get_cb(cmd->line);
		if (!cb) {
			printk("%s Unrecognozed command: %s\n",
			       prompt, cmd->line);
			nano_fiber_fifo_put(&avail_queue, cmd);
			continue;
		}

		/* Execute callback with arguments */
		cb(argc, cmd->line);

		nano_fiber_fifo_put(&avail_queue, cmd);
	}
}

void shell_init(const char *str)
{
	nano_fifo_init(&cmds_queue);
	nano_fifo_init(&avail_queue);

	memset(commands, 0, sizeof(commands));
	line_queue_init();

	prompt = str ? str : "";

	/* Built-in command init */
	shell_cmd_register("help", show_commands);

	task_fiber_start(stack, STACKSIZE, shell, 0, 0, 7, 0);

	/* Register serial console handler */
	uart_register_input(&avail_queue, &cmds_queue);
}
