/*
 * @file entry.S
 *
 * Copyright (c) 2020 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @brief RISC-V trap handling and startup code.
 *
 */

.extern __stack_top

#define PMP_R   0x01
#define PMP_W   0x02
#define PMP_X   0x04
#define PMP_L   0x80
#define PMP_TOR 0x08

#define LREG lw
#define SREG sw
#define REGBYTES 4

#define INT_SIZE_ON_STACK  (16 * REGBYTES)

#define MSTATUS_MPP_MACHINE       0x00001800
#define MCAULSE_ECALL_FROM_MMODE  11
#define MCAULSE_ECALL_FROM_UMODE  8

	.extern trap_entry
	.section      .text.entry
	.global _start
	.option norvc
_start:
	j handle_reset

.macro push_reg
	addi  sp, sp, -(INT_SIZE_ON_STACK)
	SREG x1, 0 * REGBYTES(sp)
	SREG x5, 1 * REGBYTES(sp)
	SREG x6, 2 * REGBYTES(sp)
	SREG x7, 3 * REGBYTES(sp)
	SREG x10, 4 * REGBYTES(sp)
	SREG x11, 5 * REGBYTES(sp)
	SREG x12, 6 * REGBYTES(sp)
	SREG x13, 7 * REGBYTES(sp)
	SREG x14, 8 * REGBYTES(sp)
	SREG x15, 9 * REGBYTES(sp)
	SREG x16, 10 * REGBYTES(sp)
	SREG x17, 11 * REGBYTES(sp)
	SREG x28, 12 * REGBYTES(sp)
	SREG x29, 13 * REGBYTES(sp)
	SREG x30, 14 * REGBYTES(sp)
	SREG x31, 15 * REGBYTES(sp)
	addi  sp, sp, -(INT_SIZE_ON_STACK)
.endm

.macro pop_reg
	addi  sp, sp, INT_SIZE_ON_STACK
	LREG x1, 0 * REGBYTES(sp)
	LREG x5, 1 * REGBYTES(sp)
	LREG x6, 2 * REGBYTES(sp)
	LREG x7, 3 * REGBYTES(sp)
	LREG x10, 4 * REGBYTES(sp)
	LREG x11, 5 * REGBYTES(sp)
	LREG x12, 6 * REGBYTES(sp)
	LREG x13, 7 * REGBYTES(sp)
	LREG x14, 8 * REGBYTES(sp)
	LREG x15, 9 * REGBYTES(sp)
	LREG x16, 10 * REGBYTES(sp)
	LREG x17, 11 * REGBYTES(sp)
	LREG x28, 12 * REGBYTES(sp)
	LREG x29, 13 * REGBYTES(sp)
	LREG x30, 14 * REGBYTES(sp)
	LREG x31, 15 * REGBYTES(sp)
	addi  sp, sp, INT_SIZE_ON_STACK
.endm

trap_entry_wrapper:
	j trap_entry

trap_entry:
	push_reg
	csrr t0, mcause
	/* ecall from M-mode */
	li t1, MCAULSE_ECALL_FROM_MMODE
	bne t0, t1, 1f
	li t2, MSTATUS_MPP_MACHINE
	csrc mstatus, t2
	csrr t0, mepc
	addi t0, t0, 4
	csrw mepc, t0
	pop_reg
	mret
1:
	/* ecall from U-mode */
	li t1, MCAULSE_ECALL_FROM_UMODE
	bne t0, t1, 2f
	li t2, MSTATUS_MPP_MACHINE
	csrs mstatus, t2
	csrr t0, mepc
	addi t0, t0, 4
	csrw mepc, t0
	pop_reg
	mret
2:
	/* Other exception */
	pop_reg
	j trap_entry

handle_reset:
	csrwi mstatus, 0
	csrwi mie, 0
	csrci mstatus, 0x08

	/* Initialize global pointer */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop

	/* (0) 0x00000000 - 0x00002c00 */
	li t0, 0xB00
	csrw pmpaddr0, t0

	/* (1) 0x00002c00 - 0x00008000 */
	/* fixrom: disable w;non-cacheable */
	li t0, 0x2000
	csrw pmpaddr1, t0

	/* (2) 0x00008000 - 0x0011dfc0 */
	/* RAM: disable x;non-cacheable */
	li t0, 0x477f0
	csrw pmpaddr2, t0

	/* (3) 0x0011dfc0 - 0x0011e000 */
	/* checkinfo: disable w x;non-cacheable */
	li t0, 0x47800
	csrw pmpaddr3, t0

	/* (4) 0x0011e000 - 0x003b8000 */
	/* another romboot: disable r-w-x;non-cacheable */
	li t0, 0xee000
	csrw pmpaddr4, t0

	/* (5) 0x003b8000 - 0x003fd800 */
	/* kernel_rombin: diasble r-w-x;non-cacheable */
	li t0, 0xff600
	csrw pmpaddr5, t0

	/* (6) 0x003fd800 - 0x00400000 */
	/* code_rombin(9K): diasble w;non-cacheable */
	li t0, 0x100000
	csrw pmpaddr6, t0

	/* (7) 0x00400000 - 0x60000000 */
	/* REG: disable x;non-cacheable */
	li t0, 0x18000000
	csrw pmpaddr7, t0

	/*
	 * f: Write-back Read and Write-allocate
	 * 3: Normal Non-cacheable Bufferable
	 */
	li t0, 0xf3333333
	csrw 0x7d8, t0

	li t0, ((PMP_TOR | PMP_L) << 0) | \
	       ((PMP_TOR | PMP_R | PMP_X) << 8) | \
	       ((PMP_TOR | PMP_R | PMP_W | PMP_X) << 16) | \
	       ((PMP_TOR | PMP_R) << 24)
	csrw pmpcfg0, t0

	li t0, ((PMP_TOR) << 0) | \
	       ((PMP_TOR) << 8) | \
	       ((PMP_TOR | PMP_R | PMP_X) << 16) | \
	       ((PMP_TOR | PMP_R | PMP_W) << 24)
	csrw pmpcfg1, t0

	/* disable i-cache */
	csrwi 0x7c0, 0x0
	fence

	/* disable d-cache */
	csrwi 0x7c1, 0x0
	fence

	csrwi mstatus, 0
	csrwi mie, 0

	la t0, trap_entry_wrapper
	addi t0, t0, 1

	csrw mtvec, t0

	/* ecall: M-mode -> U-mode */
	ecall

	/* jump to C func. */
	tail start_fastboot
