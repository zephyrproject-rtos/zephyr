/*
 * Copyright (c) 2018 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NATIVE_SIMULATOR_NATIVE_SRC_NSI_CMDLINE_INTERNAL_H
#define NATIVE_SIMULATOR_NATIVE_SRC_NSI_CMDLINE_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include "nsi_cmdline.h"

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

int nsi_cmd_is_option(const char *arg, const char *option, int with_value);
int nsi_cmd_is_help_option(const char *arg);
void nsi_cmd_read_option_value(const char *str, void *dest, const char type,
			   const char *option);
void nsi_cmd_args_set_defaults(struct args_struct_t args_struct[]);
bool nsi_cmd_parse_one_arg(char *argv, struct args_struct_t args_struct[]);
void nsi_cmd_print_switches_help(struct args_struct_t args_struct[]);

#ifdef __cplusplus
}
#endif

#endif /* NATIVE_SIMULATOR_NATIVE_SRC_NSI_CMDLINE_INTERNAL_H */
