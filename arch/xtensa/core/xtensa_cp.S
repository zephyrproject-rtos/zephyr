/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>
#include <zephyr/arch/xtensa/cp_sharing.h>

/* The save and load macros below are passed A3 as their only temporary */
#if XTENSA_CP_NUM_ATMPS > 1
#error "Shared coprocessor needs more than one temporary register"
#endif

#if defined(CONFIG_XTENSA_EAGER_CP_SHARING)
/*
 * Load the coprocessor registers from the cp buffer in the BSA. Round the
 * address of this buffer up to XTENSA_CP_SA_ALIGN bytes to guarantee the
 * necessary alignment.
 *
 * Upon entry ...
 * A0 - return address (do not modify)
 * A1 - address of BSA (do not modify)
 * A2 - available for use
 * A3 - available for use
 */
.global _xtensa_cp_load
.align 4
_xtensa_cp_load:
        addi a2, a1, (___xtensa_irq_bsa_t_cp_OFFSET + XTENSA_CP_SA_ALIGN - 1)
        movi a3, ~(XTENSA_CP_SA_ALIGN - 1)
        and a2, a2, a3

        XTENSA_CP_LOAD a2 a3 a3 a3 a3    /* Only A2 and A3 are used by macro */

	ret

/*
 * Save the coprocessor registers into the cp buffer in the BSA. Round the
 * address of this buffer up to XTENSA_CP_SA_ALIGN bytes to guarantee the
 * necessary alignment.
 *
 * A0 - return address (do not modify)
 * A1 - address of BSA (do not modify)
 * A2 - available for use
 * A3 - available for use
 */
.global _xtensa_cp_save
.align 4
_xtensa_cp_save:
        addi a2, a1, (___xtensa_irq_bsa_t_cp_OFFSET + XTENSA_CP_SA_ALIGN - 1)
        movi a3, ~(XTENSA_CP_SA_ALIGN - 1)
        and a2, a2, a3

        XTENSA_CP_STORE a2 a3 a3 a3 a3  /* Only A2 and A3 are used by macro */

	ret
#elif defined(CONFIG_XTENSA_LAZY_CP_SHARING)
/*
 * Load the coprocessor registers from the cp buffer in the k_thread
 * structure.
 */
.global xtensa_lazy_cp_load
.align 4
xtensa_lazy_cp_load:
	entry a1, 32
	/* Spill registers onto stack */
	mov a4, a0
	call0 xtensa_spill_reg_windows
	mov a0, a4

	/* A2 should be address of cp storage; A3 is scratch */
	XTENSA_CP_LOAD a2 a3 a3 a3 a3
	retw

/*
 * Save the coprocessor registers to the cp buffer in the k_thread
 * structure.
 */
.global xtensa_lazy_cp_save
.align 4
xtensa_lazy_cp_save:
	entry a1, 32
	/* Spill registers onto stack */
	mov a4, a0
	call0 xtensa_spill_reg_windows
	mov a0, a4

	/* A2 should be address of cp storage; A3 is scratch */
	XTENSA_CP_STORE a2 a3 a3 a3 a3  /* Only A2 and A3 are used by macro */
	retw
#endif
