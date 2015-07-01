/* ffs.S - ARM find first set assembly routines */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This library implements find_last_set() and find_first_set() which returns the
most and least significant bit set respectively.
 */

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>

_ASM_FILE_PROLOGUE

/* Exports */

GTEXT(find_last_set)
GTEXT(find_first_set)

/**
 *
 * @brief Find first set bit (searching from the most significant bit)
 *
 * This routine finds the first bit set in the argument passed it and
 * returns the index of that bit.  Bits are numbered starting
 * at 1 from the least significant bit.  A return value of zero indicates that
 * the value passed is zero.
 *
 * @return most significant bit set
 */

SECTION_FUNC(TEXT, find_last_set)

    cmp r0, #0
    itt ne
	clzne r0, r0
	rsbne r0, r0, #32

    mov pc, lr

/**
 *
 * @brief Find first set bit (searching from the least significant bit)
 *
 * This routine finds the first bit set in the argument passed it and
 * returns the index of that bit.  Bits are numbered starting
 * at 1 from the least significant bit.  A return value of zero indicates that
 * the value passed is zero.
 *
 * @return least significant bit set
 */

SECTION_FUNC(TEXT, find_first_set)

    rsb r1, r0, #0
    ands r0, r1, r0	/* r0 = x & (-x): only LSB set */

    itt ne
	clzne r0, r0	/* count leading zeroes */
	rsbne r0, r0, #32

    mov pc, lr
