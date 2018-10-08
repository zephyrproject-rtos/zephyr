/*
 * Copyright (c) 2018 SiFive Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

/* exports */
GTEXT(atomic_cas)

/**
 *
 * @brief Atomic compare-and-set primitive
 *
 * This routine provides the compare-and-set operator. If the original value at
 * <target> equals <oldValue>, then <newValue> is stored at <target> and the
 * function returns 1.
 *
 * If the original value at <target> does not equal <oldValue>, then the store
 * is not done and the function returns 0.
 *
 * The reading of the original value at <target>, the comparison,
 * and the write of the new value (if it occurs) all happen atomically with
 * respect to both interrupts and accesses of other processors to <target>.
 *
 * @param target address to be tested
 * @param old_value value to compare against
 * @param new_value value to store
 * @return Returns 1 if <new_value> is written, 0 otherwise.
 */
SECTION_FUNC(text, atomic_cas)
        /* Adapted from the Compare-and-swap example from page 62
           of The RISC-V Reader by Patterson and Waterman (2017) */

        /* a0 contains the atomic target
           a1 contains the expected old value
           a2 contains the new value to store */

        /* Load and create a memory reservation */
        lr.w a3, (a0)

        /* If the value doesn't match, fail */
        bne  a3, a1, cas_fail

        /* Try to store-conditional */
        sc.w a4, a2, (a0)

        /* If we couldn't store, fail */
        bnez a4, cas_fail

        /* Return 1 for success */
        li   a0, 1
        ret

cas_fail:
        /* Return 0 for failure */
        li   a0, 0
        ret

