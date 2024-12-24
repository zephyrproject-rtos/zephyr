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
    /* we made it to the correct target - success, return true */
    c.addi a0, 0x1
    /* explicit ret */
    ret


    /*
     * 2+4 bytes for _backward_jump_target itself
     * need to insert 122 padding bytes
     */
    .fill 126, 2, 0x2

_do_jump:
    /* jump precisely 256 bytes, the maximum distance, backwards */
    c.beqz a0, _backward_jump_target
    /* should not be reached - causes return false */
    ret

GTEXT(_riscv_edge_case_cb_trigger_forward)

/*
 * Tests that jumping 256 bytes (the maximum) forwards
 * using CB-type instruction is feasible
 */
SECTION_FUNC(TEXT, _riscv_edge_case_cb_trigger_forward)
    /*
     * tentative fail
     * this needs precise alignment - need explicit compressed instructions
     */
    addi a0, zero, 0
    /* jump precisely 254 bytes, the maximum distance, forwards */
    c.beqz a0, _forward_jump_target

    /* need to insert 252 padding bytes to pad to 254 byte jump */
    .fill 126, 2, 0x0

_forward_jump_target:
    /* we made it to the correct target - success, return true */
    li a0, 1
    /* should not be reached - causes return false */
    ret
