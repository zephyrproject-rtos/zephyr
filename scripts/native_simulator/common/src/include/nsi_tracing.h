/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_TRACING_H
#define NSI_COMMON_SRC_INCL_NSI_TRACING_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Native simulator tracing API:
 *   Print a message/warning/error to the tracing backend
 *   and in case of nsi_print_error_and_exit() also call nsi_exit()
 *
 * All print()/vprint() APIs take the same arguments as printf()/vprintf().
 */

void nsi_print_error_and_exit(const char *format, ...);
void nsi_print_warning(const char *format, ...);
void nsi_print_trace(const char *format, ...);
void nsi_vprint_error_and_exit(const char *format, va_list vargs);
void nsi_vprint_warning(const char *format, va_list vargs);
void nsi_vprint_trace(const char *format, va_list vargs);

/*
 * @brief Is the tracing backend connected to a ptty/terminal or not
 *
 * @param nbr: Which output. Options are: 0 trace output, 1: warning and error output
 *
 * @return
 *   0 : Not a ptty (i.e. probably a pipe to another program)
 *   1 : Connected to a ptty (for ex. stdout/err to the invoking terminal)
 *   -1: Unknown at this point
 */
int nsi_trace_over_tty(int nbr);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_TRACING_H */
