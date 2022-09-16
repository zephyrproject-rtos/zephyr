/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_POSIX_POSIX_TRACE_H_
#define ZEPHYR_INCLUDE_ARCH_POSIX_POSIX_TRACE_H_

#ifdef __cplusplus
extern "C" {
#endif

void posix_print_error_and_exit(const char *format, ...);
void posix_print_warning(const char *format, ...);
void posix_print_trace(const char *format, ...);
/*
 * Return 1 if traces to <output> will go to a tty.
 * When printing to a terminal we may use ASCII escapes for color or other
 * niceties.
 * But when redirecting to files, or piping to other commands, those should be
 * disabled by default.
 *
 * Where the <output> should be set to 0 to query about posix_print_trace output
 * (typically STDOUT)
 * and 1 to query about the warning and error output (posix_print_error/warning)
 * outputs (typically STDERR)
 */
int posix_trace_over_tty(int output);

#ifdef __cplusplus
}
#endif

#endif
