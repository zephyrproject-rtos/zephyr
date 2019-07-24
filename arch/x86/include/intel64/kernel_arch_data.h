/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_

/*
 * Some SSE definitions. Ideally these will ultimately be shared with 32-bit.
 */

#define X86_FXSAVE_SIZE		512	/* size and alignment of buffer ... */
#define X86_FXSAVE_ALIGN	16	/* ... for FXSAVE/FXRSTOR ops */
#define X86_MXCSR_SANE		0x1dc0	/* enable division-by-zero exception */

/*
 * A flag for k_thread.thread_state to tell __resume that the thread
 * voluntarily switched itself out, so only a portion of the register
 * state needs to be restored. See kernel_arch_thread.h and locore.S.
 */

#define _THREAD_SWAPPED BIT(7)

/*
 * GDT selectors - these must agree with the GDT layout in locore.S.
 */

#define X86_KERNEL_CS_32	0x08	/* 32-bit kernel code */
#define X86_KERNEL_DS_32	0x10	/* 32-bit kernel data */
#define X86_KERNEL_CS_64	0x18	/* 64-bit kernel code */
#define X86_KERNEL_DS_64	0x20	/* 64-bit kernel data */
#define X86_KERNEL_TSS		0x28	/* 64-bit task state segment */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_ */
