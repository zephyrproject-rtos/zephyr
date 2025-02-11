/*
 * Copyright (c) 2018 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API to the native simulator - native command line parsing utilities
 *
 * Note: The arguments structure definitions is kept fully compatible with Zephyr's native_posix
 * and BabbleSim's command line options to enable to reuse components between them.
 * And for APIs to be accessible thru a trivial shim.
 */

#ifndef NATIVE_SIMULATOR_NATIVE_SRC_NSI_CMDLINE_H
#define NATIVE_SIMULATOR_NATIVE_SRC_NSI_CMDLINE_H

#include <stdbool.h>
#include <stddef.h>
#include "nsi_cmdline_main_if.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	 * if manual is set nsi_cmd_args_parse*() will ignore it except for
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
	/* Type of option (see nsi_cmd_read_option_value()) */
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

void nsi_get_cmd_line_args(int *argc, char ***argv);
void nsi_get_test_cmd_line_args(int *argc, char ***argv);
void nsi_add_command_line_opts(struct args_struct_t *args);

#ifdef __cplusplus
}
#endif

#endif /* NATIVE_SIMULATOR_NATIVE_SRC_NSI_CMDLINE_H */
