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

#ifndef ZEPHYR_INCLUDE_KERNEL_H_
#error  Please do not include kernel-specific headers directly, use <zephyr/kernel.h> instead
#endif

#include <stddef.h>
#include <zephyr/types.h>
#include <limits.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/sflist.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel/obj_core.h>
#include <zephyr/kernel_structs.h>
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
/* FIXME This needs to be removed. Exposes some private APIs to SOF */
#include <zephyr/kernel/internal/smp.h>

#endif /* ZEPHYR_INCLUDE_KERNEL_INCLUDES_H_ */
