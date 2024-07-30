/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <ctype.h>
#include <zephyr/toolchain.h>
#include <posix_native_task.h>
#include <nsi_cmdline_main_if.h>
#include <nsi_host_trampolines.h>
#include <nsi_tracing.h>

static void remove_one_char(char *str)
{
	int i;

	for (i = 0; str[i] != 0; i++) {
		str[i] = str[i+1];
	}
}

static void register_kconfig_args(void)
{
	static char kconfig_args[] = CONFIG_NATIVE_EXTRA_CMDLINE_ARGS;
	int argc = 0;
	char **argv = NULL;
#define REALLOC_INC 100
	int alloced = 0;
	bool new_arg = true, literal = false, escape = false;

	if (kconfig_args[0] == 0) {
		return;
	}

	for (int i = 0; kconfig_args[i] != 0; i++) {
		if ((literal == false) && (escape == false) && isspace(kconfig_args[i])) {
			new_arg = true;
			kconfig_args[i] = 0;
			continue;
		}
		if ((escape == false) && (kconfig_args[i] == '\\')) {
			escape = true;
			remove_one_char(&kconfig_args[i]);
			i--;
			continue;
		}
		if ((escape == false) && (kconfig_args[i] == '"')) {
			literal = !literal;
			remove_one_char(&kconfig_args[i]);
			i--;
			continue;
		}
		escape = false;

		if (new_arg) {
			new_arg = false;
			if (argc >= alloced) {
				alloced += REALLOC_INC;
				argv = nsi_host_realloc(argv, alloced*sizeof(char *));
				if (argv == NULL) {
					nsi_print_error_and_exit("Out of memory\n");
				}
			}
			argv[argc++] = &kconfig_args[i];
		}
	}

	nsi_register_extra_args(argc, argv);

	nsi_host_free(argv);
}

NATIVE_TASK(register_kconfig_args, PRE_BOOT_1, 100);
