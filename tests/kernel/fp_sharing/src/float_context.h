/**
 * @file
 * @brief common definitions for the FPU sharing test application
 */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLOATCONTEXT_H
#define _FLOATCONTEXT_H

/*
 * Each architecture must define the following structures (which may be empty):
 *      'struct fp_volatile_register_set'
 *      'struct fp_non_volatile_register_set'
 *
 * Each architecture must also define the following macros:
 *      SIZEOF_FP_VOLATILE_REGISTER_SET
 *      SIZEOF_FP_NON_VOLATILE_REGISTER_SET
 * Those macros are used as sizeof(<an empty structure>) is compiler specific;
 * that is, it may evaluate to a non-zero value.
 *
 * Each architecture shall also have custom implementations of:
 *     _load_all_float_registers()
 *     _load_then_store_all_float_registers()
 *     _store_all_float_registers()
 */

#if defined(CONFIG_ISA_IA32)

#define FP_OPTION 0

/*
 * In the future, the struct definitions may need to be refined based on the
 * specific IA-32 processor, but for now only the Pentium4 is supported:
 *
 *          8 x 80 bit floating point registers (ST[0] -> ST[7])
 *          8 x 128 bit XMM registers           (XMM[0] -> XMM[7])
 *
 * All these registers are considered volatile across a function invocation.
 */

struct fp_register {
	unsigned char reg[10];
};

struct xmm_register {
	unsigned char reg[16];
};

struct fp_volatile_register_set {
	struct xmm_register xmm[8];     /* XMM[0] -> XMM[7] */
	struct fp_register st[8];       /* ST[0] -> ST[7] */
};

struct fp_non_volatile_register_set {
	/* No non-volatile floating point registers */
};

#define SIZEOF_FP_VOLATILE_REGISTER_SET sizeof(struct fp_volatile_register_set)
#define SIZEOF_FP_NON_VOLATILE_REGISTER_SET 0

#elif defined(CONFIG_CPU_CORTEX_M4)

#define FP_OPTION 0

/*
 * Registers s0..s15 are volatile and do not
 * need to be preserved across function calls.
 */
struct fp_volatile_register_set {
	float s[16];
};

/*
 * Registers s16..s31 are non-volatile and
 * need to be preserved across function calls.
 */
struct fp_non_volatile_register_set {
	float s[16];
};

#define SIZEOF_FP_VOLATILE_REGISTER_SET	\
	sizeof(struct fp_volatile_register_set)
#define SIZEOF_FP_NON_VOLATILE_REGISTER_SET \
	sizeof(struct fp_non_volatile_register_set)

#else

#error  "Architecture must provide the following definitions:\n"
"\t'struct fp_volatile_registers'\n"
"\t'struct fp_non_volatile_registers'\n"
"\t'SIZEOF_FP_VOLATILE_REGISTER_SET'\n"
"\t'SIZEOF_FP_NON_VOLATILE_REGISTER_SET'\n"
#endif /* CONFIG_ISA_IA32 */

/* the set of ALL floating point registers */

struct fp_register_set {
	struct fp_volatile_register_set fp_volatile;
	struct fp_non_volatile_register_set fp_non_volatile;
};

#define SIZEOF_FP_REGISTER_SET \
	(SIZEOF_FP_VOLATILE_REGISTER_SET + SIZEOF_FP_NON_VOLATILE_REGISTER_SET)

/*
 * The following constants define the initial byte value used by the background
 * task, and the thread when loading up the floating point registers.
 */

#define MAIN_FLOAT_REG_CHECK_BYTE ((unsigned char)0xe5)
#define FIBER_FLOAT_REG_CHECK_BYTE ((unsigned char)0xf9)

extern int fpu_sharing_error;

#endif /* _FLOATCONTEXT_H */
