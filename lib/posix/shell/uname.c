/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include "posix_shell.h"

#include <zephyr/posix/sys/utsname.h>
#include <zephyr/shell/shell.h>

#define UNAME_KERNEL   BIT(0)
#define UNAME_NODE     BIT(1)
#define UNAME_RELEASE  BIT(2)
#define UNAME_VERSION  BIT(3)
#define UNAME_MACHINE  BIT(4)
#define UNAME_PLATFORM BIT(5)
#define UNAME_UNKNOWN  BIT(6)
#define UNAME_ALL                                                                                  \
	(UNAME_KERNEL | UNAME_NODE | UNAME_RELEASE | UNAME_VERSION | UNAME_MACHINE | UNAME_PLATFORM)

#define HELP_USAGE                                                                                 \
	"Usage: uname [OPTION]\n"                                                                  \
	"Print system information\n"                                                               \
	"\n"                                                                                       \
	"    -a,  all informationn\n"                                                              \
	"    -s,  kernel name\n"                                                                   \
	"    -o,  operating system\n"                                                              \
	"    -n,  network node hostname\n"                                                         \
	"    -r,  kernel release\n"                                                                \
	"    -v,  kernel version\n"                                                                \
	"    -m,  machine hardware name\n"                                                         \
	"    -p,  processor type\n"                                                                \
	"    -i,  hardware platform\n"

static void uname_print_usage(const struct shell *sh)
{
	shell_print(sh, HELP_USAGE);
}

static int uname_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
	struct getopt_state *state = getopt_state_get();
	struct utsname info;
	unsigned int set;
	int option;
	char badarg = 0;
	int ret;

	set = 0;

	/* Get the uname options */

	optind = 1;
	while ((option = getopt(argc, argv, "asonrvmpi")) != -1) {
		switch (option) {
		case 'a':
			set = UNAME_ALL;
			break;

		case 'o':
		case 's':
			set |= UNAME_KERNEL;
			break;

		case 'n':
			set |= UNAME_NODE;
			break;

		case 'r':
			set |= UNAME_RELEASE;
			break;

		case 'v':
			set |= UNAME_VERSION;
			break;

		case 'm':
			set |= UNAME_MACHINE;
			break;

		case 'p':
			if (set != UNAME_ALL) {
				set |= UNAME_UNKNOWN;
			}
			break;

		case 'i':
			set |= UNAME_PLATFORM;
			break;

		case '?':
		default:
			badarg = (char)state->optopt;
			break;
		}
	}

	if (argc != optind) {
		shell_error(sh, "uname: extra operand %s", argv[optind]);
		uname_print_usage(sh);
		return -1;
	}

	/* If a bad argument was encountered, then return without processing the
	 * command
	 */

	if (badarg != 0) {
		shell_error(sh, "uname: illegal option -- %c", badarg);
		uname_print_usage(sh);
		return -1;
	}

	/* If nothing is provided on the command line, the default is -s */

	if (set == 0) {
		set = UNAME_KERNEL;
	}

	/* Get uname data */

	ret = uname(&info);
	if (ret < 0) {
		shell_error(sh, "cannot get system name");
		return -1;
	}

	/* Process each option */

	/* print the kernel/operating system name */
	if (set & UNAME_KERNEL) {
		shell_fprintf(sh, SHELL_NORMAL, "%s ", info.sysname);
	}

	/* Print nodename */
	if (set & UNAME_NODE) {
		shell_fprintf(sh, SHELL_NORMAL, "%s ", info.nodename);
	}

	/* Print the kernel release */
	if (set & UNAME_RELEASE) {
		shell_fprintf(sh, SHELL_NORMAL, "%s ", info.release);
	}

	/* Print the kernel version */
	if (set & UNAME_VERSION) {
		shell_fprintf(sh, SHELL_NORMAL, "%s ", info.version);
	}

	/* Print the machine hardware name */
	if (set & UNAME_MACHINE) {
		shell_fprintf(sh, SHELL_NORMAL, "%s ", info.machine);
	}

	/* Print the machine platform name */
	if (set & UNAME_PLATFORM) {
		shell_fprintf(sh, SHELL_NORMAL, "%s ", CONFIG_BOARD);
	}

	/* Print "unknown" */
	if (set & UNAME_UNKNOWN) {
		shell_fprintf(sh, SHELL_NORMAL, "%s ", "unknown");
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");

	return 0;
}

POSIX_CMD_ADD(uname, NULL, "Print system information", uname_cmd_handler, 1, 1);
