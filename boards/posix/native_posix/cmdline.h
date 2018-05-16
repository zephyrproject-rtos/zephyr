/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NATIVE_POSIX_CMDLINE_H
#define _NATIVE_POSIX_CMDLINE_H

#ifdef __cplusplus
extern "C" {
#endif

struct args_t {
	double stop_at;
#if defined(CONFIG_FAKE_ENTROPY_NATIVE_POSIX)
	u32_t seed;
#endif
};

void native_handle_cmd_line(int argc, char *argv[]);
void native_get_cmd_line_args(int *argc, char ***argv);
void native_get_test_cmd_line_args(int *argc, char ***argv);

#ifdef __cplusplus
}
#endif

#endif /* _NATIVE_POSIX_CMDLINE_H */
