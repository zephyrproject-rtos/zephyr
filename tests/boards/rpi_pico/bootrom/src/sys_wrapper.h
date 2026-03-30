/*
 * Copyright (c) 2026 Andy Lin <andylinpersonal@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RPI_PICO_SYS_WRAPPER_H
#define _RPI_PICO_SYS_WRAPPER_H

#include <zephyr/toolchain.h>
#include <stdint.h>

typedef int (*fn_iPmmm_t)(uint32_t *, uint32_t, uint32_t);

__syscall int sys_wrapper_iPmmm(fn_iPmmm_t fn, int *r, uint32_t *a, uint32_t b, uint32_t c);

#include <zephyr/syscalls/sys_wrapper.h>
#endif
