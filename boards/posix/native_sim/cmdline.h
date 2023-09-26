/*
 * Copyright (c) 2018 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARDS_POSIX_NATIVE_SIM_CMDLINE_H
#define BOARDS_POSIX_NATIVE_SIM_CMDLINE_H

#include "nsi_cmdline.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * To support native_posix drivers or tests which register their own arguments
 * we provide a header with the same name as in native_posix
 */
void native_get_cmd_line_args(int *argc, char ***argv);
void native_get_test_cmd_line_args(int *argc, char ***argv);
void native_add_command_line_opts(struct args_struct_t *args);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NATIVE_SIM_CMDLINE_H */
