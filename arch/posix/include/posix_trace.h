/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _POSIX_TRACE_H
#define _POSIX_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

void posix_print_error_and_exit(const char *format, ...);
void posix_print_warning(const char *format, ...);
void posix_print_trace(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
