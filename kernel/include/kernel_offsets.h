/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device.h>

#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_OFFSETS_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_OFFSETS_H_

#include <syscall_list.h>

/* All of this is build time magic, but LCOV gets confused. Disable coverage
 * for this whole file.
 *
 * LCOV_EXCL_START
 */

/*
 * The final link step uses the symbol _OffsetAbsSyms to force the linkage of
 * offsets.o into the ELF image.
 */

GEN_ABS_SYM_BEGIN(_OffsetAbsSyms)

GEN_OFFSET_SYM(_cpu_t, current);
GEN_OFFSET_SYM(_cpu_t, nested);
GEN_OFFSET_SYM(_cpu_t, irq_stack);

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(_kernel_t, threads);
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
GEN_OFFSET_SYM(_kernel_t, idle);
#endif

GEN_OFFSET_SYM(_kernel_t, ready_q);

#ifndef CONFIG_SMP
GEN_OFFSET_SYM(_ready_q_t, cache);
#endif

#ifdef CONFIG_FPU_SHARING
GEN_OFFSET_SYM(_kernel_t, current_fp);
#endif

GEN_ABSOLUTE_SYM(_STRUCT_KERNEL_SIZE, sizeof(struct z_kernel));

GEN_OFFSET_SYM(_thread_base_t, user_options);
GEN_OFFSET_SYM(_thread_base_t, thread_state);
GEN_OFFSET_SYM(_thread_base_t, prio);
GEN_OFFSET_SYM(_thread_base_t, sched_locked);
GEN_OFFSET_SYM(_thread_base_t, preempt);
GEN_OFFSET_SYM(_thread_base_t, swap_data);

GEN_OFFSET_SYM(_thread_t, base);
GEN_OFFSET_SYM(_thread_t, callee_saved);
GEN_OFFSET_SYM(_thread_t, arch);

#ifdef CONFIG_USE_SWITCH
GEN_OFFSET_SYM(_thread_t, switch_handle);
#endif

#ifdef CONFIG_THREAD_STACK_INFO
GEN_OFFSET_SYM(_thread_stack_info_t, start);
GEN_OFFSET_SYM(_thread_stack_info_t, size);

GEN_OFFSET_SYM(_thread_t, stack_info);
#endif

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(_thread_t, next_thread);
#endif

#ifdef CONFIG_THREAD_CUSTOM_DATA
GEN_OFFSET_SYM(_thread_t, custom_data);
#endif

GEN_ABSOLUTE_SYM(K_THREAD_SIZEOF, sizeof(struct k_thread));

/* size of the device structure. Used by linker scripts */
GEN_ABSOLUTE_SYM(_DEVICE_STRUCT_SIZEOF, sizeof(const struct device));

/* LCOV_EXCL_STOP */
#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_OFFSETS_H_ */
