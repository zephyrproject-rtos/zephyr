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
 *  struct _caller_saved
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef _kernel_arch_thread__h_
#define _kernel_arch_thread__h_

/**
 * Floating point register set alignment.
 *
 * If support for SSEx extensions is enabled a 16 byte boundary is required,
 * since the 'fxsave' and 'fxrstor' instructions require this. In all other
 * cases a 4 byte boundary is sufficient.
 */

#ifdef CONFIG_SSE
#define FP_REG_SET_ALIGN  16
#else
#define FP_REG_SET_ALIGN  4
#endif

#ifndef _ASMLANGUAGE
#include <stdint.h>

/*
 * The following structure defines the set of 'volatile' integer registers.
 * These registers need not be preserved by a called C function.  Given that
 * they are not preserved across function calls, they must be save/restored
 * (along with the struct _caller_saved) when a preemptive context switch
 * occurs.
 */

struct _caller_saved {

	/*
	 * The volatile registers 'eax', 'ecx' and 'edx' area not included in
	 * the definition of 'tPreempReg' since the interrupt and exception
	 * handling routunes use the stack to save and restore the values of
	 * these registers in order to support interrupt nesting.  The stubs
	 * do _not_ copy the saved values from the stack into the TCS.
	 *
	 * unsigned long eax;
	 * unsigned long ecx;
	 * unsigned long edx;
	 */

};

typedef struct _caller_saved _caller_saved_t;

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
 * The macro CONFIG_FP_SHARING shall be set to indicate that the
 * saving/restoring of the traditional x87 floating point (and MMX) registers
 * are supported by the kernel's context swapping code. The macro
 * CONFIG_SSE shall _also_ be set if saving/restoring of the XMM
 * registers is also supported in the kernel's context swapping code.
 */

#ifdef CONFIG_FP_SHARING

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

#else /* CONFIG_FP_SHARING == 0 */

/* empty floating point register definition */

typedef struct s_FpRegSet {
} tFpRegSet;

typedef struct s_FpRegSetEx {
} tFpRegSetEx;

#endif /* CONFIG_FP_SHARING == 0 */

/*
 * The following structure defines the set of 'non-volatile' x87 FPU/MMX/SSE
 * registers. These registers must be preserved by a called C function.
 * These are the only registers that need to be saved/restored when a
 * cooperative context switch occurs.
 */

typedef struct s_coopFloatReg {

	/*
	 * This structure intentionally left blank, i.e. the ST[0] -> ST[7] and
	 * XMM0 -> XMM7 registers are all 'volatile'.
	 */

} tCoopFloatReg;

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
 * The thread control stucture definition.  It contains the
 * various fields to manage a _single_ thread. The TCS will be aligned
 * to the appropriate architecture specific boundary via the
 * _new_thread() call.
 */

struct _thread_arch {

#ifdef CONFIG_GDB_INFO
	 /* pointer to ESF saved by outermost exception wrapper */
	void *esf;
#endif

#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO))
	/*
	 * Nested exception count to maintain setting of EXC_ACTIVE flag across
	 * outermost exception.  EXC_ACTIVE is used by _Swap() lazy FP
	 * save/restore and by debug tools.
	 */
	unsigned excNestCount; /* nested exception count */
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */

	/*
	 * The location of all floating point related structures/fields MUST be
	 * located at the end of struct tcs.  This way only the
	 * threads that actually utilize non-integer capabilities need to
	 * account for the increased memory required for storing FP state when
	 * sizing stacks.
	 *
	 * Given that stacks "grow down" on IA-32, and the TCS is located
	 * at the start of a thread's "workspace" memory, the stacks of
	 * threads that do not utilize floating point instruction can
	 * effectively consume the memory occupied by the 'tCoopFloatReg' and
	 * 'tPreempFloatReg' structures without ill effect.
	 */

	tCoopFloatReg coopFloatReg; /* non-volatile float register storage */
	tPreempFloatReg preempFloatReg; /* volatile float register storage */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* _kernel_arch_thread__h_ */
