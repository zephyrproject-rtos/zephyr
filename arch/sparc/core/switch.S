/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/sparc/sparc.h>

GTEXT(z_sparc_arch_switch)
GTEXT(z_sparc_context_switch)
GTEXT(z_thread_entry_wrapper)

/*
 * The routine z_sparc_context_switch() is called from arch_switch(), or from
 * the interrupt trap handler in case of preemption. The subtraction to get the
 * "old" thread from "switched_from" has already been performed and the "old"
 * thread is now in register %o1. We can address old->switch_handle in assembly
 * as: [%o1 + ___thread_t_switch_handle_OFFSET].
 *
 * The switch_handle is written in z_sparc_context_switch() after the old
 * context has been saved.
 *
 * This is a leaf function, so only out registers
 * can be used without saving their context first.
 *
 * o0: new thread to restore
 * o1: old thread to save
 */
SECTION_FUNC(TEXT, z_sparc_context_switch)
	mov	%y, %o4
	st	%o4, [%o1 + _thread_offset_to_y]
	std	%l0, [%o1 + _thread_offset_to_l0_and_l1]
	std	%l2, [%o1 + _thread_offset_to_l2]
	std	%l4, [%o1 + _thread_offset_to_l4]
	std	%l6, [%o1 + _thread_offset_to_l6]
	std	%i0, [%o1 + _thread_offset_to_i0]
	std	%i2, [%o1 + _thread_offset_to_i2]
	std	%i4, [%o1 + _thread_offset_to_i4]
	std	%i6, [%o1 + _thread_offset_to_i6]

	std	%o6, [%o1 + _thread_offset_to_o6]

	rd	%psr, %o4
	st	%o4, [%o1 + _thread_offset_to_psr]

	and	%o4, PSR_CWP, %g3       /* %g3 = CWP */
	andn	%o4, PSR_ET, %g1        /* %g1 = psr with traps disabled */
	wr	%g1, %psr               /* disable traps */
	nop
	nop
	nop

	rd	%wim, %g2               /* %g2 = wim */
	mov	1, %g4
	sll	%g4, %g3, %g4           /* %g4 = wim mask for CW invalid */

.Lsave_frame_loop:
	sll	%g4, 1, %g5             /* rotate wim left by 1 */
	srl	%g4, (CONFIG_SPARC_NWIN-1), %g4
	or	%g4, %g5, %g4           /* %g4 = wim if we do one restore */

	/* if restore would not underflow, continue */
	andcc	%g4, %g2, %g0           /* window to flush? */
	bnz    .Ldone_flushing          /* continue */
	 nop
	restore                         /* go one window back */

	/* essentially the same as window overflow */
	/* sp still points to task stack */
	std	%l0, [%sp + 0x00]
	std	%l2, [%sp + 0x08]
	std	%l4, [%sp + 0x10]
	std	%l6, [%sp + 0x18]
	std	%i0, [%sp + 0x20]
	std	%i2, [%sp + 0x28]
	std	%i4, [%sp + 0x30]
	std	%i6, [%sp + 0x38]
	ba	.Lsave_frame_loop
	 nop

.Ldone_flushing:
	/*
	 * "wrpsr" is a delayed write instruction so wait three instructions
	 * after the write before using non-global registers or instructions
	 * affecting the CWP.
	 */
	wr	%g1, %psr               /* restore cwp */
	nop
	nop
	nop
	add	%g3, 1, %g2             /* calculate desired wim */
	cmp	%g2, (CONFIG_SPARC_NWIN-1) /* check if wim is in range */
	bg,a	.Lwim_overflow
	 mov	0, %g2

.Lwim_overflow:

	mov	1, %g4
	sll	%g4, %g2, %g4           /* %g4 = new wim */
	wr	%g4, %wim
	nop
	nop
	nop

	/*
	 * We have finished saving the "old" context and are also back in the
	 * register window for which z_sparc_context_switch() was called.
	 *
	 * Now write the old thread into switch handle.
	 * "old->switch_handle = old".
	 */
	st	%o1, [%o1 + ___thread_t_switch_handle_OFFSET]

	ldd	[%o0 + _thread_offset_to_y], %o4
	mov	%o4, %y

	/* restore local registers */
	ldd	[%o0 + _thread_offset_to_l0_and_l1], %l0
	ldd	[%o0 + _thread_offset_to_l2], %l2
	ldd	[%o0 + _thread_offset_to_l4], %l4
	ldd	[%o0 + _thread_offset_to_l6], %l6

	/* restore input registers */
	ldd	[%o0 + _thread_offset_to_i0], %i0
	ldd	[%o0 + _thread_offset_to_i2], %i2
	ldd	[%o0 + _thread_offset_to_i4], %i4
	ldd	[%o0 + _thread_offset_to_i6], %i6

	/* restore output registers */
	ldd	[%o0 + _thread_offset_to_o6], %o6
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	ld	[%o0 + _thread_offset_to_tls], %g7
#endif

	ld	[%o0 + _thread_offset_to_psr], %g1  /* %g1 = new thread psr */

	andn	%g1, PSR_CWP, %g1       /* psr without cwp */
	or	%g1, %g3, %g1           /* psr with new cwp */
	wr	%g1, %psr               /* restore status register and ET */
	nop
	nop
	nop

	/* jump into thread */
	jmp	%o7 + 8
	 nop

SECTION_FUNC(TEXT, z_thread_entry_wrapper)
	mov	%g0, %o7
	mov	%i0, %o0
	mov	%i1, %o1
	mov	%i2, %o2
	mov	%i3, %o3
	call	z_thread_entry
	 nop
