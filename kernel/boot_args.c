/* SPDX-License-Identifier: Apache-2.0
 * Copyright The Zephyr Project contributors
 */

#include <zephyr/sys/util.h>
#include <ctype.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

extern const char *get_bootargs(void);
char **prepare_main_args(int *argc)
{
#ifdef CONFIG_DYNAMIC_BOOTARGS
	const char *bootargs = get_bootargs();
#else
	const char bootargs[] = CONFIG_BOOTARGS_STRING;
#endif

	/* beginning of the buffer contains argument's strings, end of it contains argvs */
	static char args_buf[CONFIG_BOOTARGS_ARGS_BUFFER_SIZE];
	char *strings_end = (char *)args_buf;
	char **argv_begin = (char **)WB_DN(
		args_buf + CONFIG_BOOTARGS_ARGS_BUFFER_SIZE - sizeof(char *));
	int i = 0;

	*argc = 0;
	*argv_begin = NULL;

#ifdef CONFIG_DYNAMIC_BOOTARGS
	if (!bootargs) {
		return argv_begin;
	}
#endif

	while (1) {
		while (isspace(bootargs[i])) {
			i++;
		}

		if (bootargs[i] == '\0') {
			return argv_begin;
		}

		if (strings_end + sizeof(char *) >= (char *)argv_begin) {
			LOG_WRN("not enough space in args buffer to accommodate all bootargs"
				" - bootargs truncated");
			return argv_begin;
		}

		argv_begin--;
		memmove(argv_begin, argv_begin + 1, *argc * sizeof(char *));
		argv_begin[*argc] = strings_end;

		bool quoted = false;

		if (bootargs[i] == '\"' || bootargs[i] == '\'') {
			char delimiter = bootargs[i];

			for (int j = i + 1; bootargs[j] != '\0'; j++) {
				if (bootargs[j] == delimiter) {
					quoted = true;
					break;
				}
			}
		}

		if (quoted) {
			char delimiter  = bootargs[i];

			i++; /* strip quotes */
			while (bootargs[i] != delimiter
				&& strings_end < (char *)argv_begin) {
				*strings_end++ = bootargs[i++];
			}
			i++; /* strip quotes */
		} else {
			while (!isspace(bootargs[i])
				&& bootargs[i] != '\0'
				&& strings_end < (char *)argv_begin) {
				*strings_end++ = bootargs[i++];
			}
		}

		if (strings_end < (char *)argv_begin) {
			*strings_end++ = '\0';
		} else {
			LOG_WRN("not enough space in args buffer to accommodate all bootargs"
				" - bootargs truncated");
			argv_begin[*argc] = NULL;
			return argv_begin;
		}
		(*argc)++;
	}
}
