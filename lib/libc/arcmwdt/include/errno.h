/*
 * Copyright (c) 1984-1999, 2012 Wind River Systems, Inc.
 * Copyright (c) 2023 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System error numbers
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_ERRNO_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_ERRNO_H_

#include_next <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MWDT supports range of error codes (ref. $(METAWARE_ROOT)/arc/lib/src/inc/errno.h)
 * Add unsupported only
 */
#ifndef ENODATA
#define ENODATA 86
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* LIB_LIBC_ARCMWDT_INCLUDE_ERRNO_H_ */
