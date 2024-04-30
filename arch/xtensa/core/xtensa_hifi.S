/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets.h>

#include <xtensa/config/tie.h>
#include <xtensa/config/tie-asm.h>

/*
 * Load the HiFi registers from the hifi buffer in the BSA. Round the address
 * of this buffer up to XCHAL_CP1_SA_ALIGN bytes to guarantee the necessary
 * alignment.
 *
 * Upon entry ...
 * A0 - return address (do not modify)
 * A1 - address of BSA (do not modify)
 * A2 - available for use
 * A3 - available for use
 */
.global _xtensa_hifi_load
.align 4
_xtensa_hifi_load:
        addi a2, a1, (___xtensa_irq_bsa_t_hifi_OFFSET + XCHAL_CP1_SA_ALIGN - 1)
        movi a3, ~(XCHAL_CP1_SA_ALIGN - 1)
        and a2, a2, a3

        xchal_cp1_load a2 a3 a3 a3 a3    /* Only A2 and A3 are used by macro */

	ret

/*
 * Save the HiFi registers into the hifi buffer in the BSA. Round the address
 * of this buffer up to XCHAL_CP1_SA_ALIGN bytes to guarantee the necessary
 * alignment.
 *
 * A0 - return address (do not modify)
 * A1 - address of BSA (do not modify)
 * A2 - available for use
 * A3 - available for use
 */
.global _xtensa_hifi_save
.align 4
_xtensa_hifi_save:
        addi a2, a1, (___xtensa_irq_bsa_t_hifi_OFFSET + XCHAL_CP1_SA_ALIGN - 1)
        movi a3, ~(XCHAL_CP1_SA_ALIGN - 1)
        and a2, a2, a3

        xchal_cp1_store a2 a3 a3 a3 a3  /* Only A2 and A3 are used by macro */

	ret
