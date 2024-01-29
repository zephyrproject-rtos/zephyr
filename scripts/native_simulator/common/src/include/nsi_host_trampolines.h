/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The native simulator provides a set of trampolines to some of the simplest
 * host C library symbols.
 * These are intended to facilitate test embedded code interacting with the host.
 *
 * We should never include here symbols which require host headers be exposed
 * to the embedded side, for example due to non-basic types being used in
 * function calls, as that would break the include path isolation
 *
 * Naming convention: nsi_host_<fun>() where <func> is the name of the equivalent
 * C library call we call thru.
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_HOST_TRAMPOLINES_H
#define NSI_COMMON_SRC_INCL_NSI_HOST_TRAMPOLINES_H

#ifdef __cplusplus
extern "C" {
#endif

void *nsi_host_calloc(unsigned long nmemb, unsigned long size);
int nsi_host_close(int fd);
/* void nsi_host_exit (int status); Use nsi_exit() instead */
void nsi_host_free(void *ptr);
char *nsi_host_getcwd(char *buf, unsigned long size);
int nsi_host_isatty(int fd);
void *nsi_host_malloc(unsigned long size);
int nsi_host_open(const char *pathname, int flags);
/* int nsi_host_printf (const char *fmt, ...); Use the nsi_tracing.h equivalents */
long nsi_host_random(void);
long nsi_host_read(int fd, void *buffer, unsigned long size);
void *nsi_host_realloc(void *ptr, unsigned long size);
void nsi_host_srandom(unsigned int seed);
char *nsi_host_strdup(const char *s);
long nsi_host_write(int fd, void *buffer, unsigned long size);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_HOST_TRAMPOLINES_H */
