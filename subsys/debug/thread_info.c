/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define THREAD_INFO_UNIMPLEMENTED	0xffffffff

enum {
	THREAD_INFO_OFFSET_VERSION,
	THREAD_INFO_OFFSET_K_CURR_THREAD,
	THREAD_INFO_OFFSET_K_THREADS,
	THREAD_INFO_OFFSET_T_ENTRY,
	THREAD_INFO_OFFSET_T_NEXT_THREAD,
	THREAD_INFO_OFFSET_T_STATE,
	THREAD_INFO_OFFSET_T_USER_OPTIONS,
	THREAD_INFO_OFFSET_T_PRIO,
	THREAD_INFO_OFFSET_T_STACK_PTR,
	THREAD_INFO_OFFSET_T_NAME,
	THREAD_INFO_OFFSET_T_ARCH,
	THREAD_INFO_OFFSET_T_PREEMPT_FLOAT,
	THREAD_INFO_OFFSET_T_COOP_FLOAT,
};

#if CONFIG_MP_NUM_CPUS > 1
#error "This code doesn't work properly with multiple CPUs enabled"
#endif

/* Forward-compatibility notes: 1) Only append items to this table; otherwise
 * debugger plugin versions that expect fewer items will read garbage values.
 * 2) Avoid incompatible changes that affect the interpretation of existing
 * items. But if you have to do them, increment THREAD_INFO_OFFSET_VERSION
 * and submit a patch for debugger plugins to deal with both the old and new
 * scheme.
 * Only version 1 is backward compatible to version 0.
 */
__attribute__((used, section(".dbg_thread_info")))
size_t _kernel_thread_info_offsets[] = {
	/* Version 0 starts */
	[THREAD_INFO_OFFSET_VERSION] = 1,
	[THREAD_INFO_OFFSET_K_CURR_THREAD] = offsetof(struct _cpu, current),
	[THREAD_INFO_OFFSET_K_THREADS] = offsetof(struct z_kernel, threads),
	[THREAD_INFO_OFFSET_T_ENTRY] = offsetof(struct k_thread, entry),
	[THREAD_INFO_OFFSET_T_NEXT_THREAD] = offsetof(struct k_thread,
						      next_thread),
	[THREAD_INFO_OFFSET_T_STATE] = offsetof(struct _thread_base,
						thread_state),
	[THREAD_INFO_OFFSET_T_USER_OPTIONS] = offsetof(struct _thread_base,
						   user_options),
	[THREAD_INFO_OFFSET_T_PRIO] = offsetof(struct _thread_base, prio),
#if defined(CONFIG_ARM64)
	/* We are assuming that the SP of interest is SP_EL1 */
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp_elx),
#elif defined(CONFIG_ARM)
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.psp),
#elif defined(CONFIG_ARC)
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#elif defined(CONFIG_X86)
#if defined(CONFIG_X86_64)
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.rsp),
#else
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.esp),
#endif
#elif defined(CONFIG_MIPS)
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#elif defined(CONFIG_NIOS2)
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#elif defined(CONFIG_RISCV)
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.sp),
#elif defined(CONFIG_SPARC)
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.o6),
#elif defined(CONFIG_ARCH_POSIX)
	[THREAD_INFO_OFFSET_T_STACK_PTR] = offsetof(struct k_thread,
						callee_saved.thread_status),
#elif defined(CONFIG_XTENSA)
	/* Xtensa does not store stack pointers inside thread objects.
	 * The registers are saved in thread stack where there is
	 * no fixed location for this to work. So mark this as
	 * unimplemented to avoid the #warning below.
	 */
	[THREAD_INFO_OFFSET_T_STACK_PTR] = THREAD_INFO_UNIMPLEMENTED,
#else
	/* Use a special value so that OpenOCD knows that obtaining the stack
	 * pointer is not possible on this particular architecture.
	 */
#warning Please define THREAD_INFO_OFFSET_T_STACK_PTR for this architecture
	[THREAD_INFO_OFFSET_T_STACK_PTR] = THREAD_INFO_UNIMPLEMENTED,
#endif
	/* Version 0 ends */

	[THREAD_INFO_OFFSET_T_NAME] = offsetof(struct k_thread, name),
	[THREAD_INFO_OFFSET_T_ARCH] = offsetof(struct k_thread, arch),
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING) && defined(CONFIG_ARM)
	[THREAD_INFO_OFFSET_T_PREEMPT_FLOAT] = offsetof(struct _thread_arch,
						    preempt_float),
	[THREAD_INFO_OFFSET_T_COOP_FLOAT] = THREAD_INFO_UNIMPLEMENTED,
#elif defined(CONFIG_FPU) && defined(CONFIG_X86)
#if defined(CONFIG_X86_64)
	[THREAD_INFO_OFFSET_T_PREEMPT_FLOAT] = offsetof(struct _thread_arch,
							sse),
#else
	[THREAD_INFO_OFFSET_T_PREEMPT_FLOAT] = offsetof(struct _thread_arch,
						    preempFloatReg),
#endif
	[THREAD_INFO_OFFSET_T_COOP_FLOAT] = THREAD_INFO_UNIMPLEMENTED,
#else
	[THREAD_INFO_OFFSET_T_PREEMPT_FLOAT] = THREAD_INFO_UNIMPLEMENTED,
	[THREAD_INFO_OFFSET_T_COOP_FLOAT] = THREAD_INFO_UNIMPLEMENTED,
#endif
	/* Version is still 1, but existence of following elements must be
	 * checked with _kernel_thread_info_num_offsets.
	 */
};
extern size_t __attribute__((alias("_kernel_thread_info_offsets")))
		_kernel_openocd_offsets;

__attribute__((used, section(".dbg_thread_info")))
size_t _kernel_thread_info_num_offsets = ARRAY_SIZE(_kernel_thread_info_offsets);
extern size_t __attribute__((alias("_kernel_thread_info_num_offsets")))
		_kernel_openocd_num_offsets;

__attribute__((used, section(".dbg_thread_info")))
uint8_t _kernel_thread_info_size_t_size = (uint8_t)sizeof(size_t);
extern uint8_t __attribute__((alias("_kernel_thread_info_size_t_size")))
		_kernel_openocd_size_t_size;
