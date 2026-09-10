/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PICOLIBC_HOOKS_H_
#define _PICOLIBC_HOOKS_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/posix/sys/stat.h>
#include <sys/time.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/errno_private.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/init.h>
#include <zephyr/sys/sem.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_MMU
#include <zephyr/kernel/mm.h>
#endif

#define LIBC_BSS	K_APP_BMEM(z_libc_partition)
#define LIBC_DATA	K_APP_DMEM(z_libc_partition)

#endif /* _PICOLIBC_HOOKS_H_ */
