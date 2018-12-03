/* ------------------------------------------------------------------------- */
/*  @file:    startup_RV32M1_ri5cy.s                                         */
/*  @purpose: RI5CY Core Device Startup File                                 */
/*            RV32M1_ri5cy                                                   */
/*  @version: 1.0                                                            */
/*  @date:    2018-10-2                                                      */
/*  @build:   b180926                                                        */
/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Copyright 1997-2016 Freescale Semiconductor, Inc.                         */
/* Copyright 2016-2018 NXP                                                   */
/* All rights reserved.                                                      */
/*                                                                           */
/* SPDX-License-Identifier: BSD-3-Clause                                     */


// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the "License"); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#define EXCEPTION_STACK_SIZE 0x58

    .text
    .section .vectors, "ax"
    .option norvc;

    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler
    jal x0, IRQ_Handler

    // reset vector
    jal x0, Reset_Handler

    // Illegal instrution exception
    jal x0, IllegalInstruction_Handler

    // ecall handler
    jal x0, Ecall_Handler

    // LSU error
    jal x0, LSU_Handler

    .section .startup

/* Reset Handler */
Reset_Handler:

    # Disable global interrupt. */
    csrci mstatus, 8

    # initialize stack pointer
    la sp, __StackTop

    # initialize global pointer
    la gp, __global_pointer

#ifndef __NO_SYSTEM_INIT
    jal SystemInit
#endif

    call __libc_init_array

    # Enable global interrupt. */
    csrsi mstatus, 8

    jal main
    ebreak

    .size Reset_Handler, . - Reset_Handler

    .global _init
    .global _fini
_init:
_fini:
    ret

  // saves all caller-saved registers (except return address)
store_regs:
    sw  x3, 0x00(x2)  // gp
    sw  x4, 0x04(x2)  // tp
    sw  x5, 0x08(x2)  // t0
    sw  x6, 0x0c(x2)  // t1
    sw  x7, 0x10(x2)  // t2
    sw x10, 0x14(x2)  // a0
    sw x11, 0x18(x2)  // a1
    sw x12, 0x1c(x2)  // a2
    sw x13, 0x20(x2)  // a3
    sw x14, 0x24(x2)  // a4
    sw x15, 0x28(x2)  // a5
    sw x16, 0x2c(x2)  // a6
    sw x17, 0x30(x2)  // a7

    csrr a0, 0x7B0
    csrr a1, 0x7B1
    csrr a2, 0x7B2
    sw a0, 0x34(x2)  // lpstart[0]
    sw a1, 0x38(x2)  // lpend[0]
    sw a2, 0x3c(x2)  // lpcount[0]
    csrr a0, 0x7B4
    csrr a1, 0x7B5
    csrr a2, 0x7B6
    sw a0, 0x40(x2)  // lpstart[1]
    sw a1, 0x44(x2)  // lpend[1]
    sw a2, 0x48(x2)  // lpcount[1]

    csrr a0, 0x341
    sw a0, 0x4c(x2)  // mepc
    csrr a1, 0x300
    sw a1, 0x50(x2)  // mstatus
    jalr x0, x1

    // load back registers from stack
end_except:
    lw a1, 0x50(x2)  // mstatus
    csrrw x0, 0x300, a1
    lw a0, 0x4c(x2)  // mepc
    csrrw x0, 0x341, a0

    lw a0, 0x40(x2)  // lpstart[1]
    lw a1, 0x44(x2)  // lpend[1]
    lw a2, 0x48(x2)  // lpcount[1]
    csrrw x0, 0x7B4, a0
    csrrw x0, 0x7B5, a1
    csrrw x0, 0x7B6, a2
    lw a0, 0x34(x2)  // lpstart[0]
    lw a1, 0x38(x2)  // lpend[0]
    lw a2, 0x3c(x2)  // lpcount[0]
    csrrw x0, 0x7B0, a0
    csrrw x0, 0x7B1, a1
    csrrw x0, 0x7B2, a2

    lw  x3, 0x00(x2)  // gp
    lw  x4, 0x04(x2)  // tp
    lw  x5, 0x08(x2)  // t0
    lw  x6, 0x0c(x2)  // t1
    lw  x7, 0x10(x2)  // t2
    lw x10, 0x14(x2)  // a0
    lw x11, 0x18(x2)  // a1
    lw x12, 0x1c(x2)  // a2
    lw x13, 0x20(x2)  // a3
    lw x14, 0x24(x2)  // a4
    lw x15, 0x28(x2)  // a5
    lw x16, 0x2c(x2)  // a6
    lw x17, 0x30(x2)  // a7

    lw  x1, 0x54(x2)
    addi x2, x2, EXCEPTION_STACK_SIZE
    mret

    .weak IRQ_Handler
    .type IRQ_Handler, %function
IRQ_Handler:
    addi x2, x2, -EXCEPTION_STACK_SIZE
    sw x1, 0x54(x2)
    jal x1, store_regs
    la x1, end_except
    csrr a0, mcause
    jal x0, SystemIrqHandler
    .size IRQ_Handler, . - IRQ_Handler

    .macro define_exception_entry entry_name handler_name
    .weak \entry_name
\entry_name:
    addi x2, x2, -EXCEPTION_STACK_SIZE
    sw x1, 0x54(x2)
    jal x1, store_regs
    la x1, end_except
    jal x0, \handler_name
    .endm

define_exception_entry IllegalInstruction_Handler IllegalInstruction_HandlerFunc
define_exception_entry Ecall_Handler Ecall_HandlerFunc
define_exception_entry LSU_Handler LSU_HandlerFunc

    .weak IllegalInstruction_HandlerFunc
    .type IllegalInstruction_HandlerFunc, %function
IllegalInstruction_HandlerFunc:
    j .
    .size IllegalInstruction_HandlerFunc, . - IllegalInstruction_HandlerFunc

    .weak Ecall_HandlerFunc
    .type Ecall_HandlerFunc, %function
Ecall_HandlerFunc:
    j .
    .size Ecall_HandlerFunc, . - Ecall_HandlerFunc

    .weak LSU_HandlerFunc
    .type LSU_HandlerFunc, %function
LSU_HandlerFunc:
    j .
    .size LSU_HandlerFunc, . - LSU_HandlerFunc
