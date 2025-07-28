/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>

#include <xtensa/config/tie.h>
#include <xtensa/config/tie-asm.h>

#if defined(CONFIG_XTENSA_EAGER_HIFI_SHARING)
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
#elif defined(CONFIG_XTENSA_LAZY_HIFI_SHARING)
/*
 * Load the HiFi registers from the HiFi buffer in the k_thread structure.
 */
.global xtensa_lazy_hifi_load
.align 4
xtensa_lazy_hifi_load:
	entry a1, 32
	/* Spill registers onto stack */
	call8 xthal_window_spill

	/* A2 should be address of hifi storage; A3 is scratch */
	xchal_cp1_load a2 a3 a3 a3 a3
	retw

/*
 * Save the HiFi registers to the HiFi buffer in the k_thread structure.
 */
.global xtensa_lazy_hifi_save
.align 4
xtensa_lazy_hifi_save:
	entry a1, 32
	/* Spill registers onto stack */
	call8 xthal_window_spill

	/* A2 should be address of hifi storage; A3 is scratch */
	xchal_cp1_store a2 a3 a3 a3 a3  /* Only A2 and A3 are used by macro */
	retw
#endif
