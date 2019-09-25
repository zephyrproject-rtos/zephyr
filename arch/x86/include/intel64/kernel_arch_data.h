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
 * GDT selectors - these must agree with the GDT layout in locore.S.
 */

#define X86_KERNEL_CS_32	0x08	/* 32-bit kernel code */
#define X86_KERNEL_DS_32	0x10	/* 32-bit kernel data */
#define X86_KERNEL_CS_64	0x18	/* 64-bit kernel code */
#define X86_KERNEL_DS_64	0x20	/* 64-bit kernel data */

#define X86_KERNEL_GS_64	0x30	/* data selector covering TSS */
#define X86_KERNEL_TSS		0x40	/* 64-bit task state segment */

#ifndef _ASMLANGUAGE

/*
 * 64-bit Task State Segment. One defined per CPU.
 */

struct x86_tss64 {
	/*
	 * Architecturally-defined portion. It is somewhat tedious to
	 * enumerate each member specifically (rather than using arrays)
	 * but we need to get (some of) their offsets from assembly.
	 */

	u8_t reserved0[4];

	u64_t rsp0;		/* privileged stacks */
	u64_t rsp1;
	u64_t rsp2;

	u8_t reserved[8];

	u64_t ist1;		/* interrupt stacks */
	u64_t ist2;
	u64_t ist3;
	u64_t ist4;
	u64_t ist5;
	u64_t ist6;
	u64_t ist7;

	u8_t reserved1[10];

	u16_t iomapb;		/* offset to I/O base */

	/*
	 * Zephyr specific portion. Stash per-CPU data here for convenience.
	 */

	struct _cpu *cpu;
} __packed __aligned(8);

typedef struct x86_tss64 x86_tss64_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_ */
