/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _CMDLINE_COMMON_H
#define _CMDLINE_COMMON_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _MAX_LINE_WIDTH 100 /*Total width of the help message*/
/* Horizontal alignment of the 2nd column of the help message */
#define _LONG_HELP_ALIGN 30

#define _MAXOPT_SWITCH_LEN  32 /* Maximum allowed length for a switch name */
#define _MAXOPT_NAME_LEN    32 /* Maximum allowed length for a variable name */

#define _HELP_SWITCH  "[-h] [--h] [--help] [-?]"
#define _HELP_DESCR   "Display this help"

#define _MAX_STRINGY_LEN (_MAXOPT_SWITCH_LEN + _MAXOPT_NAME_LEN + 2 + 1 + 2 + 1)


/**
 * Prototype for a callback function when an option is found:
 * inputs:
 *  argv: Whole argv[i] option as received in main
 *  offset: Offset to the end of the option string
 *          (including a possible ':' or '=')
 *  If the option had a value, it would be placed in &argv[offset]
 */
typedef void (*option_found_callback_f)(char *argv, int offset);

/*
 * Structure defining each command line option
 */
struct args_struct_t {
	/*
	 * if manual is set cmd_args_parse*() will ignore it except for
	 * displaying it the help messages and initializing <dest> to its
	 * default
	 */
	bool manual;
	/* For help messages, should it be wrapped in "[]" */
	bool is_mandatory;
	/* It is just a switch: it does not have something to store after */
	bool is_switch;
	/* Option name we search for: --<option> */
	char *option;
	/*
	 * Name of the option destination in the help messages:
	 * "--<option>=<name>"
	 */
	char *name;
	/* Type of option (see cmd_read_option_value()) */
	char type;
	/* Pointer to where the read value will be stored (may be NULL) */
	void *dest;
	/* Optional callback to be called when the switch is found */
	option_found_callback_f call_when_found;
	/* Long description for the help messages */
	char *descript;
};

#define ARG_TABLE_ENDMARKER \
	{false, false, false, NULL, NULL, 0, NULL, NULL, NULL}

int cmd_is_option(const char *arg, const char *option, int with_value);
int cmd_is_help_option(const char *arg);
void cmd_read_option_value(const char *str, void *dest, const char type,
			   const char *option);
void cmd_args_set_defaults(struct args_struct_t args_struct[]);
bool cmd_parse_one_arg(char *argv, struct args_struct_t args_struct[]);
void cmd_print_switches_help(struct args_struct_t args_struct[]);

#ifdef __cplusplus
}
#endif

#endif /* _CMDLINE_COMMON_H */

