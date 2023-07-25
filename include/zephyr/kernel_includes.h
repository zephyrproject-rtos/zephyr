/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Header files included by kernel.h.
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_INCLUDES_H_
#define ZEPHYR_INCLUDE_KERNEL_INCLUDES_H_

#include <stddef.h>
#include <zephyr/types.h>
#include <limits.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel/sched_priq.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/sflist.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/kernel/mempool_heap.h>
#include <zephyr/kernel_version.h>
#include <zephyr/syscall.h>
#include <zephyr/sys/printk.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/rb.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/fatal.h>
#include <zephyr/irq.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/app_memory/mem_domain.h>
#include <zephyr/sys/kobject.h>
#include <zephyr/kernel/thread.h>

#endif /* ZEPHYR_INCLUDE_KERNEL_INCLUDES_H_ */
