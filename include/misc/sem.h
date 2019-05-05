/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MISC_SEM_H_
#define ZEPHYR_INCLUDE_MISC_SEM_H_
#ifdef CONFIG_USERSPACE
#include <atomic.h>
#include <zephyr/types.h>

struct sys_sem {
	atomic_t count;
	atomic_val_t limit;
};

#define SYS_SEM_DEFINE(name, initial_count, count_limit) \
	struct sys_sem name = \
	{\
		.count = initial_count,\
		.limit = count_limit\
	}

void sys_sem_init(struct sys_sem *sem, int initial_count, unsigned int limit);
int sys_sem_give(struct sys_sem *sem);
int sys_sem_take(struct sys_sem *sem, s32_t timeout);

#else
#include <kernel.h>
#include <kernel_structs.h>

struct sys_sem {
	struct k_sem kernel_sem;
};

#define SYS_SEM_DEFINE(name, initial_count, count_limit) \
	struct sys_sem name = \
	{\
		.kernel_sem = Z_SEM_INITIALIZER(name.kernel_sem, \
				initial_count, count_limit) \
	}

void sys_sem_init(struct sys_sem *sem, int initial_count, unsigned int limit);
int sys_sem_give(struct sys_sem *sem);
int sys_sem_take(struct sys_sem *sem, s32_t timeout);

#endif
#endif
