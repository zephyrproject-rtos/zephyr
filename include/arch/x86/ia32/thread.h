/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 *
 * This file contains definitions for
 *
 *  struct _thread_arch
 *  struct _callee_saved
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_IA32_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_X86_IA32_THREAD_H_

/**
 * Floating point register set alignment.
 *
 * If support for SSEx extensions is enabled a 16 byte boundary is required,
 * since the 'fxsave' and 'fxrstor' instructions require this. In all other
 * cases a 4 byte boundary is sufficient.
 */
#if defined(CONFIG_EAGER_FPU_SHARING) || defined(CONFIG_LAZY_FPU_SHARING)
#ifdef CONFIG_SSE
#define FP_REG_SET_ALIGN  16
#else
#define FP_REG_SET_ALIGN  4
#endif
#else
/* Unused, no special alignment requirements, use default alignment for
 * char buffers on this arch
 */
#define FP_REG_SET_ALIGN  1
#endif /* CONFIG_*_FP_SHARING */

/*
 * Bits for _thread_arch.flags, see their use in intstub.S et al.
 */

#define X86_THREAD_FLAG_INT 0x01
#define X86_THREAD_FLAG_EXC 0x02
#define X86_THREAD_FLAG_ALL (X86_THREAD_FLAG_INT | X86_THREAD_FLAG_EXC)

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <arch/x86/mmustructs.h>

/*
 * The following structure defines the set of 'non-volatile' integer registers.
 * These registers must be preserved by a called C function.  These are the
 * only registers that need to be saved/restored when a cooperative context
 * switch occurs.
 */

struct _callee_saved {
	unsigned long esp;

	/*
	 * The following registers are considered non-volatile, i.e.
	 * callee-save,
	 * but their values are pushed onto the stack rather than stored in the
	 * TCS
	 * structure:
	 *
	 *  unsigned long ebp;
	 *  unsigned long ebx;
	 *  unsigned long esi;
	 *  unsigned long edi;
	 */

};

typedef struct _callee_saved _callee_saved_t;

/*
 * The macros CONFIG_{LAZY|EAGER}_FPU_SHARING shall be set to indicate that the
 * saving/restoring of the traditional x87 floating point (and MMX) registers
 * are supported by the kernel's context swapping code. The macro
 * CONFIG_SSE shall _also_ be set if saving/restoring of the XMM
 * registers is also supported in the kernel's context swapping code.
 */

#if defined(CONFIG_EAGER_FPU_SHARING) || defined(CONFIG_LAZY_FPU_SHARING)

/* definition of a single x87 (floating point / MMX) register */

typedef struct s_FpReg {
	unsigned char reg[10]; /* 80 bits: ST[0-7] */
} tFpReg;

/*
 * The following is the "normal" floating point register save area, or
 * more accurately the save area required by the 'fnsave' and 'frstor'
 * instructions.  The structure matches the layout described in the
 * "Intel(r) 64 and IA-32 Architectures Software Developer's Manual
 * Volume 1: Basic Architecture": Protected Mode x87 FPU State Image in
 * Memory, 32-Bit Format.
 */

typedef struct s_FpRegSet {  /* # of bytes: name of register */
	unsigned short fcw;      /* 2  : x87 FPU control word */
	unsigned short pad1;     /* 2  : N/A */
	unsigned short fsw;      /* 2  : x87 FPU status word */
	unsigned short pad2;     /* 2  : N/A */
	unsigned short ftw;      /* 2  : x87 FPU tag word */
	unsigned short pad3;     /* 2  : N/A */
	unsigned int fpuip;      /* 4  : x87 FPU instruction pointer offset */
	unsigned short cs;       /* 2  : x87 FPU instruction pointer selector */
	unsigned short fop : 11; /* 2  : x87 FPU opcode */
	unsigned short pad4 : 5; /*    : 5 bits = 00000 */
	unsigned int fpudp;      /* 4  : x87 FPU instr operand ptr offset */
	unsigned short ds;       /* 2  : x87 FPU instr operand ptr selector */
	unsigned short pad5;     /* 2  : N/A */
	tFpReg fpReg[8];	 /* 80 : ST0 -> ST7 */
} tFpRegSet __aligned(FP_REG_SET_ALIGN);

#ifdef CONFIG_SSE

/* definition of a single x87 (floating point / MMX) register */

typedef struct s_FpRegEx {
	unsigned char reg[10];  /* 80 bits: ST[0-7] or MM[0-7] */
	unsigned char rsrvd[6]; /* 48 bits: reserved */
} tFpRegEx;

/* definition of a single XMM register */

typedef struct s_XmmReg {
	unsigned char reg[16]; /* 128 bits: XMM[0-7] */
} tXmmReg;

/*
 * The following is the "extended" floating point register save area, or
 * more accurately the save area required by the 'fxsave' and 'fxrstor'
 * instructions.  The structure matches the layout described in the
 * "Intel 64 and IA-32 Architectures Software Developer's Manual
 * Volume 2A: Instruction Set Reference, A-M", except for the bytes from offset
 * 464 to 511 since these "are available to software use. The processor does
 * not write to bytes 464:511 of an FXSAVE area".
 *
 * This structure must be aligned on a 16 byte boundary when the instructions
 * fxsave/fxrstor are used to write/read the data to/from the structure.
 */

typedef struct s_FpRegSetEx /* # of bytes: name of register */
{
	unsigned short fcw;     /* 2  : x87 FPU control word */
	unsigned short fsw;     /* 2  : x87 FPU status word */
	unsigned char ftw;      /* 1  : x87 FPU abridged tag word */
	unsigned char rsrvd0;   /* 1  : reserved */
	unsigned short fop;     /* 2  : x87 FPU opcode */
	unsigned int fpuip;     /* 4  : x87 FPU instruction pointer offset */
	unsigned short cs;      /* 2  : x87 FPU instruction pointer selector */
	unsigned short rsrvd1;  /* 2  : reserved */
	unsigned int fpudp;     /* 4  : x87 FPU instr operand ptr offset */
	unsigned short ds;      /* 2  : x87 FPU instr operand ptr selector */
	unsigned short rsrvd2;  /* 2  : reserved */
	unsigned int mxcsr;     /* 4  : MXCSR register state */
	unsigned int mxcsrMask; /* 4  : MXCSR register mask */
	tFpRegEx fpReg[8];      /* 128 : x87 FPU/MMX registers */
	tXmmReg xmmReg[8];      /* 128 : XMM registers */
	unsigned char rsrvd3[176]; /* 176 : reserved */
} tFpRegSetEx __aligned(FP_REG_SET_ALIGN);

#else /* CONFIG_SSE == 0 */

typedef struct s_FpRegSetEx {
} tFpRegSetEx;

#endif /* CONFIG_SSE == 0 */

#else /* !CONFIG_LAZY_FPU_SHARING && !CONFIG_EAGER_FPU_SHARING */

/* empty floating point register definition */

typedef struct s_FpRegSet {
} tFpRegSet;

typedef struct s_FpRegSetEx {
} tFpRegSetEx;

#endif /* CONFIG_LAZY_FPU_SHARING || CONFIG_EAGER_FPU_SHARING */

/*
 * The following structure defines the set of 'volatile' x87 FPU/MMX/SSE
 * registers.  These registers need not be preserved by a called C function.
 * Given that they are not preserved across function calls, they must be
 * save/restored (along with s_coopFloatReg) when a preemptive context
 * switch occurs.
 */

typedef struct s_preempFloatReg {
	union {
		/* threads with K_FP_REGS utilize this format */
		tFpRegSet fpRegs;
		/* threads with K_SSE_REGS utilize this format */
		tFpRegSetEx fpRegsEx;
	} floatRegsUnion;
} tPreempFloatReg;

/*
 * The thread control structure definition.  It contains the
 * various fields to manage a _single_ thread. The TCS will be aligned
 * to the appropriate architecture specific boundary via the
 * arch_new_thread() call.
 */

struct _thread_arch {
	uint8_t flags;

#ifdef CONFIG_USERSPACE
	/* Physical address of the page tables used by this thread. Supervisor
	 * threads always use the kernel's page table, user thread use
	 * per-thread tables stored in the stack object.
	 */
	uintptr_t ptables;

	/* Track available unused space in the stack object used for building
	 * thread-specific page tables.
	 */
	uint8_t *mmu_pos;

	/* Initial privilege mode stack pointer when doing a system call.
	 * Un-set for supervisor threads.
	 */
	char *psp;
#endif

#if defined(CONFIG_LAZY_FPU_SHARING)
	/*
	 * Nested exception count to maintain setting of EXC_ACTIVE flag across
	 * outermost exception.  EXC_ACTIVE is used by z_swap() lazy FP
	 * save/restore and by debug tools.
	 */
	unsigned excNestCount; /* nested exception count */
#endif /* CONFIG_LAZY_FPU_SHARING */

	tPreempFloatReg preempFloatReg; /* volatile float register storage */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_IA32_THREAD_H_ */
