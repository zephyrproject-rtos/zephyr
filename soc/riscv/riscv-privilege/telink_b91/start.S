/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

	.option push
	.option norelax
	.org 0x0

/* exports */
GTEXT(entry)

SECTION_FUNC(init, init)
entry:

	j start

	.org 0x20
	.word ('T'<<24 | 'L'<<16 | 'N'<<8 | 'K')

	.org 0x26
	.short (0x173B)

	.option pop
	.align 2

start:

#if 0
	/* Debug LED */
	lui t0,0x80140
	li t1, 0x0f
	li t2, 0x10
	sb t1 , 0x30a(t0)
	sb t2 , 0x30b(t0)
#endif

	/* Initialize global pointer */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop

#ifdef __nds_execit
	/* Initialize EXECIT table */
	la t0, _ITB_BASE_
	csrw uitb, t0
#endif

#ifdef __riscv_flen
	/* Enable FPU */
	li t0, 0x00006000
	csrrs t0, mstatus, t0

	/* Initialize FCSR */
	fscsr zero
#endif

#ifdef CONFIG_CACHE_ENABLE
	/* Enable I/D-Cache */
	csrr   t0,  mcache_ctl
	ori    t0,  t0,  1        #/I-Cache
	ori    t0,  t0,  2        #/D-Cache
	csrw   mcache_ctl,  t0
	fence.i
#endif

	/* Enable misaligned access and non-blocking load */
	li t0, (1 << 8) | (1 << 6)
	csrs mmisc_ctl, t0

_RETENTION_DATA_INIT:
	la     t1, _RETENTION_DATA_LMA_START
	la     t2, _RETENTION_DATA_VMA_START
	la     t3, _RETENTION_DATA_VMA_END
_RETENTION_DATA_INIT_BEGIN:
	bleu   t3, t2, _RAMCODE_INIT
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RETENTION_DATA_INIT_BEGIN

_RAMCODE_INIT:
	la     t1, _RAMCODE_LMA_START
	la     t2, _RAMCODE_VMA_START
	la     t3, _RAMCODE_VMA_END
_RAMCODE_INIT_BEGIN:
	bleu   t3, t2, _START
	lw     t0, 0(t1)
	sw     t0, 0(t2)
	addi   t1, t1, 4
	addi   t2, t2, 4
	j      _RAMCODE_INIT_BEGIN

_START:
	j __start
