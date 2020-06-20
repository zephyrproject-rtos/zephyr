/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#define OPENOCD_UNIMPLEMENTED	0xffffffff

#if defined(CONFIG_OPENOCD_SUPPORT) && defined(CONFIG_THREAD_MONITOR)
enum {
	OPENOCD_OFFSET_VERSION,
	OPENOCD_OFFSET_K_CURR_THREAD,
	OPENOCD_OFFSET_K_THREADS,
	OPENOCD_OFFSET_T_ENTRY,
	OPENOCD_OFFSET_T_NEXT_THREAD,
	OPENOCD_OFFSET_T_STATE,
	OPENOCD_OFFSET_T_USER_OPTIONS,
	OPENOCD_OFFSET_T_PRIO,
	OPENOCD_OFFSET_T_STACK_PTR,
	OPENOCD_OFFSET_T_NAME,
	OPENOCD_OFFSET_T_ARCH,
	OPENOCD_OFFSET_T_PREEMPT_FLOAT,
	OPENOCD_OFFSET_T_COOP_FLOAT,
};

#if CONFIG_MP_NUM_CPUS > 1
#error "This code doesn't work properly with multiple CPUs enabled"
#endif

/* Forward-compatibility notes: 1) Only append items to this table; otherwise
 * OpenOCD versions that expect less items will read garbage values.
 * 2) Avoid incompatible changes that affect the interpretation of existing
 * items. But if you have to do them, increment OPENOCD_OFFSET_VERSION
 * and submit a patch for OpenOCD to deal with both the old and new scheme.
 * Only version 1 is backward compatible to version 0.
 */
__attribute__((used, section(".openocd_dbg")))
size_t _kernel_openocd_offsets[] = {
	/* Version 0 starts */
	[OPENOCD_OFFSET_VERSION] = 1,
	[OPENOCD_OFFSET_K_CURR_THREAD] = offsetof(struct _cpu, current),
	[OPENOCD_OFFSET_K_THREADS] = offsetof(struct z_kernel, threads),
	[OPENOCD_OFFSET_T_ENTRY] = offsetof(struct k_thread, entry),
	[OPENOCD_OFFSET_T_NEXT_THREAD] = offsetof(struct k_thread, next_thread),
	[OPENOCD_OFFSET_T_STATE] = offsetof(struct _thread_base, thread_state),
	[OPENOCD_OFFSET_T_USER_OPTIONS] = offsetof(struct _thread_base,
						   user_options),
	[OPENOCD_OFFSET_T_PRIO] = offsetof(struct _thread_base, prio),
#if defined(CONFIG_ARM64)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#elif defined(CONFIG_ARM)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.psp),
#elif defined(CONFIG_ARC)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#elif defined(CONFIG_X86)
#if defined(CONFIG_X86_64)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.rsp),
#else
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.esp),
#endif
#elif defined(CONFIG_NIOS2)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#elif defined(CONFIG_RISCV)
	[OPENOCD_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#else
	/* Use a special value so that OpenOCD knows that obtaining the stack
	 * pointer is not possible on this particular architecture.
	 */
#warning Please define OPENOCD_OFFSET_T_STACK_PTR for this architecture
	[OPENOCD_OFFSET_T_STACK_PTR] = OPENOCD_UNIMPLEMENTED,
#endif
	/* Version 0 ends */

	[OPENOCD_OFFSET_T_NAME] = offsetof(struct k_thread, name),
	[OPENOCD_OFFSET_T_ARCH] = offsetof(struct k_thread, arch),
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING) && defined(CONFIG_ARM)
	[OPENOCD_OFFSET_T_PREEMPT_FLOAT] = offsetof(struct _thread_arch,
						    preempt_float),
	[OPENOCD_OFFSET_T_COOP_FLOAT] = OPENOCD_UNIMPLEMENTED,
#elif defined(CONFIG_FPU) && defined(CONFIG_X86)
#if defined(CONFIG_X86_64)
	[OPENOCD_OFFSET_T_PREEMPT_FLOAT] = offsetof(struct _thread_arch, sse),
#else
	[OPENOCD_OFFSET_T_PREEMPT_FLOAT] = offsetof(struct _thread_arch,
						    preempFloatReg),
#endif
	[OPENOCD_OFFSET_T_COOP_FLOAT] = OPENOCD_UNIMPLEMENTED,
#else
	[OPENOCD_OFFSET_T_PREEMPT_FLOAT] = OPENOCD_UNIMPLEMENTED,
	[OPENOCD_OFFSET_T_COOP_FLOAT] = OPENOCD_UNIMPLEMENTED,
#endif
	/* Version is still 1, but existence of following elements must be
	 * checked with _kernel_openocd_num_offsets.
	 */
};

__attribute__((used, section(".openocd_dbg")))
size_t _kernel_openocd_num_offsets = ARRAY_SIZE(_kernel_openocd_offsets);

__attribute__((used, section(".openocd_dbg")))
uint8_t _kernel_openocd_size_t_size = (uint8_t)sizeof(size_t);
#endif
