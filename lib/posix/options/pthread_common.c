/*
 * Copyright (c) 2023 Meta
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_internal.h"

#include <zephyr/sys/sem.h>

__pinned_bss struct posix_thread posix_thread_pool[CONFIG_POSIX_THREAD_THREADS_MAX];
SYS_SEM_DEFINE(pthread_pool_lock, 1, 1);
