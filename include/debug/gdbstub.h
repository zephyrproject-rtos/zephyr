/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_GDBSTUB_H_
#define ZEPHYR_INCLUDE_DEBUG_GDBSTUB_H_

/* Map from CPU excpetions to GDB  */
#define GDB_EXCEPTION_INVALID_INSTRUCTION   4UL
#define GDB_EXCEPTION_BREAKPOINT            5UL
#define GDB_EXCEPTION_MEMORY_FAULT          7UL
#define GDB_EXCEPTION_DIVIDE_ERROR          8UL
#define GDB_EXCEPTION_INVALID_MEMORY        11UL
#define GDB_EXCEPTION_OVERFLOW              16UL

#endif
