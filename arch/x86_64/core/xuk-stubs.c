/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* This "C" file exists solely to include the contents of
 * separately-compiled binary stubs into the link.  It's easier than
 * trying to objcopy the contents into linkable object files,
 * especially when combined with cmake's somewhat odd special-cased
 * dependency handling (which works fine with C files, of course).
 */

/* The 32 bit stub is our entry point and goes into a separate linker
 * section so it can be placed correctly
 */
__asm__(".section .xuk_stub32\n"
	".incbin \"xuk-stub32.bin\"\n");

/* The 16 bit stub is the start of execution for auxiliary SMP CPUs
 * (also for real mode traps if we ever want to expose that
 * capability) and just lives in rodata.  It has to be copied into low
 * memory by the kernel once it is running.
 */
__asm__(".section .rodata\n"
	".globl _xuk_stub16_start\n"
	"_xuk_stub16_start:\n"
	".incbin \"xuk-stub16.bin\"\n"
	".globl _xuk_stub16_end\n"
	"_xuk_stub16_end:\n");

