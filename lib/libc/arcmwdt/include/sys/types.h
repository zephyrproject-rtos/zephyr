/*
 * Copyright (c) 2023 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_SYS_TYPES_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_SYS_TYPES_H_

#include_next "sys/types.h"

#define _DEV_T_DECLARED
#define _INO_T_DECLARED
#define _NLINK_T_DECLARED
#define _UID_T_DECLARED
#define _GID_T_DECLARED

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#ifdef CONFIG_64BIT
typedef long ssize_t;
#else  /* CONFIG_64BIT */
typedef int ssize_t;
#endif /* CONFIG_64BIT */
#endif /* _SSIZE_T_DEFINED */

#endif /* LIB_LIBC_ARCMWDT_INCLUDE_SYS_TYPES_H_ */
