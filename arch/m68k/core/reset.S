/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>

GTEXT(__start)
GTEXT(z_prep_c)
#ifdef CONFIG_THREAD_LOCAL_STORAGE
GTEXT(z_m68k_tls_init)
#endif

SECTION_FUNC(TEXT, __start)
	move.w	#0x2700, %sr
	lea	z_interrupt_stacks, %sp
	adda.l	#__z_interrupt_stack_SIZEOF, %sp
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* Keep bootstrap TLS above the downward-growing runtime stack. */
	suba.l	#__tls_size, %sp
	move.l	%sp, -(%sp)
	jbsr	z_m68k_tls_init
	addq.l	#4, %sp
#endif
	jbsr	z_prep_c
