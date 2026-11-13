/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "extmem.h"

/* Both objects of this file are relocated to EXTMEM by CMakeLists.txt. */
__noinit uint8_t extmem_buf[EXTMEM_BUF_SIZE];
K_THREAD_STACK_DEFINE(extmem_stack, EXTMEM_STACK_SIZE);
