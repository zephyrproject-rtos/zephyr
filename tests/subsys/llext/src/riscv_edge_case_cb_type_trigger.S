/*
 * Copyright (c) 2024 CISPA Helmholtz Center for Information Security
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

GTEXT(_riscv_edge_case_cb_trigger_backward)

/*
 * Tests that jumping 256 bytes (the maximum) backwards
 * using CB-type instruction is feasible
 */
SECTION_FUNC(TEXT, _riscv_edge_case_cb_trigger_backward)
    /*
     * tentative fail
     * this needs precise alignment - need explicit compressed instructions
     */
    addi a0, zero, 0
    c.j _do_jump

_backward_jump_target:

    /*
     * we need to force RISC-V compressed instructions for alignment
     * this directive is standard in RISC-V, i.e., not toolchain-specific
     */
    .option push
    .option rvc

    /* we made it to the correct target - success, return true */
    c.addi a0, 0x1
    /* explicit compressed return */
    c.jr ra


    /*
     * we need a distance of 256 bytes between _do_jump and _backward_jump_target to trigger
     * the edge case (max jump distance for c.beqz)
     * _backward_jump_target itself needs 4 bytes (two compressed instructions)
     * so we need to insert 252 additional padding bytes
     * we pad with return instructions here, causing the test to return 0 (failure)
     */
    .rept 126
        /* explicit compressed return - 2 bytes */
        c.jr ra
    .endr

_do_jump:
    /* jump precisely 256 bytes, the maximum distance, backwards */
    c.beqz a0, _backward_jump_target

    /*
     * in case we erroneously jump FORWARD instead of backwards,
     * the jump ends in the following return sled and we return 0
     * this indicates test failure
     * note that maximum distance for jump forwards is 254 bytes,
     * which is also the size of this sled
     */
    .rept 127
        /* explicit compressed return - 2 bytes */
        c.jr ra
    .endr

    /* assembler can decide whether to emit compressed instructions */
    .option pop

GTEXT(_riscv_edge_case_cb_trigger_forward)

/*
 * Tests that jumping 256 bytes (the maximum) forwards
 * using CB-type instruction is feasible
 */
SECTION_FUNC(TEXT, _riscv_edge_case_cb_trigger_forward)
    j _test_start

    /* we need to force RISC-V compressed instructions for alignment */
    .option push
    .option rvc

    /*
     * in case the relocation is incorrect and the c.beqz jumps BACKWARDS,
     * e.g., after arithmetic overflow, we jump into the following return sled
     * the return sled is 256 bytes long, covering the maximum backward jump
     */
    .rept 128
        /* explicit compressed return - 2 bytes */
        c.jr ra
    .endr

_test_start:
    /* tentative fail */
    addi a0, zero, 0

    /*
     * jump precisely 254 bytes, the maximum distance, forwards
     * in case the relocation is applied incorrectly, we jump into the padding bytes
     * this causes test failure
     * we cannot jump too far forwards, 254 bytes is the maximum distance
     */
    c.beqz a0, _forward_jump_target

    /*
     * need to insert 252 padding bytes to pad to 254 byte jump
     * we pad with return instructions here, causing the test to return 0 (failure)
     */
    .rept 126
        /* explicit compressed return - 2 bytes */
        c.jr ra
    .endr

    /* assembler can decide whether to emit compressed instructions */
    .option pop

_forward_jump_target:
    /* we made it to the correct target - success, return true */
    li a0, 1
    /* should not be reached - causes return false */
    ret
