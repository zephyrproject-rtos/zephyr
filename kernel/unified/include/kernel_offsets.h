/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <device.h>

#ifndef _kernel_offsets__h_
#define _kernel_offsets__h_

/*
 * The final link step uses the symbol _OffsetAbsSyms to force the linkage of
 * offsets.o into the ELF image.
 */

GEN_ABS_SYM_BEGIN(_OffsetAbsSyms)

GEN_OFFSET_SYM(_kernel_t, current);

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(_kernel_t, threads);
#endif

GEN_OFFSET_SYM(_kernel_t, nested);
GEN_OFFSET_SYM(_kernel_t, irq_stack);
#ifdef CONFIG_SYS_POWER_MANAGEMENT
GEN_OFFSET_SYM(_kernel_t, idle);
#endif

#ifdef CONFIG_FP_SHARING
GEN_OFFSET_SYM(_kernel_t, current_fp);
#endif

GEN_ABSOLUTE_SYM(_STRUCT_KERNEL_SIZE, sizeof(struct _kernel));

GEN_OFFSET_SYM(_thread_base_t, flags);
GEN_OFFSET_SYM(_thread_base_t, prio);
GEN_OFFSET_SYM(_thread_base_t, sched_locked);
GEN_OFFSET_SYM(_thread_base_t, swap_data);

GEN_OFFSET_SYM(_thread_t, base);
GEN_OFFSET_SYM(_thread_t, caller_saved);
GEN_OFFSET_SYM(_thread_t, callee_saved);
GEN_OFFSET_SYM(_thread_t, arch);

#if defined(CONFIG_THREAD_MONITOR)
GEN_OFFSET_SYM(_thread_t, next_thread);
#endif

#ifdef CONFIG_THREAD_CUSTOM_DATA
GEN_OFFSET_SYM(_thread_t, custom_data);
#endif

GEN_ABSOLUTE_SYM(K_THREAD_SIZEOF, sizeof(struct k_thread));

/* size of the device structure. Used by linker scripts */
GEN_ABSOLUTE_SYM(_DEVICE_STRUCT_SIZE, sizeof(struct device));

#endif /* _kernel_offsets__h_ */
