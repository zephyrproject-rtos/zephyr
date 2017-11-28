/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _POSIX_CORE_TRACING_H
#define _POSIX_CORE_TRACING_H


void simulation_engine_print_error_and_exit(const char *format, ...);
void simulation_engine_print_warning(const char *format, ...);
void simulation_engine_print_trace(const char *format, ...);


#ifdef __cplusplus
}
#endif

#endif /* _POSIX_CORE_TRACING_H */
