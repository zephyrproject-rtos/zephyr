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

#include <zephyr.h>
#include <misc/printk.h>
#include <misc/shell.h>

static int shell_cmd_ping(int argc, char *argv[])
{
	printk("pong\n");

	return 0;
}

static int shell_cmd_params(int argc, char *argv[])
{
	printk("argc = %d, argv[0] = %s\n", argc, argv[0]);

	return 0;
}

#define MY_SHELL_MODULE "sample_module"

static struct shell_cmd commands[] = {
	{ "ping", shell_cmd_ping },
	{ "params", shell_cmd_params, "print argc" },
	{ NULL, NULL }
};


void main(void)
{
	SHELL_REGISTER(MY_SHELL_MODULE, commands);
}
