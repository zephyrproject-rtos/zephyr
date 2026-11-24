/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Save, restore and clear the HVX registers of the current context
 * (the one SSR.XA points at, with XE=1).
 *
 * The buffer is a struct _hvx_context: V0-V31 followed by one vector with
 * Q0-Q3 packed into bits 0-3 of each byte. It lives in struct k_thread and
 * need not be vector aligned, so vmemu is used.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

GTEXT(z_hexagon_hvx_save)
GTEXT(z_hexagon_hvx_restore)
GTEXT(z_hexagon_hvx_zero)

#define HVX_VREGS 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, \
		  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31

/*
 * Q registers can't be loaded or stored, only converted to and from a V
 * register with vand. Q<n> goes to bit n of every byte, so all four fit
 * in one vector. QBIT(n) is that bit repeated in each byte of a word.
 */
#define QBIT(n) (0x01010101 << (n))

/* v0 |= Q<n> as bit n of each byte */
.macro q_to_v0 n
	r2 = ##QBIT(\n)
	v0 |= vand(q\n, r2)
.endm

/* Q<n> = bit n of each byte of v0 */
.macro v0_to_q n
	r2 = ##QBIT(\n)
	q\n = vand(v0, r2)
.endm

/* z_hexagon_hvx_save(buf), clobbers V0 */
SECTION_FUNC(TEXT, z_hexagon_hvx_save)
	r1 = r0
	.irp n, HVX_VREGS
	vmemu(r1++#1) = v\n
	.endr

	/* V0 is saved, reuse it to pack Q0-Q3 into the last vector */
	v0 = vxor(v0, v0)
	.irp n, 0,1,2,3
	q_to_v0 \n
	.endr
	vmemu(r1+#0) = v0
	jumpr r31

/* z_hexagon_hvx_restore(buf) */
SECTION_FUNC(TEXT, z_hexagon_hvx_restore)
	/* Q0-Q3 first, through V0, before V0 gets its own value back */
	r1 = add(r0, #(32 * 128))
	v0 = vmemu(r1+#0)
	.irp n, 0,1,2,3
	v0_to_q \n
	.endr

	r1 = r0
	.irp n, HVX_VREGS
	v\n = vmemu(r1++#1)
	.endr
	jumpr r31

/* z_hexagon_hvx_zero() */
SECTION_FUNC(TEXT, z_hexagon_hvx_zero)
	.irp n, HVX_VREGS
	v\n = vxor(v\n, v\n)
	.endr

	/* V0 is zero now, so any mask gives an all-false Q */
	.irp n, 0,1,2,3
	v0_to_q \n
	.endr
	jumpr r31
