/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright (c) 2025 ispace, inc.
 *
 * These functions are adapted from TI Halcogen generated code.
 */

/*
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com
*
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>

/* Exports */
GTEXT(_cacheEnable_)
GTEXT(_coreEnableEventBusExport_)
GTEXT(_mpuInit_)
GTEXT(soc_reset_hook)

#define MINITGCR		=#0xFFFFFF5C
#define MSIENA			=#0xFFFFFF60
#define MSTCGSTAT		=#0xFFFFFF68

#define r1Base                  =#0x00000000
#define r2Base                  =#0x00000000
#define r3Base                  =#0x08000000
#define r4Base                  =#0xF8000000
#define r5Base                  =#0x60000000
#define r6Base                  =#0x80000000
#define r7Base                  =#0xF0000000
#define r8Base                  =#0x00000000
#define r9Base                  =#0x00000000
#define r10Base                 =#0x00000000
#define r11Base                 =#0x00000000
#define r12Base                 =#0x00000000
#define r13Base                 =#0x00000000
#define r14Base                 =#0x00000000
#define r15Base                 =#0x00000000
#define r16Base                 =#0xFFF80000

/* initialise memory areas (_memInit_ from Halcogen) */
SECTION_FUNC(TEXT, soc_reset_hook)
	ldr   r12, MINITGCR	/* Load MINITGCR register address */
	mov   r4, #0xA
	str   r4, [r12]		/* Enable global memory hardware initialization */

	ldr   r11,  MSIENA	/* Load MSIENA register address */
	mov   r4, #0x1		/* Bit position 0 of MSIENA corresponds to SRAM */
	str   r4, [r11]		/* Enable auto hardware initalisation for SRAM */
mloop:				/* Loop till memory hardware initialization comletes */
	ldr   r5, MSTCGSTAT	/* check MSTCGSTAT */
	ldr   r4, [r5]
	tst   r4, #0x100
	beq   mloop

	ldr   r11,  MSIENA	/* Load MSIENA register address */
	mov   r4, #0x4		/* Bit position 2 of MSIENA corresponds to VIM RAM */
	str   r4, [r11]		/* Enable auto hardware initalisation for VIM RAM */
mloop2:				/* Loop till memory hardware initialization comletes */
	ldr   r5, MSTCGSTAT   	/* check MSTCGSTAT */
	ldr   r4, [r5]
	tst   r4, #0x100
	beq   mloop2

	mov   r4, #5
	str   r4, [r12]		/* Disable global memory hardware initialization */

	push {lr}
	bl soc_platform_init
	pop {lr}

	bx lr

SECTION_FUNC(TEXT, _coreEnableEventBusExport_)

	mrc   p15, #0x00, r0,         c9, c12, #0x00
	orr   r0,  r0,    #0x10
	mcr   p15, #0x00, r0,         c9, c12, #0x00
	bx    lr

SECTION_FUNC(TEXT, _cacheEnable_)
	stmfd sp!, {r0-r1}
	mov r0,#0

	mrc p15, #0, r1, c1, c0, #1     /* Read auxiliary control register */
	bic r1, r1, #0x1 << 5           /* bit is default set to disable ECC. Clearing bit 5 */
	mcr p15, #0, r1, c1, c0, #1     /* enable ECC, generate abort on ECC errors, enable */
					/* hardware recovery */

	mrc   p15, #0, R1, c1, c0, #0   /* Read System Control Register configuration data */
	orr   R1, R1, #0x1 <<12         /* instruction cache enable */
	orr   R1, R1, #0x1 <<2          /* data cache enable */
	dsb
	mcr   p15, #0, r0, c15, c5, #0  /* Invalidate entire data cache */
	dsb                             /* delay is required, manually added */
	mcr   p15, #0, r0, c7, c5, #0   /* Invalidate entire instruction cache */
	dsb                             /* delay is required, manually added */
	mcr   p15, #0, R1, c1, c0, #0   /* enabled cache RAMs */
	isb
	ldmfd sp!, {r0-r1}
	bx    lr

SECTION_FUNC(TEXT, _mpuInit_)
	/* Disable mpu */
	mrc   p15, #0, r0, c1, c0, #0
	bic   r0,  r0, #1
	dsb
	mcr   p15, #0, r0, c1, c0, #0
	isb
	/* Disable background region */
	mrc   p15, #0, r0,      c1, c0, #0
	bic   r0,  r0, #0x20000
	mcr   p15, #0, r0,      c1, c0, #0
	/* Setup region 1 */
	mov   r0,  #0
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r1Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0008
	orr   r0,  r0,    #0x1000
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((1 << 15) + (1 << 14) + (1 << 13) + (1 << 12) + (1 << 11) + (1 << 10) + (1 <<  9) + (1 <<  8) + (0x1F << 1) + (1)) 
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 2 */
	mov   r0,  #1
        mcr   p15, #0,    r0, c6, c2, #0
        ldr   r0,  r2Base
        mcr   p15, #0,    r0, c6, c1, #0
        /* non cached flash */
	mov   r0,  #0x000C
	/* cached flash */
	/* mov   r0,  #0x0002 */
        orr   r0,  r0,    #0x0600
        mcr   p15, #0,    r0, c6, c1, #4
        movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x15 << 1) + (1))
        mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region  3 */
	mov   r0,  #2
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r3Base
	mcr   p15, #0,    r0, c6, c1, #0    
	mov   r0,  #0x000B
	orr   r0,  r0,    #0x1300
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x12 << 1) + (1))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 4 */
	mov   r0,  #3
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r4Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0010
	orr   r0,  r0,    #0x1300
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (1 << 10) + (1 <<  9) + (1 <<  8) + (0x1A << 1) + (1))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 5 */
	mov   r0,  #4
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r5Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0000
	orr   r0,  r0,    #0x0300
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((1 << 15) + (1 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x1B << 1) + (1))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 6 */
	mov   r0,  #5
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r6Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0006
	orr   r0,  r0,    #0x0300
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x1A << 1) + (1))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 7 */
	mov   r0,  #6
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r7Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0008
	orr   r0,  r0,    #0x1200
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x16 << 1) + (1))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 8 */
	mov   r0,  #7
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r8Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0010
	orr   r0,  r0,    #0x1200
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x04 << 1) + (0))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 9 */
	mov   r0,  #8
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r9Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0006
	orr   r0,  r0,    #0x1200
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x04 << 1) + (0))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 10 */
	mov   r0,  #9
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r10Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x000C
	orr   r0,  r0,    #0x1300
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x04 << 1) + (0))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 11 */
	mov   r0,  #10
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r11Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0006
	orr   r0,  r0,    #0x0600
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x04 << 1) + (0))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 12 */
	mov   r0,  #11
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r12Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0006
	orr   r0,  r0,    #0x1600
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x04 << 1) + (0))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 13 */
	mov   r0,  #12
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r13Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0006
	orr   r0,  r0,    #0x1600
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x04 << 1) + (0))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 14 */
	mov   r0,  #13
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r14Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0006
	orr   r0,  r0,    #0x1600
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x04 << 1) + (0))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 15 */
	mov   r0,  #14
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r15Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0006
	orr   r0,  r0,    #0x1600
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x04 << 1) + (0))
	mcr   p15, #0,    r0, c6, c1, #2
	/* Setup region 16 */
	mov   r0,  #15
	mcr   p15, #0,    r0, c6, c2, #0
	ldr   r0,  r16Base
	mcr   p15, #0,    r0, c6, c1, #0
	mov   r0,  #0x0010
	orr   r0,  r0,    #0x1200
	mcr   p15, #0,    r0, c6, c1, #4
	movw  r0,  #((0 << 15) + (0 << 14) + (0 << 13) + (0 << 12) + (0 << 11) + (0 << 10) + (0 <<  9) + (0 <<  8) + (0x12 << 1) + (1))
	mcr   p15, #0,    r0, c6, c1, #2

	/* Enable mpu */
	mrc   p15, #0, r0, c1, c0, #0
	orr   r0,  r0, #1
	dsb
	mcr   p15, #0, r0, c1, c0, #0
	isb
	bx    lr
