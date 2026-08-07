/*
 * Copyright (c) BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

/*
 * These simple memset and memcpy alternatives are necessary as the optimized
 * ones depend on the MMU to be active (see commit c5b898743a20).
 *
 * Furthermore, we can't implement those in C as the compiler is just too
 * smart for its own good and replaces our simple loops into direct calls
 * to memset or memcpy on its own.
 */

/* void arch_early_memset(void *dst, int c, size_t n) */
GTEXT(arch_early_memset)
SECTION_FUNC(TEXT, arch_early_memset)

	/* is dst pointer 8-bytes aligned? */
	tst	x0, #0x7
	b.ne	2f

	/* at least 8 bytes to set? */
	cmp	x2, #8
	b.lo	2f

	/* spread the byte value across whole 64 bits */
	and	x8, x1, #0xff
        mov	x9, #0x0101010101010101
	mul	x8, x8, x9

1:	/* 8 bytes at a time */
	sub	x2, x2, #8
	cmp	x2, #7
	str	x8, [x0], #8
	b.hi	1b

2:	/* at least one byte to set? */
	cbz	x2, 4f

3:	/* one byte at a time */
	subs	x2, x2, #1
	strb	w8, [x0], #1
	b.ne	3b

4:	ret

/* void arch_early_memcpy(void *dst, const void *src, size_t n) */
GTEXT(arch_early_memcpy)
SECTION_FUNC(TEXT, arch_early_memcpy)

	/* are dst and src pointers 8-bytes aligned? */
	orr	x8, x1, x0
	tst	x8, #0x7
	b.ne	2f

	/* at least 8 bytes to copy? */
	cmp	x2, #8
	b.lo	2f

1:	/* 8 bytes at a time */
	ldr	x8, [x1], #8
	sub	x2, x2, #8
	cmp	x2, #7
	str	x8, [x0], #8
	b.hi	1b

2:	/* at least one byte to copy? */
	cbz	x2, 4f

3:	/* one byte at a time */
	ldrb	w8, [x1], #1
	subs	x2, x2, #1
	strb	w8, [x0], #1
	b.ne	3b

4:	ret
