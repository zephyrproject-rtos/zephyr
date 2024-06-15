/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_DEBUG_GDBSTUB_BACKEND_H_
#define ZEPHYR_SUBSYS_DEBUG_GDBSTUB_BACKEND_H_

#include <stdint.h>

/**
 * This is an internal header. These API is intended to be used
 * exclusively by gdbstub.
 *
 * A backend has to implement these three functions knowing that they
 * will be called in an interruption context.
 */

/**
 * @brief Initialize the gdbstub backend
 *
 * This function is called from @c gdb_start to
 * give the opportunity to the backend initialize properly.
 *
 * @retval 0 In case of success
 * @retval -1 If the backend was not initialized properly
 */
int z_gdb_backend_init(void);

/**
 * @brief Output a character
 *
 * @param ch Character to send
 */
void z_gdb_putchar(unsigned char ch);

/**
 * @brief Receive a character
 *
 * This function blocks until have a valid
 * character to return.
 *
 * @return A character
 */
char z_gdb_getchar(void);

#endif
