/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include "kernel_internal.h"

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
GEN_OFFSET_SYM(_cpu_t, arch);

GEN_OFFSET_SYM(_kernel_t, cpus);

#if defined(CONFIG_FPU_SHARING)
GEN_OFFSET_SYM(_cpu_t, fp_ctx);
#endif

#ifdef CONFIG_PM
GEN_OFFSET_SYM(_kernel_t, idle);
#endif

#ifndef CONFIG_SCHED_CPU_MASK_PIN_ONLY
GEN_OFFSET_SYM(_kernel_t, ready_q);
#endif

#ifndef CONFIG_SMP
GEN_OFFSET_SYM(_ready_q_t, cache);
#endif

#ifdef CONFIG_FPU_SHARING
GEN_OFFSET_SYM(_kernel_t, current_fp);
#endif

GEN_OFFSET_SYM(_thread_base_t, user_options);

GEN_OFFSET_SYM(_thread_t, base);
GEN_OFFSET_SYM(_thread_t, callee_saved);
GEN_OFFSET_SYM(_thread_t, arch);

#ifdef CONFIG_USE_SWITCH
GEN_OFFSET_SYM(_thread_t, switch_handle);
#endif

#ifdef CONFIG_THREAD_STACK_INFO
GEN_OFFSET_SYM(_thread_t, stack_info);
#endif

#ifdef CONFIG_THREAD_LOCAL_STORAGE
GEN_OFFSET_SYM(_thread_t, tls);
#endif

GEN_ABSOLUTE_SYM(__z_interrupt_stack_SIZEOF, sizeof(z_interrupt_stacks[0]));

/* member offsets in the device structure. Used in image post-processing */
#ifdef CONFIG_DEVICE_DEPS
GEN_ABSOLUTE_SYM(_DEVICE_STRUCT_HANDLES_OFFSET,
		 offsetof(struct device, deps));
#endif

#ifdef CONFIG_PM_DEVICE
GEN_ABSOLUTE_SYM(_DEVICE_STRUCT_PM_OFFSET,
		 offsetof(struct device, pm));
#endif

/* member offsets in the pm_device structure. Used in image post-processing */

GEN_ABSOLUTE_SYM(_PM_DEVICE_STRUCT_FLAGS_OFFSET,
		 offsetof(struct pm_device_base, flags));

GEN_ABSOLUTE_SYM(_PM_DEVICE_FLAG_PD, PM_DEVICE_FLAG_PD);

/* LCOV_EXCL_STOP */
#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_OFFSETS_H_ */
