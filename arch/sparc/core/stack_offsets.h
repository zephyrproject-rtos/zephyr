/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_SPARC_CORE_STACK_H_
#define ZEPHYR_ARCH_SPARC_CORE_STACK_H_

/*
 * Offsets for SPARC ABI stack frame.
 *
 * Reference: System V Application Binary Interface, SPARC Processor
 * Supplement, Third Edition, Page 3-35.
 */
#define STACK_FRAME_L0_OFFSET                           0x00
#define STACK_FRAME_L1_OFFSET                           0x04
#define STACK_FRAME_L2_OFFSET                           0x08
#define STACK_FRAME_L3_OFFSET                           0x0c
#define STACK_FRAME_L4_OFFSET                           0x10
#define STACK_FRAME_L5_OFFSET                           0x14
#define STACK_FRAME_L6_OFFSET                           0x18
#define STACK_FRAME_L7_OFFSET                           0x1c
#define STACK_FRAME_I0_OFFSET                           0x20
#define STACK_FRAME_I1_OFFSET                           0x24
#define STACK_FRAME_I2_OFFSET                           0x28
#define STACK_FRAME_I3_OFFSET                           0x2c
#define STACK_FRAME_I4_OFFSET                           0x30
#define STACK_FRAME_I5_OFFSET                           0x34
#define STACK_FRAME_I6_OFFSET                           0x38
#define STACK_FRAME_I7_OFFSET                           0x3c
#define STACK_FRAME_STRUCTURE_RETURN_ADDRESS_OFFSET     0x40
#define STACK_FRAME_SAVED_ARG0_OFFSET                   0x44
#define STACK_FRAME_SAVED_ARG1_OFFSET                   0x48
#define STACK_FRAME_SAVED_ARG2_OFFSET                   0x4c
#define STACK_FRAME_SAVED_ARG3_OFFSET                   0x50
#define STACK_FRAME_SAVED_ARG4_OFFSET                   0x54
#define STACK_FRAME_SAVED_ARG5_OFFSET                   0x58
#define STACK_FRAME_PAD0_OFFSET                         0x5c
#define STACK_FRAME_SIZE                                0x60


/* Interrupt stack frame */
#define ISF_PSR_OFFSET  (0x40 + 0x00)
#define ISF_PC_OFFSET   (0x40 + 0x04)
#define ISF_NPC_OFFSET  (0x40 + 0x08)
#define ISF_G1_OFFSET   (0x40 + 0x0c)
#define ISF_G2_OFFSET   (0x40 + 0x10)
#define ISF_G3_OFFSET   (0x40 + 0x14)
#define ISF_G4_OFFSET   (0x40 + 0x18)
#define ISF_Y_OFFSET    (0x40 + 0x1c)

#if !defined(_FLAT)
 #define ISF_SIZE       (0x20)
#else
/*
 * The flat ABI stores and loads "local" and "in" registers in the save area as
 * part of function prologue and epilogue. So we allocate space for a new save
 * area (0x40 byte) as part of the interrupt stack frame.
 */
 #define ISF_SIZE       (0x40 + 0x20)
#endif

#endif /* ZEPHYR_ARCH_SPARC_CORE_STACK_H_ */
