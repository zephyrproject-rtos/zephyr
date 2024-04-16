/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARCH_POSIX_CORE_NSI_TRACING_H
#define ARCH_POSIX_CORE_NSI_TRACING_H

#ifdef __cplusplus
extern "C" {
#endif

void nsi_print_error_and_exit(const char *format, ...);
void nsi_print_warning(const char *format, ...);
void nsi_print_trace(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* ARCH_POSIX_CORE_NSI_TRACING_H */
