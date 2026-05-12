/*
 * Copyright (c) 2023 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MWDT picolibc provides sys/cdefs.h; mwlibc does not.
 * Forward to it when available, otherwise no-op.
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_SYS_CDEFS_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_SYS_CDEFS_H_

#if __has_include_next(<sys/cdefs.h>)
#include_next <sys/cdefs.h>
#endif /* __has_include_next */

#endif /* LIB_LIBC_ARCMWDT_INCLUDE_SYS_CDEFS_H_ */
