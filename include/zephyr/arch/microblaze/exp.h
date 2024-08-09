/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_ARCH_MICROBLAZE_EXP_H_
#define ZEPHYR_INCLUDE_ARCH_MICROBLAZE_EXP_H_

#ifndef _ASMLANGUAGE
#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/arch/microblaze/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct __esf {

	uint32_t r31; /*  Must be saved across function calls. Callee-save. */
	uint32_t r30; /*  Must be saved across function calls. Callee-save. */
	uint32_t r29; /*  Must be saved across function calls. Callee-save. */
	uint32_t r28; /*  Must be saved across function calls. Callee-save. */
	uint32_t r27; /*  Must be saved across function calls. Callee-save. */
	uint32_t r26; /*  Must be saved across function calls. Callee-save. */
	uint32_t r25; /*  Must be saved across function calls. Callee-save. */
	uint32_t r24; /*  Must be saved across function calls. Callee-save. */
	uint32_t r23; /*  Must be saved across function calls. Callee-save. */
	uint32_t r22; /*  Must be saved across function calls. Callee-save. */
	uint32_t r21; /*  Must be saved across function calls. Callee-save. */
	/* r20: Reserved for storing a pointer to the global offset table (GOT)
	 * in position independent code (PIC). Non-volatile in non-PIC code
	 */
	uint32_t r20; /* Must be saved across function calls. Callee-save.*/
	uint32_t r19; /* must be saved across function-calls. Callee-save */

	/* general purpose registers */
	uint32_t r18; /* reserved for assembler/compiler temporaries */
	uint32_t r17; /* return address for exceptions. HW if configured else SW */
	uint32_t r16; /* return address for breaks */
	uint32_t r15; /* return address for user vectors */
	uint32_t r14; /* return address for interrupts */
	uint32_t r13; /* read/write small data anchor */
	uint32_t r12; /* temporaries */
	uint32_t r11; /* temporaries */
	uint32_t r10; /* passing parameters / temporaries */
	uint32_t r9;  /* passing parameters / temporaries */
	uint32_t r8;  /* passing parameters / temporaries */
	uint32_t r7;  /* passing parameters / temporaries */
	uint32_t r6;  /* passing parameters / temporaries */
	uint32_t r5;  /* passing parameters / temporaries */
	uint32_t r4;  /* return values / temporaries */
	uint32_t r3;  /* return values / temporaries */
	uint32_t r2;  /* Read-only small data area anchor */
	uint32_t r1;  /* Cstack pointer */
	uint32_t msr;
#if defined(CONFIG_MICROBLAZE_USE_HARDWARE_FLOAT_INSTR)
	uint32_t fsr;
#endif
};

typedef struct __esf z_arch_esf_t;

typedef struct __microblaze_register_dump {

	_callee_saved_t callee_saved;
	z_arch_esf_t esf;

	/* Other SFRs */
	uint32_t pc;
	uint32_t esr;
	uint32_t ear;
	uint32_t edr;

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* A human readable description of the exception cause.  The strings used
	 * are the same as the #define constant names found in the
	 * microblaze_exceptions_i.h header file
	 */
	char *exception_cause_str;
#endif /* defined(CONFIG_EXTRA_EXCEPTION_INFO) */

} microblaze_register_dump_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_MICROBLAZE_EXP_H_ */
