/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EXTMEM_H_
#define EXTMEM_H_

#include <zephyr/kernel.h>

#define EXTMEM_BUF_SIZE   1024
#define EXTMEM_STACK_SIZE 4096

extern uint8_t extmem_buf[EXTMEM_BUF_SIZE];
K_THREAD_STACK_DECLARE(extmem_stack, EXTMEM_STACK_SIZE);

#endif /* EXTMEM_H_ */
