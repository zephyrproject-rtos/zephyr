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
#include <toolchain.h>
#include <linker/sections.h>
#include <sys/atomic.h>
#include <sys/__assert.h>
#include <sched_priq.h>
#include <sys/dlist.h>
#include <sys/slist.h>
#include <sys/sflist.h>
#include <sys/util.h>
#include <kernel_structs.h>
#include <mempool_heap.h>
#include <kernel_version.h>
#include <syscall.h>
#include <sys/printk.h>
#include <arch/cpu.h>
#include <sys/rb.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <fatal.h>
#include <irq.h>
#include <sys/thread_stack.h>
#include <app_memory/mem_domain.h>
#include <sys/kobject.h>
#include <kernel/thread.h>

#endif /* ZEPHYR_INCLUDE_KERNEL_INCLUDES_H_ */
