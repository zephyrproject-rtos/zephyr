/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NATIVE_POSIX_CMDLINE_H
#define _NATIVE_POSIX_CMDLINE_H


void native_handle_cmd_line(int argc, char *argv[]);
void native_get_cmd_line_args(int *argc, char ***argv);

#endif /* _NATIVE_POSIX_CMDLINE_H */
